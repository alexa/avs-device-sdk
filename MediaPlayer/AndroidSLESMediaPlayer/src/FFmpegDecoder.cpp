/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <atomic>
#include <fstream>
#include <thread>
#include <bitset>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

#include <AVSCommon/Utils/AudioFormat.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/RetryTimer.h>
#include <AVSCommon/Utils/String/StringUtils.h>

#include "AndroidSLESMediaPlayer/FFmpegDecoder.h"
#include "AndroidSLESMediaPlayer/FFmpegDeleter.h"
#include "AndroidSLESMediaPlayer/PlaybackConfiguration.h"

/// String to identify log entries originating from this file.
static const std::string TAG("FFmpegDecoder");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace mediaPlayer {
namespace android {

/// Represent scenario where there is no flag enabled.
static constexpr int NO_FLAGS{0};

/// For @c av_samples_get_buffer_size we want to disable alignment to avoid empty samples (see ACSDK-1890).
static constexpr int NO_ALIGNMENT{1};

/// Represent the timeout for the initialization step.
///
/// The timeout value should be long enough to avoid interrupting a normal initialization but it shouldn't sacrifice
/// the user perception in case something goes wrong and we require to restart the initialization.
static const std::chrono::milliseconds INITIALIZE_TIMEOUT{200};

/// Constant representing a "no error" return value for FFmpeg callback methods.
static const int NO_ERROR = 0;

using namespace avsCommon::utils;

static AVSampleFormat convertFormat(PlaybackConfiguration::SampleFormat format) {
    switch (format) {
        case PlaybackConfiguration::SampleFormat::UNSIGNED_8:
            return AVSampleFormat::AV_SAMPLE_FMT_U8;
        case PlaybackConfiguration::SampleFormat::SIGNED_16:
            return AVSampleFormat::AV_SAMPLE_FMT_S16;
        case PlaybackConfiguration::SampleFormat::SIGNED_32:
            return AVSampleFormat::AV_SAMPLE_FMT_S32;
    }
    ACSDK_ERROR(LX("invalidFormat").d("format", static_cast<int>(format)));
    return AVSampleFormat::AV_SAMPLE_FMT_S16;
}

static LayoutMask convertLayout(PlaybackConfiguration::ChannelLayout layout) {
    switch (layout) {
        case PlaybackConfiguration::ChannelLayout::LAYOUT_MONO:
            return AV_CH_LAYOUT_MONO;
        case PlaybackConfiguration::ChannelLayout::LAYOUT_STEREO:
            return AV_CH_LAYOUT_STEREO;
        case PlaybackConfiguration::ChannelLayout::LAYOUT_SURROUND:
            return AV_CH_LAYOUT_SURROUND;
        case PlaybackConfiguration::ChannelLayout::LAYOUT_5POINT1:
            return AV_CH_LAYOUT_5POINT1;
    }
    ACSDK_ERROR(LX("invalidLayout").d("layout", static_cast<int>(layout)));
    return AV_CH_LAYOUT_STEREO;
}

std::unique_ptr<FFmpegDecoder> FFmpegDecoder::create(
    std::unique_ptr<FFmpegInputControllerInterface> inputController,
    const PlaybackConfiguration& outputConfig,
    const avsCommon::utils::mediaPlayer::SourceConfig& config) {
    if (!inputController) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullInputController"));
        return nullptr;
    }

    AVSampleFormat format = convertFormat(outputConfig.sampleFormat());
    auto layout = convertLayout(outputConfig.channelLayout());
    int sampleRate = outputConfig.sampleRate();

    return std::unique_ptr<FFmpegDecoder>(
        new FFmpegDecoder(std::move(inputController), format, layout, sampleRate, config));
}

FFmpegDecoder::FFmpegDecoder(
    std::unique_ptr<FFmpegInputControllerInterface> input,
    AVSampleFormat format,
    LayoutMask layout,
    int sampleRate,
    const avsCommon::utils::mediaPlayer::SourceConfig& config) :
        m_state{DecodingState::INITIALIZING},
        m_inputController{std::move(input)},
        m_outputFormat{format},
        m_outputLayout{layout},
        m_outputRate{sampleRate},
        m_unreadData{format, layout, sampleRate},
        m_sourceConfig{config} {
}

std::pair<FFmpegDecoder::Status, size_t> FFmpegDecoder::read(Byte* buffer, size_t size) {
    if (!buffer || size == 0) {
        ACSDK_ERROR(LX("readFailed").d("reason", "invalidInput").d("buffer", buffer).d("size", size));
        return {Status::ERROR, 0};
    }

    if (m_state == DecodingState::INVALID) {
        ACSDK_ERROR(LX("readFailed").d("reason", "currentStateInvalid"));
        return {Status::ERROR, 0};
    }

    if (m_state == DecodingState::FINISHED) {
        ACSDK_DEBUG3(LX("readEmpty").d("reason", "doneDecoding"));
        return {Status::DONE, 0};
    }

    m_retryCount = 0;
    size_t bytesRead = 0;
    auto decodedFrame = std::shared_ptr<AVFrame>(av_frame_alloc(), AVFrameDeleter());
    while (m_state != DecodingState::FINISHED && m_state != DecodingState::INVALID) {
        if (!m_unreadData.isEmpty()) {
            auto lastReadSize = readData(buffer, size, bytesRead);
            if (lastReadSize == 0) {
                if (bytesRead == 0) {
                    ACSDK_ERROR(LX("readFailed").d("reason", "bufferTooSmall").d("bufferSize", size));
                    return {Status::ERROR, bytesRead};
                }
                break;
            }
            bytesRead += lastReadSize;
        } else {
            if (m_state == DecodingState::INITIALIZING) {
                initialize();
            }

            if (m_state == DecodingState::FLUSHING_RESAMPLER) {
                setState(DecodingState::FINISHED);
            }

            if (DecodingState::DECODING == m_state) {
                decode();
            }

            if (m_state <= DecodingState::FLUSHING_DECODER) {
                readDecodedFrame(decodedFrame);
            }

            if (m_state < DecodingState::FLUSHING_RESAMPLER && (decodedFrame->nb_samples > 0)) {
                resample(decodedFrame);
            }
        }
    }

    auto status = (DecodingState::INVALID == m_state)
                      ? Status::ERROR
                      : (DecodingState::FINISHED == m_state) ? Status::DONE : Status::OK;
    return {status, bytesRead};
}

static int shouldInterrupt(void* decoderPtr) {
    if (!decoderPtr) {
        ACSDK_ERROR(LX("wasInterruptedFailed").d("reason", "nullDecoderPtr"));
        return AVERROR_EXTERNAL;
    }

    if (static_cast<FFmpegDecoder*>(decoderPtr)->shouldInterruptFFmpeg()) {
        ACSDK_INFO(LX(__func__).m("FFmpeg was interrupted."));
        return AVERROR(EAGAIN);
    }
    return NO_ERROR;
}

bool FFmpegDecoder::shouldInterruptFFmpeg() {
    auto runtime =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_initializeStartTime);
    return (DecodingState::INVALID == m_state) ||
           ((DecodingState::INITIALIZING == m_state) && (runtime > INITIALIZE_TIMEOUT));
}

