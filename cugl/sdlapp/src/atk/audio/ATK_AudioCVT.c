/*
 * SDL_atk:  An audio toolkit library for use with SDL
 * Copyright (C) 2022-2023 Walker M. White
 *
 * This is a library to load different types of audio files as PCM data,
 * and process them with basic DSP tools. The goal of this library is to
 * create an audio equivalent of SDL_image, where we can load and process
 * audio files without having to initialize the audio subsystem. In addition,
 * giving us direct control of the audio allows us to add more custom
 * effects than is possible in SDL_mixer.
 *
 * SDL License:
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */
#include <SDL3/SDL_atk.h>
#include "ATK_Redistribute.h"
#include "ATK_Reformat.h"

/**
 * @file ATK_AudioCVT
 *
 * This component provides support for real-time signal resampling and audio
 * spec conversion. This is some of the oldest code in this library, as we
 * wrote them to overcome a resampling bug in SDL 2.0.14. In addition, exposing
 * a resampler allows us to resample earlier in the DSP graph, as SDL only
 * converts just before the signal reaches the devive.
 */

#pragma mark -
#pragma mark Resampling
/**
 * Returns the power of two greater than or equal to x
 *
 * @param x        The original integer
 *
 * @return the power of two greater than or equal to x
 */
static inline Uint32 next_pot(Uint32 x) {
    x = x - 1;
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >>16);
    return x + 1;
}

/**
 * Returns the appropriate beta for given stopband attenuation
 *
 * Beta is the primary configuration factor (together with zero crossings) for
 * making a kaiser-windowed sinc filter. This value is found experimentally
 * and is described here:
 *
 *     https://tomroelandts.com/articles/how-to-create-a-configurable-filter-using-a-kaiser-window
 *
 * @param db    The stopband attenuation in decibels
 *
 * @return the appropriate beta for given stopband attenuation
 */
static inline double filter_beta(double db) {
    if (db > 50) {
        return 0.1102 * (db - 8.7);
    } else if (db >= 21) {
        return 0.5842*pow((db-21),0.4)+0.07886*(db-21);
    }
    return 0;
}

/**
 * A structure to resample audio to a different rate.
 *
 * This structure supports resampling via bandlimited interpolation, as described
 * here:
 *
 *     https://ccrma.stanford.edu/~jos/resample/Implementation.html
 *
 * Techinically, this process is supported by SDL_AudioCVT in SDL. However, we
 * have had problems with that resampler in the past. As of SDL 2.0.14, there
 * was a bug that could cause the resampler to be caught zero-padding in an
 * infinite loop, resulting in the audio cutting out. This was a major problem
 * on iPhones as the device can switch between 44.1k and 48k, depending on
 * whether you are using headphones or speakers.
 *
 * More importantly, in the public API of SDL (as opposed to the undocumented
 * stream API), resampling can only happen just before audio is sent to the
 * device. In an audio engine, you often want to resample much earlier in the
 * DSP graph (e.g. reading in an audio file compressed with a much lower sample
 * rate). That is why we have separated out this feature.
 */
typedef struct ATK_Resampler {
    /** The number of channels in the input (and output) streams.  */
    Uint8 channels;
    /** The sample rate of the input stream. */
    Uint32 inrate;
    /** The sample rate of the output stream. */
    Uint32 outrate;
    /** The number of zero crossings of the sinc filter. */
    Uint32 zerocross;
    /** The number of samples per zero crossing */
    Uint32 percross;

    /** The filter (table) size */
    size_t filter_size;
    /** The filter coefficients */
    float* filter_table;
    /** The filter coefficient differences */
    float* filter_diffs;

    /** The intermediate sampling buffer. */
    float* smpbuffer;
    /** The capacity of the sampling buffer. */
    size_t capacity;
    /** The padding overscan in the buffer */
    size_t smpoversc;
    /** The amount of data currently available in the sampling buffer. */
    size_t smpavail;
    /** The last (global) index processed by this conversion. */
    size_t smpindex;
    /** The global offset to lessen round-off error. */
    size_t smpoffset;
    /** The current (global) input time */
    double intime;

    /** An optional callback function to fill the buffer. */
    ATK_AudioCallback callback;
    /** The user data for the callback function. */
    void* userdata;

    /** Debug information */
    size_t frame;
} ATK_Resampler;

/**
 * Shifts the input buffer to the left, removing all data that has been read
 *
 * The zero-cross wings necessary for this resampling algorithm are complicated
 * enough as it is without trying to implement a circular buffer for the input
 * queue. Instead, whenever we need to read in new data, we just shift all
 * of the unread data to the left, erasing any data that has already been
 * processed. Given the cost of the rest of this algorithm, the additional
 * overhead is not too bad.
 *
 * @param resampler The resampler state
 */
static void shift_resampler(ATK_Resampler* resampler) {
    Uint32 chans = resampler->channels;
    size_t pos = chans*resampler->smpindex;
    size_t amt = chans*(resampler->smpavail+resampler->smpoversc+
                        resampler->zerocross-resampler->smpindex);

    if (amt > pos) {
        memmove(resampler->smpbuffer,resampler->smpbuffer+pos,amt*sizeof(float));
    } else {
        memcpy(resampler->smpbuffer,resampler->smpbuffer+pos,amt*sizeof(float));
    }

    if (resampler->smpindex > resampler->smpoversc) {
        size_t shift = resampler->smpindex-resampler->smpoversc;
        if (shift > resampler->smpavail) {
            shift = resampler->smpavail;
        }
        resampler->smpavail -= shift;
        resampler->smpoversc = 0;
    } else {
        resampler->smpoversc -= resampler->smpindex;
    }

    // DEBUG
    //Uint32 remain = (Uint32)(resampler->capacity-resampler->smpavail-resampler->smpoversc);
    //float* buff = resampler->smpbuffer+(resampler->smpavail+resampler->smpoversc+resampler->zerocross)*chans;
    //memset(buff,0,(remain+resampler->smpoversc)*sizeof(float)*chans);

    // Reset the time offset
    resampler->smpoffset += resampler->smpindex;
    resampler->smpindex = 0;
}

/**
 * Fills the sampling buffer with the next page of data.
 *
 * This method does nothing if the resampler has no associated callback function
 * for acquiring data.
 *
 * @param resampler The resampler state
 */
static inline void fill_resampler(ATK_Resampler* resampler) {
    if (resampler->callback == NULL) {
        return;
    } if (resampler->smpindex > 0) {
        shift_resampler(resampler);
    }

    // Read in up to the buffer
    Uint32 chans = resampler->channels;
    Uint32 remain = (Uint32)(resampler->capacity-resampler->smpavail-resampler->smpoversc);
    float* buff = resampler->smpbuffer+(resampler->smpavail+resampler->smpoversc+resampler->zerocross)*chans;
    size_t actual = resampler->callback(resampler->userdata, (Uint8*)buff, remain*sizeof(float)*chans);
    actual /= sizeof(float)*chans;

    if (actual == 0) {
        memset(buff, 0, remain*chans*sizeof(float));
        resampler->smpoversc = (remain > resampler->zerocross) ? 0 : resampler->zerocross-remain;
    } else {
        memset(buff+actual*chans, 0, (remain-actual)*chans*sizeof(float));
        resampler->smpoversc = resampler->zerocross;
    }
    
    if (actual > resampler->smpoversc) {
        resampler->smpavail += actual-resampler->smpoversc;
    }
}

