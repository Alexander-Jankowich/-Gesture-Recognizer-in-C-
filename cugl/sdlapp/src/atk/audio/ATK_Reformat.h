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
#include <kiss_fft.h>
#include <kiss_fftr.h>

/**
 * @file ATK_Format
 *
 * This header provides the functions for converting from one audio format to
 * another. These functions are heavily adapted from SDL 2.26. The only
 * difference is that we have decoupled this functionality from the AudioCVT.
 */

#pragma mark -
#pragma mark Type Conversion
/**
 * Converts an audio buffer of signed bytes into a buffer of floats.
 *
 * The conversion is the usual one. It assumes that the floats are in the range
 * [-1,1] where -1 is equal to the minimum byte and 1 is equal to the maximum.
 *
 * Note the the buffer length is in samples, not frames or bytes. We do not take
 * channels into account when performing a conversion -- only the data formats.
 * We assume that both buffers are of the appropriate size to hold these
 * samples.
 *
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of samples to reformat
 */
static void s8_to_f32(const Uint8 *input, Uint8* output, size_t size) {
    const Sint8 *src = (Sint8 *)input+(size-1);
    float *dst = (float*)output+(size-1);

    for (size_t ii = 0; ii < size; ++ii, --src, --dst) {
        const Sint8 sample = *src;
        if (sample == -128) {
            *dst = -1.0f;
        } else {
            *dst = sample/127.0f;
        }
    }
}

/**
 * Converts an audio buffer of unsigned bytes into a buffer of floats.
 *
 * The conversion is the usual one. It assumes that the floats are in the range
 * [-1,1] where -1 is equal to the minimum byte and 1 is equal to the maximum.
 *
 * Note the the buffer length is in samples, not frames or bytes. We do not take
 * channels into account when performing a conversion -- only the data formats.
 * We assume that both buffers are of the appropriate size to hold these
 * samples.
 *
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of samples to reformat
 */
static void u8_to_f32(const Uint8 *input, Uint8* output, size_t size) {
    const Uint8 *src = input+(size-1);
    float *dst = (float*)output+(size-1);

    for (size_t ii = 0; ii < size; ++ii, --src, --dst) {
        const Uint8 sample = *src;
        if (sample == 0) {
            *dst = -1.0f;
        } else {
            *dst = (sample-128)/127.0;
        }
    }
}

/**
 * Converts an audio buffer of signed shorts into a buffer of floats.
 *
 * The conversion is the usual one. It assumes that the floats are in the range
 * [-1,1] where -1 is equal to the minimum short and 1 is equal to the maximum.
 *
 * If swap is true, then the endianness of the data will be reversed before it
 * it is stored in the output buffer.
 *
 * Note the the buffer length is in samples, not frames or bytes. We do not take
 * channels into account when performing a conversion -- only the data formats.
 * We assume that both buffers are of the appropriate size to hold these
 * samples.
 *
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of samples to reformat
 */
static void s16_to_f32(const Uint8 *input, Uint8* output, size_t size) {
    const Sint16 *src = ((Sint16 *)input)+(size-1);
    float *dst = ((float*)output)+(size-1);

    for (size_t ii = 0; ii < size; ++ii, --src, --dst) {
        Sint16 sample = *src;
        if (sample == -32768) {
            *dst = -1.0f;
        } else {
            *dst = sample/32767.0f;
        }
    }
}

/**
 * Converts an audio buffer of signed ints into a buffer of floats.
 *
 * The conversion is the usual one. It assumes that the floats are in the range
 * [-1,1] where -1 is equal to the minimum int and 1 is equal to the maximum.
 *
 * Note the the buffer length is in samples, not frames or bytes. We do not take
 * channels into account when performing a conversion -- only the data formats.
 * We assume that both buffers are of the appropriate size to hold these
 * samples.
 *
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of samples to reformat
 */
static void s32_to_f32(const Uint8 *input, Uint8* output, size_t size) {
    const Sint32 *src = (Sint32 *)input;
    float *dst = (float*)output;
    float DIVBY8388607 = (float)(1.0/8388607.0);

    for (size_t ii = 0; ii < size; ++ii, ++src, ++dst) {
        Sint32 sample = *src;
        *dst =((float) (sample>>8)) * DIVBY8388607;
    }
}