void FFmpegDecoder::initialize() {
    FFmpegInputControllerInterface::Result result;
    std::chrono::milliseconds initialPosition;
    std::tie(result, m_formatContext, initialPosition) = m_inputController->getCurrentFormatContext();
    if (!m_formatContext) {
        if (FFmpegInputControllerInterface::Result::TRY_AGAIN == result) {
            ACSDK_DEBUG(LX("initializeFailed").d("reason", "Data unavailable. Try again."));
            sleep();
            return;
        }

        if (FFmpegInputControllerInterface::Result::OK_EMPTY == result) {
            ACSDK_DEBUG(LX("finishedEarly").d("reason", "Empty data."));
            setState(DecodingState::FINISHED);
            return;
        }

        ACSDK_ERROR(LX("initializeFailed").d("reason", "getInputFormatContextFailed").d("result", result));
        setState(DecodingState::INVALID);
        return;
    }

    m_formatContext->interrupt_callback.callback = shouldInterrupt;
    m_formatContext->interrupt_callback.opaque = this;
    m_initializeStartTime = std::chrono::steady_clock::now();

    auto status = avformat_find_stream_info(m_formatContext.get(), nullptr);
    if (!transitionStateUsingStatus(status, DecodingState::INVALID, "initialize::findStreamInfo")) {
        return;
    }

    AVCodec* codec = nullptr;  // We don't own the codec.
    auto streamIndex = av_find_best_stream(m_formatContext.get(), AVMEDIA_TYPE_AUDIO, -1, -1, &codec, NO_FLAGS);
    if (!transitionStateUsingStatus(streamIndex, DecodingState::INVALID, "initialize::findBestStream")) {
        return;
    }

    if (initialPosition != std::chrono::milliseconds::zero()) {
        // Convert the offset given in millisecond to the codec timebase.
        ACSDK_DEBUG(LX("initialPosition").d("offset(ms)", initialPosition.count()));
        auto timebase = m_formatContext->streams[streamIndex]->time_base;
        int64_t timestamp =
            std::chrono::duration_cast<std::chrono::seconds>(initialPosition).count() * timebase.den / timebase.num;
        status = av_seek_frame(m_formatContext.get(), streamIndex, timestamp, NO_FLAGS);
        if (!transitionStateUsingStatus(status, DecodingState::INVALID, "initialize::seekFrame")) {
            return;
        }
    }

    m_codecContext = std::shared_ptr<AVCodecContext>(avcodec_alloc_context3(codec), AVContextDeleter());
    avcodec_parameters_to_context(m_codecContext.get(), m_formatContext->streams[streamIndex]->codecpar);
    status = avcodec_open2(m_codecContext.get(), codec, nullptr);
    if (!transitionStateUsingStatus(status, m_state, "initialize::openCodec")) {
        return;
    }

    if (0 == m_codecContext->channel_layout) {
        // Some codecs do not fill up this property, so use default layout.
        m_codecContext->channel_layout = av_get_default_channel_layout(m_codecContext->channels);
    }

    m_swrContext = std::shared_ptr<SwrContext>(
        swr_alloc_set_opts(
            nullptr,
            m_outputLayout,                  // output channels
            m_outputFormat,                  // output format (signed 16 bits)
            m_outputRate,                    // output sample rate
            m_codecContext->channel_layout,  // input channel layout
            m_codecContext->sample_fmt,      // input sample format
            m_codecContext->sample_rate,     // input sample rate
            0,                               // logging
            NULL),
        SwrContextDeleter());
    if (!m_swrContext) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "allocResamplerFailed"));
        setState(DecodingState::INVALID);
        return;
    }

    status = swr_init(m_swrContext.get());
    if (!transitionStateUsingStatus(status, DecodingState::INVALID, "initialize::initContext")) {
        return;
    }

    if (!initializeFilters()) {
        return;
    }

    setState(DecodingState::DECODING);
}