/**
 * Filters a single frame (for all channels) of output audio
 *
 * This method processes all of the channels for the current audio frame and
 * stores the results in buffer (in order by channel). The current audio frame
 * is determined by the smpoffset value.
 *
 * @param resampler The resampler state
 * @param buffer    The buffer to store the audio frame
 */
static inline void resample_frame(ATK_Resampler* resampler, float* buffer) {
    double intime  = resampler->intime;
    size_t gbindex = (size_t)intime;
    Uint32 index   = (Uint32)(gbindex-resampler->smpoffset);
    double inrate  = resampler->inrate;
    Uint32 percross = resampler->percross;
    double currtime =  gbindex / inrate;
    double nexttime = (gbindex + 1) / inrate;
    index += resampler->zerocross;

    double interp0 = 1.0 - ((nexttime - (intime/inrate)) / (nexttime - currtime));
    Uint32 filterindex0 = (Uint32)(interp0 * percross);
    double interp1 = 1.0 - interp0;
    Uint32 filterindex1 = (Uint32)(interp1 * percross);

    Uint32 leftbound = (Uint32)((resampler->filter_size-filterindex0)/(double)percross);
    Uint32 rghtbound = (Uint32)((resampler->filter_size-filterindex1)/(double)percross);

    Uint32 leftwing = index-leftbound+1;
    Uint32 midpoint = index+1;
    Uint32 rghtwing = index+rghtbound+1;

    Uint32 chans = resampler->channels;
    for(Uint32 chan = 0; chan < chans; chan++) {
        double outsample = 0.0;

        Uint32 leftindex = filterindex0 + ((leftbound-1) * percross);
        for(Uint32 srcframe = leftwing; srcframe < midpoint; srcframe++) {
            double insample  = resampler->smpbuffer[srcframe*chans+chan];
            double component = insample * (resampler->filter_table[leftindex] + interp0 * resampler->filter_diffs[leftindex]);
            outsample += component;
            leftindex -= percross;
        }

        Uint32 rightindex = filterindex1;
        for(Uint32 srcframe = midpoint; srcframe < rghtwing; srcframe++) {
            double insample  = resampler->smpbuffer[srcframe*chans+chan];
            double component = insample * (resampler->filter_table[rightindex] + interp1 * resampler->filter_diffs[rightindex]);
            outsample += component;
            rightindex += percross;
        }
        
        // Clamp the convolution in range
        buffer[chan] = fminf(fmaxf(outsample, -1), 1);
    }

    // Update the resampler state
    resampler->frame++;
    resampler->intime += inrate/resampler->outrate;
    resampler->smpindex = index - resampler->zerocross;
}


/**
 * Reads up to frames worth of data from the sampling buffer
 *
 * This method will either read frames audio frames, or the extent of the
 * sampling input buffer, which ever comes first.
 *
 * @param resampler The resampler state
 * @param buffer    The buffer to store the audio data
 * @param frames    The maximum number of frames to process
 *
 * @return the number of frames read
 */
static inline Uint32 resample_page(ATK_Resampler* resampler, float* buffer, Uint32 frames) {
    Uint32 chans  = resampler->channels;
    Uint32 pos = 0;

    while (resampler->intime < resampler->smpoffset+resampler->smpavail && pos < frames) {
        resample_frame(resampler,buffer+pos*chans);
        pos++;
    }

    return pos;
}

/**
 * Returns a newly allocated structure to resample audio
 *
 * Audio resampling is performed using bandlimited interpolation, as described
 * here:
 *
 *     https://ccrma.stanford.edu/~jos/resample/Implementation.html
 *
 * It is not possible to change any of the resampler settings after it is
 * allocated, as the filter is specifically tailored to these values. If you
 * need to change the settings, you should allocate a new resampler. It is the
 * responsiblity of the caller to use {@link ATK_FreeResampler} to deallocate
 * the structure when done.
 *
 * @param def   The resampler settings
 */
ATK_Resampler* ATK_AllocResampler(const ATK_ResamplerDef* def) {
    if (def == NULL) {
        return NULL;
    }

    ATK_Resampler* result = NULL;
    float* filter_table = NULL;
    float* filter_diffs = NULL;
    float* smpbuffer = NULL;

    // Initialize the new filter
    Uint32 percross = (1 << ((def->bitdepth / 2) + 1));
    size_t filter_size = ((percross * def->zerocross) + 1);

    // Need to ensure large enough convolution window for paging.
    Uint32 capacity = next_pot(2*def->zerocross+2);
    capacity = capacity < def->buffsize ? def->buffsize : capacity;

    filter_table = (float*)ATK_malloc(sizeof(float)*(filter_size+1));
    if (filter_table == NULL) {
        goto out;
    }
    memset(filter_table, 0, sizeof(float)*filter_size);

    filter_diffs = (float*)ATK_malloc(sizeof(float)*filter_size);
    if (filter_diffs == NULL) {
        goto out;
    }
    memset(filter_diffs, 0, sizeof(float)*filter_size);

    smpbuffer = (float*)ATK_malloc(sizeof(float)*(capacity+2*def->zerocross)*def->channels);
    if (smpbuffer == NULL) {
        goto out;
    }
    memset(smpbuffer, 0, sizeof(float)*capacity*def->channels);

    result = (ATK_Resampler*)ATK_malloc(sizeof(ATK_Resampler));
    if (result == NULL) {
        goto out;
    }

    double beta = filter_beta(def->stopband);
    ATK_FillKaiserWindow(filter_table, 2*filter_size+1, (float)beta, 1);
    for (size_t ii = 1; ii < filter_size; ii++) {
        double x = (M_PI*ii)/percross;
        filter_table[ii] *= (float)(sin(x) / x);
        filter_diffs[ii-1] = filter_table[ii] - filter_table[ii - 1];
    }
    filter_diffs[filter_size-1] = 0.0;

    result->channels = def->channels;
    result->inrate  = def->inrate;
    result->outrate = def->outrate;
    result->zerocross = def->zerocross;
    result->percross  = percross;

    result->filter_size  = filter_size;
    result->filter_table = filter_table;
    result->filter_diffs = filter_diffs;

    result->capacity  = capacity;
    result->smpbuffer = smpbuffer;
    result->smpavail = 0;
    result->smpindex = 0;
    result->smpoversc = 0;
    result->smpoffset = 0;
    result->intime = 0;
    result->frame = 0;

    result->callback = def->callback;
    result->userdata = def->userdata;
    return result;

out:
    if (filter_table != NULL) {
        ATK_free(filter_table);
    }
    if (filter_diffs != NULL) {
        ATK_free(filter_diffs);
    }
    if (smpbuffer != NULL) {
        ATK_free(smpbuffer);
    }
    ATK_OutOfMemory();
    return NULL;
}

