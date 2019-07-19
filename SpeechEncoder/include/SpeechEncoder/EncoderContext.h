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

#ifndef ALEXA_CLIENT_SDK_SPEECHENCODER_INCLUDE_SPEECHENCODER_ENCODERCONTEXT_H_
#define ALEXA_CLIENT_SDK_SPEECHENCODER_INCLUDE_SPEECHENCODER_ENCODERCONTEXT_H_

#include <memory>

#include <AVSCommon/Utils/AudioFormat.h>

namespace alexaClientSDK {
namespace speechencoder {

/**
 * Interface class between @c SpeechEncoder and the backend codec library. This
 * class must be implemented by customer for each encoder codecs.
 */
class EncoderContext {
public:
    /**
     * Pre-initialization before actual encoding session has began. Note that
     * this function will be called everytime before new encoding session is
     * starting.
     *
     * @param inputFormat The @c AudioFormat describes the audio format of the
     * future incoming PCM frames.
     * @return true when initialization success.
     */
    virtual bool init(alexaClientSDK::avsCommon::utils::AudioFormat inputFormat) = 0;

    /**
     * The maximum number of sample can be processed at the same time. In
     * other words, this will limits input PCM stream buffering. Thus
     * nWords of processSamples call will never exceeds this limit.
     *
     * @return The number of sample (in words).
     */
    virtual size_t getInputFrameSize() = 0;

    /**
     * The maximum length of the single encoded frame.
     *
     * @return The maximum output length (in bytes).
     */
    virtual size_t getOutputFrameSize() = 0;

    /**
     * Determine whether PCM stream should be fully buffered with the maximum
     * number of samples provided at getInputFrameSize function. This value
     * will change the behavior of how processSamples is called during the
     * encoding session. This is useful when backend encoder requires fixed
     * length of input samples.
     *
     * In case the encoding session has been shutdown before the buffere is
     * filled fully, this will cause any partial data to be discarded. (e.g.
     * stopEncoding has been called, or reaches the end of the data stream.)
     *
     * @return true if processSamples should be called with fixed length of
     * input samples during the session.
     */
    virtual bool requiresFullyRead() = 0;

    /**
     * The @c AudioFormat describes the encoded audio stream.
     *
     * @return The @c AudioFormat describes the encoded audio stream.
     */
    virtual alexaClientSDK::avsCommon::utils::AudioFormat getAudioFormat() = 0;

    /**
     * The string interpretation of the output format, that AVS cloud service
     * can recognize.
     *
     * @return Output format string.
     */
    virtual std::string getAVSFormatName() = 0;

    /**
     * When an encoding session has began, this function start will be called.
     * The backend library then may be initialized for begin encoding.
     *
     * @return true when success.
     */
    virtual bool start() = 0;

    /**
     * Encode next PCM samples. This function will be called continuously
     * throughout the session.
     *
     * @param samples PCM samples to be encoded. The word size should be known
     * via @c AudioFormat that has provided at pre-init function.
     * @param numberOfWords The number of samples (in words).
     * @param buffer The buffer where the encoded frames should be written.
     * @return The total bytes of encoded frames has been written into buffer.
     * Negative value when error is occurred.
     */
    virtual ssize_t processSamples(void* samples, size_t numberOfWords, uint8_t* buffer) = 0;

    /**
     * Notify end of the session. Any backend library then may be
     * deinitialized so it cleans memory and threads.
     */
    virtual void close() = 0;

    /**
     * Destructor.
     */
    virtual ~EncoderContext() = default;
};

}  // namespace speechencoder
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SPEECHENCODER_INCLUDE_SPEECHENCODER_ENCODERCONTEXT_H_