bool FFmpegDecoder::initializeFilters() {
    if (m_sourceConfig.fadeInConfig.enabled) {
        m_filterGraph = std::shared_ptr<AVFilterGraph>(avfilter_graph_alloc(), AVFilterGraphDeleter());
        if (!m_filterGraph) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "unable to allocate filter graph"));
            setState(DecodingState::INVALID);
            return false;
        }

        // First allocated and initialize the abuffer filter which is used to buffer input to the graph.
        const AVFilter* abufferFilter = avfilter_get_by_name("abuffer");
        if (!abufferFilter) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "unable to find abuffer filter"));
            setState(DecodingState::INVALID);
            return false;
        }

        m_filterInput = std::shared_ptr<AVFilterContext>(
            avfilter_graph_alloc_filter(m_filterGraph.get(), abufferFilter, "abufferFilter"), AVFilterContextDeleter());
        if (!m_filterInput) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "unable to allocate abuffer context"));
            setState(DecodingState::INVALID);
            return false;
        }

        std::bitset<64> channelBits(m_codecContext->channel_layout);
        int64_t channelCount = channelBits.count();
        av_opt_set_int(m_filterInput.get(), "channels", channelCount, AV_OPT_SEARCH_CHILDREN);
        char layoutBuffer[64];
        av_get_channel_layout_string(layoutBuffer, sizeof(layoutBuffer), channelCount, m_codecContext->channel_layout);
        av_opt_set(m_filterInput.get(), "channel_layout", layoutBuffer, AV_OPT_SEARCH_CHILDREN);
        av_opt_set_sample_fmt(m_filterInput.get(), "sample_fmt", m_codecContext->sample_fmt, AV_OPT_SEARCH_CHILDREN);
        av_opt_set_int(m_filterInput.get(), "sample_rate", m_codecContext->sample_rate, AV_OPT_SEARCH_CHILDREN);

        if (avfilter_init_str(m_filterInput.get(), nullptr) < 0) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "unable to initialize abuffer context"));
            setState(DecodingState::INVALID);
            return false;
        }

        // Next, allocate and initialize the volume filter which performs the fade.
        const AVFilter* volumeFilter = avfilter_get_by_name("volume");
        if (!volumeFilter) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "unable to find volume filter"));
            setState(DecodingState::INVALID);
            return false;
        }

        AVFilterContext* volumeContext = avfilter_graph_alloc_filter(m_filterGraph.get(), volumeFilter, "volumeFilter");
        if (!volumeContext) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "unable to allocate volume filter context"));
            setState(DecodingState::INVALID);
            return false;
        }

        // We have to adjust the time argument ('t') to the fade linear interpolation.
        // This filter may operate on an audio clip that has less duration than the duration of the
        // alarm volume ramp. In that case, we need to know how much audio we have already faded,
        // so we adjust 't' by m_decodedSampleTime

        // Create the AVExpr in the volume filter parameters to configure the volume ramp.
        char expressionBuffer[128];
        std::snprintf(
            expressionBuffer,
            sizeof(expressionBuffer),
            "volume='min(1,((t+%llu)/%llu)+%.2f)':eval=frame",
            m_decodedSampleTime,
            m_sourceConfig.fadeInConfig.duration.count(),
            m_sourceConfig.fadeInConfig.startGain / 100.0);

        if (avfilter_init_str(volumeContext, expressionBuffer) < 0) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "unable to initialize volume filter context"));
            setState(DecodingState::INVALID);
            return false;
        }

        // Next allocate the buffer sink filter
        const AVFilter* abuffersinkFilter = avfilter_get_by_name("abuffersink");
        if (!abuffersinkFilter) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "unable to find abuffersink filter"));
            setState(DecodingState::INVALID);
            return false;
        }

        m_filterOutput = std::shared_ptr<AVFilterContext>(
            avfilter_graph_alloc_filter(m_filterGraph.get(), abuffersinkFilter, "abuffersinkFilter"),
            AVFilterContextDeleter());
        if (!m_filterOutput.get()) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "unable to allocate abuffersink context"));
            setState(DecodingState::INVALID);
            return false;
        }

        if (avfilter_init_str(m_filterOutput.get(), nullptr) < 0) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "unable to initialize abuffersink context"));
            setState(DecodingState::INVALID);
            return false;
        }

        // Connect all filters in the graph.
        if (avfilter_link(m_filterInput.get(), 0, volumeContext, 0) < 0 ||
            avfilter_link(volumeContext, 0, m_filterOutput.get(), 0) < 0) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "unable to link filter graph"));
            setState(DecodingState::INVALID);
            return false;
        }

        // Configure (initialize) the entire graph.
        // Now frames can be written into the m_filterInput source and received
        // from the m_filterOutput.get() sink.
        if (avfilter_graph_config(m_filterGraph.get(), NULL) < 0) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "unable to configure filter graph"));
            setState(DecodingState::INVALID);
            return false;
        }
    }
    return true;
}

