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

#include <algorithm>

#include <AVSCommon/Utils/Error/FinallyGuard.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include "AVSCommon/Utils/Metrics/MetricEventBuilder.h"
#include "AVSCommon/Utils/Metrics/DataPointCounterBuilder.h"

#include "ContextManager/ContextManager.h"

namespace alexaClientSDK {
namespace contextManager {

using namespace rapidjson;
using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::metrics;

/// String to identify log entries originating from this file.
static const std::string TAG("ContextManager");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// An empty token to identify @c setState that is a proactive setter.
static const ContextRequestToken EMPTY_TOKEN = 0;

static const std::string STATE_PROVIDER_TIMEOUT_METRIC_PREFIX = "ERROR.StateProviderTimeout.";

std::shared_ptr<ContextManagerInterface> ContextManager::createContextManagerInterface(
    const std::shared_ptr<DeviceInfo>& deviceInfo,
    const std::shared_ptr<avsCommon::utils::timing::MultiTimer>& multiTimer,
    const std::shared_ptr<MetricRecorderInterface>& metricRecorder) {
    if (!deviceInfo) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullDeviceInfo"));
        return nullptr;
    }
    return create(*deviceInfo, multiTimer, metricRecorder);
}

std::shared_ptr<ContextManager> ContextManager::create(
    const DeviceInfo& deviceInfo,
    std::shared_ptr<avsCommon::utils::timing::MultiTimer> multiTimer,
    std::shared_ptr<MetricRecorderInterface> metricRecorder) {
    if (!multiTimer) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMultiTimer"));
        return nullptr;
    }

    std::shared_ptr<ContextManager> contextManager(
        new ContextManager(deviceInfo.getDefaultEndpointId(), multiTimer, metricRecorder));
    return contextManager;
}

ContextManager::~ContextManager() {
    m_shutdown = true;
    m_executor.shutdown();
    m_observers.clear();
    m_pendingRequests.clear();
    m_pendingStateRequest.clear();
}

/**
 * A helper no-op function used as a default value for onContextAvailable and onContextFailure callbacks.
 */
static void NoopCallback() {
    // No-op
}

ContextManager::RequestTracker::RequestTracker() :
        timerToken{0},
        contextRequester{nullptr},
        skipReportableStateProperties{false} {
}

ContextManager::RequestTracker::RequestTracker(
    avsCommon::utils::timing::MultiTimer::Token timerToken,
    std::shared_ptr<ContextRequesterInterface> contextRequester,
    bool skipReportableProperties) :
        timerToken{timerToken},
        contextRequester{contextRequester},
        skipReportableStateProperties{skipReportableProperties} {
}

void ContextManager::setStateProvider(
    const CapabilityTag& stateProviderName,
    std::shared_ptr<StateProviderInterface> stateProvider) {
    if (!stateProvider) {
        removeStateProvider(std::move(stateProviderName));
    } else {
        addStateProvider(stateProviderName, std::move(stateProvider));
    }
}

void ContextManager::addStateProvider(
    const avsCommon::avs::CapabilityTag& capabilityIdentifier,
    std::shared_ptr<avsCommon::sdkInterfaces::StateProviderInterface> stateProvider) {
    ACSDK_DEBUG5(LX(__func__).sensitive("capability", capabilityIdentifier));
    if (!stateProvider) {
        ACSDK_ERROR(LX("addStateProviderFailed").d("reason", "nullStateProvider"));
        return;
    }

    std::lock_guard<std::mutex> stateProviderLock(m_endpointsStateMutex);
    auto& endpointId = capabilityIdentifier.endpointId.empty() ? m_defaultEndpointId : capabilityIdentifier.endpointId;
    auto& capabilitiesState = m_endpointsState[endpointId];
    capabilitiesState[capabilityIdentifier] = StateInfo(std::move(stateProvider), Optional<CapabilityState>());
}

void ContextManager::removeStateProvider(const avs::CapabilityTag& capabilityIdentifier) {
    ACSDK_DEBUG5(LX(__func__).sensitive("capability", capabilityIdentifier));
    std::lock_guard<std::mutex> stateProviderLock(m_endpointsStateMutex);
    auto& endpointId = capabilityIdentifier.endpointId.empty() ? m_defaultEndpointId : capabilityIdentifier.endpointId;
    auto& capabilitiesState = m_endpointsState[endpointId];
    capabilitiesState.erase(capabilityIdentifier);
}