/**
 * Frees a previously allocated resampler
 *
 * @param resampler The resampler state
 */
void ATK_FreeResampler(ATK_Resampler* resampler) {
    if (resampler == NULL) {
        return;
    }

    ATK_free(resampler->filter_table);
    resampler->filter_table = NULL;
    ATK_free(resampler->filter_diffs);
    resampler->filter_diffs = NULL;
    ATK_free(resampler->smpbuffer);
    resampler->smpbuffer = NULL;
    ATK_free(resampler);
}

/**
 * Resets a resampler back to its initial (zero-padded) state.
 *
 * Resamplers have to keep state of the conversion performed so far. This makes
 * it not safe to use a resampler on multiple streams simultaneously. Resetting
 * a resampler zeroes the state so that it is the same as if the filter were
 * just allocated.
 *
 * @param resampler The resampler state
 */
void ATK_ResetResampler(ATK_Resampler* resampler) {
    memset(resampler->smpbuffer, 0, sizeof(float)*resampler->capacity*resampler->channels);
    resampler->smpavail = 0;
    resampler->smpindex = 0;
    resampler->smpoversc = 0;
    resampler->smpoffset = 0;
    resampler->intime = 0;
}

/**
 * Pulls converted data from the resampler, populating it in output
 *
 * This function will convert up to len audio frames, storing the result in
 * output. An audio frame is a collection of samples for all of the available
 * channels, so output must be able to support len*channels many elements.
 *
 * It is possible for this function to convert less than len audio frames,
 * particularly if the buffer empties and there is no callback function to
 * repopulate it. In that case, the return value is the number of audio frames
 * read. The output will always consist of complete audio frames. It will never
 * convert some channels for an audio frame while not converting others.
 *
 * @param resampler The resampler state
 * @param output    The output buffer
 * @param frames    The number of audio frames to convert
 *
 * @return the number of audio frames read or -1 on error
 */
Sint64 ATK_PollResampler(ATK_Resampler* resampler, float* output, size_t frames) {
    Sint64 take = 0;
    int abort = 0;
    while (take < (Sint64)frames && !abort) {
        if (resampler->callback) {
            fill_resampler(resampler);
        }
        float* buff = output+take*resampler->channels;
        Uint32 amount = resample_page(resampler,buff,(Uint32)(frames-take));
        take += amount;
        if (amount == 0) {
            abort = 1;
        }
    }
    return take;
}

/**
 * Pushes data to the resampler buffer
 *
 * This is an optional way to repopulate the resampler buffer, particularly if
 * no callback function was specified at the time it was allocated. Data is
 * pushed as complete audio frames. An audio frame is a collection of samples
 * for all of the available channels, so input must hold len*channels many
 * elements. It is not possible to push an incomplete audio frame that stores
 * data for some channels, but not others.
 *
 * The limits on the buffer capacity may mean that not all data can be pushed
 * (particularly if this function is competing with a callback function). The
 * value returned is the number of audio frames successfully stored in the buffer.
 *
 * @param resampler The resampler state
 * @param input     The input buffer
 * @param len       The number of audio frames to push
 *
 * @return the number of audio frames pushed or -1 on error
 */
Sint64 ATK_PushResampler(ATK_Resampler* resampler, const float* input, size_t len) {
    if (resampler == NULL || input == NULL) {
        return -1;
    }
    
    if (resampler->smpindex > 0) {
        shift_resampler(resampler);
    }
    
    // Read in up to the buffer
    Uint32 chans = resampler->channels;
    Uint32 remain = (Uint32)(resampler->capacity-resampler->smpavail-resampler->smpoversc);
    float* buff = resampler->smpbuffer+(resampler->smpavail+resampler->smpoversc+resampler->zerocross)*chans;

    if (len < remain) {
        memcpy(buff,input,len*chans*sizeof(float));
        Uint32 tail = (Uint32)(remain-len);
        buff = buff+(len*chans);
        memset(buff, 0, tail*chans*sizeof(float));
        resampler->smpoversc = (tail > resampler->zerocross) ? 0 : resampler->zerocross-tail;
    } else {
        len = remain;
        memcpy(buff,input,len*chans*sizeof(float));
        resampler->smpoversc = resampler->zerocross;
    }
    if (len > resampler->smpoversc) {
        resampler->smpavail += len-resampler->smpoversc;
    }
    return len;
}

/**
 * Returns true if the resampler buffer has any data that is not yet processed.
 *
 * This function allows us to check if a resampler has finished processing
 * (once we have finished pushing input data) without providing it a buffer
 * to write to.
 *
 * @param resampler The resampler state
 *
 * @return true if the resampler buffer has any data that is not yet processed.
 */
bool ATK_PeekResampler(ATK_Resampler* resampler) {
    return (resampler->intime < resampler->smpoffset+resampler->smpavail);
}

#pragma mark -
#pragma mark Redistribution
/**
 * A structure to redistribute audio channels.
 *
 * Channel redistribution works by using a matrix to redistribute the input
 * channels, in much the same way that a matrix decoder works. However, unlike
 * a matrix decoder, it is possible to use a redistributor to reduce the number
 * of channels (with a matrix whose rows are less that is columns). Furthermore,
 * a redistributor does not support phase shifting.
 */
typedef struct ATK_Redistributor {
    /** The number of input channels */
    Uint32 inchan;
    /** The number of output channels */
    Uint32 outchan;
    /** The redistribution matrix */
    float* matrix;
} ATK_Redistributor;

/**
 * Redistributes from buffer input to buffer output
 *
 * The buffer input should have {@link #getConduits()} channels worth of data,
 * while output should support {@link #getChannels()} channels. However, this
 * method is safe to call in-place, provided that the buffer is large enough to
 * support the new output data.
 *
 * This version of the method assumes that output has more channels than input.
 * This distinction is necessary to support in-place redistribution. The value
 * size is specified in terms of frames, not samples, so that it does not
 * include any channel information.
 *
 * @param distrib   The redistributor
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of audio frames to process
 */
static inline void scale_up(ATK_Redistributor* distrib, const float* input,
                            float* output, size_t size) {
    Uint32 rows = distrib->outchan;
    Uint32 cols = distrib->inchan;
    Uint32 work = rows*cols;

    float* matrix = distrib->matrix;
    const float* src = input + size*cols;
    float* dst = output + size*rows;
    for(size_t ii = 0; ii < size; ii++) {
        dst -= rows;
        src -= cols;

        for(Uint32 jj = 0; jj < rows; jj++) {
            float total = 0;
            for (Uint32 kk = 0; kk < cols; kk++) {
                total += matrix[jj*cols+kk]*src[kk];
            }
            matrix[work+jj] = total;
        }
        memcpy(dst, matrix+work, rows*sizeof(float));
    }
}

