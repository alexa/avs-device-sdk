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

#ifndef ALEXA_CLIENT_SDK_DIAGNOSTICS_INCLUDE_DIAGNOSTICS_DIAGNOSTICSUTILS_H_
#define ALEXA_CLIENT_SDK_DIAGNOSTICS_INCLUDE_DIAGNOSTICS_DIAGNOSTICSUTILS_H_

#include <string>
#include <vector>

#include <AVSCommon/Utils/WavUtils.h>

namespace alexaClientSDK {
namespace diagnostics {
namespace utils {

/**
 * Validates the audio format.
 *
 * Note: Following should be the specifications of the wav file :
 * Sample Size: 16 bits
 * Sample Rate: 16 KHz
 * Number of Channels : 1
 * Endianness : Little
 * Encoding Format : LPCM
 *
 * @param wavFileHeader The header of a wav file.
 * @return true if successful, else false.
 */
bool validateAudioFormat(const avsCommon::utils::WavHeader& wavHeader);

/**
 * Reads the wav file into the audio buffer vector.
 *
 * Note: Following should be the specifications of the wav file :
 * Sample Size: 16 bits
 * Sample Rate: 16 KHz
 * Number of Channels : 1
 * Endianness : Little
 * Encoding Format : LPCM
 *
 * @param absoluteFilePath The absolute path of the wav file.
 * @param [out] audioBuffer The pointer to the audioBuffer vector.
 * @return true if successful, else false.
 */
bool readWavFileToBuffer(const std::string& absoluteFilePath, std::vector<uint16_t>* audioBuffer);

}  // namespace utils
}  // namespace diagnostics
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_DIAGNOSTICS_INCLUDE_DIAGNOSTICS_DIAGNOSTICSUTILS_H_
