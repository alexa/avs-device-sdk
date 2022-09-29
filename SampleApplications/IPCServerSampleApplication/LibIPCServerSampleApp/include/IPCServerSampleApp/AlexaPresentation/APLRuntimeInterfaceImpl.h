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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLRUNTIMEINTERFACEIMPL_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLRUNTIMEINTERFACEIMPL_H_

#include <mutex>

#include <acsdk/APLCapabilityCommonInterfaces/APLDocumentSessionInterface.h>
#include <acsdk/APLCapabilityCommonInterfaces/APLRuntimeInterface.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorClientInterface.h>
#include "IPCServerSampleApp/AlexaPresentation/APLViewhostInterface.h"
#include "IPCServerSampleApp/GUI/GUIClientInterface.h"

#include "IPCServerSampleApp/AlexaPresentation/APLDocumentSession.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

class APLRuntimeInterfaceImpl : public aplCapabilityCommonInterfaces::APLRuntimeInterface {
public:
    /**
     * Destructor
     */
    ~APLRuntimeInterfaceImpl();

    /**
     * Create a new APLRuntimeInterfaceImpl object
     *
     * @param runtime @c IAPLRuntime instance
     * @return shared_ptr to APLRuntimeInterfaceImpl
     */
    static std::shared_ptr<APLRuntimeInterfaceImpl> create(const APLViewhostInterfacePtr& runtime);

    /// @name APLRuntimeInterface functions
    /// @{
    void renderDocument(
        const std::string& document,
        const std::string& data,
        const aplCapabilityCommonInterfaces::PresentationSession& presentationSession,
        const aplCapabilityCommonInterfaces::PresentationOptions& presentationOptions,
        std::shared_ptr<aplCapabilityCommonInterfaces::APLDocumentObserverInterface> observer) override;

    std::string getMaxAPLVersion() const override;
    /// @}

    void setDefaultWindowId(const std::string& windowId);

    void setPresentationOrchestrator(
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorClientInterface> poClient);

    std::shared_ptr<APLDocumentSession> createDocumentSession(
        const std::string& document,
        const std::string& data,
        const aplCapabilityCommonInterfaces::PresentationSession& presentationSession,
        const aplCapabilityCommonInterfaces::PresentationOptions& presentationOptions,
        std::shared_ptr<aplCapabilityCommonInterfaces::APLDocumentObserverInterface> observer,
        bool hasPresentationAssociation);

private:
    /**
     * Construct a new APLRuntimeInterfaceImpl object
     *
     * @param viewhost @c APLViewhostInterface instance
     */
    explicit APLRuntimeInterfaceImpl(APLViewhostInterfacePtr viewhost);

    /// Pointer to the APL Client/Viewhost.
    APLViewhostInterfacePtr m_viewhost;

    /// Pointer to the presentation orchestrator client.
    std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorClientInterface>
        m_presentationOrchestratorClient;

    /// Id of the window used by default to render experiences.
    std::string m_defaultWindowId;

    /// mutex for guarding members set by different thread.
    std::mutex m_mutex;
};
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLRUNTIMEINTERFACEIMPL_H_