/**
 * Converts an audio buffer of floats into a buffer of signed bytes.
 *
 * The conversion is the usual one. It assumes that the floats are in the range
 * [-1,1] where -1 converts to the minimum byte and 1 converts to the maximum.
 *
 * Note the the buffer length is in samples, not frames or bytes. We do not take
 * channels into account when performing a conversion -- only the data formats.
 * We assume that both buffers are of the appropriate size to hold these
 * samples.
 *
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of samples to reformat
 */
static void f32_to_s8(const Uint8* input, Uint8* output, size_t size) {
    const float *src = (float *)input;
    Sint8 *dst = (Sint8 *)output;

    for (size_t ii = 0; ii < size; ++ii, ++src, ++dst) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 127;
        } else if (sample <= -1.0f) {
            *dst = -128;
        } else {
            *dst = (Sint8)(sample * 127.0f);
        }
    }
}

/**
 * Converts an audio buffer of floats into a buffer of unsigned bytes.
 *
 * The conversion is the usual one. It assumes that the floats are in the range
 * [-1,1] where -1 converts to the minimum byte and 1 converts to the maximum.
 *
 * Note the the buffer length is in samples, not frames or bytes. We do not take
 * channels into account when performing a conversion -- only the data formats.
 * We assume that both buffers are of the appropriate size to hold these
 * samples.
 *
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of samples to reformat
 */
static void f32_to_u8(const Uint8 *input, Uint8* output, size_t size) {
    const float *src = (float*)input;
    Uint8 *dst = output;

    for (size_t ii = 0; ii < size; ++ii, ++src, ++dst) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 255;
        } else if (sample <= -1.0f) {
            *dst = 0;
        } else {
            *dst = (Uint8)((sample * 127.0f) + 128.0f);
        }
    }
}

/**
 * Converts an audio buffer of floats into a buffer of signed shorts.
 *
 * The conversion is the usual one. It assumes that the floats are in the range
 * [-1,1] where -1 converts to the minimum short and 1 converts to the maximum.
 *
 * Note the the buffer length is in samples, not frames or bytes. We do not take
 * channels into account when performing a conversion -- only the data formats.
 * We assume that both buffers are of the appropriate size to hold these
 * samples.
 *
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of samples to reformat
 */
static void f32_to_s16(const Uint8 *input, Uint8* output, size_t size) {
    const float *src = (float*)input;
    Sint16 *dst = (Sint16 *)output;

    for (size_t ii = 0; ii < size; ++ii, ++src, ++dst) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 32767;
        } else if (sample <= -1.0f) {
            *dst = -32768;
        } else {
            *dst = (Sint16)(sample * 32767.0f);
        }
    }
}

/**
 * Converts an audio buffer of floats into a buffer of signed ints.
 *
 * The conversion is the usual one. It assumes that the floats are in the range
 * [-1,1] where -1 converts to the minimum int and 1 converts to the maximum.
 *
 * Note the the buffer length is in samples, not frames or bytes. We do not take
 * channels into account when performing a conversion -- only the data formats.
 * We assume that both buffers are of the appropriate size to hold these
 * samples.
 *
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of samples to reformat
 */
static void f32_to_s32(const Uint8 *input, Uint8* output, size_t size) {
    const float *src = (float*)input;
    Sint32 *dst = (Sint32 *) output;

    for (size_t ii = 0; ii < size; ++ii, ++src, ++dst) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 2147483647;
        } else if (sample <= -1.0f) {
            *dst = (Sint32) -2147483648LL;
        } else {
            Sint32 temp = (Sint32)(sample * 8388607.0f);
            *dst = (Sint32)((Uint32)temp << 8);
        }
    }
}

#pragma mark -
#pragma mark Endian Swaps
/**
 * Swaps the endianness of input, storing the result in output.
 *
 * This function is designed to be used on any 16-bit datatype. It is safe
 * for input and output to be the same buffer.
 *
 * @param input     The input data
 * @param output    The output buffer
 * @param size      The number of elements to swap
 */
static void swapVec16(const Uint16 *input, Uint16* output, size_t size) {
    const Uint16* src = input;
    Uint16* dst = output;
    while (size--) {
        *dst++ = SDL_Swap16(*src++);
    }
}

/**
 * Swaps the endianness of input, storing the result in output.
 *
 * This function is designed to be used on any 32-bit datatype. It is safe
 * for input and output to be the same buffer.
 *
 * @param input     The input data
 * @param output    The output buffer
 * @param size      The number of elements to swap
 */
static void swapVec32(const Uint32 *input, Uint32* output, size_t size) {
    const Uint32* src = input;
    Uint32* dst = output;
    while (size--) {
        *dst++ = SDL_Swap32(*src++);
    }
}

