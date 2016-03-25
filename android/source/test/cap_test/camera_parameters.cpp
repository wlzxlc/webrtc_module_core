/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#define LOG_TAG "CameraParams"

#include <string>
#include <cstring>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "camera_parameters.h"

#if 1
#define ALOGD(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define ALOGE(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define ALOGD(fmt, ...)
#define ALOGE(fmt, ...)
#endif
// Parameter keys to communicate between camera application and driver.
const char CameraParameters::KEY_PREVIEW_SIZE[] = "preview-size";
const char CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES[] = "preview-size-values";
const char CameraParameters::KEY_PREVIEW_FORMAT[] = "preview-format";
const char CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS[] = "preview-format-values";
const char CameraParameters::KEY_PREVIEW_FRAME_RATE[] = "preview-frame-rate";
const char CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES[] = "preview-frame-rate-values";
const char CameraParameters::KEY_PREVIEW_FPS_RANGE[] = "preview-fps-range";
const char CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE[] = "preview-fps-range-values";
const char CameraParameters::KEY_PICTURE_SIZE[] = "picture-size";
const char CameraParameters::KEY_SUPPORTED_PICTURE_SIZES[] = "picture-size-values";
const char CameraParameters::KEY_PICTURE_FORMAT[] = "picture-format";
const char CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS[] = "picture-format-values";
const char CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH[] = "jpeg-thumbnail-width";
const char CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT[] = "jpeg-thumbnail-height";
const char CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES[] = "jpeg-thumbnail-size-values";
const char CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY[] = "jpeg-thumbnail-quality";
const char CameraParameters::KEY_JPEG_QUALITY[] = "jpeg-quality";
const char CameraParameters::KEY_ROTATION[] = "rotation";
const char CameraParameters::KEY_GPS_LATITUDE[] = "gps-latitude";
const char CameraParameters::KEY_GPS_LONGITUDE[] = "gps-longitude";
const char CameraParameters::KEY_GPS_ALTITUDE[] = "gps-altitude";
const char CameraParameters::KEY_GPS_TIMESTAMP[] = "gps-timestamp";
const char CameraParameters::KEY_GPS_PROCESSING_METHOD[] = "gps-processing-method";
const char CameraParameters::KEY_WHITE_BALANCE[] = "whitebalance";
const char CameraParameters::KEY_SUPPORTED_WHITE_BALANCE[] = "whitebalance-values";
const char CameraParameters::KEY_EFFECT[] = "effect";
const char CameraParameters::KEY_SUPPORTED_EFFECTS[] = "effect-values";
const char CameraParameters::KEY_ANTIBANDING[] = "antibanding";
const char CameraParameters::KEY_SUPPORTED_ANTIBANDING[] = "antibanding-values";
const char CameraParameters::KEY_SCENE_MODE[] = "scene-mode";
const char CameraParameters::KEY_SUPPORTED_SCENE_MODES[] = "scene-mode-values";
const char CameraParameters::KEY_FLASH_MODE[] = "flash-mode";
const char CameraParameters::KEY_SUPPORTED_FLASH_MODES[] = "flash-mode-values";
const char CameraParameters::KEY_FOCUS_MODE[] = "focus-mode";
const char CameraParameters::KEY_SUPPORTED_FOCUS_MODES[] = "focus-mode-values";
const char CameraParameters::KEY_MAX_NUM_FOCUS_AREAS[] = "max-num-focus-areas";
const char CameraParameters::KEY_FOCUS_AREAS[] = "focus-areas";
const char CameraParameters::KEY_FOCAL_LENGTH[] = "focal-length";
const char CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE[] = "horizontal-view-angle";
const char CameraParameters::KEY_VERTICAL_VIEW_ANGLE[] = "vertical-view-angle";
const char CameraParameters::KEY_EXPOSURE_COMPENSATION[] = "exposure-compensation";
const char CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION[] = "max-exposure-compensation";
const char CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION[] = "min-exposure-compensation";
const char CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP[] = "exposure-compensation-step";
const char CameraParameters::KEY_AUTO_EXPOSURE_LOCK[] = "auto-exposure-lock";
const char CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED[] = "auto-exposure-lock-supported";
const char CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK[] = "auto-whitebalance-lock";
const char CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED[] = "auto-whitebalance-lock-supported";
const char CameraParameters::KEY_MAX_NUM_METERING_AREAS[] = "max-num-metering-areas";
const char CameraParameters::KEY_METERING_AREAS[] = "metering-areas";
const char CameraParameters::KEY_ZOOM[] = "zoom";
const char CameraParameters::KEY_MAX_ZOOM[] = "max-zoom";
const char CameraParameters::KEY_ZOOM_RATIOS[] = "zoom-ratios";
const char CameraParameters::KEY_ZOOM_SUPPORTED[] = "zoom-supported";
const char CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED[] = "smooth-zoom-supported";
const char CameraParameters::KEY_FOCUS_DISTANCES[] = "focus-distances";
const char CameraParameters::KEY_VIDEO_FRAME_FORMAT[] = "video-frame-format";
const char CameraParameters::KEY_VIDEO_SIZE[] = "video-size";
const char CameraParameters::KEY_SUPPORTED_VIDEO_SIZES[] = "video-size-values";
const char CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO[] = "preferred-preview-size-for-video";
const char CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW[] = "max-num-detected-faces-hw";
const char CameraParameters::KEY_MAX_NUM_DETECTED_FACES_SW[] = "max-num-detected-faces-sw";
const char CameraParameters::KEY_RECORDING_HINT[] = "recording-hint";
const char CameraParameters::KEY_VIDEO_SNAPSHOT_SUPPORTED[] = "video-snapshot-supported";
const char CameraParameters::KEY_VIDEO_STABILIZATION[] = "video-stabilization";
const char CameraParameters::KEY_VIDEO_STABILIZATION_SUPPORTED[] = "video-stabilization-supported";
const char CameraParameters::KEY_LIGHTFX[] = "light-fx";