SetStateResult ContextManager::setState(
    const CapabilityTag& capabilityIdentifier,
    const std::string& jsonState,
    const StateRefreshPolicy& refreshPolicy,
    const ContextRequestToken stateRequestToken) {
    ACSDK_DEBUG5(LX(__func__).sensitive("capability", capabilityIdentifier));

    if (EMPTY_TOKEN == stateRequestToken) {
        m_executor.submit([this, capabilityIdentifier, jsonState, refreshPolicy] {
            updateCapabilityState(capabilityIdentifier, jsonState, refreshPolicy);
        });
        return SetStateResult::SUCCESS;
    }

    // TODO: Remove m_requestsMutex once this method gets deleted.
    std::lock_guard<std::mutex> requestsLock{m_requestsMutex};
    auto requestIt = m_pendingStateRequest.find(stateRequestToken);
    if (requestIt == m_pendingStateRequest.end()) {
        ACSDK_ERROR(LX("setStateFailed")
                        .d("reason", "outdatedStateToken")
                        .sensitive("capability", capabilityIdentifier)
                        .sensitive("suppliedToken", stateRequestToken));
        return SetStateResult::STATE_TOKEN_OUTDATED;
    }

    auto capabilityIt = requestIt->second.find(capabilityIdentifier);
    if (capabilityIt == requestIt->second.end()) {
        ACSDK_ERROR(LX("setStateFailed")
                        .d("reason", "capabilityNotPending")
                        .sensitive("capability", capabilityIdentifier)
                        .sensitive("suppliedToken", stateRequestToken));
        return SetStateResult::STATE_PROVIDER_NOT_REGISTERED;
    }

    m_executor.submit([this, capabilityIdentifier, jsonState, refreshPolicy, stateRequestToken] {
        updateCapabilityState(capabilityIdentifier, jsonState, refreshPolicy);
        if (jsonState.empty() && (StateRefreshPolicy::ALWAYS == refreshPolicy)) {
            ACSDK_ERROR(LX("setStateFailed")
                            .d("missingState", capabilityIdentifier.nameSpace + "::" + capabilityIdentifier.name));
            std::function<void()> contextFailureCallback = NoopCallback;
            {
                std::lock_guard<std::mutex> requestsLock{m_requestsMutex};
                contextFailureCallback =
                    getContextFailureCallbackLocked(stateRequestToken, ContextRequestError::BUILD_CONTEXT_ERROR);
            }
            /// Callback method should be called outside the lock.
            contextFailureCallback();

        } else {
            std::function<void()> contextAvailableCallback = NoopCallback;
            {
                std::lock_guard<std::mutex> requestsLock{m_requestsMutex};
                auto requestIt = m_pendingStateRequest.find(stateRequestToken);
                if (requestIt != m_pendingStateRequest.end()) {
                    requestIt->second.erase(capabilityIdentifier);
                }
                contextAvailableCallback = getContextAvailableCallbackIfReadyLocked(stateRequestToken, "");
            }
            /// Callback method should be called outside the lock.
            contextAvailableCallback();
        }
    });
    return SetStateResult::SUCCESS;
}

ContextManager::ContextManager(
    const std::string& defaultEndpointId,
    std::shared_ptr<avsCommon::utils::timing::MultiTimer> multiTimer,
    std::shared_ptr<MetricRecorderInterface> metricRecorder) :
        m_metricRecorder{std::move(metricRecorder)},
        m_requestCounter{0},
        m_shutdown{false},
        m_defaultEndpointId{defaultEndpointId},
        m_multiTimer{multiTimer} {
}

void ContextManager::reportStateChange(
    const CapabilityTag& capabilityIdentifier,
    const CapabilityState& capabilityState,
    AlexaStateChangeCauseType cause) {
    ACSDK_DEBUG5(LX(__func__).sensitive("capability", capabilityIdentifier));

    m_executor.submit([this, capabilityIdentifier, capabilityState, cause] {
        updateCapabilityState(capabilityIdentifier, capabilityState);
        std::lock_guard<std::mutex> observerMutex{m_observerMutex};
        for (auto& observer : m_observers) {
            observer->onStateChanged(capabilityIdentifier, capabilityState, cause);
        }
    });
}