/**
 * Redistributes from buffer input to buffer output
 *
 * The buffer input should have inchan channels worth of data, while output
 * should support outchan channels. However, this method is safe to call
 in-place, provided that the output buffer is large enough to support the
 * new output data.
 *
 * This version of the method assumes that input has more channels than output.
 * This distinction is necessary to support in-place redistribution. The value
 * size is specified in terms of frames, not samples, so that it does not
 * include any channel information.
 *
 * @param distrib   The redistributor
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of audio frames to process
 */
static inline void scale_down(ATK_Redistributor* distrib, const float* input,
                              float* output, size_t size) {
    Uint32 rows = distrib->outchan;
    Uint32 cols = distrib->inchan;
    Uint32 work = rows*cols;

    const float* src = input;
    float* dst = output;
    float* matrix = distrib->matrix;

    for(size_t ii = 0; ii < size; ii++, dst += rows, src += cols) {
        for(Uint32 jj = 0; jj < rows; jj++) {
            float total = 0;
            for (Uint32 kk = 0; kk < cols; kk++) {
                total += matrix[jj*cols+kk]*src[kk];
            }
            matrix[work+jj] = total;
        }
        memcpy(dst, matrix+work, rows*sizeof(float));
    }
}

/**
 * Redistributes from buffer input to buffer output
 *
 * The buffer input should have inchan channels worth of data, while output
 * should support outchan channels. However, this method is safe to call
 * in-place, provided that the output buffer is large enough to support the
 * new output data.
 *
 * This version of the method is optimized for the case in which input and
 * output are NOT the same buffer.  Otherwise, it delegates to either scale_up
 * or scale_down.
 *
 * @param distrib   The redistributor
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of audio frames to process
 */
static void apply_matrix(ATK_Redistributor* distrib, const float* input,
                         float* output, size_t size) {
    if (input == output) {
        if (distrib->outchan > distrib->inchan) {
            scale_up(distrib,input,output,size);
        } else {
            scale_down(distrib,input,output,size);
        }
    } else {
        Uint32 rows = distrib->outchan;
        Uint32 cols = distrib->inchan;

        const float* src = input;
        float* dst = output;
        float* matrix = distrib->matrix;
        size_t len = size;
        for(size_t ii = 0; ii < len; ii++, src += cols) {
            for(Uint32 jj = 0; jj < rows; jj++, dst += 1) {
                *dst = 0;
                for (Uint32 kk = 0; kk < cols; kk++) {
                    *dst += matrix[jj*cols+kk]*src[kk];
                }
            }
        }
    }
}

/**
 * Returns a newly allocated channel redistributor
 *
 * Redistribution works by using a matrix to redistribute the input channels,
 * in much the same way that a matrix decoder works. The value matrix should
 * be a MxN matrix in row major order, where N is the number of input channels
 * and M is the number of output channels.
 *
 * The matrix will be copied. The redistributor will not claim ownership of the
 * existing matrix. It is possible for matrix to be NULL. In that case, the
 * redistributor will use the default redistribution matrix.
 */
ATK_Redistributor* ATK_AllocRedistributor(Uint32 inchan, Uint32 outchan,
                                          const float* matrix) {
    float* copy = NULL;
    if (matrix != NULL) {
        size_t size = (inchan+1)*outchan;
        copy = (float*)ATK_malloc(size*sizeof(float));
        if (copy == NULL) {
            ATK_OutOfMemory();
            return NULL;
        }
        memcpy(copy,matrix,inchan*outchan*sizeof(float));
        memset(copy+inchan*outchan,0,outchan*sizeof(float));
    }

    ATK_Redistributor* result = (ATK_Redistributor*)ATK_malloc(sizeof(ATK_Redistributor));
    if (result == NULL) {
        ATK_OutOfMemory();
        if (copy != NULL) {
            ATK_free(copy);
        }
        return NULL;
    }

    result->inchan  = inchan;
    result->outchan = outchan;
    result->matrix  = copy;
    return result;
}

/**
 * Frees a previously allocated channel redistributor
 *
 * @param distrib   The redistributor state
 */
void ATK_FreeRedistributor(ATK_Redistributor* distrib) {
    if (distrib == NULL) {
        return;
    }
    if (distrib->matrix != NULL) {
        ATK_free(distrib->matrix);
        distrib->matrix = NULL;
    }
    ATK_free(distrib);
}

/**
 * Applies channel redistribution to input, storing the result in output.
 *
 * Frames is the number of audio frames, which is a collection of simultaneous
 * samples for each channel. Thus input should hold frames*inchan samples, while
 * output should be able to store frames*outchan samples (inchan and outchan
 * were specified when the redistributor was allocated).
 *
 * Redistributors are not stateful, and can freely be applied to multiple
 * streams.
 *
 * @param distrib   The redistributor state
 * @param input     The input stream
 * @param output    The output stream
 * @param frames    The number of audio frames to process
 *
 * @return the number of frames processed, or -1 on error
 */
Sint64 ATK_ApplyRedistributor(ATK_Redistributor* distrib,
                              const float* input, float* output, size_t frames) {
    if (distrib->matrix) {
        apply_matrix(distrib,input,output,frames);
        return frames;
    }

    switch(distrib->inchan) {
        case 1:
            convert_mono(input, output, distrib->outchan, frames);
            break;
        case 2:
            convert_stereo(input, output, distrib->outchan, frames);
            break;
        case 3:
            convert_21(input, output, distrib->outchan, frames);
            break;
        case 4:
            convert_quad(input, output, distrib->outchan, frames);
            break;
        case 5:
            convert_41(input, output, distrib->outchan, frames);
            break;
        case 6:
            convert_51(input, output, distrib->outchan, frames);
            break;
        case 7:
            convert_61(input, output, distrib->outchan, frames);
            break;
        case 8:
            convert_71(input, output, distrib->outchan, frames);
            break;
        default:
            ATK_SetError("Nonstandard input width %d requires an explicit matrix.",
                         distrib->inchan);
            return -1;
    }

    return frames;
}

/**
 * Returns the underlying matrix in a channel redistributor
 *
 * This value may be NULL
 *
 * @param distrib   The redistributor state
 */
const float* ATK_GetRedistributorMatrix(ATK_Redistributor* distrib) {
    if (distrib->matrix) {
        return distrib->matrix;
    }
    return NULL;
}

#pragma mark -
#pragma mark Audio Format Conversion
/**
 * Returns the number of bytes associated with the given audio format.
 *
 * @param format    The audio format
 *
 * @return the number of bytes associated with the given audio format.
 */