// keys extends
const char CameraParameters::KEY_QC_PREVIEW_FLIP[] = "preview-flip";
const char CameraParameters::KEY_QC_VIDEO_FLIP[] = "video-flip";
const char CameraParameters::KEY_QC_VIDEO_ROTATION[] = "video-rotation";
const char CameraParameters::KEY_DAY_NIGHT_MODE[] = "day-night";
const char CameraParameters::KEY_SUPPORTED_DAY_NIGHT_MODE[] = "day-night-mode";
const char CameraParameters::KEY_CONTRAST[] = "contrast";
const char CameraParameters::KEY_SHARPNESS[] = "sharpness";
const char CameraParameters::KEY_SATURATION[] = "saturation";

const char CameraParameters::TRUE[] = "true";
const char CameraParameters::FALSE[] = "false";
const char CameraParameters::FOCUS_DISTANCE_INFINITY[] = "Infinity";

// Values for white balance settings.
const char CameraParameters::WHITE_BALANCE_AUTO[] = "auto";
const char CameraParameters::WHITE_BALANCE_INCANDESCENT[] = "incandescent";
const char CameraParameters::WHITE_BALANCE_FLUORESCENT[] = "fluorescent";
const char CameraParameters::WHITE_BALANCE_WARM_FLUORESCENT[] = "warm-fluorescent";
const char CameraParameters::WHITE_BALANCE_DAYLIGHT[] = "daylight";
const char CameraParameters::WHITE_BALANCE_CLOUDY_DAYLIGHT[] = "cloudy-daylight";
const char CameraParameters::WHITE_BALANCE_TWILIGHT[] = "twilight";
const char CameraParameters::WHITE_BALANCE_SHADE[] = "shade";
const char CameraParameters::WHITE_BALANCE_MANUAL_CCT[] = "manual-cct";

// Values for effect settings.
const char CameraParameters::EFFECT_NONE[] = "none";
const char CameraParameters::EFFECT_MONO[] = "mono";
const char CameraParameters::EFFECT_NEGATIVE[] = "negative";
const char CameraParameters::EFFECT_SOLARIZE[] = "solarize";
const char CameraParameters::EFFECT_SEPIA[] = "sepia";
const char CameraParameters::EFFECT_POSTERIZE[] = "posterize";
const char CameraParameters::EFFECT_WHITEBOARD[] = "whiteboard";
const char CameraParameters::EFFECT_BLACKBOARD[] = "blackboard";
const char CameraParameters::EFFECT_AQUA[] = "aqua";

