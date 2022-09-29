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

#ifndef ACSDK_SAMPLE_ENDPOINT_ENDPOINTFOCUSADAPTER_H_
#define ACSDK_SAMPLE_ENDPOINT_ENDPOINTFOCUSADAPTER_H_

#include <functional>
#include <memory>
#include <string>

#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorInterface.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorStateTrackerInterface.h>
#include <AVSCommon/SDKInterfaces/ChannelObserverInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

/**
 * Class to handle focus management.
 */
class EndpointFocusAdapter
        : public avsCommon::sdkInterfaces::ChannelObserverInterface
        , public std::enable_shared_from_this<EndpointFocusAdapter> {
public:
    using FocusAcquiredCallback = std::function<void(void)>;
    /**
     * Create an @c EndpointFocusAdapter object.
     *
     * @param focusManager The @c FocusManager, to handle audio focus.
     * @param presentationOrchestrator The @c PresentationOrchestrator, to dismiss any existing presentations.
     * @param presentationOrchestratorStateTracker The @c PresentationOrchestratorStateTracker, to set and release the
     * interface for visual focus.
     * @return A pointer to a new @c EndpointFocusAdapter object if it succeeds; otherwise, @c nullptr.
     */
    static std::shared_ptr<EndpointFocusAdapter> create(
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorInterface> presentationOrchestrator,
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorStateTrackerInterface>
            presentationOrchestratorStateTracker);

    /**
     * Destructor
     */
    virtual ~EndpointFocusAdapter() = default;

    /// @name ChannelObserverInterface Functions
    /// @{
    void onFocusChanged(avsCommon::avs::FocusState focusState, avsCommon::avs::MixingBehavior behavior) override;
    /// @}

    /**
     * Acquire visual and audio focus.
     *
     * @param interfaceName The interface requesting focus.
     * @param callback The function to be invoked when focus is acquired.
     */
    void acquireFocus(const std::string& interfaceName, FocusAcquiredCallback callback);

private:
    /**
     * Constructor.
     *
     * @param focusManager The @c FocusManager, to handle audio focus.
     * @param presentationOrchestrator The @c PresentationOrchestrator, to dismiss any existing presentations.
     * @param presentationOrchestratorStateTracker The @c PresentationOrchestratorStateTracker, to set and release the
     * interface for visual focus.
     */
    EndpointFocusAdapter(
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorInterface> presentationOrchestrator,
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorStateTrackerInterface>
            presentationOrchestratorStateTracker);

    /**
     * Utility function to acquire the Content channel for focus management.
     */
    void executeAcquireContentChannel();

    /**
     * Utility function to release the Content channel for focus management.
     */
    void executeReleaseContentChannel();

    /**
     * Utility function to acquire the window.
     */
    void executeAcquireWindow();

    /**
     * Utility function to release the window.
     */
    void executeReleaseWindow();

    /**
     * Struct containing the name of the interface and corresponding callback for acquireFocus requests.
     */
    struct FocusAcquireInterfaceAndCallback {
        /**
         * Constructor.
         *
         * @param interfaceName The name of the interface requesting focus.
         * @param callback The callback to be invoked when focus is acquired.
         */
        FocusAcquireInterfaceAndCallback(std::string interfaceName, FocusAcquiredCallback callback) :
                interfaceName{std::move(interfaceName)},
                callback(callback) {
        }

        /// The interface name for focus requests.
        std::string interfaceName;

        /// The callback to to be invoked when focus is acquired.
        FocusAcquiredCallback callback;
    };

    /// Convenience typedef
    using FocusAcquireInterfaceAndCallbackPtr = std::shared_ptr<FocusAcquireInterfaceAndCallback>;

    /// The @c FocusManager used to manage audio focus.
    std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> m_focusManager;

    /// Pointer to the presentation orchestrator.
    std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorInterface> m_presentationOrchestrator;

    /// Presentation Orchestrator State Tracker.
    std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorStateTrackerInterface>
        m_presentationOrchestratorStateTracker;

    /// Pointer to the struct storing the information (interface name and callback) about the focus acquire request.
    FocusAcquireInterfaceAndCallbackPtr m_focusAcquirePtr;

    /// Shared pointer to the executor.
    std::shared_ptr<avsCommon::utils::threading::Executor> m_executor;
};

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ACSDK_SAMPLE_ENDPOINT_ENDPOINTFOCUSADAPTER_H_