static size_t format_bytes(SDL_AudioFormat format) {
    switch (format) {
    case SDL_AUDIO_S8:
    case SDL_AUDIO_U8:
    case SDL_AUDIO_UNKNOWN:
        return 1;
    case SDL_AUDIO_S16LE:
    case SDL_AUDIO_S16BE:
        return 2;
    case SDL_AUDIO_S32LE:
    case SDL_AUDIO_S32BE:
    case SDL_AUDIO_F32LE:
    case SDL_AUDIO_F32BE:
        return 4;
    }
    return 1;
}

/**
 * Converts the audio data in input to the format required by output
 *
 * This version uses an intermediate buffer for multistep computation when
 * necessary. The intermediate buffer should be large enough to hold samples
 * number of floats. Note that this function is specified in
 * of the input data.
 *
 * Note the the length is in samples, not frames. We do not take channels into
 * account when performing a conversion -- only the data formats.
 *
 * It is safe for input and output (and buffer) to be the same buffer.
 *
 * @param input     The input data
 * @param informat  The input format
 * @param output    The output buffer
 * @param outformat The output format
 * @param buffer    The intermediate buffer
 * @param samples   The number of samples to convert
 *
 * @return true if conversion is successful
 */
static bool ATK_ConvertAudioFormatWithBuffer(const Uint8* input, SDL_AudioFormat informat,
                                             Uint8* output, SDL_AudioFormat outformat,
                                             Uint8* buffer, size_t samples) {
    switch (informat) {
    case SDL_AUDIO_S8:
    case SDL_AUDIO_UNKNOWN:
        convert_s8(input, output, buffer, outformat, samples);
        break;
    case SDL_AUDIO_U8:
        convert_u8(input, output, buffer, outformat, samples);
        break;
    case SDL_AUDIO_S16LE:
    case SDL_AUDIO_S16BE:
        if (informat != SDL_AUDIO_S16) {
            swapVec16((Uint16*)input, (Uint16*)buffer, samples);
            convert_s16(buffer, output, buffer, outformat, samples);
        } else {
            convert_s16(input, output, buffer, outformat, samples);
        }
        break;
    case SDL_AUDIO_S32LE:
    case SDL_AUDIO_S32BE:
        if (informat != SDL_AUDIO_S32) {
            swapVec32((Uint32*)input, (Uint32*)buffer, samples);
            convert_s32(buffer, output, buffer, outformat, samples);
        } else {
            convert_s32(input, output, buffer, outformat, samples);
        }
        break;
    case SDL_AUDIO_F32LE:
    case SDL_AUDIO_F32BE:
        if (informat != SDL_AUDIO_F32) {
            swapVec32((Uint32*)input, (Uint32*)buffer, samples);
            convert_f32(buffer, output, outformat, samples);
        } else {
            convert_f32(input, output, outformat, samples);
        }
        break;
    default:
        ATK_SetError("Unrecognized format %d",informat);
        return false;
    }
    return true;
}

/**
 * Converts the audio data in input to the format required by output
 *
 * Note the the length is in samples, not frames. We do not take channels into
 * account when performing a conversion -- only the data formats.
 *
 * It is safe for input and output to be the same buffer.
 *
 * @param input     The input data
 * @param informat  The input format
 * @param output    The output buffer
 * @param outformat The output format
 * @param samples   The number of samples to convert
 *
 * @return true if conversion is successful
 */
bool ATK_ConvertAudioFormat(const Uint8* input, SDL_AudioFormat informat,
                            Uint8* output, SDL_AudioFormat outformat,
                            size_t samples) {

    if (format_bytes(outformat) == 4) {
        // We can convert in-place if output is large enough
        return ATK_ConvertAudioFormatWithBuffer(input,informat,output,outformat,output,samples);
    }
    
    Uint8* buffer = (Uint8*)ATK_malloc(samples*sizeof(float));
    if (buffer == NULL) {
        ATK_OutOfMemory();
        return -1;
    }
    bool result = ATK_ConvertAudioFormatWithBuffer(input,informat,output,outformat,buffer,samples);
    ATK_free(buffer);
    return result;
}

#pragma mark -
#pragma mark Audio Spec Conversion
/**
 * A structure to convert audio data between different formats.
 *
 * This structure is an alternative to SDL_AudioCVT. We have had problems with
 * that structure in the past. While it is fine for format and channel conversion,
 * we have found it to be quite unreliable for rate conversion. As of SDL 2.0.14,
 * there was a bug in the resampler that could cause the converter to be caught
 * in an infinite zero-padding loop, resulting in the audio cutting out. While
 * this may have been fixed in more recent versions of SDL, we prefer this
 * version which gives us a little more control over the conversion process. In
 * particular, it is possible to convert audio before it is sent to the device.
 */
typedef struct ATK_AudioCVT {
    /** The input specification */
    SDL_AudioSpec input;
    /** The output specification */
    SDL_AudioSpec output;
    /** The circular buffer to store the input data stream (before format conversion)  */
    Uint8* incoming;
    /** An intermediate buffer to handle format conversions, resampling, and redistribution */
    Uint8* intermed;
    /** The buffer to store the output data stream (before format conversion) */
    Uint8* outgoing;
    /** The buffer capacity of thie CVT */
    size_t capacity;
    /** The ideal number of frames this CVT is designed for */
    size_t frames;
    /** The location of the next available read-location in incoming */
    size_t inhead;
    /** The location of the next available write-location in incoming */
    size_t intail;
    /** The number of bytes available for reading in incoming */
    size_t insize;
    /** The function for populating incoming (can be NULL) */
    ATK_AudioCallback callback;
    /** The resampler for frequency changes (can be NULL) */
    ATK_Resampler* resampler;
    /** The redistributor for channel changes (can be NULL) */
    ATK_Redistributor* distributor;
    /** User data for the callback (can be NULL) */
    void* userdata;
} ATK_AudioCVT;

/**
 * Fills the CVT buffer with the next page of data.
 *
 * If the buffer is full this function does nothing. The value returned is
 * the number of bytes available in the buffer, which may be more than len.
 *
 * @param cvt   The audio CVT
 * @param len   The number of bytes to read
 *
 * @return the number of bytes available in the input buffer
 */
static inline size_t fill_cvt_buffer(ATK_AudioCVT* cvt, size_t len) {
    if (len < cvt->insize) {
        return len;
    } else if (cvt->callback == NULL) {
        // Cannot read any more
        return cvt->insize;
    }

    size_t remain = len-cvt->insize;
    size_t upper  = cvt->capacity-cvt->intail;
    if (upper > remain) {
        upper = remain;
    }
    size_t lower = remain-upper;
    if (lower > cvt->inhead) {
        lower = cvt->inhead;
    }

    if (remain) {
        size_t actual = remain;
        actual = cvt->callback(cvt->userdata,cvt->incoming+cvt->intail,upper);
        cvt->intail += actual;
        if (actual == upper && lower) {
            cvt->intail = cvt->callback(cvt->userdata,cvt->incoming,lower);
            actual += cvt->intail;
        } else if (cvt->intail == cvt->capacity) {
            cvt->intail = 0;
        }

        cvt->insize += actual;
    }
    return cvt->insize;
}