// Values for antibanding settings.
const char CameraParameters::ANTIBANDING_AUTO[] = "auto";
const char CameraParameters::ANTIBANDING_50HZ[] = "50hz";
const char CameraParameters::ANTIBANDING_60HZ[] = "60hz";
const char CameraParameters::ANTIBANDING_OFF[] = "off";

// Values for flash mode settings.
const char CameraParameters::FLASH_MODE_OFF[] = "off";
const char CameraParameters::FLASH_MODE_AUTO[] = "auto";
const char CameraParameters::FLASH_MODE_ON[] = "on";
const char CameraParameters::FLASH_MODE_RED_EYE[] = "red-eye";
const char CameraParameters::FLASH_MODE_TORCH[] = "torch";

// Values for scene mode settings.
const char CameraParameters::SCENE_MODE_AUTO[] = "auto";
const char CameraParameters::SCENE_MODE_ACTION[] = "action";
const char CameraParameters::SCENE_MODE_PORTRAIT[] = "portrait";
const char CameraParameters::SCENE_MODE_LANDSCAPE[] = "landscape";
const char CameraParameters::SCENE_MODE_NIGHT[] = "night";
const char CameraParameters::SCENE_MODE_NIGHT_PORTRAIT[] = "night-portrait";
const char CameraParameters::SCENE_MODE_THEATRE[] = "theatre";
const char CameraParameters::SCENE_MODE_BEACH[] = "beach";
const char CameraParameters::SCENE_MODE_SNOW[] = "snow";
const char CameraParameters::SCENE_MODE_SUNSET[] = "sunset";
const char CameraParameters::SCENE_MODE_STEADYPHOTO[] = "steadyphoto";
const char CameraParameters::SCENE_MODE_FIREWORKS[] = "fireworks";
const char CameraParameters::SCENE_MODE_SPORTS[] = "sports";
const char CameraParameters::SCENE_MODE_PARTY[] = "party";
const char CameraParameters::SCENE_MODE_CANDLELIGHT[] = "candlelight";
const char CameraParameters::SCENE_MODE_BARCODE[] = "barcode";
const char CameraParameters::SCENE_MODE_HDR[] = "hdr";

const char CameraParameters::PIXEL_FORMAT_YUV422SP[] = "yuv422sp";
const char CameraParameters::PIXEL_FORMAT_YUV420SP[] = "yuv420sp";
const char CameraParameters::PIXEL_FORMAT_YUV422I[] = "yuv422i-yuyv";
const char CameraParameters::PIXEL_FORMAT_YUV420P[]  = "yuv420p";
const char CameraParameters::PIXEL_FORMAT_RGB565[] = "rgb565";
const char CameraParameters::PIXEL_FORMAT_RGBA8888[] = "rgba8888";
const char CameraParameters::PIXEL_FORMAT_JPEG[] = "jpeg";
const char CameraParameters::PIXEL_FORMAT_BAYER_RGGB[] = "bayer-rggb";
const char CameraParameters::PIXEL_FORMAT_ANDROID_OPAQUE[] = "android-opaque";

// Values for focus mode settings.
const char CameraParameters::FOCUS_MODE_AUTO[] = "auto";
const char CameraParameters::FOCUS_MODE_INFINITY[] = "infinity";
const char CameraParameters::FOCUS_MODE_MACRO[] = "macro";
const char CameraParameters::FOCUS_MODE_FIXED[] = "fixed";
const char CameraParameters::FOCUS_MODE_EDOF[] = "edof";
const char CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO[] = "continuous-video";
const char CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE[] = "continuous-picture";
const char CameraParameters::FOCUS_MODE_MANUAL_POSITION[] = "manual";

// Values for light fx settings
const char CameraParameters::LIGHTFX_LOWLIGHT[] = "low-light";
const char CameraParameters::LIGHTFX_HDR[] = "high-dynamic-range";

CameraParameters::CameraParameters()
                : mMap()
{
}

CameraParameters::~CameraParameters()
{
}

