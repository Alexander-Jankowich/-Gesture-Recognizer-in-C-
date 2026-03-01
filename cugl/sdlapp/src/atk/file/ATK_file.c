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
#include "ATK_file_c.h"

#pragma mark Managed Files
// String to reference state from the IOStream
#define ATK_IO_STATE_PROPERTY "edu.cornell.gdiac.atk.iostate"

/**
 * Loads an inactive ATK_IOState into memory
 *
 * This function assumes that there is capacity in the associated file pool to
 * load in the file. See {@link ATK_ActivateIO} for a version that will make
 * room in the file pool if necessary
 *
 * This function does nothing if the ATK_IOState is already active.
 *
 * @param state the ATK_IOState to load into memory
 *
 * @return true on success
 */
static bool ATK_LoadIO(ATK_IOState* state) {
    if (state->active) {
        // Nothing to do
        return true;
    }
    
    char mode[4];
    memset(mode, 0, 4);
    mode[0] = 'r';
    int next = 1;
    if (state->mode & ATK_IO_MODE_WRITE) {
        mode[next++] = '+';
    } else if (state->mode & ATK_IO_MODE_APPEND) {
        mode[0] = 'a';
        mode[1] = '+';
        next = 2;
    }
    if (state->mode & ATK_IO_MODE_BINARY) {
        mode[next] = 'b';
    }
    
    SDL_IOStream* source = SDL_IOFromFile(state->name,mode);
    if (source == NULL || SDL_SeekIO(source, state->pos, SDL_IO_SEEK_SET) < 0) {
        SDL_CloseIO(source);
        return false;
    }

    state->source = source;
    state->active = true;
    state->touch  = true;
    state->pool->active++;
    return true;
}

/**
 * Deactivates a file in the file pool, making room for another
 *
 * This function uses a classic LRU clock algorithm to find the
 * next file to deactivate. It fails if there are no active files.
 *
 * @param pool  the file pool to evict from
 *
 * @return true on success
 */
static bool ATK_PageOutFile(ATK_FilePool* pool) {
    if (pool->head == NULL) {
        ATK_SetError("Attempt to page out an empty file pool");
        return -1;
    }
    if (pool->evict == NULL) {
        pool->evict = pool->head;
    }
    
    // Classic clock-style LRU algorithm
    ATK_IOState* state = (ATK_IOState*)pool->evict->state;
    size_t inactives = 0;
    // Allow us to loop through twice to apply the clock
    while ((!state->active || state->touch) && inactives < 2*pool->total) {
        if (!state->active) {
            inactives++;
        }
        state->touch = false;
        pool->evict = pool->evict->next;
        state = (ATK_IOState*)pool->evict->state;
    }
    ATK_IOState* curr = (ATK_IOState*)pool->evict->state;
    pool->evict = pool->evict->next;
    return curr->active && ATK_DeactivateIO(curr);
}

/**
 * Internal implementation of SDL_SizeIO
 *
 * This function returns the total size of the file in bytes.
 *
 * @param context a pointer to an ATK_IOState structure
 *
 * @return the total size of the file in bytes or -1 on error
 */
static Sint64 ATK_IOStateSize(void* context) {
    if (context == NULL) {
        ATK_SetError("Attempted to query a null context");
        return -1;
    }
    
    ATK_IOState* state = (ATK_IOState*)context;
    if (!ATK_ActivateIO(state)) {
        return -1;
    }
    return SDL_GetIOSize(state->source);
}

/**
 * Internal implementation of SDL_SeekIO
 *
 * This function seeks within an ATK_IOState reference
 *
 * @param context   a pointer to an ATK_IOState structure
 * @param offset    an offset in bytes, relative to whence location; can be negative
 * @param whence    any of SDL_IO_SEEK_SET, SDL_IO_SEEK_CUR, SDL_IO_SEEK_END
 *
 * @return the final offset in the file after the seek or -1 on error
 */
static Sint64 ATK_IOStateSeek(void* context, Sint64 offset, SDL_IOWhence whence) {
    if (context == NULL) {
        ATK_SetError("Attempted to seek a null context");
        return -1;
    }

    ATK_IOState* state = (ATK_IOState*)context;
    if (!ATK_ActivateIO(state)) {
        return -1;
    }
    return SDL_SeekIO(state->source, offset, whence);
}

/**
 * Internal implementation of SDL_ReadIO
 *
 * This function reads from an ATK_IOState reference
 *
 * @param context   a pointer to an ATK_IOState structure
 * @param ptr       a pointer to a buffer to read data into
 * @param size      the size of each object to read, in bytes
 * @param status    the status on an error result
 *
 * @return the number of objects read, or 0 at error or end of file
 */
