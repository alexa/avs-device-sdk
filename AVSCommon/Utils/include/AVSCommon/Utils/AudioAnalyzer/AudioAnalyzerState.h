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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_AUDIOANALYZER_AUDIOANALYZERSTATE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_AUDIOANALYZER_AUDIOANALYZERSTATE_H_

#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace audioAnalyzer {

/**
 * Structure to hold metadata about the AudioAnalyzerState
 */
struct AudioAnalyzerState {
    /**
     * Constructor.
     */
    AudioAnalyzerState(const std::string& interfaceName, const std::string& enableState) :
            name(interfaceName),
            enableState(enableState) {
    }

    /// Name of the audio analyzer
    std::string name;

    /// on/off state of the audio analyzer
    std::string enableState;

    bool operator==(const AudioAnalyzerState& state) const {
        return name == state.name && enableState == state.enableState;
    }
};

}  // namespace audioAnalyzer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_AUDIOANALYZER_AUDIOANALYZERSTATE_H_
