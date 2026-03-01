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
#ifndef __ATK_FILE_C_H__
#define __ATK_FILE_C_H__
#include <SDL3/SDL_atk.h>
#include <SDL3/SDL_begin_code.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file ATK_file_c.h
 *
 * Internal header file for the file manager of SDL_atk
 */

/** The "hidden" default file pool. Use ATK_DefaultFilePool to access. */
extern ATK_FilePool* _atk_default_pool;

/**
 * Bit vector entries for the various file modes
 *
 * As we will need to close and reopen files, we need to keep track of the
 * appropriate file modes.
 */
enum {
    /** Whether the file is open for reading */
    ATK_IO_MODE_READ   = 0x0001,
    /** Whether the file is open for writing */
    ATK_IO_MODE_WRITE  = 0x0002,
    /** Whether the file is open for appending */
    ATK_IO_MODE_APPEND = 0x0004,
    /** Whether the file is in binary mode */
    ATK_IO_MODE_BINARY = 0x0008,
} ATK_IOmodes;

// Forward declarations for reference cycle
typedef struct ATK_IOState ATK_IOState;
typedef struct ATK_FileNode ATK_FileNode;

/**
 * Entry in the file pools (doubly-linked) list of files
 *
 * This list allows for rapid insertion and deletion of files.
 * Searching is possible, but is slower.
 */
struct ATK_FileNode {
    ATK_IOState*    state;
    struct ATK_FileNode* next;
    struct ATK_FileNode* prev;
};

/**
 * Internal data for a managed file
 *
 * This struct is the first entry (data1) of the hidden attribute
 * in SDL_IOStream.
 */
struct ATK_IOState {
    /** The underlying source stream (NULL if not active) */
    SDL_IOStream* source;
    /** The path to the file being managed */
    char* name;
    /** Whether the file has been touched recently */
    bool touch;
    /** Whether the file is current active */
    bool active;
    /** The bit vector of the file mode */
    Uint32 mode;
    /** The last known file position */
    size_t pos;
    /** The associated file pool */
    ATK_FilePool* pool;
    /** The associated node in the file pool */
    ATK_FileNode* node;
};

/**
 * The type for a managed file pool
 *
 * Managed files are associated with a file pool. A file pool is a collection of
 * managed files, and which only allows a small number of files to be active
 * (i.e. in memory) at a time. If a file needs to be reactivated, and the number
 * of active files is at capacity, the file pool will first page out one of its
 * active members to make room.
 *
 * Deleting a file pool immediately disposes of all of its managed files.
 */
struct ATK_FilePool {
    /** The maximum number of active files */
    size_t capacity;
    /** The current number of active files */
    size_t active;
    /** The total number of files open (active and inactive) */
    size_t total;
    /** A mutex to make the file functions thread safe(ish) */
    SDL_Mutex*    mutex;
    /** The head of the list of all open files */
    ATK_FileNode* head;
    /** The tail of the list of all open files */
    ATK_FileNode* tail;
    /** The next candidate file for eviction */
    ATK_FileNode* evict;
};

/**
 * Activates a file in the file pool, loading it into memory
 *
 * If the file pool is already at capacity, this function will instruct
 * the file pool to deactivate another file, to make room. If no file
 * can be deactivated, this function will fail.
 *
 * @param context   the state of the ATK_IOStream to activate
 *
 * @return true on success
 */
extern SDL_DECLSPEC bool SDLCALL ATK_ActivateIO(ATK_IOState* context);

/**
 * Deactivates an active ATK_IOStream
 *
 * This function saves the current state of the ATK_IOStream and pages it
 * out, making room for more active files. This function does nothing
 * if the file is not active.
 *
 * @param context   the state of the ATK_IOStream to deactivate
 *
 * @return true on success
 */
extern SDL_DECLSPEC bool SDLCALL ATK_DeactivateIO(ATK_IOState* context);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include <SDL3/SDL_close_code.h>

#endif /* __ATK_FILE_C_H__ */
