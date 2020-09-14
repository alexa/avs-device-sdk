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

#ifndef ALEXA_CLIENT_SDK_CONTEXTMANAGER_INCLUDE_CONTEXTMANAGER_CONTEXTMANAGER_H_
#define ALEXA_CLIENT_SDK_CONTEXTMANAGER_INCLUDE_CONTEXTMANAGER_CONTEXTMANAGER_H_

#include <atomic>
#include <list>
#include <memory>
#include <utility>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include <AVSCommon/AVS/CapabilityTag.h>
#include <AVSCommon/AVS/StateRefreshPolicy.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ContextRequesterInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h>
#include <AVSCommon/SDKInterfaces/StateProviderInterface.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/Timing/MultiTimer.h>

namespace alexaClientSDK {
namespace contextManager {

/**
 * Class manages the requests for getting context from @c ContextRequesters and updating the state from
 * @c StateProviders.
 */
class ContextManager : public avsCommon::sdkInterfaces::ContextManagerInterface {
public:
    /**
     * Create a new @c ContextManager instance.
     *
     * @param deviceInfo Structure used to retrieve the default endpoint id.
     * @param multiTimer Object used to schedule request timeout.
     * @param metricRecorder The metric recorder.
     * @return Returns a new @c ContextManager.
     */
    static std::shared_ptr<ContextManagerInterface> createContextManagerInterface(
        const std::shared_ptr<avsCommon::utils::DeviceInfo>& deviceInfo,
        const std::shared_ptr<avsCommon::utils::timing::MultiTimer>& multiTimer =
            std::make_shared<avsCommon::utils::timing::MultiTimer>(),
        const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder = nullptr);

    /**
     * Create a new @c ContextManager instance.
     *
     * @deprecated
     * @param deviceInfo Structure used to retrieve the default endpoint id.
     * @param multiTimer Object used to schedule request timeout.
     * @param metricRecorder The metric recorder.
     * @return Returns a new @c ContextManager.
     */
    static std::shared_ptr<ContextManager> create(
        const avsCommon::utils::DeviceInfo& deviceInfo,
        std::shared_ptr<avsCommon::utils::timing::MultiTimer> multiTimer =
            std::make_shared<avsCommon::utils::timing::MultiTimer>(),
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder = nullptr);

    /// Destructor.
    ~ContextManager() override;

    /// @name ContextManagerInterface methods.
    /// @{
    void setStateProvider(
        const avsCommon::avs::CapabilityTag& stateProviderName,
        std::shared_ptr<avsCommon::sdkInterfaces::StateProviderInterface> stateProvider) override;

    void addStateProvider(
        const avsCommon::avs::CapabilityTag& capabilityIdentifier,
        std::shared_ptr<avsCommon::sdkInterfaces::StateProviderInterface> stateProvider) override;

    void removeStateProvider(const avsCommon::avs::CapabilityTag& capabilityIdentifier) override;

    avsCommon::sdkInterfaces::SetStateResult setState(
        const avsCommon::avs::CapabilityTag& stateProviderName,
        const std::string& jsonState,
        const avsCommon::avs::StateRefreshPolicy& refreshPolicy,
        const avsCommon::sdkInterfaces::ContextRequestToken stateRequestToken = 0) override;

    avsCommon::sdkInterfaces::ContextRequestToken getContext(
        std::shared_ptr<avsCommon::sdkInterfaces::ContextRequesterInterface> contextRequester,
        const std::string& endpointId,
        const std::chrono::milliseconds& timeout) override;

    avsCommon::sdkInterfaces::ContextRequestToken getContextWithoutReportableStateProperties(
        std::shared_ptr<avsCommon::sdkInterfaces::ContextRequesterInterface> contextRequester,
        const std::string& endpointId,
        const std::chrono::milliseconds& timeout) override;

    void reportStateChange(
        const avsCommon::avs::CapabilityTag& capabilityIdentifier,
        const avsCommon::avs::CapabilityState& capabilityState,
        avsCommon::sdkInterfaces::AlexaStateChangeCauseType cause) override;

    void provideStateResponse(
        const avsCommon::avs::CapabilityTag& capabilityIdentifier,
        const avsCommon::avs::CapabilityState& capabilityState,
        avsCommon::sdkInterfaces::ContextRequestToken stateRequestToken) override;