string CameraParameters::flatten() const
{
    string flattened("");
    size_t size = mMap.size();
#if 0
    for (size_t i = 0; i < size; i++) {
        string k, v;
        k = mMap.keyAt(i);
        v = mMap.valueAt(i);

        flattened += k;
        flattened += "=";
        flattened += v;
        if (i != size-1)
            flattened += ";";
    }
#endif
    ParamMap::const_iterator it = mMap.begin(); 
    size_t i = 0;
    while(it != mMap.end()) {
        string k,v;
        k = it->first;
        v = it->second;

        flattened += k;
        flattened += "=";
        flattened += v;
        if (i != size-1)
            flattened += ";";
        i++;
        ++it;
    }

    return flattened;
}

void CameraParameters::unflatten(const string &params)
{
    const char *a = params.c_str();
    const char *b;

    mMap.clear();

    for (;;) {
        // Find the bounds of the key name.
        b = strchr(a, '=');
        if (b == 0)
            break;

        // Create the key string.
        string k(a, (size_t)(b-a));

        // Find the value.
        a = b+1;
        b = strchr(a, ';');
        if (b == 0) {
            // If there's no semicolon, this is the last item.
            string v(a);
            mMap.insert(std::make_pair(k, v));
            break;
        }

        string v(a, (size_t)(b-a));
        mMap.insert(std::make_pair(k, v));
        a = b+1;
    }
}


void CameraParameters::set(const char *key, const char *value)
{
    // XXX i think i can do this with strspn()
    if (strchr(key, '=') || strchr(key, ';')) {
        //XXX ALOGE("Key \"%s\"contains invalid character (= or ;)", key);
        return;
    }

    if (strchr(value, '=') || strchr(value, ';')) {
        //XXX ALOGE("Value \"%s\"contains invalid character (= or ;)", value);
        return;
    }
#if 0
    mMap.replaceValueFor(string(key), string(value));
#endif
    ParamMap::iterator it = mMap.find(key);
    if (it != mMap.end()) {
        it->second = string(value);
    }
}

void CameraParameters::set(const char *key, int value)
{
    char str[16];
    sprintf(str, "%d", value);
    set(key, str);
}

void CameraParameters::setFloat(const char *key, float value)
{
    char str[16];  // 14 should be enough. We overestimate to be safe.
    snprintf(str, sizeof(str), "%g", value);
    set(key, str);
}

const char *CameraParameters::get(const char *key) const
{
    ParamMap::const_iterator it = mMap.find(string(key));
    if (it == mMap.end())
        return 0;

    string v = it->second;
    if (v.length() == 0)
        return 0;
    return v.c_str();
}

int CameraParameters::getInt(const char *key) const
{
    const char *v = get(key);
    if (v == 0)
        return -1;
    return strtol(v, 0, 0);
}

float CameraParameters::getFloat(const char *key) const
{
    const char *v = get(key);
    if (v == 0) return -1;
    return strtof(v, 0);
}

void CameraParameters::remove(const char *key)
{
    //mMap.removeItem(string(key));
    ParamMap::iterator it = mMap.find(string(key));
    if (it != mMap.end()){
        mMap.erase(it);
    }
}

// Parse string like "640x480" or "10000,20000"
static int parse_pair(const char *str, int *first, int *second, char delim,
                      char **endptr = NULL)
{
    // Find the first integer.
    char *end;
    int w = (int)strtol(str, &end, 10);
    // If a delimeter does not immediately follow, give up.
    if (*end != delim) {
        ALOGE("Cannot find delimeter (%c) in str=%s", delim, str);
        return -1;
    }

    // Find the second integer, immediately after the delimeter.
    int h = (int)strtol(end+1, &end, 10);

    *first = w;
    *second = h;

    if (endptr) {
        *endptr = end;
    }

    return 0;
}

static void parseSizesList(const char *sizesStr, vector<Size> &sizes)
{
    if (sizesStr == 0) {
        return;
    }

    char *sizeStartPtr = (char *)sizesStr;

    while (true) {
        int width, height;
        int success = parse_pair(sizeStartPtr, &width, &height, 'x',
                                 &sizeStartPtr);
        if (success == -1 || (*sizeStartPtr != ',' && *sizeStartPtr != '\0')) {
            ALOGE("Picture sizes string \"%s\" contains invalid character.", sizesStr);
            return;
        }

        sizes.push_back(Size(width, height));

        if (*sizeStartPtr == '\0') {
            return;
        }
        sizeStartPtr++;
    }
}

