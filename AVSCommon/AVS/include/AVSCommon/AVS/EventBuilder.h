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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_EVENTBUILDER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_EVENTBUILDER_H_

#include <string>
#include <unordered_set>
#include <utility>

#include <AVSCommon/Utils/Optional.h>

#include "AVSCommon/AVS/AVSContext.h"
#include "AVSCommon/AVS/AVSMessageEndpoint.h"
#include "AVSCommon/AVS/AVSMessageHeader.h"
#include "AVSCommon/AVS/CapabilityConfiguration.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace constants {

/// The namespace key in the header of the event.
static const std::string NAMESPACE_KEY_STRING = "namespace";

/// The name key in the header of the event.
static const std::string NAME_KEY_STRING = "name";

/// The header key in the event.
static const std::string HEADER_KEY_STRING = "header";

/// The payload key in the event.
static const std::string PAYLOAD_KEY_STRING = "payload";

}  // namespace constants

/**
 * Builds a JSON event string which includes the header, the @c payload and an optional @c context.
 * The header includes the namespace, name, message Id and an optional @c dialogRequestId.
 * The message Id required for the header is a random string that is generated and added to the
 * header.
 *
 * @deprecated This method is being deprecated in favor of the method with Endpoint information.
 *
 * @param nameSpace The namespace of the event to be include in the header.
 * @param eventName The name of the event to be include in the header.
 * @param dialogRequestIdString Optional value associated with the "dialogRequestId" key.
 * @param payload Optional payload value associated with the "payload" key.
 * @param context Optional @c context to be sent with the event message.
 * @return A pair object consisting of the messageId and the event JSON string if successful,
 * else a pair of empty strings.
 */
const std::pair<std::string, std::string> buildJsonEventString(
    const std::string& nameSpace,
    const std::string& eventName,
    const std::string& dialogRequestIdValue = "",
    const std::string& jsonPayloadValue = "{}",
    const std::string& jsonContext = "");

/**
 * Builds a JSON event string which includes the header, optional endpoint (if the endpoint is the source
 * of the event), the @c payload, and jsonContext.
 *
 * @param eventHeader The event's @c AVSMessageHeader.
 * @param endpoint The optional endpoint which was the source of this event.
 * @param jsonPayloadValue The payload value associated with the "payload" key. The value must be a stringified json.
 * @param jsonContext The context value associated with the "context" key. The value must be a stringified json.
 * @return The event JSON string if successful, else an empty string.
 */
std::string buildJsonEventString(
    const AVSMessageHeader& eventHeader,
    const utils::Optional<AVSMessageEndpoint>& endpoint,
    const std::string& jsonPayloadValue,
    const std::string& jsonContext);

/**
 * Builds a JSON event string which includes the header, the @c payload and an optional @c context.
 * The header includes the namespace, name, message Id and an optional @c dialogRequestId.
 * The message Id required for the header is a random string that is generated and added to the
 * header.
 *
 * @param eventHeader The event's @c AVSMessageHeader.
 * @param endpoint The optional endpoint which was the source of this event.
 * @param jsonPayloadValue The payload value associated with the "payload" key. The value must be a stringified json.
 * @param context Optional @c AVSContext to be sent with the event message.
 * @return The event JSON string if successful, else an empty string.
 */
std::string buildJsonEventString(
    const AVSMessageHeader& eventHeader,
    const utils::Optional<AVSMessageEndpoint>& endpoint = utils::Optional<AVSMessageEndpoint>(),
    const std::string& jsonPayloadValue = "{}",
    const utils::Optional<AVSContext>& context = utils::Optional<AVSContext>());

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_EVENTBUILDER_H_