void ContextManager::provideStateResponse(
    const avs::CapabilityTag& capabilityIdentifier,
    const avs::CapabilityState& capabilityState,
    ContextRequestToken stateRequestToken) {
    ACSDK_DEBUG5(LX(__func__).sensitive("capability", capabilityIdentifier));

    m_executor.submit([this, capabilityIdentifier, capabilityState, stateRequestToken] {
        std::function<void()> contextAvailableCallback = NoopCallback;
        {
            std::lock_guard<std::mutex> requestsLock{m_requestsMutex};
            auto requestIt = m_pendingStateRequest.find(stateRequestToken);
            if (requestIt == m_pendingStateRequest.end()) {
                ACSDK_ERROR(LX("provideStateResponseFailed")
                                .d("reason", "outdatedStateToken")
                                .sensitive("capability", capabilityIdentifier)
                                .sensitive("suppliedToken", stateRequestToken));
                return;
            }

            auto capabilityIt = requestIt->second.find(capabilityIdentifier);
            if (capabilityIt == requestIt->second.end()) {
                ACSDK_ERROR(LX("provideStateResponseFailed")
                                .d("reason", "capabilityNotPending")
                                .sensitive("capability", capabilityIdentifier)
                                .sensitive("suppliedToken", stateRequestToken));
                return;
            }

            updateCapabilityState(capabilityIdentifier, capabilityState);

            if (requestIt != m_pendingStateRequest.end()) {
                requestIt->second.erase(capabilityIdentifier);
            }
            contextAvailableCallback =
                getContextAvailableCallbackIfReadyLocked(stateRequestToken, capabilityIdentifier.endpointId);
        }
        /// Callback method should be called outside the lock.
        contextAvailableCallback();

    });
}

void ContextManager::provideStateUnavailableResponse(
    const CapabilityTag& capabilityIdentifier,
    ContextRequestToken stateRequestToken,
    bool isEndpointUnreachable) {
    ACSDK_DEBUG5(LX(__func__).sensitive("capability", capabilityIdentifier));

    m_executor.submit([this, capabilityIdentifier, stateRequestToken, isEndpointUnreachable] {
        std::function<void()> contextAvailableCallback = NoopCallback;
        std::function<void()> contextFailureCallback = NoopCallback;
        {
            std::lock_guard<std::mutex> requestsLock{m_requestsMutex};
            auto requestIt = m_pendingStateRequest.find(stateRequestToken);
            if (requestIt == m_pendingStateRequest.end()) {
                ACSDK_ERROR(LX("provideStateUnavailableResponseFailed")
                                .d("reason", "outdatedStateToken")
                                .sensitive("capability", capabilityIdentifier)
                                .sensitive("suppliedToken", stateRequestToken));
                return;
            }

            auto capabilityIt = requestIt->second.find(capabilityIdentifier);
            if (capabilityIt == requestIt->second.end()) {
                ACSDK_ERROR(LX("provideStateUnavailableResponseFailed")
                                .d("reason", "capabilityNotPending")
                                .sensitive("capability", capabilityIdentifier)
                                .sensitive("suppliedToken", stateRequestToken));
                return;
            }

            if (!isEndpointUnreachable) {
                auto& endpointState = m_endpointsState[capabilityIdentifier.endpointId];
                auto cachedState = endpointState.find(capabilityIdentifier);
                if ((cachedState != endpointState.end()) && cachedState->second.capabilityState.hasValue()) {
                    if (requestIt != m_pendingStateRequest.end()) {
                        requestIt->second.erase(capabilityIdentifier);
                    }
                    contextAvailableCallback =
                        getContextAvailableCallbackIfReadyLocked(stateRequestToken, capabilityIdentifier.endpointId);

                } else {
                    contextFailureCallback =
                        getContextFailureCallbackLocked(stateRequestToken, ContextRequestError::BUILD_CONTEXT_ERROR);
                }
            } else {
                contextFailureCallback =
                    getContextFailureCallbackLocked(stateRequestToken, ContextRequestError::ENDPOINT_UNREACHABLE);
            }
        }
        /// Callback methods should be called outside the lock.
        contextAvailableCallback();
        contextFailureCallback();
    });
}

void ContextManager::addContextManagerObserver(std::shared_ptr<ContextManagerObserverInterface> observer) {
    if (observer) {
        std::lock_guard<std::mutex> lock{m_observerMutex};
        m_observers.push_back(observer);
    }
}

void ContextManager::removeContextManagerObserver(const std::shared_ptr<ContextManagerObserverInterface>& observer) {
    std::lock_guard<std::mutex> lock{m_observerMutex};
    m_observers.remove(observer);
}

ContextRequestToken ContextManager::generateToken() {
    return ++m_requestCounter;
}

ContextRequestToken ContextManager::getContext(
    std::shared_ptr<ContextRequesterInterface> contextRequester,
    const std::string& endpointId,
    const std::chrono::milliseconds& timeout) {
    ACSDK_DEBUG5(LX(__func__));
    return getContextInternal(contextRequester, endpointId, timeout, false);
}

ContextRequestToken ContextManager::getContextWithoutReportableStateProperties(
    std::shared_ptr<ContextRequesterInterface> contextRequester,
    const std::string& endpointId,
    const std::chrono::milliseconds& timeout) {
    ACSDK_DEBUG5(LX(__func__));
    return getContextInternal(contextRequester, endpointId, timeout, true);
}