size_t FFmpegDecoder::readData(Byte* buffer, size_t size, size_t bytesRead) {
    // TODO(ACSDK-1517): use av_samples_copy to partially copy the frame and avoid buffer too small issue.
    auto& frame = m_unreadData.getFrame();

    size_t sampleSizeBytes = av_samples_get_buffer_size(
        nullptr, frame.channels, frame.nb_samples, (AVSampleFormat)frame.format, NO_ALIGNMENT);
    if ((sampleSizeBytes % (sizeof(buffer[0]) * frame.channels)) != 0) {
        // Sample size should be format size * number of channels. This may cause glitches in the audio.
        ACSDK_WARN(LX("readDataTruncated")
                       .d("reason", "Unexpected sample size")
                       .d("sampleSize", sampleSizeBytes)
                       .d("wordSize", sizeof(buffer[0]))
                       .d("channels", frame.channels));
    }

    if (size >= bytesRead + sampleSizeBytes) {
        // Have enough space. Read the entire frame.
        size_t writeOffset = bytesRead;
        memcpy(buffer + writeOffset, frame.data[0], sampleSizeBytes);
        m_unreadData.setOffset(frame.nb_samples);
        return sampleSizeBytes;
    }

    return 0;
}

void FFmpegDecoder::resample(std::shared_ptr<AVFrame> inputFrame) {
    // Filter the frame for alarm volume ramp.
    if (m_sourceConfig.fadeInConfig.enabled) {
        auto frame = inputFrame.get();
        auto error = av_buffersrc_add_frame_flags(m_filterInput.get(), frame, AV_BUFFERSRC_FLAG_PUSH);
        if (!transitionStateUsingStatus(error, DecodingState::INVALID, __func__)) {
            return;
        }
        error = av_buffersink_get_frame(m_filterOutput.get(), frame);
        if (!transitionStateUsingStatus(error, DecodingState::INVALID, __func__)) {
            return;
        }
    }

    // Increase the measurement of total audio decoded time by the amount of audio present
    // in the frame.
    double secondsPerSample = 1.0 / inputFrame->sample_rate;
    double frameTimeSeconds = secondsPerSample * inputFrame->nb_samples;
    m_decodedSampleTime += (frameTimeSeconds * 1000);  // convert to milliseconds

    int outSamples = av_rescale_rnd(
        swr_get_delay(m_swrContext.get(), m_codecContext->sample_rate) + inputFrame->nb_samples,
        m_outputRate,
        m_codecContext->sample_rate,
        AV_ROUND_UP);
    m_unreadData.resize(outSamples);
    auto error = swr_convert_frame(m_swrContext.get(), &m_unreadData.getFrame(), inputFrame.get());
    transitionStateUsingStatus(error, DecodingState::INVALID, __func__);
}