/**
 * Converts the data in the input buffer into the stream
 *
 * This function assumes that the input buffer and the output buffer have
 * the same specification (format, channels, and frequency). It essentially
 * performs a memcopy from the incoming buffer to stream.
 *
 * If a callback function is defined, it will fill the incoming buffer to
 * satisfy the request for len bytes. Otherwise, it will rely on what is
 * currently available in the buffer.
 *
 * @param userdata  The audio CVT
 * @param stream    The stream to store the output
 * @param frames    The number of frames to process
 *
 * @return the number of frames written to the stream
 */
static size_t apply_cvt_direct(void* userdata, Uint8* stream, size_t frames) {
    ATK_AudioCVT* cvt = (ATK_AudioCVT*)userdata;

    size_t taken = 0;
    size_t limit = frames*(format_bytes(cvt->input.format)*cvt->input.channels);
    while (taken < limit) {
        size_t amt = fill_cvt_buffer(cvt,limit-taken);
        if (amt == 0) {
            return taken;
        }

        size_t upper = cvt->capacity-cvt->inhead;
        size_t lower = amt-upper;
        if (upper > amt) {
            upper = amt;
            lower = 0;
        }

        memcpy(stream,cvt->incoming+cvt->inhead,upper);
        cvt->inhead += upper;
        if (lower) {
            memcpy(stream+upper,cvt->incoming,lower);
            cvt->inhead = lower;
        } else if (cvt->inhead == cvt->capacity) {
            cvt->inhead = 0;
        }
        cvt->insize -= amt;
        taken += amt;
    }

    return taken/(format_bytes(cvt->input.format)*cvt->input.channels);
}

/**
 * Converts the data in the input buffer into the stream
 *
 * This function assumes that the input buffer and the output buffer have
 * different formats, but are otherwise (channels and frequency) the same.
 * This means that there may be a difference between the length of the
 * input data and the output data. In this case, len is specified in terms
 * of the input stream.
 *
 * If a callback function is defined, it will fill the incoming buffer to
 * satisfy the request for len bytes. Otherwise, it will rely on what is
 * currently available in the buffer.
 *
 * @param userdata  The audio CVT
 * @param stream    The stream to store the output
 * @param frames    The number of frames to process
 *
 * @return the number of frames written to the stream
 */
static inline size_t apply_cvt_format(void* userdata, Uint8* stream, SDL_AudioFormat format, size_t frames) {
    ATK_AudioCVT* cvt = (ATK_AudioCVT*)userdata;

    size_t inbytes  = format_bytes(cvt->input.format);
    size_t outbytes = format_bytes(format);

    size_t taken = 0;
    while (taken < frames) {
        size_t amt = (frames-taken)*(inbytes*cvt->input.channels);
        size_t pos = taken*(outbytes*cvt->output.channels);

        amt = fill_cvt_buffer(cvt,amt);
        if (amt == 0) {
            return taken;
        }

        size_t upper = cvt->capacity-cvt->inhead;
        size_t lower = amt-upper;
        if (upper > amt) {
            upper = amt;
            lower = 0;
        }

        if (!ATK_ConvertAudioFormatWithBuffer(cvt->incoming+cvt->inhead,cvt->input.format,
                                              stream+pos, format, cvt->intermed, upper/inbytes)) {
            return taken;
        }

        taken += upper/(inbytes*cvt->input.channels);
        cvt->inhead += upper;
        cvt->insize -= upper;
        if (cvt->inhead == cvt->capacity) {
            cvt->inhead = 0;
        }

        if (lower) {
            pos = taken*(outbytes*cvt->input.channels);
            if (!ATK_ConvertAudioFormatWithBuffer(cvt->incoming,cvt->input.format,
                                                  stream+pos, format, cvt->intermed, lower/inbytes)) {
                return taken;
            }
            cvt->inhead = lower;
            cvt->insize -= lower;
            taken += lower/(inbytes*cvt->input.channels);
        }
    }

    return taken;
}

/**
 * Converts the data in the input buffer into the stream
 *
 * This function assumes that the input buffer and the output buffer have
 * different channels and formats, but the frequency is the same. This means
 * that there may be a difference between the length of the input data and the
 * output data. In this case, len is specified in terms of the input stream.
 *
 * If a callback function is defined, it will fill the incoming buffer to
 * satisfy the request for len bytes.  Otherwise, it will rely on what is
 * currently available in the buffer.
 *
 * @param userdata  The audio CVT
 * @param stream    The stream to store the output
 * @param frames    The number of frames to process
 *
 * @return the number of frames written into stream
 */
static size_t apply_cvt_distribute(void* userdata, Uint8* stream, size_t frames) {
    ATK_AudioCVT* cvt = (ATK_AudioCVT*)userdata;

    size_t taken = 0;
    size_t inbytes = format_bytes(cvt->input.format);
    if (cvt->input.format == SDL_AUDIO_F32) {
        while (taken < frames) {
            Sint64 amt = (frames-taken);
            if (amt*(inbytes*cvt->input.channels) >= cvt->capacity) {
                amt = cvt->capacity/(inbytes*cvt->input.channels);
            }

            amt = apply_cvt_direct(userdata, cvt->intermed, amt);
            if (amt == 0) {
                return taken;
            }

            amt = ATK_ApplyRedistributor(cvt->distributor, (float*)cvt->intermed, (float*)(stream)+taken, amt);
            if (amt <= 0) {
                return taken;
            }

            taken += amt;
        }
    } else {
        while (taken < frames) {
            Sint64 amt = (frames-taken);
            if (amt*(inbytes*cvt->input.channels) >= cvt->capacity) {
                amt = cvt->capacity/(inbytes*cvt->input.channels);
            }

            amt = apply_cvt_format(userdata, cvt->intermed, SDL_AUDIO_F32, amt);
            if (amt == 0) {
                return taken;
            }

            amt = ATK_ApplyRedistributor(cvt->distributor, (float*)cvt->intermed, (float*)(stream)+taken, amt);
            if (amt <= 0) {
                return taken;
            }

            taken += amt;
        }
    }
    return taken;
}

/**
 * Converts the data in the input buffer into the stream
 *
 * This function assumes that the input buffer and the output buffer have
 * different frequencies, forcing the use of an intermediate resampler. This
 * means that there may be a difference between the length of the input data
 * and the output data. In this case, len is specified in terms of the input
 * stream.
 *
 * If a callback function is defined, it will fill the incoming buffer to
 * satisfy the request for len bytes.  Otherwise, it will rely on what is
 * currently available in the buffer.
 *
 * @param userdata  The audio CVT
 * @param stream    The stream to store the output
 * @param len       The number of input bytes to process
 *
 * @return the number of bytes written into stream
 */
