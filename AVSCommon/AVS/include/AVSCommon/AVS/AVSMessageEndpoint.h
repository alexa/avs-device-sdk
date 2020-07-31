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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_AVSMESSAGEENDPOINT_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_AVSMESSAGEENDPOINT_H_

#include <map>
#include <string>

#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/**
 * The structure representing the endpoint attributes that may be included in AVS Directives and Events.
 *
 * See https://developer.amazon.com/docs/alexa/alexa-voice-service/versioning.html for more details.
 */
struct AVSMessageEndpoint {
    /**
     * Default constructor.
     */
    AVSMessageEndpoint() = default;

    /**
     * Constructor.
     *
     * @param endpointId This endpoint id.
     */
    AVSMessageEndpoint(const std::string& endpointId);

    /// The mandatory endpoint identifier which should uniquely identify a single endpoint across all user devices.
    const std::string endpointId;

    /// The optional custom key value pair associated with the device.
    std::map<std::string, std::string> cookies;
};

inline AVSMessageEndpoint::AVSMessageEndpoint(const std::string& endpointId) : endpointId{endpointId} {
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_AVSMESSAGEENDPOINT_H_
