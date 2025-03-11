#pragma once

#include "render_settings.hpp"
#include "export.hpp"

#include <Geode/Result.hpp>

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

class AVFormatContext;
class AVCodec;
class AVStream;
class AVCodecContext;
class AVBufferRef;
class AVFrame;
class AVPacket;
class SwsContext;
class AVFilterContext;
class AVFilter;
class AVFilterGraph;

BEGIN_FFMPEG_NAMESPACE_V

class FFMPEG_API_DLL Recorder {
private:
    class Impl {
    public:
        AVFormatContext* m_formatContext = nullptr;
        const AVCodec* m_codec = nullptr;
        AVStream* m_videoStream = nullptr;
        AVCodecContext* m_codecContext = nullptr;
        AVBufferRef* m_hwDevice = nullptr;
        AVFrame* m_frame = nullptr;
        AVFrame* m_convertedFrame = nullptr;
        AVFrame* m_filteredFrame = nullptr;
        AVPacket* m_packet = nullptr;
        SwsContext* m_swsCtx = nullptr;
        AVFilterGraph* m_filterGraph = nullptr;
        AVFilterContext* m_buffersrcCtx = nullptr;
        AVFilterContext* m_buffersinkCtx = nullptr;
        AVFilterContext* m_colorspaceCtx = nullptr;
        AVFilterContext* m_vflipCtx = nullptr;

        size_t m_frameCount = 0;
        size_t m_expectedSize = 0;
        bool m_init = false;

        geode::Result<> init(const RenderSettings& settings);
        void stop();
        geode::Result<> writeFrame(const std::vector<uint8_t>& frameData);
        geode::Result<> filterFrame(AVFrame* inputFrame, AVFrame* outputFrame);
    };

    std::unique_ptr<Impl> m_impl = nullptr;

public:
    /**
     * @brief Initializes the Recorder with the specified rendering settings.
     *
     * This function configures the recorder with the given render settings,
     * allocates necessary resources, and prepares for video encoding.
     *
     * @param settings The rendering settings that define the output characteristics, 
     *                 including codec, bitrate, resolution, and pixel format.
     * 
     * @return true if initialization is successful, false otherwise.
     */
    geode::Result<> init(const RenderSettings& settings) {
        m_impl = std::make_unique<Impl>();
        return m_impl->init(settings);
    }

    /**
     * @brief Stops the recording process and finalizes the output file.
     *
     * This function ensures that all buffered frames are written to the output file,
     * releases allocated resources, and properly closes the output file.
     */
    void stop() const { m_impl->stop(); }

    /**
     * @brief Writes a single video frame to the output.
     *
     * This function takes the frame data as a byte vector and encodes it 
     * to the output file. The frame data must match the expected format and 
     * dimensions defined during initialization.
     *
     * @param frameData A vector containing the raw frame data to be written.
     * 
     * @return true if the frame is successfully written, false if there is an error.
     * 
     * @warning Ensure that the frameData size matches the expected dimensions of the frame.
     */
    geode::Result<> writeFrame(const std::vector<uint8_t>& frameData) const {
        return m_impl->writeFrame(frameData);
    }

    /**
     * @brief Retrieves a list of available codecs for video encoding.
     *
     * This function iterates through all available codecs in FFmpeg and 
     * returns a sorted vector of codec names.
     * 
     * @return A vector representing the names of available codecs.
     */
    static std::vector<std::string> getAvailableCodecs();

private:
    geode::Result<> filterFrame(AVFrame* inputFrame, AVFrame* outputFrame) const {
        return m_impl->filterFrame(inputFrame, outputFrame);
    }
};

END_FFMPEG_NAMESPACE_V