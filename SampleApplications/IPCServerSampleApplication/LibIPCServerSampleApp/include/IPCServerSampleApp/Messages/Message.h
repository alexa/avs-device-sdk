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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_MESSAGES_MESSAGE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_MESSAGES_MESSAGE_H_

#include <string>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include "MessageInterface.h"

/// The payload json key in the message.
const char MSG_PAYLOAD_TAG[] = "payload";

/// The windowId json key in the message.
const char MSG_WINDOWID_TAG[] = "windowId";

/// The enabled json key in the message.
const char MSG_ENABLED_TAG[] = "enabled";

/// The token json key in the message.
const char MSG_TOKEN_TAG[] = "token";

/// The state json key in the message.
const char MSG_STATE_TAG[] = "state";

/// String representation of empty json
const std::string EMPTY_JSON("{}");

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace messages {

/**
 * Helper class to construct a @c MessageInterface message.
 */
class Message : public MessageInterface {
private:
    rapidjson::Document mParsedDocument;

public:
    /**
     * Constructor
     * @param namespace The namespace from this message
     * @param version The version from this message
     * @param name The handler name from this message
     */
    Message(const std::string& nameSpace, const int version, const std::string& name) :
            MessageInterface(version, nameSpace, name),
            mParsedDocument(&alloc()) {
    }

    /**
     * Add a new member to the json
     * @param name The name of the new member
     * @param value The value of the new member
     * @return this
     */
    Message& addMember(const std::string& name, const std::string& value) {
        mDocument.AddMember(
            rapidjson::Value(name.c_str(), mDocument.GetAllocator()).Move(),
            rapidjson::Value(value.c_str(), mDocument.GetAllocator()).Move(),
            mDocument.GetAllocator());
        return *this;
    }

    /**
     * Add a new member to the json
     * @param name The name of the new member
     * @param value The value of the new member
     * @return this
     */
    Message& addMember(const std::string& name, unsigned value) {
        mDocument.AddMember(
            rapidjson::Value(name.c_str(), mDocument.GetAllocator()).Move(), value, mDocument.GetAllocator());
        return *this;
    }

    /**
     * Add a new member to existing payload
     * @param name The name of the new member
     * @param value The value of the new member
     * @return this
     */
    Message& addMemberInPayload(const std::string& name, const std::string& value) {
        mPayload.AddMember(
            rapidjson::Value(name.c_str(), mDocument.GetAllocator()).Move(),
            rapidjson::Value(value.c_str(), mDocument.GetAllocator()).Move(),
            mDocument.GetAllocator());
        return *this;
    }

    /**
     * Add an integer to existing payload
     * @param name The name of the new member
     * @param value The value of the new member
     * @return this
     */
    Message& addIntegerInPayload(const std::string& name, const int value) {
        mPayload.AddMember(
            rapidjson::Value(name.c_str(), mDocument.GetAllocator()).Move(), value, mDocument.GetAllocator());
        return *this;
    }

    /**
     * Add a new member to the existing payload
     * @param name The name of the new member
     * @param value The value of the new member
     * @return this
     */
    Message& addMemberInPayload(const std::string& name, unsigned value) {
        mPayload.AddMember(
            rapidjson::Value(name.c_str(), mDocument.GetAllocator()).Move(), value, mDocument.GetAllocator());
        return *this;
    }

    /**
     * Sets the json enabled for this message
     * @param enabled The enabled bool to send
     * @return this
     */
    Message& setEnabledInPayload(bool enabled) {
        addMemberInPayload(MSG_ENABLED_TAG, enabled);
        return *this;
    }

    /**
     * Sets the json state for this message
     * @param state The state to send
     * @return this
     */
    Message& setStateInPayload(const std::string& state) {
        mPayload.AddMember(
            MSG_STATE_TAG, rapidjson::Value(state.c_str(), mDocument.GetAllocator()).Move(), mDocument.GetAllocator());
        return *this;
    }

    /**
     * Sets the token for this message
     * @param token
     * @return this
     */
    Message& setTokenInPayload(const std::string& token) {
        mPayload.AddMember(MSG_TOKEN_TAG, token, mDocument.GetAllocator());
        return *this;
    }

    /**
     * Sets the token for this message
     * @param token
     * @return this
     */
    Message& setTokenInPayload(unsigned token) {
        mPayload.AddMember(MSG_TOKEN_TAG, token, mDocument.GetAllocator());
        return *this;
    }

    /**
     * Sets the json payload for this message
     * @param payload The payload to parse and send
     * @return this
     */
    Message& setParsedPayload(const std::string& payload) {
        mDocument.AddMember(MSG_PAYLOAD_TAG, mParsedDocument.Parse(payload), mDocument.GetAllocator());
        return *this;
    }

    /**
     * Sets the json payload for this message
     * @param payload The payload to parse and send
     * @return this
     */
    Message& setParsedPayloadInPayload(const std::string& payload) {
        mPayload.AddMember(MSG_PAYLOAD_TAG, mParsedDocument.Parse(payload), mDocument.GetAllocator());
        return *this;
    }

    /**
     * Sets the windowId for this message
     * @param windowId The target windowId of
     * @return this
     */
    Message& setWindowIdInPayload(const std::string& windowId) {
        mPayload.AddMember(MSG_WINDOWID_TAG, windowId, mDocument.GetAllocator());
        return *this;
    }

    /**
     * Add the payload to this message
     * @return this
     */
    Message& addPayload() {
        mDocument.AddMember(MSG_PAYLOAD_TAG, mPayload, mDocument.GetAllocator());
        return *this;
    }

    /**
     * Sets the json payload for this message
     * @param payload The payload to send
     * @return this
     */
    Message& setPayload(rapidjson::Value&& payload) {
        mDocument.AddMember(MSG_PAYLOAD_TAG, std::move(payload), mDocument.GetAllocator());
        return *this;
    }

    /**
     * Retrieves the rapidjson allocator
     * @return The allocator
     */
    auto alloc() -> decltype(mDocument.GetAllocator()) {
        return mDocument.GetAllocator();
    };

    /**
     * Retrieves the json string representing this message
     * @return json string representation of message
     */
    std::string get() {
        rapidjson::StringBuffer buffer;
        buffer.Clear();
        rapidjson::Writer<
            rapidjson::StringBuffer,
            rapidjson::UTF8<>,
            rapidjson::UTF8<>,
            rapidjson::CrtAllocator,
            rapidjson::kWriteNanAndInfFlag>
            writer(buffer);
        if (!mDocument.Accept(writer)) {
            return EMPTY_JSON;
        }
        return std::string(buffer.GetString(), buffer.GetSize());
    }

    /**
     * Retrieves the @c rapidjson::Value object representation of this message
     * @return @c rapidjson::Value object representation of message
     */
    rapidjson::Value&& getValue() {
        return std::move(mDocument);
    };
};
}  // namespace messages
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_MESSAGES_MESSAGE_H_
