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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_WINDOWMANAGERHANDLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_WINDOWMANAGERHANDLERINTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

/// Characteristics json key in the message.
static const char CHARACTERISTICS_TAG[] = "characteristics";

/// Device display json key in the message.
static const char DEVICE_DISPLAY_TAG[] = "deviceDisplay";

/// Window templates json key in the message.
static const char WINDOW_TEMPLATES_TAG[] = "windowTemplates";

/// Interaction modes json key in the message.
static const char INTERACTION_MODES_TAG[] = "interactionModes";

/// Default window id json key in the message.
static const char DEFAULT_WINDOW_ID_TAG[] = "defaultWindowId";

/// Audio playback UI window id json key in the message.
static const char AUDIO_PLAYBACK_UI_WINDOW_ID_TAG[] = "audioPlaybackUIWindowId";

/// The window instances json key in the message.
static const char WINDOW_INSTANCES_TAG[] = "windowInstances";

/// The window id json key in the message.
static const char WINDOW_ID_TAG[] = "windowId";

/// The template id json key in the message.
static const char TEMPLATE_ID_TAG[] = "templateId";

/// The interaction mode json key in the message.
static const char INTERACTION_MODE_TAG[] = "interactionMode";

/// The size configuration id json key in the message.
static const char SIZE_CONFIGURATION_ID_TAG[] = "sizeConfigurationId";

/// The supported interfaces json key in the message.
static const char SUPPORTED_INTERFACES[] = "supportedInterfaces";

/// The zOrderIndex json key in the message.
static const char Z_ORDER_INDEX[] = "zOrderIndex";

/// The window ids json key in the message.
static const char WINDOW_IDS_TAG[] = "windowIds";

/**
 * A contract to handle WindowManager.
 */
class WindowManagerHandlerInterface {
public:
    /**
     * Destructor
     */
    virtual ~WindowManagerHandlerInterface() = default;

    /**
     * IPC Client sends this event to request the visual characteristics asserted by SDK.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void visualCharacteristicsRequest(const std::string& message) = 0;

    /**
     * IPC Client sends this event to inform the SDK of a change to the default window instance id to report in
     * WindowState.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void defaultWindowInstanceChanged(const std::string& message) = 0;

    /**
     * IPC Client sends this event to inform the SDK of the window instances created in the IPC Client to track for
     * visual presentation.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void windowInstancesReport(const std::string& message) = 0;

    /**
     * IPC Client sends this event to inform the SDK of the window instances added in the IPC Client to track for visual
     * presentation.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void windowInstancesAdded(const std::string& message) = 0;

    /**
     * IPC Client sends this event to inform the SDK of the window instances removed in the IPC Client to no longer
     * track for visual presentation.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void windowInstancesRemoved(const std::string& message) = 0;

    /**
     * IPC Client sends this event to inform the SDK of the window instances updated in the IPC Client.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void windowInstancesUpdated(const std::string& message) = 0;
};

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_WINDOWMANAGERHANDLERINTERFACE_H_
