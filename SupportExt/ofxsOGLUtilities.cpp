/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of openfx-supportext <https://github.com/NatronGitHub/openfx-supportext>,
 * (C) 2018-2021 The Natron Developers
 * (C) 2013-2018 INRIA
 *
 * openfx-supportext is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * openfx-supportext is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openfx-supportext.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#include <glad/glad.h>
#if !defined(_WIN32) && !defined(__CYGWIN__) && !defined(__APPLE__) && !defined(__HAIKU__)
#include <glad/gladegl.h>
#endif

#include "ofxsOGLUtilities.h"

#include "ofxsOGLFunctions.h"

#include "ofxsMultiThread.h"
#ifndef OFX_USE_MULTITHREAD_MUTEX
// some OFX hosts do not have mutex handling in the MT-Suite (e.g. Sony Catalyst Edit)
// prefer using the fast mutex by Marcus Geelnard http://tinythreadpp.bitsnbites.eu/
#include "fast_mutex.h"
#endif
#ifdef OFX_USE_MULTITHREAD_MUTEX
typedef OFX::MultiThread::Mutex Mutex;
typedef OFX::MultiThread::AutoMutex AutoMutex;
#else
typedef tthread::fast_mutex Mutex;
typedef OFX::MultiThread::AutoMutexT<tthread::fast_mutex> AutoMutex;
#endif

static Mutex g_glLoadOnceMutex;
static bool g_glLoaded = false;

namespace OFX {
bool
ofxsLoadOpenGLOnce()
{
    // Ensure that OpenGL functions loading is thread-safe
    AutoMutex locker(&g_glLoadOnceMutex);

    if (g_glLoaded) {
        // Already loaded, don't do it again
        return true;
    }

    // Reasons for failure might be:
    // - opengl32.dll was not found, or libGL.so was not found or OpenGL.framework was not found
    // - glGetString does not return a valid version
    // Note: It does NOT check that required extensions and functions have actually been found
#if !defined(_WIN32) && !defined(__CYGWIN__) && !defined(__APPLE__) && !defined(__HAIKU__)
    bool glLoaded = false;
    const char *onWayland = getenv("WAYLAND_DISPLAY");
    const char *disableWayland = getenv("NATRON_DISABLE_WAYLAND");
    if (onWayland && onWayland[0] != '\0' && !disableWayland) {
        glLoaded = gladLoadEGL();
    } else {
        glLoaded = gladLoadGL();
    }
#else
    bool glLoaded = gladLoadGL();
#endif

    g_glLoaded = glLoaded;

    // If only EXT_framebuffer is present and not ARB link functions
    if (glLoaded && GLAD_GL_EXT_framebuffer_object && !GLAD_GL_ARB_framebuffer_object) {
        glad_glIsRenderbuffer = glad_glIsRenderbufferEXT;
        glad_glBindRenderbuffer = glad_glBindRenderbufferEXT;
        glad_glDeleteRenderbuffers = glad_glDeleteRenderbuffersEXT;
        glad_glGenRenderbuffers = glad_glGenRenderbuffersEXT;
        glad_glRenderbufferStorage = glad_glRenderbufferStorageEXT;
        glad_glGetRenderbufferParameteriv = glad_glGetRenderbufferParameterivEXT;
        glad_glBindFramebuffer = glad_glBindFramebufferEXT;
        glad_glIsFramebuffer = glad_glIsFramebufferEXT;
        glad_glDeleteFramebuffers = glad_glDeleteFramebuffersEXT;
        glad_glGenFramebuffers = glad_glGenFramebuffersEXT;
        glad_glCheckFramebufferStatus = glad_glCheckFramebufferStatusEXT;
        glad_glFramebufferTexture1D = glad_glFramebufferTexture1DEXT;
        glad_glFramebufferTexture2D = glad_glFramebufferTexture2DEXT;
        glad_glFramebufferTexture3D = glad_glFramebufferTexture3DEXT;
        glad_glFramebufferRenderbuffer = glad_glFramebufferRenderbufferEXT;
        glad_glGetFramebufferAttachmentParameteriv = glad_glGetFramebufferAttachmentParameterivEXT;
        glad_glGenerateMipmap = glad_glGenerateMipmapEXT;
    }

    if (glLoaded && GLAD_GL_APPLE_vertex_array_object && !GLAD_GL_ARB_vertex_buffer_object) {
        glad_glBindVertexArray = glad_glBindVertexArrayAPPLE;
        glad_glDeleteVertexArrays = glad_glDeleteVertexArraysAPPLE;
        glad_glGenVertexArrays = glad_glGenVertexArraysAPPLE;
        glad_glIsVertexArray = glad_glIsVertexArrayAPPLE;
    }

    return g_glLoaded;
} // ofxsLoadGLOnce

int
getOpenGLMajorVersion()
{
    return GLVersion.major;
}

int
getOpenGLMinorVersion()
{
    return GLVersion.minor;
}

bool
getOpenGLSupportsTextureFloat()
{
    return GLAD_GL_ARB_texture_float;
}

bool
getOpenGLSupportFramebuffer()
{
    return GLAD_GL_ARB_framebuffer_object || GLAD_GL_EXT_framebuffer_object;
}

bool
getOpenGLSupportPixelbuffer()
{
    return GLAD_GL_ARB_pixel_buffer_object;
}

bool
getOpenGLSupportVertexArray()
{
    return GLAD_GL_ARB_vertex_array_object || GLAD_GL_APPLE_vertex_array_object;
}
} // namespace OFX
