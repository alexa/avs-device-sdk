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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_APLCLIENTHANDLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_APLCLIENTHANDLERINTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

/**
 * A contract to handle APLClient.
 */
class APLClientHandlerInterface {
public:
    /**
     * Destructor
     */
    virtual ~APLClientHandlerInterface() = default;

    /**
     * IPC Client sends this event to initialize any native APLClientRenderer instances required for the IPC client’s
     * windows.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void initializeRenderersRequest(const std::string& message) = 0;

    /**
     * This event is sent from the APL IPC Viewhost to report APL Metrics defined in the Viewhost. Not intended to be
     * manually invoked from the IPC client.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void metricsReport(const std::string& message) = 0;

    /**
     * This event is sent from the APL IPC Viewhost for communicating with APLCore. Not intended to be manually invoked
     * from the IPC client.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void viewhostEvent(const std::string& message) = 0;

    /**
     * IPC Client sends this event when the IPC APL Viewhost has completed inflation of an APL document after receipt of
     * a createRenderer directive.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void renderCompleted(const std::string& message) = 0;

    /**
     * IPC Client sends this event to initiate the local rendering of an APL document.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void renderDocumentRequest(const std::string& message) = 0;

    /**
     * IPC Client sends this event to initiate APL commands on a document rendered via renderDocumentRequest.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void executeCommandsRequest(const std::string& message) = 0;

    /**
     * IPC Client sends this event to clear a local APL document rendered via renderDocumentRequest and release the
     * native renderer.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void clearDocumentRequest(const std::string& message) = 0;
};

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_APLCLIENTHANDLERINTERFACE_H_
