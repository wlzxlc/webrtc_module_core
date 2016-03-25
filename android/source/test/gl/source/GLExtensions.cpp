/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "GLExtensions.h"

namespace CCStone {
// ---------------------------------------------------------------------------

ANDROID_SINGLETON_STATIC_INSTANCE( GLExtensions )

GLExtensions::GLExtensions()
    : mHaveFramebufferObject(false)
{
}

void GLExtensions::initWithGLStrings(
        GLubyte const* vendor,
        GLubyte const* renderer,
        GLubyte const* version,
        GLubyte const* extensions)
{
    mVendor     = (char const*)vendor;
    mRenderer   = (char const*)renderer;
    mVersion    = (char const*)version;
    mExtensions = (char const*)extensions;

    char const* curr = (char const*)extensions;
    char const* head = curr;
    do {
        head = strchr(curr, ' ');
        String8 s(curr, head ? head-curr : strlen(curr));
        if (s.length()) {
            mExtensionList.add(s);
        }
        curr = head+1;
    } while (head);

    if (hasExtension("GL_OES_framebuffer_object")) {
        mHaveFramebufferObject = true;
    }
}

bool GLExtensions::hasExtension(char const* extension) const
{
    const std::string s(extension);
    std::vect<std::string>::const_iterator begin = mExtensionList.begin();
    std::vect<std::string>::const_iterator end = mExtensionList.end();

    std::vect<std::string>::const_iterator result = 
        std::find(begin, end, s);

    return result != end;
}

char const* GLExtensions::getVendor() const {
    return mVendor.c_str();
}

char const* GLExtensions::getRenderer() const {
    return mRenderer.c_str();
}

char const* GLExtensions::getVersion() const {
    return mVersion.c_str();
}

char const* GLExtensions::getExtension() const {
    return mExtensions.c_str();
}

// ---------------------------------------------------------------------------
}; // namespace android
