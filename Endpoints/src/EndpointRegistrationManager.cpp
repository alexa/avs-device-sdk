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
#include <thread>
#include <utility>

#include <AVSCommon/Utils/Error/FinallyGuard.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "Endpoints/EndpointRegistrationManager.h"

namespace alexaClientSDK {
namespace endpoints {

/// String to identify log entries originating from this file.
static const std::string TAG("EndpointRegistrationManager");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::endpoints;
using namespace avsCommon::utils;

std::unique_ptr<EndpointRegistrationManager> EndpointRegistrationManager::create(
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> directiveSequencer,
    std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate,
    const EndpointIdentifier& defaultEndpointId) {
    if (!directiveSequencer) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullDirectiveSequencer"));
        return nullptr;
    }

    if (!capabilitiesDelegate) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullCapabilitiesDelegate"));
        return nullptr;
    }

    if (defaultEndpointId.empty()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "invalidDefaultEndpointId"));
        return nullptr;
    }
    return std::unique_ptr<EndpointRegistrationManager>(
        new EndpointRegistrationManager(directiveSequencer, capabilitiesDelegate, defaultEndpointId));
}

void EndpointRegistrationManager::waitForPendingRegistrationsToEnqueue() {
    auto waiter = m_executor.submit([] {});
    waiter.wait();
}

std::future<EndpointRegistrationManager::RegistrationResult> EndpointRegistrationManager::registerEndpoint(
    std::shared_ptr<EndpointInterface> endpoint) {
    ACSDK_DEBUG5(LX(__func__));

    if (!endpoint) {
        ACSDK_ERROR(LX("registerEndpointFailed").d("reason", "nullEndpoint"));
        std::promise<RegistrationResult> promise;
        promise.set_value(RegistrationResult::CONFIGURATION_ERROR);
        return promise.get_future();
    }

    std::lock_guard<std::mutex> lock{m_endpointsMutex};

    auto endpointId = endpoint->getEndpointId();

    /// Modifying the default endpoint is not permitted.
    if (endpointId == m_defaultEndpointId && m_endpoints.end() != m_endpoints.find(endpointId)) {
        ACSDK_ERROR(LX("registerEndpointFailed")
                        .d("reason", "defaultEndpointAlreadyRegistered")
                        .sensitive("endpointId", endpointId));
        std::promise<RegistrationResult> promise;
        promise.set_value(RegistrationResult::CONFIGURATION_ERROR);
        return promise.get_future();
    }

    if (m_pendingRegistrations.end() != m_pendingRegistrations.find(endpointId)) {
        ACSDK_ERROR(LX("registerEndpointFailed")
                        .d("reason", "endpointRegistrationInProgress")
                        .sensitive("endpointId", endpoint->getEndpointId()));
        std::promise<RegistrationResult> promise;
        promise.set_value(RegistrationResult::REGISTRATION_IN_PROGRESS);
        return promise.get_future();
    }

    if (m_pendingDeregistrations.end() != m_pendingDeregistrations.find(endpointId)) {
        ACSDK_ERROR(LX("registerEndpointFailed")
                        .d("reason", "endpointDeregistrationInProgress")
                        .sensitive("endpointId", endpoint->getEndpointId()));
        std::promise<RegistrationResult> promise;
        promise.set_value(RegistrationResult::PENDING_DEREGISTRATION);
        return promise.get_future();
    }

    m_executor.submit([this, endpoint]() { executeRegisterEndpoint(endpoint); });

    auto& pending = m_pendingRegistrations[endpointId];
    pending.first = std::move(endpoint);
    return pending.second.get_future();
}

