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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_GUICLIENTINTERFACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_GUICLIENTINTERFACE_H_

#include <rapidjson/document.h>

#include <AVSCommon/AVS/FocusState.h>
#ifdef ENABLE_COMMS
#include <AVSCommon/SDKInterfaces/CallStateObserverInterface.h>
#endif
#include <AVSCommon/SDKInterfaces/ChannelObserverInterface.h>

#include <acsdk/APLCapabilityCommonInterfaces/VisualStateProviderInterface.h>

#include "IPCServerSampleApp/IPC/IPCRouter.h"
#include "IPCServerSampleApp/Messages/Message.h"

#include "GUIServerInterface.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace gui {

/**
 * An interface representing the GUI component responsible for rendering display card and for APL.
 */
// TODO: Elaborate the requirements for an implementation of GUIClientInterface (ARC-905)
class GUIClientInterface {
public:
    /// Alias for GUI provided token.
    using GUIToken = uint64_t;

    /**
     * Destructor
     */
    virtual ~GUIClientInterface() = default;

    /**
     * Set a reference to a GUI Manager
     * @param guiManager Client related operations.
     */
    virtual void setGUIManager(std::shared_ptr<gui::GUIServerInterface> guiManager) = 0;

    /**
     * Request audio focus.
     * @param avsInterface The AVS Interface requesting focus.
     * @param channelName The channel to be requested.
     * @param contentType The type of content acquiring focus.
     * @param channelObserver the channelObserver to be notified.
     */
    virtual bool acquireFocus(
        std::string avsInterface,
        std::string channelName,
        avsCommon::avs::ContentType contentType,
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> channelObserver) = 0;

    /**
     * Release audio focus.
     * @param avsInterface The AVS Interface releasing focus.
     * @param channelName The channel to be released.
     * @param channelObserver the channelObserver to be notified.
     */
    virtual bool releaseFocus(
        std::string avsInterface,
        std::string channelName,
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> channelObserver) = 0;

    /**
     * Send call state info.
     * @param callStateInfo The call state info.
     */
#ifdef ENABLE_COMMS
    virtual void sendCallStateInfo(
        const avsCommon::sdkInterfaces::CallStateObserverInterface::CallStateInfo& callStateInfo) = 0;
#endif

    /**
     * Sends a GUI Message to the server.
     * @param message The message to be written.
     */
    virtual void sendMessage(messages::MessageInterface& message) = 0;

    /**
     * Finalize the client initialization with the error state so far passed from the caller.
     *
     * @param errorState Error state of the client initialization so far passed by the caller.
     * @return true if finalization succeeds.
     */
    virtual bool finalizeClientInitialization(bool errorState) = 0;

    /**
     * Handle initialization of APL Renderers based on window and supported extensions reported by GUI app
     * @param payload The payload retrieved from message holding this event.
     */
    virtual void handleInitializeAPLRenderers(rapidjson::Document& payload) = 0;

    /**
     * Handle displayMetrics message.
     * @param payload The payload retrieved from message holding this event.
     */
    virtual void handleDisplayMetrics(rapidjson::Document& payload) = 0;

    /**
     * Handle aplEvent message.
     *
     * @param payload The payload retrieved from message holding this event.
     */
    virtual void handleAplEvent(rapidjson::Document& payload) = 0;

    /**
     * Handle renderComplete message.
     * @param payload The payload retrieved from message holding this event.
     */
    virtual void handleRenderComplete(rapidjson::Document& payload) = 0;

    /**
     * Handle Local RenderDocument message.
     *
     * @param payload The payload holding the local render document directive.
     */
    virtual void handleLocalRenderDocument(rapidjson::Document& payload) = 0;

    /**
     * Handle Local ExecuteCommands message.
     *
     * @param payload The payload holding the local execute commands directive.
     */
    virtual void handleLocalExecuteCommands(rapidjson::Document& payload) = 0;

    /**
     * Handle Local ClearDocument message.
     *
     * @param payload The payload holding the local clear document directive.
     */
    virtual void handleLocalClearDocument(rapidjson::Document& payload) = 0;

    /**
     * Handle focus message received confirmation messages.
     *
     * @param token Token field in the focusChangedReport event.
     */
    virtual void handleOnFocusChangedReceivedConfirmation(GUIToken token) = 0;

    /// Internal function to execute @see processFocusAcquireRequest
    virtual void focusAcquireRequest(
        const GUIToken token,
        const std::string& avsInterface,
        const std::string& channelName,
        avsCommon::avs::ContentType contentType) = 0;

    /// Internal function to execute @see processFocusReleaseRequest
    virtual void focusReleaseRequest(
        const GUIToken token,
        const std::string& avsInterface,
        const std::string& channelName) = 0;

    /**
     * Informs the  IPC Client of changes in supported locales for the SDK.
     *
     * @param localeStr The locale(s) for the device. In single-locale mode, contains one locale string.
     * In multi-locale mode, the first string indicates the primary locale, and any other strings correspond to
     * secondary locales.
     */
    virtual void setLocales(const std::string& localeStr) = 0;

    /// Returns a reference to @c m_ipcRouter.
    virtual std::shared_ptr<ipc::IPCRouter> getIPCRouter() = 0;
};

}  // namespace gui
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_GUICLIENTINTERFACE_H_
