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

#ifndef ALEXA_CLIENT_SDK_SPEECHENCODER_OPUSENCODERCONTEXT_INCLUDE_SPEECHENCODER_OPUSENCODERCONTEXT_H_
#define ALEXA_CLIENT_SDK_SPEECHENCODER_OPUSENCODERCONTEXT_INCLUDE_SPEECHENCODER_OPUSENCODERCONTEXT_H_

#include <memory>

#include "SpeechEncoder/EncoderContext.h"

// We don't want to expose OPUS related structure type for user who utilize this class.
// (And those types are only used in private fields anyway.)
#ifdef OPUS_H
// Only an implementation file should know about the actual type.
#define OPUS_ENCODER OpusEncoder
#else
// Otherwise keep it general void pointer, to avoid opus.h dependency.
#define OPUS_ENCODER void
#endif

namespace alexaClientSDK {
namespace speechencoder {

/**
 * @c EncoderContext implemenation using libopus as a backend library
 */
class OpusEncoderContext : public EncoderContext {
public:
    /**
     * Constructor.
     */
    OpusEncoderContext() = default;

    /**
     * Destructor.
     */
    ~OpusEncoderContext();

    /**
     * Pre-initialization. Will verify given @c AudioFormat if the format is
     * acceptable. The correct getAudioFormat will available only after this
     * call is completed successfully.
     *
     * @param inputFormat The @c AudioFormat describes the audio format of the
     * future incoming PCM frames.
     * @return true when initialization success.
     */
    bool init(alexaClientSDK::avsCommon::utils::AudioFormat inputFormat) override;

    /**
     * The maximum number of sample can be processed at the same time.
     *
     * @return The number of sample (in words).
     */
    size_t getInputFrameSize() override;

    /**
     * The maximum length of the single encoded frame.
     *
     * @return The maximum output length (in bytes).
     */
    size_t getOutputFrameSize() override;

    /**
     * Returns true when CBR.
     *
     * @return true if processSamples should be called with fixed length of
     * input samples during the session.
     */
    bool requiresFullyRead() override;

    /**
     * The @c AudioFormat describes the encoded audio stream.
     *
     * @return The @c AudioFormat describes the encoded audio stream.
     */
    alexaClientSDK::avsCommon::utils::AudioFormat getAudioFormat() override;

    /**
     * The string interpretation of the OPUS format, that AVS cloud service
     * can recognize.
     *
     * @return Output format string.
     */
    std::string getAVSFormatName() override;

    /**
     * This will allocate new libopus encoder state and perform CTL functions.
     *
     * @return true when success.
     */
    bool start() override;

    /**
     * Encode next PCM samples.
     *
     * @param samples PCM samples to be encoded. The word size should be known
     * via @c AudioFormat that has provided at pre-init function.
     * @param numberOfWords The number of samples (in words).
     * @param buffer The buffer where the encoded frames should be written.
     * @return The total bytes of encoded frames has been written into buffer.
     * Negative value when error is occurred.
     */
    ssize_t processSamples(void* samples, size_t numberOfWords, uint8_t* buffer) override;

    /**
     * Destroy current libopus encoder state.
     */
    void close() override;

private:
    /**
     * Perform @c opus_encoder_ctl() calls.
     *
     * @return true when success.
     */
    bool configureEncoder();

    /// OPUS encoder handle
    OPUS_ENCODER* m_encoder = NULL;

    /// @c AudioFormat to describe output format
    alexaClientSDK::avsCommon::utils::AudioFormat m_outputFormat;

    /// @c AudioFormat to describe input format
    alexaClientSDK::avsCommon::utils::AudioFormat m_inputFormat;
};

}  // namespace speechencoder
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SPEECHENCODER_OPUSENCODERCONTEXT_INCLUDE_SPEECHENCODER_OPUSENCODERCONTEXT_H_