std::future<EndpointRegistrationManager::DeregistrationResult> EndpointRegistrationManager::deregisterEndpoint(
    const EndpointIdentifier& endpointId) {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock{m_endpointsMutex};

    /// Deleting the default endpoint is not permitted.
    if (endpointId == m_defaultEndpointId) {
        ACSDK_ERROR(LX("deregisterEndpointFailed")
                        .d("reason", "deregisteringDefaultEndpointNotPermitted")
                        .sensitive("endpointId", endpointId));
        std::promise<DeregistrationResult> promise;
        promise.set_value(DeregistrationResult::CONFIGURATION_ERROR);
        return promise.get_future();
    }

    if (m_pendingRegistrations.end() != m_pendingRegistrations.find(endpointId)) {
        ACSDK_ERROR(LX("deregisterEndpointFailed")
                        .d("reason", "endpointRegistrationInProgress")
                        .sensitive("endpointId", endpointId));
        std::promise<DeregistrationResult> promise;
        promise.set_value(DeregistrationResult::REGISTRATION_IN_PROGRESS);
        return promise.get_future();
    }

    if (m_endpoints.end() == m_endpoints.find(endpointId)) {
        ACSDK_ERROR(
            LX("deregisterEndpointFailed").d("reason", "endpointNotRegistered").sensitive("endpointId", endpointId));
        std::promise<DeregistrationResult> promise;
        promise.set_value(DeregistrationResult::NOT_REGISTERED);
        return promise.get_future();
    }

    if (m_pendingDeregistrations.end() != m_pendingDeregistrations.find(endpointId)) {
        ACSDK_ERROR(LX("deregisterEndpointFailed")
                        .d("reason", "endpointDeregistrationInProgress")
                        .sensitive("endpointId", endpointId));
        std::promise<DeregistrationResult> promise;
        promise.set_value(DeregistrationResult::PENDING_DEREGISTRATION);
        return promise.get_future();
    }

    auto endpoint = m_endpoints[endpointId];

    m_executor.submit([this, endpoint]() { executeDeregisterEndpoint(endpoint); });

    auto& pending = m_pendingDeregistrations[endpointId];
    pending.first = std::move(endpoint);
    return pending.second.get_future();
}

void EndpointRegistrationManager::executeRegisterEndpoint(const std::shared_ptr<EndpointInterface>& endpoint) {
    ACSDK_DEBUG5(LX(__func__));
    auto endpointId = endpoint->getEndpointId();

    RegistrationResult result = RegistrationResult::INTERNAL_ERROR;
    error::FinallyGuard notifyObserver([this, &endpoint, &result] {
        if (result != RegistrationResult::SUCCEEDED) {
            std::list<std::shared_ptr<EndpointRegistrationObserverInterface>> observers;
            {
                std::lock_guard<std::mutex> lock{m_observersMutex};
                observers = m_observers;
            }

            for (auto& observer : observers) {
                observer->onEndpointRegistration(endpoint->getEndpointId(), endpoint->getAttributes(), result);
            }

            std::lock_guard<std::mutex> lock{m_endpointsMutex};
            auto it = m_pendingRegistrations.find(endpoint->getEndpointId());
            it->second.second.set_value(result);
            m_pendingRegistrations.erase(it);
        }
    });

    // Since an endpoint may reuse the same directive handlers, keep track of things that were added already.
    // Create logic that will restore the previous endpoint and its handlers, in case of any failure
    // while registering the new endpoint.
    std::unordered_set<std::shared_ptr<DirectiveHandlerInterface>> handlersAdded;
    std::unordered_set<std::shared_ptr<DirectiveHandlerInterface>> handlersRemoved;

    std::shared_ptr<EndpointInterface> previousEndpoint;
    {
        std::lock_guard<std::mutex> lock{m_endpointsMutex};
        if (m_endpoints.end() != m_endpoints.find(endpoint->getEndpointId())) {
            previousEndpoint = m_endpoints[endpointId];
        }
    }

    error::FinallyGuard revertDirectiveRouting([this, &result, &handlersAdded, &handlersRemoved, previousEndpoint] {
        if (result != RegistrationResult::SUCCEEDED && previousEndpoint) {
            auto endpointId = previousEndpoint->getEndpointId();
            ACSDK_DEBUG5(LX("restoreDirectiveRoutingForPreviousEndpoint").sensitive("endpointId", endpointId));
            for (auto& handler : handlersAdded) {
                m_directiveSequencer->removeDirectiveHandler(handler);
            }
            for (auto& handler : handlersRemoved) {
                m_directiveSequencer->addDirectiveHandler(handler);
            }

            std::lock_guard<std::mutex> lock{m_endpointsMutex};
            m_endpoints[endpointId] = previousEndpoint;
        }
    });

    /// Remove capabilities of the previous endpoint, since we are replacing it with a new one, and add
    /// capabilities for the new endpoint. The previous endpoint will be restored if registering the new one
    /// fails.
    if ((previousEndpoint && !removeCapabilities(previousEndpoint, &handlersRemoved)) ||
        !addCapabilities(endpoint, &handlersAdded)) {
        result = RegistrationResult::CONFIGURATION_ERROR;
        return;
    }

    if (!m_capabilitiesDelegate->addOrUpdateEndpoint(
            endpoint->getAttributes(), endpoint->getCapabilityConfigurations())) {
        ACSDK_ERROR(LX("registerEndpointFailed").d("reason", "registerEndpointCapabilitiesFailed"));
        result = RegistrationResult::INTERNAL_ERROR;
        return;
    }

    result = RegistrationResult::SUCCEEDED;
    ACSDK_DEBUG2(LX(__func__).d("result", "finished").sensitive("endpointId", endpoint->getEndpointId()));
    return;
}

