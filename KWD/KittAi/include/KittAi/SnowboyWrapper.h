/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_KWD_KITTAI_INCLUDE_KITTAI_SNOWBOYWRAPPER_H_
#define ALEXA_CLIENT_SDK_KWD_KITTAI_INCLUDE_KITTAI_SNOWBOYWRAPPER_H_

namespace alexaClientSDK {
namespace kwd {

/**
 * The wrapper class for snowboy::SnowboyDetect class.
 *
 * Since the original API exposes std::string which will be ABI incompatible
 * with GCC 5.1, this class replace them with const char*.
 *
 * We keep the actual instantiation in void* pointer in this header, to prevent
 * from other files to include snowboy-detect.h file.
 *
 * Thanks to the community for the original idea:
 * https://github.com/Kitt-AI/snowboy/issues/99
 */
class SnowboyWrapper {
public:
    /**
     * Call snowboy::SnowboyDetect::SnowboyDetect constructor
     *
     * @param resourceFilename for the first argument of the original API call.
     * @param model for the second argument of the original API call.
     */
    SnowboyWrapper(const char* resourceFilename, const char* model);

    /**
     * Destructor
     */
    ~SnowboyWrapper() = default;

    /**
     * Call snowboy::SnowboyDetect::RunDetection
     *
     * @param data for the first argument of the original API call.
     * @param arrayLength for the second argument of the original API call.
     * @param isEnd for the third argument of the original API call.
     * @return the value of the original API call result.
     */
    int RunDetection(const int16_t* const data, const int arrayLength, bool isEnd = false);

    /**
     * Call snowboy::SnowboyDetect::SetSensitivity
     *
     * @param sensitivity for the first argument of the original API call.
     */
    void SetSensitivity(const char* sensitivity);

    /**
     * Call snowboy::SnowboyDetect::SetAudioGain
     *
     * @param audioGain for the first argument of the original API call.
     */
    void SetAudioGain(const float audioGain);

    /**
     * Call snowboy::SnowboyDetect::ApplyFrontend
     *
     * @param applyFrontend for the first argument of the original API call.
     */
    void ApplyFrontend(const bool applyFrontend);

    /**
     * Call snowboy::SnowboyDetect::SampleRate
     *
     * @return the value of the original API call result.
     */
    int SampleRate() const;

    /**
     * Call snowboy::SnowboyDetect::NumChannels
     *
     * @return the value of the original API call result.
     */
    int NumChannels() const;

    /**
     * Call snowboy::SnowboyDetect::BitsPerSample
     *
     * @return the value of the original API call result.
     */
    int BitsPerSample() const;

private:
    /// The actual Kitt.ai engine instantiation in pointer
    void* m_snowboy;
};

}  // namespace kwd
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_KWD_KITTAI_INCLUDE_KITTAI_SNOWBOYWRAPPER_H_