static void parseRangesList(const char *rangesStr, vector<Range> &ranges)
{
    if (rangesStr == 0) {
        return;
    }

    char *rangeStartPtr = (char *)rangesStr;

    while (true) {
        int start, end;
        if (*rangeStartPtr != '(') {
            return;
        }else {
            ++rangeStartPtr;
        }

        int success = parse_pair(rangeStartPtr, &start, &end, ',',
                                 &rangeStartPtr);
        if (success == -1 || *rangeStartPtr != ')') {
            ALOGE("Range string \"%s\" contains invalid character.", rangesStr);
            return;
        }
        ++rangeStartPtr;

        ranges.push_back(Range(start, end));

        if (*rangeStartPtr == '\0' || *rangeStartPtr != ',') {
            return;
        }
        rangeStartPtr++;
    }
}

void CameraParameters::setPreviewSize(int width, int height)
{
    char str[32];
    sprintf(str, "%dx%d", width, height);
    set(KEY_PREVIEW_SIZE, str);
}

void CameraParameters::getPreviewSize(int *width, int *height) const
{
    *width = *height = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_PREVIEW_SIZE);
    if (p == 0)  return;
    parse_pair(p, width, height, 'x');
}

void CameraParameters::getPreferredPreviewSizeForVideo(int *width, int *height) const
{
    *width = *height = -1;
    const char *p = get(KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO);
    if (p == 0)  return;
    parse_pair(p, width, height, 'x');
}

void CameraParameters::getSupportedPreviewSizes(vector<Size> &sizes) const
{
    const char *previewSizesStr = get(KEY_SUPPORTED_PREVIEW_SIZES);
    parseSizesList(previewSizesStr, sizes);
}

void CameraParameters::setVideoSize(int width, int height)
{
    char str[32];
    sprintf(str, "%dx%d", width, height);
    set(KEY_VIDEO_SIZE, str);
}

void CameraParameters::getVideoSize(int *width, int *height) const
{
    *width = *height = -1;
    const char *p = get(KEY_VIDEO_SIZE);
    if (p == 0) return;
    parse_pair(p, width, height, 'x');
}

void CameraParameters::getSupportedVideoSizes(vector<Size> &sizes) const
{
    const char *videoSizesStr = get(KEY_SUPPORTED_VIDEO_SIZES);
    parseSizesList(videoSizesStr, sizes);
}

void CameraParameters::setPreviewFrameRate(int fps)
{
    set(KEY_PREVIEW_FRAME_RATE, fps);
}

int CameraParameters::getPreviewFrameRate() const
{
    return getInt(KEY_PREVIEW_FRAME_RATE);
}

void CameraParameters::getPreviewFpsRange(int *min_fps, int *max_fps) const
{
    *min_fps = *max_fps = -1;
    const char *p = get(KEY_PREVIEW_FPS_RANGE);
    if (p == 0) return;
    parse_pair(p, min_fps, max_fps, ',');
}

void CameraParameters::setPreviewFpsRange(int min_fps, int max_fps)
{
    char str[32];
    sprintf(str, "%d,%d", min_fps, max_fps);
    set(KEY_PREVIEW_FPS_RANGE, str);
}

void CameraParameters::getSupportedPreviewFpsRange(vector<Range> &ranges) const
{
    ranges.clear();
    const char *p = get(KEY_SUPPORTED_PREVIEW_FPS_RANGE);
    if (p==0) return;
    parseRangesList(p, ranges);
}

void CameraParameters::setPreviewFormat(const char *format)
{
    set(KEY_PREVIEW_FORMAT, format);
}

const char *CameraParameters::getPreviewFormat() const
{
    return get(KEY_PREVIEW_FORMAT);
}

void CameraParameters::setPictureSize(int width, int height)
{
    char str[32];
    sprintf(str, "%dx%d", width, height);
    set(KEY_PICTURE_SIZE, str);
}

void CameraParameters::getPictureSize(int *width, int *height) const
{
    *width = *height = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_PICTURE_SIZE);
    if (p == 0) return;
    parse_pair(p, width, height, 'x');
}