void EndpointRegistrationManager::executeDeregisterEndpoint(const std::shared_ptr<EndpointInterface>& endpoint) {
    ACSDK_DEBUG5(LX(__func__));
    DeregistrationResult result = DeregistrationResult::INTERNAL_ERROR;

    error::FinallyGuard notifyObserver([this, &endpoint, &result] {
        if (result != DeregistrationResult::SUCCEEDED) {
            std::list<std::shared_ptr<EndpointRegistrationObserverInterface>> observers;
            {
                std::lock_guard<std::mutex> lock{m_observersMutex};
                observers = m_observers;
            }

            for (auto& observer : observers) {
                observer->onEndpointDeregistration(endpoint->getEndpointId(), result);
            }

            std::lock_guard<std::mutex> lock{m_endpointsMutex};
            auto it = m_pendingDeregistrations.find(endpoint->getEndpointId());
            it->second.second.set_value(result);
            m_pendingDeregistrations.erase(it);
        }
    });

    // Since an endpoint may reuse the same directive handler, keep track of things that were removed.
    // Create logic that will restore the handlers removed in case of any failure while deleting the endpoint.
    std::unordered_set<std::shared_ptr<DirectiveHandlerInterface>> handlersRemoved;
    error::FinallyGuard revertDirectiveRouting([this, &result, &handlersRemoved] {
        if (result != DeregistrationResult::SUCCEEDED) {
            for (auto& handler : handlersRemoved) {
                m_directiveSequencer->addDirectiveHandler(handler);
            }
        }
    });

    /// Remove capabilities. If deleting the endpoint fails, the endpoint will be restored.
    if (!removeCapabilities(endpoint, &handlersRemoved)) {
        ACSDK_ERROR(LX("deregisterEndpointFailed")
                        .d("reason", "removeCapabilitiesFailed")
                        .sensitive("endpointId", endpoint->getEndpointId()));
        result = DeregistrationResult::CONFIGURATION_ERROR;
        return;
    }

    if (!m_capabilitiesDelegate->deleteEndpoint(endpoint->getAttributes(), endpoint->getCapabilityConfigurations())) {
        ACSDK_ERROR(LX("deregisterEndpointFailed").sensitive("endpointId", endpoint->getEndpointId()));
        result = DeregistrationResult::INTERNAL_ERROR;
        return;
    }

    result = DeregistrationResult::SUCCEEDED;
    ACSDK_DEBUG2(LX(__func__).d("result", "finished").sensitive("endpointId", endpoint->getEndpointId()));
    return;
}

void EndpointRegistrationManager::addObserver(std::shared_ptr<EndpointRegistrationObserverInterface> observer) {
    std::lock_guard<std::mutex> lock{m_observersMutex};
    m_observers.push_back(observer);
}

void EndpointRegistrationManager::removeObserver(
    const std::shared_ptr<EndpointRegistrationObserverInterface>& observer) {
    std::lock_guard<std::mutex> lock{m_observersMutex};
    m_observers.remove(observer);
}

void EndpointRegistrationManager::onCapabilityRegistrationStatusChanged(
    const std::pair<RegistrationResult, std::vector<EndpointIdentifier>>& addedOrUpdatedEndpoints,
    const std::pair<DeregistrationResult, std::vector<EndpointIdentifier>>& deletedEndpoints) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor.submit([this, addedOrUpdatedEndpoints, deletedEndpoints] {
        updateAddedOrUpdatedEndpoints(addedOrUpdatedEndpoints);
        removeDeletedEndpoints(deletedEndpoints);
    });
}

EndpointRegistrationManager::EndpointRegistrationManager(
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> directiveSequencer,
    std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate,
    const EndpointIdentifier& defaultEndpointId) :
        m_directiveSequencer{directiveSequencer},
        m_capabilitiesDelegate{capabilitiesDelegate},
        m_defaultEndpointId{defaultEndpointId},
        m_capabilityRegistrationProxy{std::make_shared<CapabilityRegistrationProxy>()} {
    m_capabilityRegistrationProxy->setCallback(std::bind(
        &EndpointRegistrationManager::onCapabilityRegistrationStatusChanged,
        this,
        std::placeholders::_1,
        std::placeholders::_2));
    m_capabilitiesDelegate->addCapabilitiesObserver(m_capabilityRegistrationProxy);
}