    void provideStateUnavailableResponse(
        const avsCommon::avs::CapabilityTag& capabilityIdentifier,
        avsCommon::sdkInterfaces::ContextRequestToken stateRequestToken,
        bool isEndpointUnreachable) override;

    void addContextManagerObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerObserverInterface> observer) override;

    void removeContextManagerObserver(
        const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerObserverInterface>& observer) override;
    /// @}

private:  // Private type declarations.
    /**
     * This class has all the information about a @c StateProviderInterface needed by the contextManager.
     */
    struct StateInfo {
        /// Pointer to the StateProvider.
        std::shared_ptr<avsCommon::sdkInterfaces::StateProviderInterface> stateProvider;

        /// The state of the capability.
        avsCommon::utils::Optional<avsCommon::avs::CapabilityState> capabilityState;

        /// Whether this capability state should always be reported independently of isRetrievable.
        /// @note This is used for capabilities that use the old interface with refresh token.
        bool legacyCapability;

        /// The refresh policy which is only used for legacy capabilities.
        avsCommon::avs::StateRefreshPolicy refreshPolicy;

        /**
         * Constructor.
         *
         * @param initStateProvider The @c StateProviderInterface.
         * @param initJsonState The state of the @c StateProviderInterface.
         * @param refreshPolicy The refresh policy of the state of the @c StateProviderInterface.
         *
         * @deprecated @c StateRefreshPolicy has been deprecated.
         */
        StateInfo(
            std::shared_ptr<avsCommon::sdkInterfaces::StateProviderInterface> initStateProvider = nullptr,
            const std::string& initJsonState = "",
            avsCommon::avs::StateRefreshPolicy initRefreshPolicy = avsCommon::avs::StateRefreshPolicy::ALWAYS);

        /**
         * Constructor.
         *
         * @param initStateProvider The @c StateProviderInterface.
         * @param initCapabilityState The initial capability state.
         */
        StateInfo(
            std::shared_ptr<avsCommon::sdkInterfaces::StateProviderInterface> initStateProvider,
            const avsCommon::utils::Optional<avsCommon::avs::CapabilityState>& initCapabilityState);

        /**
         * Copy constructor.
         */
        StateInfo(const StateInfo&) = default;
    };

    /// Map of capabilities and their last known state.
    using CapabilitiesState = std::unordered_map<avsCommon::avs::CapabilityTag, StateInfo>;

    /// Alias for endpoint id.
    using EndpointIdentifier = avsCommon::sdkInterfaces::endpoints::EndpointIdentifier;

    /**
     * Structure used to save information about a request.
     */
    struct RequestTracker {
        /// The token returned by the @c MultiTimer.
        avsCommon::utils::timing::MultiTimer::Token timerToken;
        /// The context requester.
        std::shared_ptr<avsCommon::sdkInterfaces::ContextRequesterInterface> contextRequester;
        /// If Reportable Properties should be skipped for this request.
        bool skipReportableStateProperties;

        /**
         * Default Constructor.
         */
        RequestTracker();

        /**
         * Constructor.
         * @param timerToken The token returned by the @c MultiTimer.
         * @param contextRequester The @c ContextRequesterInterface pointer.
         * @param skipReportableProperties The boolean indicating if the reportable properties should be skipped for
         * this request.
         */
        RequestTracker(
            avsCommon::utils::timing::MultiTimer::Token timerToken,
            std::shared_ptr<avsCommon::sdkInterfaces::ContextRequesterInterface> contextRequester,
            bool skipReportableProperties);
    };

private:  // Private method declarations.
    /**
     * Constructor.
     *
     * @param defaultEndpointId The default endpoint used for legacy methods where no endpoint id is provided.
     * @param multiTimer Object used to schedule request timeout.
     * @param metricRecorder The metric recorder.
     */
    ContextManager(
        const std::string& defaultEndpointId,
        std::shared_ptr<avsCommon::utils::timing::MultiTimer> multiTimer,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder);
    ///

    /**
     * Updates the state of a capability.
     *
     * @param capabilityIdentifier The capability identifier.
     * @param jsonState The state of the @c StateProviderInterface.
     * @param refreshPolicy The refresh policy for the state.
     * @deprecated @c StateRefreshPolicy has been deprecated.
     */
    void updateCapabilityState(
        const avsCommon::avs::CapabilityTag& capabilityIdentifier,
        const std::string& jsonState,
        const avsCommon::avs::StateRefreshPolicy& refreshPolicy);

