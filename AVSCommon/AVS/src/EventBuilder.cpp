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

#include <map>

#include "AVSCommon/AVS/EventBuilder.h"

#include "AVSCommon/Utils/JSON/JSONGenerator.h"
#include "AVSCommon/Utils/Logger/LogEntry.h"
#include "AVSCommon/Utils/Logger/Logger.h"
#include "AVSCommon/Utils/Metrics.h"
#include "AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

using namespace utils;
using namespace avs::constants;

/// String to identify log entries originating from this file.
static const std::string TAG("EventBuilder");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The context key in the event.
static const std::string CONTEXT_KEY_STRING = "context";

/// The cookie key.
static const std::string COOKIE_KEY_STRING = "cookie";

/// The dialog request Id key in the header of the event.
static const std::string DIALOG_REQUEST_ID_KEY_STRING = "dialogRequestId";

/// The endpoint key.
static const std::string ENDPOINT_KEY_STRING = "endpoint";

/// The endpointId key.
static const std::string ENDPOINT_ID_KEY_STRING = "endpointId";

/// The event key.
static const std::string EVENT_KEY_STRING = "event";

/// The instance key.
static const std::string INSTANCE_KEY_STRING = "instance";

/// The message key in the event.
static const std::string MESSAGE_KEY_STRING = "message";

/// The message id key in the header of the event
static const std::string MESSAGE_ID_KEY_STRING = "messageId";

/// The scope key.
static const std::string SCOPE_KEY_STRING = "scope";

/// The scope type.
static const std::string SCOPE_TYPE_KEY_STRING = "type";

/// The scope token.
static const std::string SCOPE_TOKEN_KEY_STRING = "token";

/// The scope partition.
static const std::string SCOPE_PARTITION_KEY_STRING = "partition";

/// The scope user id.
static const std::string SCOPE_USER_ID_KEY_STRING = "userId";

/// The bearer token type.
static const std::string BEARER_TOKEN_TYPE = "BearerToken";

/// The bearer token with partition type.
static const std::string BEARER_TOKEN_WITH_PARTITION_TYPE = "BearerTokenWithPartition";

/// The type key in the event.
static const std::string TYPE_KEY_STRING = "type";

/**
 * Builds a JSON header object. The header includes the namespace, name, message Id and an optional
 * @c dialogRequestId. The message Id required for the header is a random string that is generated and added to the
 * header.
 *
 * @param nameSpace The namespace of the event to be included in the header.
 * @param eventName The name of the event to be included in the header.
 * @param dialogRequestIdValue The value associated with the "dialogRequestId" key.
 * @param jsonGenerator The generator that will be populated with the header json.
 * @return The generated messageId of the header.
 */
static std::string buildHeader(
    const std::string& nameSpace,
    const std::string& eventName,
    const std::string& dialogRequestIdValue,
    json::JsonGenerator& jsonGenerator) {
    jsonGenerator.startObject(HEADER_KEY_STRING);

    auto messageId = avsCommon::utils::uuidGeneration::generateUUID();
    jsonGenerator.addMember(NAMESPACE_KEY_STRING, nameSpace);
    jsonGenerator.addMember(NAME_KEY_STRING, eventName);
    jsonGenerator.addMember(MESSAGE_ID_KEY_STRING, messageId);

    if (!dialogRequestIdValue.empty()) {
        jsonGenerator.addMember(DIALOG_REQUEST_ID_KEY_STRING, dialogRequestIdValue);
    }
    jsonGenerator.finishObject();
    return messageId;
}

const std::pair<std::string, std::string> buildJsonEventString(
    const std::string& nameSpace,
    const std::string& eventName,
    const std::string& dialogRequestIdValue,
    const std::string& jsonPayloadValue,
    const std::string& jsonContext) {
    const std::pair<std::string, std::string> emptyPair;

    json::JsonGenerator jsonGenerator;
    if (!jsonContext.empty()) {
        if (!jsonGenerator.addRawJsonMember(CONTEXT_KEY_STRING, jsonContext)) {
            ACSDK_ERROR(
                LX("buildJsonEventStringFailed").d("reason", "parseContextFailed").sensitive("context", jsonContext));
            return emptyPair;
        }
    }

    jsonGenerator.startObject(EVENT_KEY_STRING);
    auto messageId = buildHeader(nameSpace, eventName, dialogRequestIdValue, jsonGenerator);
    if (eventName == "SpeechStarted" || eventName == "SpeechFinished" || eventName == "Recognize") {
        ACSDK_METRIC_IDS(TAG, eventName, messageId, dialogRequestIdValue, Metrics::Location::BUILDING_MESSAGE);
    }

    jsonGenerator.addRawJsonMember(PAYLOAD_KEY_STRING, jsonPayloadValue);
    jsonGenerator.finishObject();

    ACSDK_DEBUG(LX("buildJsonEventString").d("messageId", messageId).d("namespace", nameSpace).d("name", eventName));

    auto eventJson = jsonGenerator.toString();
    ACSDK_DEBUG0(LX(__func__).d("event", eventJson));
    return std::make_pair(messageId, eventJson);
}

/**
 * Add a json object from a map with the given name.
 * @param name The name of the object.
 * @param map The key, value pairs to add.
 * @param jsonGenerator The json generator being used to generate the json object.
 */
static void addJsonObjectFromMap(
    const std::string& name,
    const std::map<std::string, std::string>& map,
    json::JsonGenerator& jsonGenerator) {
    if (!map.empty()) {
        jsonGenerator.startObject(name);
        for (const auto& element : map) {
            jsonGenerator.addMember(element.first, element.second);
        }
        jsonGenerator.finishObject();
    }
}

/**
 * Adds a json object representing the @c endpoint.
 *
 * @param endpoint The endpoint to be encoded as a json object.
 * @param jsonGenerator The json generator being used to generate the json object.
 */
static void addEndpointToJson(const AVSMessageEndpoint& endpoint, json::JsonGenerator& jsonGenerator) {
    jsonGenerator.startObject(ENDPOINT_KEY_STRING);
    jsonGenerator.addMember(ENDPOINT_ID_KEY_STRING, endpoint.endpointId);
    addJsonObjectFromMap(COOKIE_KEY_STRING, endpoint.cookies, jsonGenerator);
    jsonGenerator.finishObject();
}

std::string buildJsonEventString(
    const AVSMessageHeader& eventHeader,
    const Optional<AVSMessageEndpoint>& endpoint,
    const std::string& jsonPayloadValue,
    const Optional<AVSContext>& context) {
    json::JsonGenerator jsonGenerator;
    jsonGenerator.startObject(EVENT_KEY_STRING);
    {
        if (endpoint.hasValue()) {
            addEndpointToJson(endpoint.value(), jsonGenerator);
        }
        jsonGenerator.addRawJsonMember(HEADER_KEY_STRING, eventHeader.toJson());
        jsonGenerator.addRawJsonMember(PAYLOAD_KEY_STRING, jsonPayloadValue);
    }
    jsonGenerator.finishObject();

    if (context.hasValue()) {
        jsonGenerator.addRawJsonMember(CONTEXT_KEY_STRING, context.value().toJson());
    }
    return jsonGenerator.toString();
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