EndpointRegistrationManager::~EndpointRegistrationManager() {
    m_executor.shutdown();

    auto registrationResult = RegistrationResult::INTERNAL_ERROR;
    auto deregistrationResult = DeregistrationResult::INTERNAL_ERROR;

    /// Set promises for pending registrations and deregistrations.
    std::list<std::pair<EndpointIdentifier, avsCommon::avs::AVSDiscoveryEndpointAttributes>>
        addOrUpdateEndpointIdToAttributesPairs;
    std::list<EndpointIdentifier> deleteEndpointIds;
    {
        std::lock_guard<std::mutex> lock{m_endpointsMutex};

        for (auto& pendingRegistration : m_pendingRegistrations) {
            auto& pending = pendingRegistration.second;
            auto& endpoint = pending.first;
            auto endpointId = endpoint->getEndpointId();

            ACSDK_DEBUG5(LX(__func__).d("endpointId", endpointId));
            addOrUpdateEndpointIdToAttributesPairs.emplace_back(std::make_pair(endpointId, endpoint->getAttributes()));

            pending.second.set_value(registrationResult);
        }

        for (auto& pendingDeregistration : m_pendingDeregistrations) {
            auto& pending = pendingDeregistration.second;
            auto& endpoint = pending.first;
            auto endpointId = endpoint->getEndpointId();

            ACSDK_DEBUG5(LX(__func__).d("endpointId", endpointId));
            deleteEndpointIds.push_back(endpointId);

            pending.second.set_value(deregistrationResult);
        }
    }

    /// Notify observers.
    std::list<std::shared_ptr<EndpointRegistrationObserverInterface>> observers;
    {
        std::lock_guard<std::mutex> lock{m_observersMutex};
        observers = m_observers;
    }

    for (auto& observer : observers) {
        for (auto& addOrUpdateEndpointIdToAttributesPair : addOrUpdateEndpointIdToAttributesPairs) {
            observer->onEndpointRegistration(
                addOrUpdateEndpointIdToAttributesPair.first,
                addOrUpdateEndpointIdToAttributesPair.second,
                registrationResult);
        }

        for (auto& deleteEndpointId : deleteEndpointIds) {
            observer->onEndpointDeregistration(deleteEndpointId, deregistrationResult);
        }
    }

    m_capabilitiesDelegate->removeCapabilitiesObserver(m_capabilityRegistrationProxy);
}

void EndpointRegistrationManager::CapabilityRegistrationProxy::onCapabilitiesStateChange(
    CapabilitiesObserverInterface::State newState,
    CapabilitiesObserverInterface::Error newError,
    const std::vector<EndpointIdentifier>& addedOrUpdatedEndpointIds,
    const std::vector<EndpointIdentifier>& deletedEndpointIds) {
    ACSDK_DEBUG5(LX(__func__).d("state", newState).d("error", newError).d("callback", static_cast<bool>(m_callback)));
    if (m_callback && (CapabilitiesObserverInterface::State::RETRIABLE_ERROR != newState)) {
        auto registrationResult = (CapabilitiesObserverInterface::State::SUCCESS == newState)
                                      ? RegistrationResult::SUCCEEDED
                                      : RegistrationResult::CONFIGURATION_ERROR;
        auto deregistrationResult = (CapabilitiesObserverInterface::State::SUCCESS == newState)
                                        ? DeregistrationResult::SUCCEEDED
                                        : DeregistrationResult::CONFIGURATION_ERROR;
        m_callback(
            std::make_pair(registrationResult, addedOrUpdatedEndpointIds),
            std::make_pair(deregistrationResult, deletedEndpointIds));
    }
}

void EndpointRegistrationManager::CapabilityRegistrationProxy::setCallback(
    std::function<void(
        const std::pair<RegistrationResult, std::vector<EndpointIdentifier>>& addedOrUpdatedEndpoints,
        const std::pair<DeregistrationResult, std::vector<EndpointIdentifier>>& deletedEndpoints)> callback) {
    m_callback = callback;
}

