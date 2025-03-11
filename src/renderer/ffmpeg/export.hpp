#pragma once

#include <Geode/platform/cplatform.h>

#ifdef GEODE_IS_WINDOWS
    #ifdef FFMPEG_API_EXPORTING
        #define FFMPEG_API_DLL __declspec(dllexport)
    #else
        #define FFMPEG_API_DLL __declspec(dllimport)
    #endif
#else
    #define FFMPEG_API_DLL __attribute__((visibility("default")))
#endif

#define FFMPEG_API_VERSION 2

#define BEGIN_FFMPEG_NAMESPACE namespace ffmpeg {
#define FFMPEG_API_VERSION_NS GEODE_CONCAT(v, FFMPEG_API_VERSION)
#define BEGIN_FFMPEG_NAMESPACE_V \
    BEGIN_FFMPEG_NAMESPACE \
    inline namespace FFMPEG_API_VERSION_NS {
#define END_FFMPEG_NAMESPACE }
#define END_FFMPEG_NAMESPACE_V }}