static size_t ATK_IOStateRead(void *context, void *ptr, size_t size, SDL_IOStatus* status) {
    if (context == NULL) {
        ATK_SetError("Attempted to read a null context");
        *status = SDL_IO_STATUS_ERROR;
        return 0;
    }
    
    ATK_IOState* state = (ATK_IOState*)context;
    if (!ATK_ActivateIO(state)) {
        *status = SDL_IO_STATUS_NOT_READY;
        return 0;
    }

    return SDL_ReadIO(state->source, ptr, size);
}

/**
 * Internal implementation of SDL_FlushIO
 *
 * This function flushes an ATK_IOState reference
 *
 * @param context   a pointer to an ATK_IOState structure
 * @param status    the status on an error result
 *
 * @return true on success
 */
static bool ATK_IOStateFlush(void *context, SDL_IOStatus* status) {
    if (context == NULL) {
        ATK_SetError("Attempted to read a null context");
        *status = SDL_IO_STATUS_ERROR;
        return false;
    }
    
    ATK_IOState* state = (ATK_IOState*)context;
    if (!ATK_ActivateIO(state)) {
        *status = SDL_IO_STATUS_NOT_READY;
        return false;
    }

    return SDL_FlushIO(state->source);
}

/**
 * Internal implementation of SDL_WriteIO
 *
 * This function writes to an ATK_IOState reference
 *
 * @param context   a pointer to an ATK_IOState structure
 * @param ptr       a pointer to a buffer containing data to write
 * @param size      the size of each object to write, in bytes
 * @param status    the status on an error result
 *
 * @return the number of objects written, which is less than num on error
 */
static size_t ATK_IOStateWrite(void *context, const void *ptr, size_t size, SDL_IOStatus* status) {
    if (context == NULL) {
        ATK_SetError("Attempted to read a null context");
        *status = SDL_IO_STATUS_ERROR;
        return 0;
    }
    
    ATK_IOState* state = (ATK_IOState*)context;
    if (!ATK_ActivateIO(state)) {
        *status = SDL_IO_STATUS_NOT_READY;
        return 0;
    }

    return SDL_WriteIO(state->source, ptr, size);
}


/**
 * Internal implementation of SDL_CloseIO
 *
 * This function closes and frees an allocated ATK_IOState reference
 *
 * @param context   a pointer to an ATK_IOState structure to close
 *
 * @return true on success
 */
static bool ATK_IOStateClose(void *context) {
    if (context == NULL) {
        ATK_SetError("Attempted to close a null context");
        return false;
    }

    ATK_IOState* state = (ATK_IOState*)context;
    ATK_FilePool* pool = state->pool;
    SDL_LockMutex(pool->mutex);
    if (state->active) {
        pool->total--;
        pool->active--;
    }
    
    SDL_IOStream* source = state->source;
    if (source != NULL) {
        int val = SDL_CloseIO(source);
        if (val < 0) {
            SDL_UnlockMutex(pool->mutex);
            return val;
        }
    }
    state->source = NULL;
    
    ATK_FileNode* node = state->node;
    if (node->next != node) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
        if (node == pool->head) {
            pool->head = node->next;
        }
        if (node == pool->tail) {
            pool->tail = node->prev;
        }
        if (node == pool->evict) {
            pool->evict = NULL;
        }
    } else {
        pool->head = NULL;
        pool->tail = NULL;
        pool->evict = NULL;
    }
    SDL_UnlockMutex(pool->mutex);

    // Time to delete the state
    node->prev  = NULL;
    node->next  = NULL;
    node->state = NULL;
    ATK_free(node);
    
    ATK_free(state->name);
    state->name = NULL;
    state->pool = NULL;
    state->node = NULL;
    ATK_free(state);
    return true;
}

/**
 * Deactivates an active ATK_IOState
 *
 * This function saves the current state of the ATK_IOState and pages it
 * out, making room for more active files. This function does nothing
 * if the file is not active.
 *
 * @param context   the ATK_IOState to deactivate
 *
 * @return true on success
 */
bool ATK_DeactivateIO(ATK_IOState* context) {
    if (context == NULL) {
        ATK_SetError("Attempted to page out a null context");
        return false;
    }
    
    if (!context->active) {
        // Nothing to do
        return true;
    }
    
    SDL_IOStream* source = context->source;
    Sint64 pos = SDL_TellIO(source);
    if (pos < 0 || !SDL_CloseIO(source)) {
        return false;
    }
    
    context->source = NULL;
    context->active = false;
    context->pos = (size_t)pos;
    context->pool->active--;
    return true;
}

