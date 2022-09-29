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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_WINDOWMANAGERNAMESPACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_WINDOWMANAGERNAMESPACE_H_

#include <string>

#include <rapidjson/document.h>

#include "IPCServerSampleApp/Messages/Message.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {
namespace IPCNamespaces {

/// The message namespace for Window Manager.
static const char IPC_MSG_NAMESPACE_WINDOW_MANAGER[] = "WindowManager";

/// The message name for set visual characteristics.
static const char IPC_MSG_NAME_SET_VISUAL_CHARACTERISTICS[] = "setVisualCharacteristics";

/// The message name for clearWindow.
static const char IPC_MSG_NAME_CLEAR_WINDOW[] = "clearWindow";

/// The device display json key in visual characteristics message.
static const char IPC_MSG_DEVICE_DISPLAY_TAG[] = "deviceDisplay";

/// The interaction modes json key in visual characteristics message.
static const char IPC_MSG_INTERACTION_MODES_TAG[] = "interactionModes";

/// The window templates json key in visual characteristics message.
static const char IPC_MSG_WINDOW_TEMPLATES_TAG[] = "windowTemplates";

/// The namespace version for Window Manager.
static const int IPC_MSG_VERSION_WINDOW_MANAGER(1);

/// The key in visual characteristics capability config for Device display characteristics
static const char VISUAL_CHARACTERISTICS_CAPABILITY_CONFIG_DISPLAY_TAG[] = "display";

/// The key in visual characteristics capability config for interaction modes configuration
static const char VISUAL_CHARACTERISTICS_CAPABILITY_CONFIG_INTERACTION_MODES_TAG[] = "interactionModes";

/// The key in visual characteristics capability config for window templates configuration
static const char VISUAL_CHARACTERISTICS_CAPABILITY_CONFIG_WINDOW_TEMPLATES_TAG[] = "templates";

/**
 * The @c SetVisualCharacteristicsMessage contains visual characteristics asserted by the
 * Client.
 */
class SetVisualCharacteristicsMessage : public messages::Message {
public:
    /**
     * Constructor.
     *
     * @param displayCharacteristics serialized display characteristics reported by Visual Characteristics Capability
     * @param interactionModes serialized interaction modes configuration reported by Visual Characteristics Capability
     * @param windowTemplates serialized window templates configuration reported by Visual Characteristics Capability
     */
    SetVisualCharacteristicsMessage(
        const avsCommon::utils::Optional<std::string>& displayCharacteristics,
        const avsCommon::utils::Optional<std::string>& interactionModes,
        const avsCommon::utils::Optional<std::string>& windowTemplates) :
            Message(
                IPC_MSG_NAMESPACE_WINDOW_MANAGER,
                IPC_MSG_VERSION_WINDOW_MANAGER,
                IPC_MSG_NAME_SET_VISUAL_CHARACTERISTICS) {
        rapidjson::Value payload(rapidjson::kObjectType);
        rapidjson::Document messageDocument(&alloc());

        if (displayCharacteristics.hasValue()) {
            payload.AddMember(
                IPC_MSG_DEVICE_DISPLAY_TAG,
                messageDocument.Parse(
                    displayCharacteristics.value())[VISUAL_CHARACTERISTICS_CAPABILITY_CONFIG_DISPLAY_TAG],
                alloc());
        }

        if (interactionModes.hasValue()) {
            payload.AddMember(
                IPC_MSG_INTERACTION_MODES_TAG,
                messageDocument.Parse(
                    interactionModes.value())[VISUAL_CHARACTERISTICS_CAPABILITY_CONFIG_INTERACTION_MODES_TAG],
                alloc());
        }

        if (windowTemplates.hasValue()) {
            payload.AddMember(
                IPC_MSG_WINDOW_TEMPLATES_TAG,
                messageDocument.Parse(
                    windowTemplates.value())[VISUAL_CHARACTERISTICS_CAPABILITY_CONFIG_WINDOW_TEMPLATES_TAG],
                alloc());
        }

        setPayload(std::move(payload));
    }
};

/**
 *  The @c ClearWindowMessage instructs the IPC Client to clear visual content for the given window.
 */
class ClearWindowMessage : public messages::Message {
public:
    /**
     * Constructor
     *
     * @param windowId The id of the window to clear.
     */
    explicit ClearWindowMessage(const std::string& windowId) :
            Message(IPC_MSG_NAMESPACE_WINDOW_MANAGER, IPC_MSG_VERSION_WINDOW_MANAGER, IPC_MSG_NAME_CLEAR_WINDOW) {
        setWindowIdInPayload(windowId);
        addPayload();
    }
};

}  // namespace IPCNamespaces
}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_NAMESPACES_WINDOWMANAGERNAMESPACE_H_