void CameraParameters::getSupportedPictureSizes(vector<Size> &sizes) const
{
    const char *pictureSizesStr = get(KEY_SUPPORTED_PICTURE_SIZES);
    parseSizesList(pictureSizesStr, sizes);
}

void CameraParameters::setPictureFormat(const char *format)
{
    set(KEY_PICTURE_FORMAT, format);
}

const char *CameraParameters::getPictureFormat() const
{
    return get(KEY_PICTURE_FORMAT);
}

void CameraParameters::dump() const
{
#if 0
    ALOGD("dump: mMap.size = %zu", mMap.size());
    for (size_t i = 0; i < mMap.size(); i++) {
        string k, v;
        k = mMap.keyAt(i);
        v = mMap.valueAt(i);
        ALOGD("%s: %s\n", k.string(), v.string());
    }
#endif
    ALOGD("dump: mMap.size = %zu\n", mMap.size());
    ParamMap::const_iterator it = mMap.begin(); 
    while(it != mMap.end()) {
        string k,v;
        k = it->first;
        v = it->second;
        ALOGD("%s: %s\n", k.c_str(), v.c_str());
        ++it;
    }

}

int CameraParameters::dump(int fd, const vector<string>& /*args*/) const
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    string result;
    snprintf(buffer, 255, "CameraParameters::dump: mMap.size = %zu\n", mMap.size());
    result.append(buffer);
#if 0
    for (size_t i = 0; i < mMap.size(); i++) {
        string k, v;
        k = mMap.keyAt(i);
        v = mMap.valueAt(i);
        snprintf(buffer, 255, "\t%s: %s\n", k.string(), v.string());
        result.append(buffer);
    }
#endif
    ParamMap::const_iterator it = mMap.begin(); 
    while(it != mMap.end()) {
        string k,v;
        k = it->first;
        v = it->second;
        snprintf(buffer, 255, "\t%s: %s\n", k.c_str(), v.c_str());
        result.append(buffer);
        ALOGD("%s: %s\n", k.c_str(), v.c_str());
        ++it;
    } 
    write(fd, result.c_str(), result.size());
    return 0;
}

void CameraParameters::getSupportedPreviewFormats(vector<int>& formats) const {
    const char* supportedPreviewFormats =
          get(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS);

    string fmtStr(supportedPreviewFormats);
    char* prevFmts = (char *)fmtStr.c_str();//lockBuffer(fmtStr.size());

    char* savePtr;
    char* fmt = strtok_r(prevFmts, ",", &savePtr);
    while (fmt) {
        int actual = previewFormatToEnum(fmt);
        if (actual != -1) {
            formats.push_back(actual);
        }
        fmt = strtok_r(NULL, ",", &savePtr);
    }
    //fmtStr.unlockBuffer(fmtStr.size());
}


int CameraParameters::previewFormatToEnum(const char* format) {
    return
        !format ?
            HAL_PIXEL_FORMAT_YCrCb_420_SP :
        !strcmp(format, PIXEL_FORMAT_YUV422SP) ?
            HAL_PIXEL_FORMAT_YCbCr_422_SP : // NV16
        !strcmp(format, PIXEL_FORMAT_YUV420SP) ?
            HAL_PIXEL_FORMAT_YCrCb_420_SP : // NV21
        !strcmp(format, PIXEL_FORMAT_YUV422I) ?
            HAL_PIXEL_FORMAT_YCbCr_422_I :  // YUY2
#if 0
        !strcmp(format, PIXEL_FORMAT_YUV420P) ?
            HAL_PIXEL_FORMAT_YV12 :         // YV12
#endif
        !strcmp(format, PIXEL_FORMAT_RGB565) ?
            HAL_PIXEL_FORMAT_RGB_565 :      // RGB565
        !strcmp(format, PIXEL_FORMAT_RGBA8888) ?
            HAL_PIXEL_FORMAT_RGBA_8888 :    // RGB8888
#if 0
        !strcmp(format, PIXEL_FORMAT_BAYER_RGGB) ?
            HAL_PIXEL_FORMAT_RAW_SENSOR :   // Raw sensor data
#endif
        -1;
}
