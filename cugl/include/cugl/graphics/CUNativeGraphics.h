//
//  CUNativeGra[jhocs.h
//  Cornell University Game Library (CUGL)
//
//  This header includes the necessary includes to use either the native
//  graphics API (either OpenGL, OpenGLES, or Vulkan) for the current build.
//  This file is excluded from the cugl header package. It is provided only
//  for those that need direct access to their graphics API.
//
//  CUGL MIT License:
//      This software is provided 'as-is', without any express or implied
//      warranty.  In no event will the authors be held liable for any damages
//      arising from the use of this software.
//
//      Permission is granted to anyone to use this software for any purpose,
//      including commercial applications, and to alter it and redistribute it
//      freely, subject to the following restrictions:
//
//      1. The origin of this software must not be misrepresented; you must not
//      claim that you wrote the original software. If you use this software
//      in a product, an acknowledgment in the product documentation would be
//      appreciated but is not required.
//
//      2. Altered source versions must be plainly marked as such, and must not
//      be misrepresented as being the original software.
//
//      3. This notice may not be removed or altered from any source distribution.
//
//  Author: Walker White
//  Version: 9/9/24 (Vulkan integration)
//
#ifndef __CU_NATIVE_GRAPHICS_H__
#define __CU_NATIVE_GRAPHICS_H__
#include <cugl/core/CUBase.h>
#include <cugl/graphics/CUGraphicsBase.h>

// Pull in the right headers
#if CU_GRAPHICS_API == CU_HEADLESS
#elif CU_GRAPHICS_API == CU_VULKAN
#include <vulkan/vulkan.h>
// Add some typedefs to make it like OpenGL
typedef void GLvoid;
typedef uint32_t GLenum;
typedef uint8_t GLboolean;
typedef int8_t GLbyte;
typedef uint8_t GLubyte;
typedef int32_t GLint;
typedef uint32_t GLuint;
typedef uint32_t GLsizei;
typedef float GLfloat;
typedef uint32_t GLsizeiptr; // Vulkan hates size_t for some reason
#else
#if defined (SDL_PLATFORM_IOS)
    #include <TargetConditionals.h>
#if defined(TARGET_OS_MACCATALYST) && TARGET_OS_MACCATALYST
    // Don't let this be built.
    #error "Mac Catalyst only supports headless and Vulkan backends"
    #include <OpenGL/OpenGL.h>
    #include <OpenGL/gl3.h>
    #include <OpenGL/gl3ext.h>
#else
    #include <OpenGLES/ES3/gl.h>
    #include <OpenGLES/ES3/glext.h>
#endif
#elif defined (SDL_PLATFORM_ANDROID)
    #include <GLES3/gl3platform.h>
    #include <GLES3/gl3.h>
    #include <GLES3/gl3ext.h>
#elif defined (SDL_PLATFORM_MACOS)
    #include <OpenGL/OpenGL.h>
    #include <OpenGL/gl3.h>
    #include <OpenGL/gl3ext.h>
#elif defined (SDL_PLATFORM_WIN32)
    #include <GL/glew.h>
    #include <SDL3/SDL_opengl.h>
    #include <GL/gl.h>
    #include <GL/glu.h>
#elif defined (SDL_PLATFORM_LINUX)
    #include <GL/glew.h>
    #include <SDL3/SDL_opengl.h>
    #include <GL/gl.h>
    #include <GL/glu.h>
    #include <GL/glut.h>
#endif
#endif

#endif /* __CU_NATIVE_GRAPHICS_H__ */
