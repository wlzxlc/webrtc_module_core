/* ------------------------------------------------------------------
 * Copyright (C) 1998-2009 PacketVideo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */
/**
 * Copyright (c) 2008 The Khronos Group Inc. 
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions: 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software. 
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
 *
 */

/** 
 *  @file OMX_Video.h - OpenMax IL version 1.1.2
 *  The structures is needed by Video components to exchange parameters 
 *  and configuration data with OMX components.
 */
#ifndef OMX_STONE_Video_h
#define OMX_STONE_Video_h

/** @defgroup video OpenMAX IL Video Domain
 * @ingroup iv
 * Structures for OpenMAX IL Video domain
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/**
 * Each OMX header must include all required header files to allow the
 * header to compile without errors.  The includes below are required
 * for this header file to compile successfully 
 */

#include <OMX_IVCommon.h>


/**
 * Enumeration used to define the possible video compression codings.  
 * NOTE:  This essentially refers to file extensions. If the coding is 
 *        being used to specify the ENCODE type, then additional work 
 *        must be done to configure the exact flavor of the compression 
 *        to be used.  For decode cases where the user application can 
 *        not differentiate between MPEG-4 and H.264 bit streams, it is 
 *        up to the codec to handle this.
 */
typedef enum OMX_STONE_VIDEO_CODINGTYPE {
    OMX_STONE_VIDEO_CodingSVAC = 0x7F000001
} OMX_STONE_VIDEO_CODINGTYPE;

/** 
 *  * Structure for dynamically configuring qp of a codec. 
 **
 ** STRUCT MEMBERS:
 **  nSize          : Size of the struct in bytes
 **  nVersion       : OMX spec version info
 **  nPortIndex     : Port that this struct applies to
 **  nQPMax         : Target max qp
 **  nQPMin         : Target min qp
 **/
typedef struct OMX_STONE_VIDEO_CONFIG_QP {
    OMX_U32 nSize;     
    OMX_VERSIONTYPE nVersion;     
    OMX_U32 nPortIndex;     
    OMX_U32 nQPMax;     
    OMX_U32 nQPMin;     
} OMX_STONE_VIDEO_CONFIG_QP;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
/* File EOF */