#pragma mark -
#pragma mark Grouped Conversion
/**
 * Converts the signed bytes in input to the desired format in output.
 *
 * Sometimes this conversion is multistep. That is the purpose of the
 * intermediate buffer. This buffer should be large enough to hold size
 * many floats.
 *
 * @param input     The input data
 * @param output    The output buffer
 * @param buffer    The intermediate buffer
 * @param format    The output format
 * @param size      The number of samples to reformat
 */
static void convert_s8(const Uint8 *input, Uint8* output, Uint8* buffer,
                       SDL_AudioFormat format, size_t size) {
    switch (format) {
    case SDL_AUDIO_S8:
    case SDL_AUDIO_UNKNOWN:
        if (output != input) {
            memcpy(output,input,size);
        }
        return;
    case SDL_AUDIO_U8:
        s8_to_f32(input,buffer,size);
        f32_to_u8(buffer,output,size);
        break;
    case SDL_AUDIO_S16LE:
    case SDL_AUDIO_S16BE:
        s8_to_f32(input,buffer,size);
        f32_to_s16(buffer,output,size);
        if (format != SDL_AUDIO_S16) {
            swapVec16((Uint16*)output,(Uint16*)output,size);
        }
        break;
    case SDL_AUDIO_S32LE:
    case SDL_AUDIO_S32BE:
        s8_to_f32(input,buffer,size);
        f32_to_s32(buffer,output,size);
        if (format != SDL_AUDIO_S32) {
            swapVec32((Uint32*)output,(Uint32*)output,size);
        }
        break;
    case SDL_AUDIO_F32LE:
    case SDL_AUDIO_F32BE:
        s8_to_f32(input,output,size);
        if (format != SDL_AUDIO_F32) {
            swapVec32((Uint32*)output,(Uint32*)output,size);
        }
        break;
    }
}

/**
 * Converts the unsigned bytes in input to the desired format in output.
 *
 * Sometimes this conversion is multistep. That is the purpose of the
 * intermediate buffer. This buffer should be large enough to hold size
 * many floats.
 *
 * @param input     The input data
 * @param output    The output buffer
 * @param buffer    The intermediate buffer
 * @param format    The output format
 * @param size      The number of samples to reformat
 */
static void convert_u8(const Uint8 *input, Uint8* output, Uint8* buffer,
                       SDL_AudioFormat format, size_t size) {
    switch (format) {
    case SDL_AUDIO_S8:
    case SDL_AUDIO_UNKNOWN:
        u8_to_f32(input,buffer,size);
        f32_to_s8(buffer,output,size);
        break;
    case SDL_AUDIO_U8:
        if (output != input) {
            memcpy(output,input,size);
        }
        return;
    case SDL_AUDIO_S16LE:
    case SDL_AUDIO_S16BE:
        u8_to_f32(input,buffer,size);
        f32_to_s16(buffer,output,size);
        if (format != SDL_AUDIO_S16) {
            swapVec16((Uint16*)output,(Uint16*)output,size);
        }
        break;
    case SDL_AUDIO_S32LE:
    case SDL_AUDIO_S32BE:
        u8_to_f32(input,buffer,size);
        f32_to_s32(buffer,output,size);
        if (format != SDL_AUDIO_S32) {
            swapVec32((Uint32*)output,(Uint32*)output,size);
        }
        break;
    case SDL_AUDIO_F32LE:
    case SDL_AUDIO_F32BE:
        u8_to_f32(input,output,size);
        if (format != SDL_AUDIO_F32) {
            swapVec32((Uint32*)output,(Uint32*)output,size);
        }
        break;
    }
}

/**
 * Converts the signed shorts in input to the desired format in output.
 *
 * The shorts are assumed to be in the native byte order.
 *
 * Sometimes this conversion is multistep. That is the purpose of the
 * intermediate buffer. This buffer should be large enough to hold size
 * many floats.
 *
 * @param input     The input data
 * @param output    The output buffer
 * @param buffer    The intermediate buffer
 * @param format    The output format
 * @param size      The number of samples to reformat
 */