/**
 * Activates a file in the file pool, loading it into memory
 *
 * If the file pool is already at capacity, this function will instruct
 * the file pool to deactivate another file, to make room. If no file
 * can be deactivated, this function will fail.
 *
 * @param context   the ATK_IOState to activate
 *
 * @return true on success
 */
bool ATK_ActivateIO(ATK_IOState* context) {
    if (context->active) {
        context->touch = true;
        return true;
    }

    ATK_FilePool* pool = context->pool;
    SDL_LockMutex(pool->mutex);
    int result = 0;
    if (pool->active >= pool->capacity && !ATK_PageOutFile(pool)) {
        result = false;
    } else {
        result = ATK_LoadIO(context);
    }
    SDL_UnlockMutex(pool->mutex);
    return result;
}

#pragma mark -
#pragma mark File Pools
/** The "hidden" default file pool. Use ATK_DefaultFilePool to access. */
ATK_FilePool* _atk_default_pool = NULL;

/**
 * Initializes the managed file subsystem for ATK
 *
 * All of the ATK codec functions have the option to use a default managed
 * file subsystem, guaranteeing that we will never run out of file handles
 * as long as there is suitable memory. Calling this function will initialize
 * this subsystem.
 *
 * If this function is not called, all ATK codec functions will use the OS
 * for accessing files. This means that it is possible for a codec function
 * to fail if there are too many open files at once.
 *
 * @param capacity  The maximum number of active files in the pool
 *
 * @return true on success
 */
bool ATK_init(Uint32 capacity) {
    if (_atk_default_pool != NULL) {
        ATK_SetError("ATK subsystem already initialized");
        return false;
    }
    
    _atk_default_pool = ATK_AllocFilePool(capacity);
    return _atk_default_pool != NULL;
}

/**
 * Shuts down the managed file subsystem for ATK
 *
 * Any files associated with the managed file subsystem will be immediately
 * closed and disposed. This function does nothing if the subsystem was not
 * initialized.
 */
void ATK_quit(void) {
    if (_atk_default_pool != NULL) {
        ATK_FreeFilePool(_atk_default_pool);
        _atk_default_pool = NULL;
    }
    kiss_fft_cleanup(); // Just in case
}

/**
 * Returns the default managed file subsystem for ATK
 *
 * This is the managed file pool used by all ATK codec functions. If it is
 * null, those functions will all use the OS for accessing files instead.
 *
 * @return the default managed file subsystem for ATK
 */
ATK_FilePool* ATK_DefaultFilePool(void) {
    return _atk_default_pool;
}

/**
 * Returns a newly allocated file pool
 *
 * The file pool will only allow capacity many files to be active at once.
 * Note that this capacity is subject to the same file handle limits as
 * everything else. In particular, if the capacity exceeds the limit of
 * the number of simultaneously open files, it can still fail to open
 * files when there is too much demand. In addition, if there are multiple
 * file pools, their capacity should not sum more than the open file limit.
 *
 * @param capacity  The maximum number of active files in the pool
 *
 * @return a newly allocated file pool (or NULL on failure)
 */
ATK_FilePool* ATK_AllocFilePool(Uint32 capacity) {
    if (capacity == 0) {
        ATK_SetError("ATK capacity must be non-zero");
        return NULL;
    }
    
    ATK_FilePool* pool = ATK_malloc(sizeof(ATK_FilePool));
    if (pool) {
        memset(pool, 0, sizeof(ATK_FilePool));
        pool->capacity = capacity;
        pool->mutex = SDL_CreateMutex();
    }
    return pool;
}

/**
 * Frees a previously allocated file pool
 *
 * Any files associated with the managed file subsystem will be immediately
 * closed and disposed.
 *
 * @param pool  The file pool to dispose
 */
void ATK_FreeFilePool(ATK_FilePool* pool) {
    if (pool == NULL) {
        return;
    }
    
    while (pool->tail) {
        ATK_IOStateClose(pool->tail->state);
    }
    
    SDL_DestroyMutex(pool->mutex);
    pool->mutex = NULL;
    ATK_free(pool);
}

/**
 * Returns a newly opened ATK_IOStream from a named file
 *
 * This function is, for all intents and purposes, equivalent to SDL_IOFromFile.
 * It supports exactly the same file modes. The only difference is that the
 * file is associated with the given file pool.
 *
 * @param file  a UTF-8 string representing the filename to open
 * @param mode  an ASCII string representing the mode to be used for opening the file
 * @param pool  the associated file pool
 *
 * @return a newly opened ATK_RWops from a named file (or NULL on failure)
 */
