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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AIP_INCLUDE_AIP_ASRPROFILE_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AIP_INCLUDE_AIP_ASRPROFILE_H_

#include <ostream>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace aip {

#include <ostream>

/**
 * Enumerates the different ASR profiles supported by AVS.
 * @see https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/speechrecognizer#profiles
 */
enum class ASRProfile {
    /// Client determines end of user speech (0 to 2.5 ft).
    CLOSE_TALK,
    /// Cloud determines end of user speech (0 to 5 ft).
    NEAR_FIELD,
    /// Cloud determines end of user speech (0 to 20+ ft).
    FAR_FIELD,
    /// Indicates that ASR profile is undefined.
    UNDEFINED
};

/**
 * Write an @c ASRProfile value to an @c ostream as an AVS-compliant string.
 *
 * @param stream The stream to write the value to.
 * @param profile The profile value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, ASRProfile profile) {
    switch (profile) {
        case ASRProfile::CLOSE_TALK:
            stream << "CLOSE_TALK";
            break;
        case ASRProfile::NEAR_FIELD:
            stream << "NEAR_FIELD";
            break;
        case ASRProfile::FAR_FIELD:
            stream << "FAR_FIELD";
            break;
        case ASRProfile::UNDEFINED:
            stream << "";
            break;
    }
    return stream;
}

/**
 * Convert the @c ASRProfile to an AVS-compliant string.
 *
 * @param profile The profile value to convert to a string.
 * @return String representing the profile value. If the profile given is not valid, an empty string will be returned.
 */
inline std::string asrProfileToString(ASRProfile profile) {
    switch (profile) {
        case ASRProfile::CLOSE_TALK:
            return "CLOSE_TALK";
        case ASRProfile::NEAR_FIELD:
            return "NEAR_FIELD";
        case ASRProfile::FAR_FIELD:
            return "FAR_FIELD";
        case ASRProfile::UNDEFINED:
            return "";
    }
    return "";
}

/**
 * This function reverse maps the provided string to corresponding ASRProfile Implementation as specified by
 * asrProfileToString
 * @param input string to convert to corresponding ASRProfile
 * @return @c ASRProfile that corresponds to the input string. In case of error
 * the API returns ASRProfile::UNDEFINED
 */
inline ASRProfile getASRProfile(const std::string& input) {
    ASRProfile profile = ASRProfile::UNDEFINED;
    if (asrProfileToString(ASRProfile::CLOSE_TALK) == input) {
        profile = ASRProfile::CLOSE_TALK;
    } else if (asrProfileToString(ASRProfile::NEAR_FIELD) == input) {
        profile = ASRProfile::NEAR_FIELD;
    } else if (asrProfileToString(ASRProfile::FAR_FIELD) == input) {
        profile = ASRProfile::FAR_FIELD;
    }
    return profile;
}

}  // namespace aip
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AIP_INCLUDE_AIP_ASRPROFILE_H_