static void convert_s16(const Uint8 *input, Uint8* output, Uint8* buffer,
                        SDL_AudioFormat format, size_t size) {
    switch (format) {
    case SDL_AUDIO_S8:
    case SDL_AUDIO_UNKNOWN:
        s16_to_f32(input,buffer,size);
        f32_to_s8(buffer,output,size);
        break;
    case SDL_AUDIO_U8:
        s16_to_f32(input,buffer,size);
        f32_to_u8(buffer,output,size);
        return;
    case SDL_AUDIO_S16LE:
    case SDL_AUDIO_S16BE:
        if (format != SDL_AUDIO_S16) {
            swapVec16((Uint16*)input,(Uint16*)output,size);
        } else if (input != output) {
            memcpy(output,input,size*sizeof(Sint16));
        }
        break;
    case SDL_AUDIO_S32LE:
    case SDL_AUDIO_S32BE:
        s16_to_f32(input,buffer,size);
        f32_to_s32(buffer,output,size);
        if (format != SDL_AUDIO_S32) {
            swapVec32((Uint32*)output,(Uint32*)output,size);
        }
        break;
    case SDL_AUDIO_F32LE:
    case SDL_AUDIO_F32BE:
        s16_to_f32(input,output,size);
        if (format != SDL_AUDIO_F32) {
            swapVec32((Uint32*)output,(Uint32*)output,size);
        }
        break;
    }
}

/**
 * Converts the signed ints in input to the desired format in output.
 *
 * The ints are assumed to be in the native byte order.
 *
 * Sometimes this conversion is multistep. That is the purpose of the
 * intermediate buffer. This buffer should be large enough to hold size
 * many floats.
 *
 * @param input     The input data
 * @param output    The output buffer
 * @param buffer    The intermediate buffer
 * @param format    The output format
 * @param size      The number of samples to reformat
 */
static void convert_s32(const Uint8 *input, Uint8* output, Uint8* buffer,
                        SDL_AudioFormat format, size_t size) {
    switch (format) {
    case SDL_AUDIO_S8:
    case SDL_AUDIO_UNKNOWN:
        s32_to_f32(input,buffer,size);
        f32_to_s8(buffer,output,size);
        break;
    case SDL_AUDIO_U8:
        s32_to_f32(input,buffer,size);
        f32_to_u8(buffer,output,size);
        return;
    case SDL_AUDIO_S16LE:
    case SDL_AUDIO_S16BE:
        s32_to_f32(input,buffer,size);
        f32_to_s16(buffer,output,size);
        if (format != SDL_AUDIO_S16) {
            swapVec16((Uint16*)output,(Uint16*)output,size);
        }
        break;
    case SDL_AUDIO_S32LE:
    case SDL_AUDIO_S32BE:
        if (format != SDL_AUDIO_S32) {
            swapVec32((Uint32*)input,(Uint32*)output,size);
        } else if (input != output) {
            memcpy(output,input,size*sizeof(Sint32));
        }
        break;
    case SDL_AUDIO_F32LE:
    case SDL_AUDIO_F32BE:
        s32_to_f32(input,output,size);
        if (format != SDL_AUDIO_F32) {
            swapVec32((Uint32*)output,(Uint32*)output,size);
        }
        break;
    }
}

/**
 * Converts the floats in input to the desired format in output.
 *
 * The floats are assumed to be in the native byte order.
 *
 * This function never needs and intermediate buffer, as output is large
 * enough for calculations in place.
 *
 * @param input     The input data
 * @param output    The output buffer
 * @param format    The output format
 * @param size      The number of samples to reformat
 */
static void convert_f32(const Uint8 *input, Uint8* output,
                        SDL_AudioFormat format, size_t size) {
    switch (format) {
    case SDL_AUDIO_S8:
    case SDL_AUDIO_UNKNOWN:
        f32_to_s8(input,output,size);
        break;
    case SDL_AUDIO_U8:
        f32_to_u8(input,output,size);
        return;
    case SDL_AUDIO_S16LE:
    case SDL_AUDIO_S16BE:
        f32_to_s16(input,output,size);
        if (format != SDL_AUDIO_S16) {
            swapVec16((Uint16*)output,(Uint16*)output,size);
        }
        break;
    case SDL_AUDIO_S32LE:
    case SDL_AUDIO_S32BE:
        f32_to_s32(input,output,size);
        if (format != SDL_AUDIO_S32) {
            swapVec32((Uint32*)output,(Uint32*)output,size);
        }
        break;
    case SDL_AUDIO_F32LE:
    case SDL_AUDIO_F32BE:
        if (format != SDL_AUDIO_F32) {
            swapVec32((Uint32*)input,(Uint32*)output,size);
        } else if (input != output) {
            memcpy(output,input,size*sizeof(float));
        }
        break;
    }
}