void FFmpegDecoder::decode() {
    auto packet = std::shared_ptr<AVPacket>(av_packet_alloc(), AVPacketDeleter());
    auto status = av_read_frame(m_formatContext.get(), packet.get());
    if (transitionStateUsingStatus(status, m_state, "decode::readFrame")) {
        if (AVERROR_EOF == status) {
            if (!m_inputController->hasNext()) {
                setState(DecodingState::FLUSHING_DECODER);
            } else {
                next();
            }
        }

        // Note: We still need to send empty packet when we find an EOF.
        status = avcodec_send_packet(m_codecContext.get(), packet.get());
        transitionStateUsingStatus(status, m_state, "decode::sendPacket");
    }
}

void FFmpegDecoder::next() {
    if (!m_inputController->next()) {
        ACSDK_ERROR(LX("nextFailed").d("reason", "inputNextFailed"));
        setState(DecodingState::INVALID);
    } else {
        setState(DecodingState::INITIALIZING);
    }
}

void FFmpegDecoder::readDecodedFrame(std::shared_ptr<AVFrame>& decodedFrame) {
    auto status = avcodec_receive_frame(m_codecContext.get(), decodedFrame.get());
    transitionStateUsingStatus(status, DecodingState::FLUSHING_RESAMPLER, __func__);
}

void FFmpegDecoder::abort() {
    setState(DecodingState::INVALID);
    m_abortCondition.notify_one();
}

void FFmpegDecoder::sleep() {
    /// Approximate amount of time to wait between retries in ms.
    static std::vector<int> retryTable = {10, 25, 50, 100, 200};
    auto waitTime = avsCommon::utils::RetryTimer(retryTable).calculateTimeToRetry(m_retryCount);
    std::mutex mutex;
    std::unique_lock<std::mutex> lock{mutex};
    m_abortCondition.wait_for(lock, waitTime);
    m_retryCount++;
}