ContextRequestToken ContextManager::getContextInternal(
    std::shared_ptr<ContextRequesterInterface> contextRequester,
    const std::string& endpointId,
    const std::chrono::milliseconds& timeout,
    bool bSkipReportableStateProperties) {
    ACSDK_DEBUG5(LX(__func__).sensitive("endpointId", endpointId));
    auto token = generateToken();
    m_executor.submit([this, contextRequester, endpointId, token, timeout, bSkipReportableStateProperties] {
        auto timerToken = m_multiTimer->submitTask(timeout, [this, token] {
            // Cancel request after timeout.
            m_executor.submit([this, token] {
                std::function<void()> contextFailureCallback = NoopCallback;
                {
                    std::lock_guard<std::mutex> lock{m_requestsMutex};
                    contextFailureCallback =
                        getContextFailureCallbackLocked(token, ContextRequestError::STATE_PROVIDER_TIMEDOUT);
                }
                contextFailureCallback();
            });
        });

        std::lock_guard<std::mutex> requestsLock{m_requestsMutex};
        auto& requestEndpointId = endpointId.empty() ? m_defaultEndpointId : endpointId;
        m_pendingRequests.emplace(token, RequestTracker(timerToken, contextRequester, bSkipReportableStateProperties));

        std::function<void()> contextAvailableCallback = NoopCallback;
        {
            std::lock_guard<std::mutex> statesLock{m_endpointsStateMutex};
            for (auto& capability : m_endpointsState[requestEndpointId]) {
                auto stateInfo = capability.second;
                auto stateProvider = capability.second.stateProvider;

                if (stateProvider) {
                    bool requestState = false;
                    if (stateInfo.legacyCapability && stateInfo.refreshPolicy != StateRefreshPolicy::NEVER) {
                        requestState = true;
                    } else if (!stateInfo.legacyCapability && stateProvider->canStateBeRetrieved()) {
                        if (stateProvider->hasReportableStateProperties()) {
                            /// Check if the reportable state properties should be skipped.
                            if (!bSkipReportableStateProperties) {
                                requestState = true;
                            }
                        } else {
                            requestState = true;
                        }
                    }

                    if (requestState) {
                        stateProvider->provideState(capability.first, token);
                        m_pendingStateRequest[token].emplace(capability.first);
                    }
                }
            }

            contextAvailableCallback = getContextAvailableCallbackIfReadyLocked(token, requestEndpointId);
        }
        /// Callback method should be called outside the lock.
        contextAvailableCallback();
    });

    return token;
}

std::function<void()> ContextManager::getContextFailureCallbackLocked(
    unsigned int requestToken,
    ContextRequestError error) {
    ACSDK_DEBUG5(LX(__func__).d("token", requestToken));
    // Make sure the request gets cleared in the end of this function no matter the outcome.
    error::FinallyGuard clearRequestGuard{[this, requestToken] {
        auto requestIt = m_pendingRequests.find(requestToken);
        if (requestIt != m_pendingRequests.end()) {
            m_multiTimer->cancelTask(requestIt->second.timerToken);
            m_pendingRequests.erase(requestIt);
        }
        m_pendingStateRequest.erase(requestToken);
    }};

    auto& request = m_pendingRequests[requestToken];
    if (!request.contextRequester) {
        ACSDK_DEBUG0(LX(__func__).d("result", "nullRequester").d("token", requestToken));
        return NoopCallback;
    }
    for (auto& pendingState : m_pendingStateRequest[requestToken]) {
        auto metricName = STATE_PROVIDER_TIMEOUT_METRIC_PREFIX + pendingState.nameSpace;
        recordMetric(
            m_metricRecorder,
            MetricEventBuilder{}
                .setActivityName("CONTEXT_MANAGER-" + metricName)
                .addDataPoint(DataPointCounterBuilder{}.setName(metricName).increment(1).build())
                .build());
    }
    auto contextRequester = request.contextRequester;

    return [contextRequester, error, requestToken]() {
        if (contextRequester) {
            contextRequester->onContextFailure(error, requestToken);
        }
    };
}