void EndpointRegistrationManager::updateAddedOrUpdatedEndpoints(
    const std::pair<RegistrationResult, std::vector<EndpointIdentifier>>& addedOrUpdatedEndpoints) {
    auto registrationResult = addedOrUpdatedEndpoints.first;

    /// Add the added or updated endpoints.
    std::list<std::pair<EndpointIdentifier, avsCommon::avs::AVSDiscoveryEndpointAttributes>>
        addOrUpdateEndpointIdToAttributesPairs;
    {
        std::lock_guard<std::mutex> lock{m_endpointsMutex};
        for (auto& addedOrUpdatedId : addedOrUpdatedEndpoints.second) {
            auto pendingEndpointId = m_pendingRegistrations.find(addedOrUpdatedId);
            if (m_pendingRegistrations.end() != pendingEndpointId) {
                auto& pending = pendingEndpointId->second;
                auto& endpoint = pending.first;

                addOrUpdateEndpointIdToAttributesPairs.emplace_back(
                    std::make_pair(addedOrUpdatedId, endpoint->getAttributes()));

                pending.second.set_value(registrationResult);

                if (RegistrationResult::SUCCEEDED == registrationResult) {
                    ACSDK_DEBUG9(LX(__func__).d("result", "success").sensitive("endpointId", addedOrUpdatedId));
                    m_endpoints[addedOrUpdatedId] = endpoint;
                } else {
                    ACSDK_ERROR(LX(__func__).d("result", "failed").sensitive("endpointId", addedOrUpdatedId));

                    /// If registering the new endpoint failed, remove its capabilities.
                    if (!removeCapabilities(endpoint)) {
                        ACSDK_ERROR((LX("failedToRemoveCapabilitiesFromFailedEndpointRegistration")));
                    }

                    /// Restore the original endpoint, if it exists.
                    auto originalEndpoint = m_endpoints.find(addedOrUpdatedId);
                    if ((m_endpoints.end() != originalEndpoint) && (!addCapabilities(originalEndpoint->second))) {
                        ACSDK_ERROR((LX("failedToRestorePreviousEndpoint").d("result", "removingPreviousEndpoint")));
                        m_endpoints.erase(addedOrUpdatedId);
                    }
                }

                m_pendingRegistrations.erase(addedOrUpdatedId);
            } else {
                ACSDK_DEBUG9(LX("updateAddedOrUpdatedEndpointsSkippedForEndpoint")
                                 .d("reason", "endpoint not found in pending operations")
                                 .sensitive("endpointId", addedOrUpdatedId));
            }
        }
    }

    /// Notify observers.
    std::list<std::shared_ptr<EndpointRegistrationObserverInterface>> observers;
    {
        std::lock_guard<std::mutex> lock{m_observersMutex};
        observers = m_observers;
    }

    for (auto& observer : observers) {
        for (auto& addOrUpdateEndpointIdToAttributesPair : addOrUpdateEndpointIdToAttributesPairs) {
            observer->onEndpointRegistration(
                addOrUpdateEndpointIdToAttributesPair.first,
                addOrUpdateEndpointIdToAttributesPair.second,
                registrationResult);
        }
    }
}

void EndpointRegistrationManager::removeDeletedEndpoints(
    const std::pair<DeregistrationResult, std::vector<EndpointIdentifier>>& deletedEndpoints) {
    auto deregistrationResult = deletedEndpoints.first;

    /// Remove deleted endpoints.
    {
        std::lock_guard<std::mutex> lock{m_endpointsMutex};
        for (auto& deletedId : deletedEndpoints.second) {
            auto pendingEndpointId = m_pendingDeregistrations.find(deletedId);
            if (m_pendingDeregistrations.end() != pendingEndpointId) {
                auto& pending = pendingEndpointId->second;

                pending.second.set_value(deregistrationResult);

                if (DeregistrationResult::SUCCEEDED == deregistrationResult) {
                    ACSDK_DEBUG5(LX(__func__).d("result", "success").sensitive("endpointId", deletedId));
                    m_endpoints.erase(deletedId);
                } else {
                    /// If deregistration failed, restore the previous endpoint.
                    auto previousEndpoint = m_endpoints.find(deletedId);
                    if ((m_endpoints.end() != previousEndpoint) && (!addCapabilities(previousEndpoint->second))) {
                        ACSDK_ERROR((LX("failedToRestorePreviousEndpoint")
                                         .d("result", "removingEndpoint")
                                         .sensitive("endpointId", deletedId)));
                        m_endpoints.erase(deletedId);
                    } else {
                        ACSDK_ERROR((LX("deregisterEndpointFailed")
                                         .d("result", "restoringEndpoint")
                                         .sensitive("endpointId", deletedId)));
                    }
                }
                m_pendingDeregistrations.erase(deletedId);
            } else {
                if (m_endpoints.end() != m_endpoints.find(deletedId)) {
                    ACSDK_DEBUG9(LX("removeDeletedEndpointsSkippedForEndpoint")
                                     .d("reason", "endpoint not found in pending operations")
                                     .sensitive("endpointId", deletedId));
                }
            }
        }
    }

    /// Notify observers.
    std::list<std::shared_ptr<EndpointRegistrationObserverInterface>> observers;
    {
        std::lock_guard<std::mutex> lock{m_observersMutex};
        observers = m_observers;
    }

    for (auto& deletedId : deletedEndpoints.second) {
        for (auto& observer : observers) {
            observer->onEndpointDeregistration(deletedId, deregistrationResult);
        }
    }
}

