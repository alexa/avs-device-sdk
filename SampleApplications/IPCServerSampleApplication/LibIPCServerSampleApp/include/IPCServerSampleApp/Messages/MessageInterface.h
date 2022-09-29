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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_MESSAGES_MESSAGEINTERFACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_MESSAGES_MESSAGEINTERFACE_H_

#include <string>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace messages {

/// The namespace json key in the message.
const char MSG_HEADER_TAG[] = "header";

/// The payload json key in the message.
const char MSG_PAYLOAD_TAG[] = "payload";

/// The version json key in the message.
const char MSG_VERSION_TAG[] = "version";

/// The namespace json key in the message.
const char MSG_NAMESPACE_TAG[] = "namespace";

/// The name json key in the message.
const char MSG_NAME_TAG[] = "name";

/**
 * An interface for rapidjson::Document based messages.
 *
 * All messages have the format:
 * {
 * header : { version, namespace, name }
 * payload : {}
 * }
 */
class MessageInterface {
protected:
    rapidjson::Document mDocument;
    rapidjson::Value mPayload;

public:
    /**
     * Constructor
     * @param version The version for this message
     * @param nameSpace The namespace for this message
     * @param name The type from this message
     */
    MessageInterface(const int version, const std::string& nameSpace, const std::string& name) :
            mDocument(rapidjson::kObjectType),
            mPayload(rapidjson::kObjectType) {
        auto& alloc = mDocument.GetAllocator();

        rapidjson::Value header(rapidjson::kObjectType);
        header.AddMember(MSG_VERSION_TAG, version, alloc);
        header.AddMember(MSG_NAMESPACE_TAG, nameSpace, alloc);
        header.AddMember(MSG_NAME_TAG, name, alloc);
        mDocument.AddMember(MSG_HEADER_TAG, header, alloc);
    }

    /**
     * Retrieves the json string representing this message
     * @return json string representation of message
     */
    virtual std::string get() = 0;

    /**
     * Retrieves the @c rapidjson::Value object representation of this message
     * @return @c rapidjson::Value object representation of message
     */
    virtual rapidjson::Value&& getValue() = 0;
};
}  // namespace messages
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_MESSAGES_MESSAGEINTERFACE_H_