ATK_IOStream* ATK_IOFromFilePool(const char *file, const char *mode, ATK_FilePool* pool) {
    // Need to initially open the file
    if (pool == NULL) {
        ATK_SetError("File pool is NULL");
        return NULL;
    } else if (pool->active >= pool->capacity && !ATK_PageOutFile(pool)) {
        return NULL;
    }
    
    // Make sure we can load the file normally.
    ATK_IOStream* source = SDL_IOFromFile(file, mode);
    if (source == NULL) {
        SDL_Log("%s",SDL_GetError());
        return NULL;
    }
    
    ATK_IOStream* result = NULL;

    ATK_IOState* state = NULL;
    ATK_FileNode* node = NULL;
    char* namecopy =  NULL;
    
    state = ATK_malloc(sizeof(ATK_IOState));
    if (state == NULL) {
        ATK_OutOfMemory();
        goto fail;
    }
    memset(state, 0, sizeof(ATK_IOState));

    node = ATK_malloc(sizeof(ATK_FileNode));
    if (node == NULL) {
        ATK_OutOfMemory();
        goto fail;
    }
    memset(node, 0, sizeof(ATK_FileNode));

    namecopy = ATK_malloc(strlen(file)+1);
    if (namecopy == NULL) {
        ATK_OutOfMemory();
        goto fail;
    }
    memset(namecopy, 0, strlen(file)+1);
    strcpy(namecopy,file);
    
    Uint32 bitset = 0;
    for(const char* c = mode; *c != '\0'; c++) {
        switch(*c) {
            case 'r':
                bitset |= ATK_IO_MODE_READ;
                break;
            case 'w':
                bitset |= ATK_IO_MODE_WRITE;
                break;
            case 'a':
                bitset |= ATK_IO_MODE_APPEND;
                break;
            case 'b':
                bitset |= ATK_IO_MODE_BINARY;
                break;
            case '+':
                if (bitset & ATK_IO_MODE_READ) {
                    bitset |= ATK_IO_MODE_WRITE;
                } else {
                    bitset |= ATK_IO_MODE_READ;
                }
                break;
        }
    }
    
    SDL_LockMutex(pool->mutex);
    
    state->name   = namecopy;
    state->node   = node;
    state->mode   = bitset;
    state->active = true;
    state->touch  = true;
    state->pool   = pool;
    state->source = source;
    
    node->state  = state;
    if (pool->tail != NULL) {
        node->prev = pool->tail;
        node->next = pool->head;
        pool->tail->next = node;
        pool->head->prev = node;
        if (pool->head->next == pool->head) {
            pool->head->next = node;
        }
    } else {
        node->next = node;
        node->prev = node;
        pool->head = node;
    }
    pool->tail = node;
    pool->active++;
    pool->total++;

    SDL_UnlockMutex(pool->mutex);
    
    // Now lets allocate the actual stream
    SDL_IOStreamInterface iface;
    SDL_INIT_INTERFACE(&iface);
    iface.size = ATK_IOStateSize;
    iface.seek = ATK_IOStateSeek;
    iface.read = ATK_IOStateRead;
    iface.write = ATK_IOStateWrite;
    iface.flush = ATK_IOStateFlush;
    iface.close = ATK_IOStateClose;

    result = SDL_OpenIO(&iface, state);
    if (!result) {
        goto fail;
    }
    
    SDL_PropertiesID props = SDL_GetIOProperties(result);
    SDL_SetPointerProperty(props, ATK_IO_STATE_PROPERTY, state);
    
    return result;
    
fail:
    if (result != NULL) {
        ATK_free(result);
    }
    if (state != NULL) {
        ATK_free(state);
    }
    if (node != NULL) {
        ATK_free(node);
    }
    if (namecopy != NULL) {
        ATK_free(namecopy);
    }
    return NULL;
}

/**
 * Returns true if context is managed by pool; false otherwise
 *
 * As both ATK_IOStream and ATK_FilePool are somewhat opaque types, we provide
 * this function to check if a file is managed by a particular pool.
 *
 * @param stream    the ATK_IOStream to check
 * @param pool      the file pool to query from
 *
 * @return true if context is managed by pool
 */
bool ATK_IOInFilePool(ATK_IOStream* stream, ATK_FilePool* pool) {
    if (stream == NULL || pool == NULL) {
        return false;
    }
    
    SDL_PropertiesID props = SDL_GetIOProperties(stream);
    void* data = SDL_GetPointerProperty(props, ATK_IO_STATE_PROPERTY, NULL);
    if (data == NULL) {
        return false;
    }
    
    ATK_IOState* state = (ATK_IOState*)data;
    return state->pool == pool;
}
