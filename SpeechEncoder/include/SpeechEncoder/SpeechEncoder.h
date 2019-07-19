/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_SPEECHENCODER_INCLUDE_SPEECHENCODER_SPEECHENCODER_H_
#define ALEXA_CLIENT_SDK_SPEECHENCODER_INCLUDE_SPEECHENCODER_SPEECHENCODER_H_

#include <atomic>
#include <memory>
#include <thread>

#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/Utils/AudioFormat.h>
#include <AVSCommon/Utils/Threading/Executor.h>

#include "EncoderContext.h"

namespace alexaClientSDK {
namespace speechencoder {

/**
 * This class provides generic interface between backend encoder implementation
 * and the application who wants to encode audio stream within
 * @c AudioInputStream
 */
class SpeechEncoder {
public:
    /**
     * Constructor.
     *
     * @param encoder The backend encoder implmentation.
     */
    SpeechEncoder(const std::shared_ptr<EncoderContext>& encoder);

    /**
     * Destructor.
     */
    ~SpeechEncoder();

    /**
     * Start the new encoding session. @c SpeechEncoder can manage only single
     * session at the same time, thus this call will be failed when ongoing
     * encoding session exists, or pre-initialization on @c EncoderContext is
     * failed.
     *
     * @param inputStream The @c AudioInputStream to stream audio from. (i.e.
     * input stream that has raw PCM frames.)
     * @param inputFormat The @c AudioFormat to describe the audio format of
     * the input stream.
     * @param begin The index where encoding should begin.
     * @param reference The reference for the index.
     * @return true if encoding session has been started successfully,
     * otherwise false.
     */
    bool startEncoding(
        const std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream>& inputStream,
        alexaClientSDK::avsCommon::utils::AudioFormat inputFormat,
        avsCommon::avs::AudioInputStream::Index begin,
        avsCommon::avs::AudioInputStream::Reader::Reference reference);

    /**
     * Stop current encoding session.
     *
     * @param stopImmediately Flag indicating that encoding should stop immediatly.
     * If this flag is set to @c false (the default), encoding will continue until
     * any existing data in the buffer has been encoded. If this flag is set to
     * @c true, existing data in the buffer which has not already been encoded will
     * be discarded, and encoding will stop immediately.
     */
    void stopEncoding(bool stopImmediately = false);

    /**
     * Retrieve @c AudioInputStream for encoding results.
     *
     * @return @c AudioInputStream for streaming encoding results for further
     * processing.
     */
    std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> getEncodedStream();

    /**
     * Retrieve a @c EncoderContext that was given with initialization.
     *
     * @return @c EncoderContext
     */
    std::shared_ptr<EncoderContext> getContext();

private:
    /**
     * Thread loop
     */
    void encodeLoop(
        avsCommon::avs::AudioInputStream::Index begin,
        avsCommon::avs::AudioInputStream::Reader::Reference reference);

    /// Backend implementation
    std::shared_ptr<EncoderContext> m_encoder;

    /// Input AudioFormat (PCM)
    alexaClientSDK::avsCommon::utils::AudioFormat m_inputAudioFormat;

    /// Input AudioInputStream (i.e. PCM frames)
    std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> m_inputStream;

    /// AudioFormat initialized from EncoderContext
    alexaClientSDK::avsCommon::utils::AudioFormat m_outputAudioFormat;

    /// Maximum single encoded frame size (byte) initialized from EncoderContext
    size_t m_maxFrameSize;

    /// AudioInputStream for streaming results
    std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> m_encodedStream;

    /// true when the current session is active
    std::atomic<bool> m_isEncoding;

    /// true when stopEncoding has been called with stopImmediately=false
    std::atomic<bool> m_stopRequested;

    /// Internal Executor for managing encoding thread
    avsCommon::utils::threading::Executor m_executor;

    /// Mutex for thread safety
    std::mutex m_mutex;
};

}  // namespace speechencoder
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SPEECHENCODER_INCLUDE_SPEECHENCODER_SPEECHENCODER_H_