static size_t apply_cvt_resample(void* userdata, Uint8* stream, size_t len) {
    ATK_AudioCVT* cvt = (ATK_AudioCVT*)userdata;

    int distrib = (cvt->distributor && cvt->input.channels > cvt->output.channels);
    
    // All other functions work in frames
    size_t frames = len/(sizeof(float)*cvt->input.channels);

    if (distrib) {
        return apply_cvt_distribute(userdata, stream, frames)*(sizeof(float)*cvt->output.channels);
    } else if (cvt->input.format == SDL_AUDIO_F32) {
        return apply_cvt_direct(userdata, stream, frames)*(sizeof(float)*cvt->input.channels);
    } else {
        return apply_cvt_format(userdata, stream, SDL_AUDIO_F32, frames)*(sizeof(float)*cvt->input.channels);
    }
}

/**
 * Returns a newly allocated ATK_AudioCVT to convert between audio specs
 *
 * It is not possible to change any of the CVT settings after it is allocated,
 * as the conversion stream is specifically tailored to these values. If you
 * need to change the settings, you should allocate a new resampler. It is the
 * responsiblity of the caller to use {@link ATK_FreeAudioCVT} to deallocate
 * the structure when done.
 *
 * @param def   The CVT settings
 *
 * @return a newly allocated ATK_AudioCVT to convert between audio specs
 */
ATK_AudioCVT* ATK_AllocAudioCVT(const ATK_AudioCVTDef* def) {
    if (def == NULL) {
        return NULL;
    }

    Uint8* incoming = NULL;
    Uint8* intermed = NULL;
    Uint8* outgoing = NULL;
    ATK_Resampler* resample = NULL;
    ATK_Redistributor* distrib = NULL;
    ATK_AudioCVT* result = NULL;

    size_t buffsize = def->frames*def->input.channels*format_bytes(def->input.format);
    incoming = (Uint8*)ATK_malloc(buffsize);
    if (incoming == NULL) {
        goto out;
    }
    memset(incoming,0,buffsize);

    size_t channels  = def->input.channels < def->output.channels ? def->output.channels : def->input.channels;
    intermed = (Uint8*)ATK_malloc(def->frames*channels*sizeof(float));
    if (intermed == NULL) {
        goto out;
    }
    memset(intermed,0,def->frames*channels*sizeof(float));

    outgoing = (Uint8*)ATK_malloc(def->frames*channels*sizeof(float));
    if (outgoing == NULL) {
        goto out;
    }
    memset(outgoing,0,def->frames*channels*sizeof(float));

    result = (ATK_AudioCVT*)ATK_malloc(sizeof(ATK_AudioCVT));
    if (result == NULL) {
        goto out;
    }

    if (def->input.channels != def->output.channels) {
        distrib = ATK_AllocRedistributor(def->input.channels, def->output.channels, NULL);
        if (distrib == NULL) {
            goto out;
        }
    }

    if (def->input.freq != def->output.freq) {
        ATK_ResamplerDef rdef;
        rdef.bitdepth  = ATK_RESAMPLE_BITDEPTH;
        rdef.stopband  = ATK_RESAMPLE_STOPBAND;
        rdef.zerocross = ATK_RESAMPLE_ZEROCROSS;
        rdef.inrate  = def->input.freq;
        rdef.outrate = def->output.freq;
        rdef.buffsize = (Uint32)def->frames;
        rdef.userdata = result;
        rdef.channels = def->input.channels >= def->output.channels ? def->output.channels : def->input.channels;
        rdef.callback = apply_cvt_resample;
        resample = ATK_AllocResampler(&rdef);
        if (resample == NULL) {
            goto out;
        }
    }

    result->input = def->input;
    result->output = def->output;
    result->callback = def->callback;
    result->userdata = def->userdata;
    result->incoming = incoming;
    result->intermed = intermed;
    result->outgoing = outgoing;
    result->resampler = resample;
    result->distributor = distrib;
    result->capacity = buffsize;
    result->frames  = def->frames;

    result->insize = 0;
    result->inhead = 0;
    result->intail = 0;
    return result;

out:
    ATK_OutOfMemory();
    if (result != NULL) {
        ATK_free(result);
    }
    if (distrib != NULL) {
        ATK_FreeRedistributor(distrib);
    }
    if (resample != NULL) {
        ATK_FreeResampler(resample);
    }
    if (outgoing != NULL) {
        ATK_free(outgoing);
    }
    if (intermed != NULL) {
        ATK_free(intermed);
    }
    if (incoming != NULL) {
        ATK_free(incoming);
    }
    return NULL;
}

/**
 * Frees a previously allocated audio CVT
 *
 * @param cvt   The audio CVT
 */
void ATK_FreeAudioCVT(ATK_AudioCVT* cvt) {
    if (cvt == NULL) {
        return;
    }
    ATK_free(cvt->incoming);
    cvt->incoming = NULL;
    ATK_free(cvt->intermed);
    cvt->intermed = NULL;
    ATK_free(cvt->outgoing);
    cvt->outgoing = NULL;
    if (cvt->resampler != NULL) {
        ATK_FreeResampler(cvt->resampler);
        cvt->resampler = NULL;
    }
    if (cvt->distributor != NULL) {
        ATK_FreeRedistributor(cvt->distributor);
        cvt->distributor = NULL;
    }
    ATK_free(cvt);
}

/**
 * Resets an audio CVT back to its initial (zero-padded) state.
 *
 * Specification converters have to keep state of the conversion performed so
 * far. This makes it not safe to use an audio CVT on multiple streams
 * simultaneously. Resetting an audio CVT zeroes the state so that it is the
 * same as if the converter were just allocated.
 *
 * @param cvt   The audio CVT
 */
void ATK_ResetAudioCVT(ATK_AudioCVT* cvt) {
    if (cvt->resampler != NULL) {
        ATK_ResetResampler(cvt->resampler);
    }

    size_t channels = cvt->input.channels < cvt->output.channels ? cvt->output.channels : cvt->input.channels;
    memset(cvt->incoming,0,cvt->capacity);
    memset(cvt->intermed,0,cvt->frames*channels*sizeof(float));
    memset(cvt->outgoing,0,cvt->frames*channels*sizeof(float));
    cvt->inhead = 0;
    cvt->intail = 0;
    cvt->insize = 0;
}

/**
 * Pulls converted data from the input buffer, populating it in output
 *
 * This function will convert up to len bytes, storing the result in output.
 * In line with SDL, we do not require that len represent a full audio frame,
 * or even a complete aligned sample.
 *
 * It is possible for this function to convert less than len bytes, particularly
 * if the buffer empties and there is no callback function to repopulate it. In
 * that case, the return value is the number of bytes read.
 *
 * @param cvt       The audio CVT
 * @param output    The output buffer
 * @param len       The number of bytes to convert
 *
 * @return the number of bytes read or -1 on error
 */
