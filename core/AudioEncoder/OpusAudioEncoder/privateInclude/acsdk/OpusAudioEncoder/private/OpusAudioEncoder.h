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

#ifndef ACSDK_OPUSAUDIOENCODER_PRIVATE_OPUSAUDIOENCODER_H_
#define ACSDK_OPUSAUDIOENCODER_PRIVATE_OPUSAUDIOENCODER_H_

#include <memory>
#include <opus.h>

#include <acsdk/AudioEncoderInterfaces/BlockAudioEncoderInterface.h>
#include <AVSCommon/Utils/AudioFormat.h>

namespace alexaClientSDK {
namespace opusAudioEncoder {

/**
 * @brief Block audio encoder implementation using libopus as a backend library.
 */
class OpusAudioEncoder : public audioEncoderInterfaces::BlockAudioEncoderInterface {
public:
    /**
     * Factory method.
     *
     * @return A @c std::unique_ptr to a new @c OpusAudioEncoder.
     */
    static std::unique_ptr<BlockAudioEncoderInterface> createEncoder();

    /**
     * Constructor.
     */
    OpusAudioEncoder();

    /**
     * Destructor.
     */
    ~OpusAudioEncoder();

    /// @name Methods from @c BlockAudioEncoderInterface
    /// @{
    bool init(alexaClientSDK::avsCommon::utils::AudioFormat inputFormat) override;
    size_t getInputFrameSize() override;
    size_t getOutputFrameSize() override;
    bool requiresFullyRead() override;
    alexaClientSDK::avsCommon::utils::AudioFormat getAudioFormat() override;
    std::string getAVSFormatName() override;
    bool start(Bytes& preamble) override;
    bool processSamples(Bytes::const_iterator begin, Bytes::const_iterator end, Bytes& buffer) override;
    bool flush(Bytes& buffer) override;
    void close() override;
    /// @}

private:
    /**
     * @brief Helper to configure OPUS handle.
     *
     * This methods prepares @a m_encoder for encoding operation.
     *
     * @return true when success.
     */
    bool configureEncoder();

    /// OPUS encoder handle
    OpusEncoder* m_encoder;

    /// @c AudioFormat to describe output format
    alexaClientSDK::avsCommon::utils::AudioFormat m_outputFormat;

    /// @c AudioFormat to describe input format
    alexaClientSDK::avsCommon::utils::AudioFormat m_inputFormat;
};

}  // namespace opusAudioEncoder
}  // namespace alexaClientSDK

#endif  // ACSDK_OPUSAUDIOENCODER_PRIVATE_OPUSAUDIOENCODER_H_
