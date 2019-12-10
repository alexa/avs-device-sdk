/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
    std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate) {
    if (!directiveSequencer) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullDirectiveSequencer"));
        return nullptr;
    }

    if (!capabilitiesDelegate) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullCapabilitiesDelegate"));
        return nullptr;
    }
    return std::unique_ptr<EndpointRegistrationManager>(
        new EndpointRegistrationManager(directiveSequencer, capabilitiesDelegate));
}

std::future<EndpointRegistrationManager::RegistrationResult> EndpointRegistrationManager::registerEndpoint(
    std::shared_ptr<EndpointInterface> endpoint) {
    if (!endpoint) {
        ACSDK_ERROR(LX("registerEndpointFailed").d("reason", "nullEndpoint"));
        std::promise<RegistrationResult> promise;
        promise.set_value(RegistrationResult::CONFIGURATION_ERROR);
        return promise.get_future();
    }

    std::lock_guard<std::mutex> lock{m_mutex};
    auto endpointId = endpoint->getEndpointId();
    if (m_pendingRegistrations.find(endpointId) != m_pendingRegistrations.end()) {
        ACSDK_ERROR(LX("registerEndpointFailed").d("reason", "registrationInProgress"));
        std::promise<RegistrationResult> promise;
        promise.set_value(RegistrationResult::PENDING_REGISTRATION);
        return promise.get_future();
    }

    m_executor.submit([this, endpoint]() { executeEndpointRegistration(endpoint); });

    auto& pending = m_pendingRegistrations[endpointId];
    pending.first = std::move(endpoint);
    return pending.second.get_future();
}

void EndpointRegistrationManager::executeEndpointRegistration(const std::shared_ptr<EndpointInterface>& endpoint) {
    RegistrationResult result = RegistrationResult::INTERNAL_ERROR;
    error::FinallyGuard notifyObserver([this, &endpoint, &result] {
        if (result != RegistrationResult::SUCCEEDED) {
            std::lock_guard<std::mutex>{m_mutex};
            for (auto& observer : m_observers) {
                observer->onEndpointRegistration(endpoint->getEndpointId(), endpoint->getAttributes(), result);
            }
            auto it = m_pendingRegistrations.find(endpoint->getEndpointId());
            it->second.second.set_value(result);
            m_pendingRegistrations.erase(it);
        }
    });

    if (!m_registrationEnabled) {
        ACSDK_ERROR(LX("registerEndpointFailed").d("reason", "registrationDisabled"));
        result = RegistrationResult::REGISTRATION_DISABLED;
        return;
    }

    if (m_endpoints.find(endpoint->getEndpointId()) != m_endpoints.end()) {
        ACSDK_ERROR(LX("registerEndpointFailed")
                        .d("reason", "alreadyExist")
                        .sensitive("endpointId", endpoint->getEndpointId()));
        result = RegistrationResult::ALREADY_REGISTERED;
        return;
    }

    // Since an endpoint may reuse the same directive handler, keep track of things that were added already.
    // Create logic that will remove the handlers registered in case of any failure while adding the endpoint.
    std::unordered_set<std::shared_ptr<DirectiveHandlerInterface>> handlersAdded;
    error::FinallyGuard revertDirectiveRouting([this, &result, &handlersAdded] {
        // Remove the directive handlers for this endpoint if registration fails.
        if (result != RegistrationResult::SUCCEEDED) {
            for (auto& revert : handlersAdded) {
                m_directiveSequencer->removeDirectiveHandler(revert);
            }
        }
    });

    auto capabilities = endpoint->getCapabilities();
    for (auto& capability : capabilities) {
        auto& handler = capability.second;
        if (handler) {
            if ((handlersAdded.find(handler) == handlersAdded.end()) &&
                !m_directiveSequencer->addDirectiveHandler(handler)) {
                auto& configuration = capability.first;
                ACSDK_ERROR(LX("registerEndpointFailed")
                                .d("reason", "addDirectiveHandlerFailed")
                                .d("interface", configuration.interfaceName)
                                .d("instance", configuration.instanceName.valueOr("")));
                result = RegistrationResult::CONFIGURATION_ERROR;
                return;
            }

            handlersAdded.insert(handler);
        } else {
            ACSDK_DEBUG5(LX("registerEndpoint")
                             .d("emptyHandler", capability.first.interfaceName)
                             .sensitive("endpoint", endpoint->getEndpointId()));
        }
    }

    if (!m_capabilitiesDelegate->registerEndpoint(endpoint->getAttributes(), endpoint->getCapabilityConfigurations())) {
        ACSDK_ERROR(LX("registerEndpointFailed").d("reason", "registerEndpointCapabilitiesFailed"));
        result = RegistrationResult::INTERNAL_ERROR;
        return;
    }

    result = RegistrationResult::SUCCEEDED;
    ACSDK_DEBUG2(LX("registerEndpoint").sensitive("endpointId", endpoint->getEndpointId()));
    return;
}

