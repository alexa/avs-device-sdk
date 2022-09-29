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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_AUDIOFOCUSMANAGERHANDLER_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_AUDIOFOCUSMANAGERHANDLER_H_

#include <string>

#include <AVSCommon/AVS/FocusState.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

#include "IPCServerSampleApp/IPC/IPCHandlerRegistrationInterface.h"
#include "IPCServerSampleApp/IPC/HandlerInterfaces/AudioFocusManagerHandlerInterface.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

/**
 * A contract for the handler that will handle the AudioFocusManager namespace messages coming from the IPC client.
 */
class AudioFocusManagerHandler
        : public IPCHandlerBase
        , public alexaClientSDK::avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<AudioFocusManagerHandler> {
public:
    /**
     * Create an instance of @c AudioFocusManagerHandler
     *
     * @param ipcHandlerRegistrar Used to register @c AudioFocusManagerHandler.
     * @param audioFocusManagerComponent Handles the parsed AudioFocusManager payloads.
     * @return Shared pointer to @c AudioFocusManagerHandler
     */
    static std::shared_ptr<AudioFocusManagerHandler> create(
        std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
        std::shared_ptr<AudioFocusManagerHandlerInterface> audioFocusManagerComponent);

    /**
     * Sends a processChannelResult IPC message to the IPC client.
     *
     * @param token Token identifying the requestor.
     * @param result Result of channel focus acquisition or release request.
     */
    void processChannelResult(unsigned token, bool result);

    /**
     * Sends a processFocusChanged IPC message to the IPC client.
     *
     * @param token Token identifying the requestor.
     * @param focusState Enumerated focus state for the associated audio channel.
     */
    void processFocusChanged(unsigned token, avsCommon::avs::FocusState focusState);

    /// @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /// Register this handler for its namespace and its methods for the corresponding namespace functions.
    void registerHandlers();

private:
    /**
     * Constructor.
     *
     * @param ipcHandlerRegistrar Used to register @c AudioFocusManagerHandler.
     * @param audioFocusManagerComponent Handles the parsed AudioFocusManager payloads.
     */
    AudioFocusManagerHandler(
        std::shared_ptr<IPCHandlerRegistrationInterface> ipcHandlerRegistrar,
        std::shared_ptr<AudioFocusManagerHandlerInterface> audioFocusManagerComponent);

    /// A reference to the @c IPCHandlerRegistrationInterface used to register/deregister @c AudioFocusManagerHandler.
    std::shared_ptr<IPCHandlerRegistrationInterface> m_ipcHandlerRegistrar;

    /// A reference to the IPC dispatcher to dispatch messages to the IPC Client.
    std::shared_ptr<IPCDispatcherInterface> m_ipcDispatcher;

    /// A reference to the @c AudioFocusManagerHandlerInterface that will handle the parsed payloads.
    std::shared_ptr<AudioFocusManagerHandlerInterface> m_audioFocusManagerComponent;
};

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_COMPONENTS_AUDIOFOCUSMANAGERHANDLER_H_