    /**
     * Updates the state of a capability.
     *
     * @param capabilityIdentifier The capability identifier.
     * @param capabilityState The capability state.
     * @deprecated @c StateRefreshPolicy has been deprecated.
     */
    void updateCapabilityState(
        const avsCommon::avs::CapabilityTag& capabilityIdentifier,
        const avsCommon::avs::CapabilityState& capabilityState);

    /**
     * Request the @c ContextManager for context.
     *
     * @param contextRequester The context requester asking for context.
     * @param endpointId The @c endpointId used to select which context is being requested.
     * @param timeout The maximum time this request should take. After the timeout, the context manager will abort the
     * request.
     * @param skipReportableStateProperties The flag which indicates if ReportableStateProperties can be ignored from
     * the context response.
     * @return A token that can be used to correlate this request with the context response.
     */
    avsCommon::sdkInterfaces::ContextRequestToken getContextInternal(
        std::shared_ptr<avsCommon::sdkInterfaces::ContextRequesterInterface> contextRequester,
        const std::string& endpointId,
        const std::chrono::milliseconds& timeout,
        bool skipReportableStateProperties);

    /**
     * This method returns a callback which should be invoked once the context is ready.
     * If the context is not ready, this method will return a no-op function.
     *
     * @note The callback method that is returned should only be called outside of a lock to prevent deadlock scenarios.
     *
     * @note If the context is ready, the method also removes the request from the pending requests map.
     *
     * @param requestToken The request token associated with the context request.
     * @param endpointId The endpointId associated with the context request.
     * @return A callback method to notify the context requester when the context is ready. An empty no-op function
     * if the context is not ready.
     */
    std::function<void()> getContextAvailableCallbackIfReadyLocked(
        avsCommon::sdkInterfaces::ContextRequestToken requestToken,
        const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& endpointId);

    /**
     * This method returns a callback which should be invoked once there is a context failure.
     * If the contextRequester is invalid, this method returns a no-op function.
     *
     * @note The callback method that is returned should only be called outside of a lock to prevent deadlock scenarios.
     *
     * @note The method also cleans up the request from the pending requests map.
     *
     * @param requestToken The request token associated with the context request.
     * @param error The @c ContextRequestError to be notified to the requester.
     * @return A callback method to notify the context requester of a context fetch failure. An empty no-op function if
     * the context requester is invalid.
     */
    std::function<void()> getContextFailureCallbackLocked(
        avsCommon::sdkInterfaces::ContextRequestToken requestToken,
        avsCommon::sdkInterfaces::ContextRequestError error);

    /**
     * Generate a request token.
     *
     * @return A new unique request token.
     */
    inline avsCommon::sdkInterfaces::ContextRequestToken generateToken();

private:  // Member variable declarations
    /// Mutex used to guard m_endpointsState access.
    std::mutex m_endpointsStateMutex;

    /// Map of state provider namespace and name to the state information. @c m_stateProviderMutex must be acquired
    /// before accessing the map.
    std::unordered_map<EndpointIdentifier, CapabilitiesState> m_endpointsState;

    /// Mutex used to guard the pending state requests. This is only needed because of @c setState.
    std::mutex m_requestsMutex;

    /// The metric recorder.
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> m_metricRecorder;

    /// The request token counter.
    std::atomic<unsigned int> m_requestCounter;

    /// Map of pending states per ongoing request.
    std::unordered_map<unsigned int, std::unordered_set<avsCommon::avs::CapabilityTag>> m_pendingStateRequest;

    /// Map of requester per ongoing request and their respective tokens.
    std::unordered_map<avsCommon::sdkInterfaces::ContextRequestToken, RequestTracker> m_pendingRequests;

    /// Mutex used to guard the observers.
    std::mutex m_observerMutex;

    /// List of observers.
    std::list<std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerObserverInterface>> m_observers;

    /// Whether the contextManager is shutting down.
    std::atomic_bool m_shutdown;

    /// Endpoint identifier used to keep backward compatibility with capabilities without endpoint information.
    const std::string m_defaultEndpointId;

    /// Timer to handler timeouts.
    std::shared_ptr<avsCommon::utils::timing::MultiTimer> m_multiTimer;

    /// Executor used to handle the context requests.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace contextManager
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CONTEXTMANAGER_INCLUDE_CONTEXTMANAGER_CONTEXTMANAGER_H_
