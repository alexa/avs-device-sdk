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

#ifndef ACSDK_APLCAPABILITYCOMMONINTERFACES_APLCOMMANDEXECUTIONEVENT_H_
#define ACSDK_APLCAPABILITYCOMMONINTERFACES_APLCOMMANDEXECUTIONEVENT_H_

#include <chrono>

#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace aplCapabilityCommonInterfaces {

/// Enumeration of APL Command Execution Events that can be reported to the APL capability agent.
enum class APLCommandExecutionEvent {
    /// command sequence was resolved.
    RESOLVED,

    /// command sequence was terminated.
    TERMINATED,

    /// failed to parse/handle the command sequence.
    FAILED
};

/**
 * This is a function to convert the @c AplCommandExecutionEvent to a string.
 */
inline std::string commandExecutionEventToString(const APLCommandExecutionEvent event) {
    switch (event) {
        case APLCommandExecutionEvent::RESOLVED:
            return "RESOLVED";
        case APLCommandExecutionEvent::TERMINATED:
            return "TERMINATED";
        case APLCommandExecutionEvent::FAILED:
            return "FAILED";
    }
    return "UNKNOWN";
}

/**
 * This is a function to convert a string to a @c AplCommandExecutionEvent.
 */
inline APLCommandExecutionEvent stringToCommandExecutionEvent(const std::string& eventStr) {
    if ("RESOLVED" == eventStr) {
        return APLCommandExecutionEvent::RESOLVED;
    } else if ("TERMINATED" == eventStr) {
        return APLCommandExecutionEvent::TERMINATED;
    } else if ("FAILED" == eventStr) {
        return APLCommandExecutionEvent::FAILED;
    }

    return APLCommandExecutionEvent::FAILED;
}

}  // namespace aplCapabilityCommonInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_APLCAPABILITYCOMMONINTERFACES_APLCOMMANDEXECUTIONEVENT_H_