void FFmpegDecoder::setState(FFmpegDecoder::DecodingState nextState) {
    DecodingState expectedState;
    switch (nextState) {
        case DecodingState::INITIALIZING:
            expectedState = DecodingState::DECODING;
            break;
        case DecodingState::DECODING:
            expectedState = DecodingState::INITIALIZING;
            break;
        case DecodingState::FLUSHING_DECODER:
            expectedState = DecodingState::DECODING;
            break;
        case DecodingState::FLUSHING_RESAMPLER:
            expectedState = DecodingState::FLUSHING_DECODER;
            break;
        case DecodingState::FINISHED:
            // Transitions to FINISHED are possible from INITIALIZED or FLUSHING_RESAMPLER only.
            expectedState = DecodingState::FLUSHING_RESAMPLER;
            if (m_state.compare_exchange_strong(expectedState, nextState)) {
                ACSDK_DEBUG5(LX("setState").d("from", expectedState).d("to", nextState));
                return;
            }

            expectedState = DecodingState::INITIALIZING;
            if (m_state.compare_exchange_strong(expectedState, nextState)) {
                ACSDK_DEBUG5(LX("setState").d("from", expectedState).d("to", nextState));
                return;
            }

            ACSDK_ERROR(LX("InvalidTransition").d("from", m_state).d("to", nextState));
            return;
        case DecodingState::INVALID:
            // All transitions to invalid are possible.
            ACSDK_DEBUG5(LX("setState").d("from", m_state).d("to", DecodingState::INVALID));
            m_state = DecodingState::INVALID;
            return;
    }

    if (!m_state.compare_exchange_strong(expectedState, nextState)) {
        ACSDK_ERROR(LX("InvalidTransition").d("from", m_state).d("to", nextState));
        return;
    }
    ACSDK_DEBUG5(LX("setState").d("from", expectedState).d("to", nextState));
}

bool FFmpegDecoder::transitionStateUsingStatus(
    int status,
    FFmpegDecoder::DecodingState nextState,
    const std::string& functionName) {
    if (status < 0) {
        // We'll try to keep decoding if error was due to buffer under run or corrupted data (e.g.: ACSDK-1684).
        if (-EAGAIN == status || AVERROR_INVALIDDATA == status) {
            ACSDK_ERROR(LX(functionName + "Failed").d("error", "tryAgain"));
            // Manually reset these variables since aviobuf::fill_buffer() set eof_reached even for EAGAIN error, which
            // invalidate future read operations.
            m_formatContext->pb->eof_reached = 0;
            m_formatContext->pb->error = 0;
            sleep();
            return false;
        }

        if (status != AVERROR_EOF) {
            ACSDK_ERROR(LX(functionName + "Failed").d("error", av_err2str(status)));
            setState(DecodingState::INVALID);
            return false;
        }

        if (nextState != m_state) {
            setState(nextState);
        }
    }
    return true;
}

FFmpegDecoder::UnreadData::UnreadData(AVSampleFormat format, LayoutMask layout, int sampleRate) :
        m_capacity{0},
        m_offset{0},
        m_frame{av_frame_alloc(), AVFrameDeleter()} {
    m_frame->format = format;
    m_frame->sample_rate = sampleRate;
    m_frame->channel_layout = layout;
}

AVFrame& FFmpegDecoder::UnreadData::getFrame() {
    return *m_frame;
}

int FFmpegDecoder::UnreadData::getOffset() const {
    return m_offset;
}

void FFmpegDecoder::UnreadData::resize(size_t minimumCapacity) {
    if (m_capacity < minimumCapacity) {
        auto oldFrame = m_frame;
        m_frame = std::shared_ptr<AVFrame>(av_frame_alloc(), AVFrameDeleter());
        m_frame->format = oldFrame->format;
        m_frame->sample_rate = oldFrame->sample_rate;
        m_frame->channel_layout = oldFrame->channel_layout;
        m_capacity = minimumCapacity;
    }
    m_frame->nb_samples = m_capacity;
    m_offset = 0;
}

bool FFmpegDecoder::UnreadData::isEmpty() const {
    return m_frame->nb_samples <= m_offset || !m_frame->data[0];
}

void FFmpegDecoder::UnreadData::setOffset(int offset) {
    m_offset = offset;
}

#define STATE_TO_STREAM(name, stream)        \
    case FFmpegDecoder::DecodingState::name: \
        return stream << #name;

std::ostream& operator<<(std::ostream& stream, const FFmpegDecoder::DecodingState state) {
    switch (state) {
        STATE_TO_STREAM(DECODING, stream)
        STATE_TO_STREAM(FLUSHING_DECODER, stream)
        STATE_TO_STREAM(FLUSHING_RESAMPLER, stream)
        STATE_TO_STREAM(FINISHED, stream)
        STATE_TO_STREAM(INVALID, stream)
        STATE_TO_STREAM(INITIALIZING, stream)
    }
    return stream << "INVALID";
}

}  // namespace android
}  // namespace mediaPlayer
}  // namespace alexaClientSDK
