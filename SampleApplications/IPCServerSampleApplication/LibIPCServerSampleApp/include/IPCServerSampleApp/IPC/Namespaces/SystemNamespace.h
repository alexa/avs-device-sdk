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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_SYSTEMNAMESPACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_SYSTEMNAMESPACE_H_

#include <string>

#include <rapidjson/document.h>

#include "IPCServerSampleApp/Messages/Message.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {
namespace IPCNamespaces {

/// The namespace namespace for System.
static const char IPC_MSG_NAMESPACE_SYSTEM[] = "System";

/// The namespace version for System.
static const int IPC_MSG_VERSION_SYSTEM(1);

/// The message name for completeAuthorization.
static const char IPC_MSG_NAME_COMPLETE_AUTH[] = "completeAuthorization";

/// The message name for setAlexaState.
static const char IPC_MSG_NAME_SET_ALEXA_STATE[] = "setAlexaState";

/// The message name for setAuthorizationState.
static const char IPC_MSG_NAME_SET_AUTH_STATE[] = "setAuthorizationState";

/// The message name for setLocales.
static const char IPC_MSG_NAME_SET_LOCALES[] = "setLocales";

/// The locales json key in the message.
static const char IPC_MSG_LOCALES_TAG[] = "locales";

/// The auth url json key in the message.
static const char IPC_MSG_AUTH_URL_TAG[] = "url";

/// The auth code json key in the message.
static const char IPC_MSG_AUTH_CODE_TAG[] = "code";

/// The clientId json key in the message.
static const char IPC_MSG_CLIENT_ID_TAG[] = "clientId";

/**
 * The @c CompleteAuthorizationMessage provides the GUI Client with information to present to the user to complete CBL
 * device authorization.
 */
class CompleteAuthorizationMessage : public messages::Message {
public:
    /**
     * Constructor.
     *
     * @param url The URL that the user needs to navigate to.
     * @param code The code that the user needs to enter once authorized.
     * @param authClientId The device's Client Id.
     */
    CompleteAuthorizationMessage(const std::string& url, const std::string& code, const std::string& authClientId) :
            Message(IPC_MSG_NAMESPACE_SYSTEM, IPC_MSG_VERSION_SYSTEM, IPC_MSG_NAME_COMPLETE_AUTH) {
        addMemberInPayload(IPC_MSG_AUTH_URL_TAG, url);
        addMemberInPayload(IPC_MSG_AUTH_CODE_TAG, code);
        addMemberInPayload(IPC_MSG_CLIENT_ID_TAG, authClientId);
        addPayload();
    }
};

/**
 * The @c SetAuthorizationStateMessage provides the GUI Client with information about changes to the state of
 * authorization.
 */
class SetAuthorizationStateMessage : public messages::Message {
public:
    /**
     * Constructor.
     *
     * @param authorizationState The state of authorization.
     */
    SetAuthorizationStateMessage(const std::string& authorizationState) :
            Message(IPC_MSG_NAMESPACE_SYSTEM, IPC_MSG_VERSION_SYSTEM, IPC_MSG_NAME_SET_AUTH_STATE) {
        setStateInPayload(authorizationState);
        addPayload();
    }
};

/**
 * The @c SetAlexaStateMessage contains information for communicating Alexa state to the GUI Client.
 */
class SetAlexaStateMessage : public messages::Message {
public:
    /**
     * Constructor.
     *
     * @param alexaState The state of the Alexa client.
     */
    explicit SetAlexaStateMessage(const std::string& alexaState) :
            Message(IPC_MSG_NAMESPACE_SYSTEM, IPC_MSG_VERSION_SYSTEM, IPC_MSG_NAME_SET_ALEXA_STATE) {
        setStateInPayload(alexaState);
        addPayload();
    }
};

/**
 *  The @c SetLocalesMessage informs the GUI Client of Alexa locale setting changes.
 */
class SetLocalesMessage : public messages::Message {
public:
    /**
     * Constructor.
     *
     *  @param localeStr the string containing the locale
     */
    explicit SetLocalesMessage(const std::string& localeStr) :
            Message(IPC_MSG_NAMESPACE_SYSTEM, IPC_MSG_VERSION_SYSTEM, IPC_MSG_NAME_SET_LOCALES) {
        rapidjson::Value payload(rapidjson::kObjectType);
        rapidjson::Document messageDocument(&alloc());
        payload.AddMember(
            rapidjson::Value(IPC_MSG_LOCALES_TAG, alloc()).Move(), messageDocument.Parse(localeStr), alloc());
        setPayload(std::move(payload));
    };
};

}  // namespace IPCNamespaces
}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_SYSTEMNAMESPACE_H_