void EndpointRegistrationManager::addObserver(std::shared_ptr<EndpointRegistrationObserverInterface> observer) {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_observers.push_back(observer);
}

void EndpointRegistrationManager::removeObserver(
    const std::shared_ptr<EndpointRegistrationObserverInterface>& observer) {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_observers.remove(observer);
}

void EndpointRegistrationManager::disableRegistration() {
    // Ensure that we finish any ongoing registration.
    auto disabled = m_executor.submit([this] { m_registrationEnabled = false; });
    disabled.wait();
}

void EndpointRegistrationManager::onCapabilityRegistrationStatusChanged(
    EndpointRegistrationManagerInterface::RegistrationResult result) {
    m_executor.submit([this, result] {
        std::lock_guard<std::mutex> lock{m_mutex};
        for (auto& item : m_pendingRegistrations) {
            auto& id = item.first;
            auto& pending = item.second;
            auto& endpoint = pending.first;
            auto attributes = endpoint->getAttributes();
            for (auto& observer : m_observers) {
                observer->onEndpointRegistration(id, attributes, result);
            }
            pending.second.set_value(result);
            if (result == RegistrationResult::SUCCEEDED) {
                m_endpoints.insert({id, endpoint});
            }
        }
        m_pendingRegistrations.clear();
    });
}

EndpointRegistrationManager::EndpointRegistrationManager(
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> directiveSequencer,
    std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate) :
        m_directiveSequencer{directiveSequencer},
        m_capabilitiesDelegate{capabilitiesDelegate},
        m_capabilityRegistrationProxy{std::make_shared<CapabilityRegistrationProxy>()},
        m_registrationEnabled{true} {
    m_capabilityRegistrationProxy->setCallback(
        std::bind(&EndpointRegistrationManager::onCapabilityRegistrationStatusChanged, this, std::placeholders::_1));
    m_capabilitiesDelegate->addCapabilitiesObserver(m_capabilityRegistrationProxy);
}

EndpointRegistrationManager::~EndpointRegistrationManager() {
    m_capabilitiesDelegate->removeCapabilitiesObserver(m_capabilityRegistrationProxy);
}

void EndpointRegistrationManager::CapabilityRegistrationProxy::onCapabilitiesStateChange(
    CapabilitiesObserverInterface::State newState,
    CapabilitiesObserverInterface::Error newError) {
    ACSDK_DEBUG5(LX(__func__).d("state", newState).d("error", newError).d("callback", static_cast<bool>(m_callback)));
    if (m_callback && (newState != CapabilitiesObserverInterface::State::RETRIABLE_ERROR)) {
        auto result = (newState == CapabilitiesObserverInterface::State::SUCCESS)
                          ? RegistrationResult::SUCCEEDED
                          : RegistrationResult::CONFIGURATION_ERROR;
        m_callback(result);
    }
}  // namespace endpoints

void EndpointRegistrationManager::CapabilityRegistrationProxy::setCallback(
    std::function<void(RegistrationResult)> callback) {
    m_callback = callback;
}

}  // namespace endpoints
}  // namespace alexaClientSDK