Sint64 ATK_PollAudioCVT(ATK_AudioCVT* cvt, Uint8* output, size_t len) {
    if (cvt == NULL) {
        return -1;
    }

    size_t inbytes  = format_bytes(cvt->input.format);
    size_t outbytes = format_bytes(cvt->output.format);

    // Make sure we read at aligned boundaries
    size_t over = len % (cvt->output.channels*outbytes);
    len -= (int)over;

    // Unroll all combinations
    size_t frames = len / (cvt->output.channels*outbytes);
    
    // Unroll the possibilities
    if (cvt->resampler == NULL && cvt->distributor == NULL) {
        return apply_cvt_format(cvt, output, cvt->output.format, frames)*cvt->output.channels*outbytes;
    } else if (cvt->resampler == NULL) {
        Sint64 taken = 0;
        while (taken < (Sint64)frames) {
            Sint64 amt = (frames-taken);
            Sint64 pos = taken*(cvt->output.channels*outbytes);
            if (amt*(cvt->input.channels*inbytes) > cvt->capacity) {
                amt = cvt->capacity/(cvt->input.channels*inbytes);
            }
            
            amt = apply_cvt_distribute(cvt, cvt->intermed, amt);
            if (amt == 0) {
                return taken*(cvt->output.channels*outbytes);
            }

            if (!ATK_ConvertAudioFormat(cvt->intermed, SDL_AUDIO_F32, output+pos, cvt->output.format, amt*cvt->output.channels)) {
                return -1;
            }

            taken += amt;
        }
        return taken*(cvt->output.channels*outbytes);
    } else if (cvt->distributor == NULL || cvt->input.channels > cvt->output.channels) {
        if (cvt->output.format == SDL_AUDIO_F32) {
            Sint64 taken = 0;
            while (taken < (Sint64)frames) {
                float* buffer = ((float*)output)+taken*cvt->output.channels;
                Sint64 amt = ATK_PollResampler(cvt->resampler,buffer, frames-taken);

                if (amt < 0) {
                    return -1;
                } else if (amt == 0) {
                    return taken*(cvt->output.channels*outbytes);
                }
                taken += amt;
            }
            return taken*(cvt->output.channels*outbytes);
        } else {
            Sint64 taken = 0;
            while (taken < (Sint64)frames) {
                Sint64 amt = cvt->frames;
                Sint64 pos = taken*(cvt->output.channels*outbytes);
                if (amt > (Sint64)(frames-taken)) {
                    amt = frames-taken;
                }

                amt = ATK_PollResampler(cvt->resampler, (float*)cvt->outgoing, amt);
                if (amt < 0) {
                    return -1;
                } else if (amt == 0) {
                    return taken*(cvt->output.channels*outbytes);
                }

                if (!ATK_ConvertAudioFormatWithBuffer(cvt->outgoing, SDL_AUDIO_F32, output+pos,
                                                      cvt->output.format, cvt->outgoing,
                                                      amt*cvt->output.channels)) {
                    return taken*(cvt->output.channels*outbytes);
                }
                taken += amt;
            }
            return taken*(cvt->output.channels*outbytes);
        }
    } else {
        // Distributor AFTER resampler
        if (cvt->output.format == SDL_AUDIO_F32) {
            Sint64 taken = 0;
            while (taken < (Sint64)frames) {
                Sint64 amt = cvt->frames;
                Sint64 pos = taken*(cvt->output.channels*outbytes);
                if (amt > (Sint64)(frames-taken)) {
                    amt = frames-taken;
                }

                amt = ATK_PollResampler(cvt->resampler, (float*)cvt->outgoing, amt);
                if (amt < 0) {
                    return -1;
                }
                amt = ATK_ApplyRedistributor(cvt->distributor, (float*)cvt->outgoing,
                                             (float*)(output+pos), amt);
                if (amt < 0) {
                    return -1;
                } else if (amt == 0) {
                    return taken*(cvt->output.channels*outbytes);
                }

                taken += amt;
            }
            return taken*(cvt->output.channels*outbytes);
        } else {
            Sint64 taken = 0;
            while (taken < (Sint64)frames) {
                Sint64 amt = cvt->frames;
                Sint64 pos = taken*(cvt->output.channels*outbytes);
                if (amt > (Sint64)(frames-taken)) {
                    amt = frames-taken;
                }

                amt = ATK_PollResampler(cvt->resampler, (float*)cvt->outgoing, amt);
                if (amt < 0) {
                    return -1;
                } if (amt == 0) {
                    return taken*(cvt->output.channels*outbytes);
                }
                
                amt = ATK_ApplyRedistributor(cvt->distributor, (float*)cvt->outgoing,
                                             (float*)cvt->outgoing, amt);
                if (amt < 0) {
                    return -1;
                } else if (amt == 0) {
                    return taken*(cvt->output.channels*outbytes);
                }

                if (!ATK_ConvertAudioFormatWithBuffer(cvt->outgoing, SDL_AUDIO_F32, output+pos,
                                                     cvt->output.format, cvt->outgoing,
                                                     amt*cvt->output.channels)) {
                    return taken*(cvt->output.channels*outbytes);
                }
                taken += amt;
            }
            return taken*(cvt->output.channels*outbytes);
        }
    }
    return 0;  // Should not happen
}

/**
 * Pushes data to the audio CVT buffer
 *
 * This is an optional way to repopulate the audio CVT buffer, particularly if
 * no callback function was specified at the time it was allocated. Data is does
 * not have to be pushed as complete audio frames, or even aligned samples.
 *
 * The limits on the buffer capacity may mean that not all data can be pushed
 * (particularly if this function is competing with a callback function). The
 * value returned is the number of bytes successfully stored in the buffer.
 *
 * @param cvt       The audio CVT
 * @param input     The input buffer
 * @param len       The number of bytes to push
 *
 * @return the number of bytes pushed or -1 on error
 */
Sint64 ATK_PushAudioCVT(ATK_AudioCVT* cvt, const Uint8* input, size_t len) {
    // Figure out what we can push
    size_t remain = cvt->capacity-cvt->insize;
    size_t upper  = cvt->capacity-cvt->intail;
    if (upper > remain) {
        upper = remain;
    }

    size_t lower = remain-upper;
    if (len < upper) {
        upper = len;
        lower = 0;
    } else if (len < remain) {
        lower = len-upper;
    }

    memcpy(cvt->incoming+cvt->intail,input,upper);
    cvt->intail += upper;
    if (lower) {
        memcpy(cvt->incoming,input+upper,lower);
        cvt->intail = lower;
    } else if (cvt->intail == cvt->capacity) {
        cvt->intail = 0;
    }

    cvt->insize += upper+lower;
    return upper+lower;
}