std::function<void()> ContextManager::getContextAvailableCallbackIfReadyLocked(
    unsigned int requestToken,
    const EndpointIdentifier& endpointId) {
    auto& pendingStates = m_pendingStateRequest[requestToken];
    if (!pendingStates.empty()) {
        ACSDK_DEBUG5(LX(__func__).d("result", "stateNotAvailableYet").d("pendingStates", pendingStates.size()));
        return NoopCallback;
    }

    ACSDK_DEBUG5(LX(__func__).sensitive("endpointId", endpointId).d("token", requestToken));
    // Make sure the request gets cleared in the end of this function no matter the outcome.
    error::FinallyGuard clearRequestGuard{[this, requestToken] {
        auto requestIt = m_pendingRequests.find(requestToken);
        if (requestIt != m_pendingRequests.end()) {
            m_multiTimer->cancelTask(requestIt->second.timerToken);
            m_pendingRequests.erase(requestIt);
        }
        m_pendingStateRequest.erase(requestToken);
    }};

    auto& request = m_pendingRequests[requestToken];
    if (!request.contextRequester) {
        ACSDK_ERROR(
            LX("getContextAvailableCallbackIfReadyLockedFailed").d("reason", "nullRequester").d("token", requestToken));
        return NoopCallback;
    }

    AVSContext context;
    auto& requestEndpointId = endpointId.empty() ? m_defaultEndpointId : endpointId;
    for (auto& capability : m_endpointsState[requestEndpointId]) {
        auto stateProvider = capability.second.stateProvider;
        auto stateInfo = capability.second;
        bool addState = false;

        if (stateInfo.legacyCapability) {
            // Ignore if the state is not available for legacy SOMETIMES refresh policy.
            if ((stateInfo.refreshPolicy == StateRefreshPolicy::SOMETIMES) && !stateInfo.capabilityState.hasValue()) {
                ACSDK_DEBUG5(LX(__func__).d("skipping state for legacy capabilityIdentifier", capability.first));
            } else {
                addState = true;
            }
        } else {
            if (stateProvider && stateProvider->canStateBeRetrieved()) {
                /// Check if the reportable state properties should be skipped.
                if (stateProvider->hasReportableStateProperties()) {
                    if (!request.skipReportableStateProperties) {
                        addState = true;
                    }
                } else {
                    addState = true;
                }
            }
        }

        if (addState) {
            ACSDK_DEBUG5(LX(__func__).sensitive("addState", capability.first));
            context.addState(capability.first, stateInfo.capabilityState.value());
        }
    }
    auto contextRequester = request.contextRequester;

    return [contextRequester, context, endpointId, requestToken]() {
        if (contextRequester) {
            contextRequester->onContextAvailable(endpointId, context, requestToken);
        }
    };
}

void ContextManager::updateCapabilityState(
    const avsCommon::avs::CapabilityTag& capabilityIdentifier,
    const avsCommon::avs::CapabilityState& capabilityState) {
    std::lock_guard<std::mutex> statesLock{m_endpointsStateMutex};
    auto& endpointId = capabilityIdentifier.endpointId.empty() ? m_defaultEndpointId : capabilityIdentifier.endpointId;
    auto& capabilitiesState = m_endpointsState[endpointId];
    auto& stateProvider = capabilitiesState[capabilityIdentifier].stateProvider;
    capabilitiesState[capabilityIdentifier] = StateInfo(stateProvider, capabilityState);
}

void ContextManager::updateCapabilityState(
    const avsCommon::avs::CapabilityTag& capabilityIdentifier,
    const std::string& jsonState,
    const avsCommon::avs::StateRefreshPolicy& refreshPolicy) {
    std::lock_guard<std::mutex> statesLock{m_endpointsStateMutex};
    auto& endpointId = capabilityIdentifier.endpointId.empty() ? m_defaultEndpointId : capabilityIdentifier.endpointId;
    auto& capabilityInfo = m_endpointsState[endpointId];
    auto& stateProvider = capabilityInfo[capabilityIdentifier].stateProvider;
    capabilityInfo[capabilityIdentifier] = StateInfo(stateProvider, jsonState, refreshPolicy);
}

ContextManager::StateInfo::StateInfo(
    std::shared_ptr<StateProviderInterface> initStateProvider,
    const std::string& initJsonState,
    StateRefreshPolicy initRefreshPolicy) :
        stateProvider{initStateProvider},
        capabilityState{initJsonState.empty() ? Optional<CapabilityState>() : Optional<CapabilityState>(initJsonState)},
        legacyCapability{true},
        refreshPolicy{initRefreshPolicy} {
}

ContextManager::StateInfo::StateInfo(
    std::shared_ptr<StateProviderInterface> initStateProvider,
    const Optional<CapabilityState>& initCapabilityState) :
        stateProvider{initStateProvider},
        capabilityState{std::move(initCapabilityState)},
        legacyCapability{false},
        refreshPolicy{StateRefreshPolicy::ALWAYS} {
}

}  // namespace contextManager
}  // namespace alexaClientSDK