bool EndpointRegistrationManager::removeCapabilities(
    const std::shared_ptr<EndpointInterface>& endpoint,
    std::unordered_set<std::shared_ptr<DirectiveHandlerInterface>>* handlersRemoved) {
    if (!endpoint) {
        ACSDK_ERROR(LX("removeCapabilitiesFailed").d("reason", "Null endpoint"));
        return false;
    }

    if (!handlersRemoved) {
        ACSDK_ERROR(LX("removeCapabilitiesFailed").d("reason", "Null handlersRemoved"));
        return false;
    }

    // Remove capabilities.
    for (auto& capability : endpoint->getCapabilities()) {
        auto& handler = capability.second;
        if (handler) {
            if ((handlersRemoved->find(handler) == handlersRemoved->end()) &&
                !m_directiveSequencer->removeDirectiveHandler(handler)) {
                auto& configuration = capability.first;
                ACSDK_ERROR(LX("deregisterEndpointFailed")
                                .d("reason", "removeDirectiveHandlerFailed")
                                .d("interface", configuration.interfaceName)
                                .d("instance", configuration.instanceName.valueOr("")));
                return false;
            } else {
                handlersRemoved->insert(handler);
            }
        } else {
            ACSDK_DEBUG5(LX("deregisterEndpoint")
                             .d("emptyHandler", capability.first.interfaceName)
                             .sensitive("endpoint", endpoint->getEndpointId()));
        }
    }
    return true;
}

bool EndpointRegistrationManager::addCapabilities(
    const std::shared_ptr<EndpointInterface>& endpoint,
    std::unordered_set<std::shared_ptr<DirectiveHandlerInterface>>* handlersAdded) {
    if (!endpoint) {
        ACSDK_ERROR(LX("addCapabilitiesFailed").d("reason", "Null endpoint"));
        return false;
    }

    if (!handlersAdded) {
        ACSDK_ERROR(LX("addCapabilitiesFailed").d("reason", "Null handlersAdded"));
        return false;
    }

    /// Add directive handlers for the new endpoint.
    auto capabilitiesToAdd = endpoint->getCapabilities();
    for (auto& capability : capabilitiesToAdd) {
        auto& handler = capability.second;
        if (handler) {
            if ((handlersAdded->end() == handlersAdded->find(handler)) &&
                !m_directiveSequencer->addDirectiveHandler(handler)) {
                auto& configuration = capability.first;
                ACSDK_ERROR(LX("registerEndpointFailed")
                                .d("reason", "addDirectiveHandlerFailed")
                                .d("interface", configuration.interfaceName)
                                .d("instance", configuration.instanceName.valueOr("")));
                return false;
            }

            handlersAdded->insert(handler);
        } else {
            ACSDK_DEBUG5(LX("registerEndpoint")
                             .d("emptyHandler", capability.first.interfaceName)
                             .sensitive("endpoint", endpoint->getEndpointId()));
        }
    }
    return true;
}

bool EndpointRegistrationManager::addCapabilities(const std::shared_ptr<EndpointInterface>& endpoint) {
    std::unordered_set<std::shared_ptr<DirectiveHandlerInterface>> handlersAdded;
    return addCapabilities(endpoint, &handlersAdded);
}

bool EndpointRegistrationManager::removeCapabilities(const std::shared_ptr<EndpointInterface>& endpoint) {
    std::unordered_set<std::shared_ptr<DirectiveHandlerInterface>> handlersRemoved;
    return removeCapabilities(endpoint, &handlersRemoved);
}

}  // namespace endpoints
}  // namespace alexaClientSDK
