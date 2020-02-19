/*
 * Copyright 2018-2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <functional>

#include "CapabilitiesDelegate/CapabilitiesDelegate.h"
#include "CapabilitiesDelegate/DiscoveryEventSenderInterface.h"
#include "CapabilitiesDelegate/PostConnectCapabilitiesPublisher.h"
#include "CapabilitiesDelegate/Utils/DiscoveryUtils.h"

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace capabilitiesDelegate {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::endpoints;
using namespace avsCommon::utils;
using namespace avsCommon::utils::configuration;
using namespace capabilitiesDelegate::utils;

/// String to identify log entries originating from this file.
static const std::string TAG("CapabilitiesDelegate");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<CapabilitiesDelegate> CapabilitiesDelegate::create(
    const std::shared_ptr<AuthDelegateInterface>& authDelegate,
    const std::shared_ptr<storage::CapabilitiesDelegateStorageInterface>& capabilitiesDelegateStorage,
    const std::shared_ptr<registrationManager::CustomerDataManager>& customerDataManager) {
    if (!authDelegate) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullAuthDelegate"));
    } else if (!capabilitiesDelegateStorage) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullCapabilitiesDelegateStorage"));
    } else if (!customerDataManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullCustomerDataManager"));
    } else {
        std::shared_ptr<CapabilitiesDelegate> instance(
            new CapabilitiesDelegate(authDelegate, capabilitiesDelegateStorage, customerDataManager));
        if (!(instance->init())) {
            ACSDK_ERROR(LX("createFailed").d("reason", "CapabilitiesDelegateInitFailed"));
            return nullptr;
        }

        return instance;
    }

    return nullptr;
}

CapabilitiesDelegate::CapabilitiesDelegate(
    const std::shared_ptr<AuthDelegateInterface>& authDelegate,
    const std::shared_ptr<storage::CapabilitiesDelegateStorageInterface>& capabilitiesDelegateStorage,
    const std::shared_ptr<registrationManager::CustomerDataManager>& customerDataManager) :
        RequiresShutdown{"CapabilitiesDelegate"},
        CustomerDataHandler{customerDataManager},
        m_capabilitiesState{CapabilitiesObserverInterface::State::UNINITIALIZED},
        m_capabilitiesError{CapabilitiesObserverInterface::Error::UNINITIALIZED},
        m_authDelegate{authDelegate},
        m_capabilitiesDelegateStorage{capabilitiesDelegateStorage} {
}

void CapabilitiesDelegate::doShutdown() {
    {
        std::lock_guard<std::mutex> lock(m_observerMutex);
        m_capabilitiesObservers.clear();
    }
    resetDiscoveryEventSender();
}

void CapabilitiesDelegate::clearData() {
    ACSDK_DEBUG5(LX(__func__));
    if (!m_capabilitiesDelegateStorage->clearDatabase()) {
        ACSDK_ERROR(LX("clearDataFailed").d("reason", "unable to clear database"));
    }
}

bool CapabilitiesDelegate::init() {
    const std::string errorEvent = "initFailed";
    std::string infoEvent = "CapabilitiesDelegateInit";

    if (!m_capabilitiesDelegateStorage->open()) {
        ACSDK_INFO(LX(__func__).m("Couldn't open database. Creating."));
        if (!m_capabilitiesDelegateStorage->createDatabase()) {
            ACSDK_ERROR(LX("initFailed").m("Could not create database"));
            return false;
        }
    }

    return true;
}

void CapabilitiesDelegate::addCapabilitiesObserver(std::shared_ptr<CapabilitiesObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("addCapabilitiesObserverFailed").d("reason", "nullObserver"));
        return;
    }

    ACSDK_DEBUG5(LX("addCapabilitiesObserver").d("observer", observer.get()));
    std::lock_guard<std::mutex> lock(m_observerMutex);
    if (!m_capabilitiesObservers.insert(observer).second) {
        ACSDK_WARN(LX("addCapabilitiesObserverFailed").d("reason", "observerAlreadyAdded"));
        return;
    }
    observer->onCapabilitiesStateChange(m_capabilitiesState, m_capabilitiesError);
}

void CapabilitiesDelegate::removeCapabilitiesObserver(std::shared_ptr<CapabilitiesObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("removeCapabilitiesObserverFailed").d("reason", "nullObserver"));
        return;
    }

    ACSDK_DEBUG5(LX("removeCapabilitiesObserver").d("observer", observer.get()));
    std::lock_guard<std::mutex> lock(m_observerMutex);
    if (m_capabilitiesObservers.erase(observer) == 0) {
        ACSDK_WARN(LX("removeCapabilitiesObserverFailed").d("reason", "observerNotAdded"));
    }
}

void CapabilitiesDelegate::setCapabilitiesState(
    CapabilitiesObserverInterface::State newCapabilitiesState,
    CapabilitiesObserverInterface::Error newCapabilitiesError) {
    ACSDK_DEBUG5(LX("setCapabilitiesState").d("newCapabilitiesState", newCapabilitiesState));

    std::lock_guard<std::mutex> lock(m_observerMutex);
    if ((newCapabilitiesState == m_capabilitiesState) && (newCapabilitiesError == m_capabilitiesError)) {
        return;
    }
    m_capabilitiesState = newCapabilitiesState;
    m_capabilitiesError = newCapabilitiesError;

    if (!m_capabilitiesObservers.empty()) {
        ACSDK_DEBUG9(
            LX("callingOnCapabilitiesStateChange").d("state", m_capabilitiesState).d("error", m_capabilitiesError));
        for (auto& observer : m_capabilitiesObservers) {
            observer->onCapabilitiesStateChange(m_capabilitiesState, m_capabilitiesError);
        }
    }
}

void CapabilitiesDelegate::invalidateCapabilities() {
    ACSDK_DEBUG5(LX(__func__));
    if (!m_capabilitiesDelegateStorage->clearDatabase()) {
        ACSDK_ERROR(LX("invalidateCapabilitiesFailed").d("reason", "unable to clear database"));
    }
}

bool CapabilitiesDelegate::registerEndpoint(
    const AVSDiscoveryEndpointAttributes& endpointAttributes,
    const std::vector<CapabilityConfiguration>& capabilities) {
    ACSDK_DEBUG5(LX(__func__));

    if (!validateEndpointAttributes(endpointAttributes)) {
        ACSDK_ERROR(LX("registerEndpointFailed").d("reason", "invalidAVSDiscoveryEndpointAttributes"));
        return false;
    }

    if (capabilities.empty()) {
        ACSDK_ERROR(LX("registerEndpointFailed").d("reason", "invalidCapabilities"));
        return false;
    }

    for (auto& capability : capabilities) {
        if (!validateCapabilityConfiguration(capability)) {
            ACSDK_ERROR(LX("registerEndpointFailed")
                            .d("reason", "invalidCapabilityConfiguration")
                            .d("capability", capability.interfaceName));
            return false;
        }
    }

    // Format endpoint JSONs.
    std::string endpointId = endpointAttributes.endpointId;
    std::lock_guard<std::mutex> lock{m_endpointMapMutex};
    auto it = m_endpointIdToConfigMap.find(endpointId);
    if (it != m_endpointIdToConfigMap.end()) {
        ACSDK_ERROR(LX("registerEndpointFailed").d("reason", "endpointAlreadyRegistered"));
        return false;
    } else {
        std::string endpointConfigJson = getEndpointConfigJson(endpointAttributes, capabilities);
        m_endpointIdToConfigMap.insert({endpointId, endpointConfigJson});
    }

    return true;
}

void CapabilitiesDelegate::onAlexaEventProcessedReceived(const std::string& eventCorrelationToken) {
    ACSDK_DEBUG5(LX(__func__));
    /// Forward the eventCorrelation token to the discovery event sender.
    std::lock_guard<std::mutex> lock{m_discoveryEventSenderMutex};
    if (m_currentDiscoveryEventSender) {
        m_currentDiscoveryEventSender->onAlexaEventProcessedReceived(eventCorrelationToken);
    }
}

std::shared_ptr<PostConnectOperationInterface> CapabilitiesDelegate::createPostConnectOperation() {
    ACSDK_DEBUG5(LX(__func__));

    std::unordered_map<std::string, std::string> addOrUpdateEndpointIdToConfigPairs;
    std::unordered_map<std::string, std::string> deletedEndpointIdToConfigPairs;
    ACSDK_DEBUG5(LX(__func__).d("num of endpoints registered", m_endpointIdToConfigMap.size()));

    std::unordered_map<std::string, std::string> storedEndpointConfig;
    if (!m_capabilitiesDelegateStorage->load(&storedEndpointConfig)) {
        ACSDK_ERROR(LX("createPostConnectOperationFailed").m("Could not load previous config from database."));
        return nullptr;
    }
    ACSDK_DEBUG5(LX(__func__).d("num endpoints stored", storedEndpointConfig.size()));

    {
        std::lock_guard<std::mutex> lock{m_endpointMapMutex};

        /// Find the endpoints whose configuration has changed.
        for (auto& endpointIdToConfigPair : m_endpointIdToConfigMap) {
            auto storedEndpointConfigIt = storedEndpointConfig.find(endpointIdToConfigPair.first);
            if (storedEndpointConfigIt != storedEndpointConfig.end()) {
                if (endpointIdToConfigPair.second != storedEndpointConfigIt->second) {
                    /// Endpoint configuration has been updated.
                    ACSDK_DEBUG5(LX(__func__)
                                     .d("step", "endpointUpdated")
                                     .sensitive("configuration", endpointIdToConfigPair.second));
                    addOrUpdateEndpointIdToConfigPairs.insert(endpointIdToConfigPair);
                }
                /// Remove this endpoint from the stored endpoint list.
                storedEndpointConfig.erase(endpointIdToConfigPair.first);
            } else {
                /// Endpoint has been freshly added.
                ACSDK_DEBUG5(
                    LX(__func__).d("step", "newEndpoint").sensitive("configuration", endpointIdToConfigPair.second));
                addOrUpdateEndpointIdToConfigPairs.insert(endpointIdToConfigPair);
            }
        }
        /// The remaining items in the stored list are the endpoints that need to be deleted.
        for (auto& it : storedEndpointConfig) {
            deletedEndpointIdToConfigPairs.insert({it.first, getDeleteReportEndpointConfigJson(it.first)});
        }
    }

    if (addOrUpdateEndpointIdToConfigPairs.empty() && deletedEndpointIdToConfigPairs.empty()) {
        ACSDK_DEBUG5(LX(__func__).m("No change in Capabilities, skipping post connect step"));
    } else {
        auto instance = PostConnectCapabilitiesPublisher::create(
            addOrUpdateEndpointIdToConfigPairs, deletedEndpointIdToConfigPairs, m_authDelegate);

        if (instance) {
            addDiscoveryEventSender(instance);
        }

        return instance;
    }
    return nullptr;
}

void CapabilitiesDelegate::onDiscoveryCompleted(
    const std::unordered_map<std::string, std::string>& addOrUpdateReportEndpoints,
    const std::unordered_map<std::string, std::string>& deleteReportEndpoints) {
    ACSDK_DEBUG5(LX(__func__));
    if (!updateEndpointConfigInStorage(addOrUpdateReportEndpoints, deleteReportEndpoints)) {
        ACSDK_ERROR(LX("publishCapabilitiesFailed").d("reason", "failed to save endpointConfig to database"));
        setCapabilitiesState(
            CapabilitiesObserverInterface::State::FATAL_ERROR, CapabilitiesObserverInterface::Error::UNKNOWN_ERROR);
        return;
    }
    setCapabilitiesState(CapabilitiesObserverInterface::State::SUCCESS, CapabilitiesObserverInterface::Error::SUCCESS);
}

void CapabilitiesDelegate::onDiscoveryFailure(MessageRequestObserverInterface::Status status) {
    ACSDK_DEBUG5(LX(__func__));
    if (status == MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalidStatus").d("status", status));
        return;
    }
    ACSDK_ERROR(LX(__func__).d("reason", status));

    switch (status) {
        case MessageRequestObserverInterface::Status::INVALID_AUTH:
            setCapabilitiesState(
                CapabilitiesObserverInterface::State::FATAL_ERROR, CapabilitiesObserverInterface::Error::FORBIDDEN);
            break;
        case MessageRequestObserverInterface::Status::BAD_REQUEST:
            setCapabilitiesState(
                CapabilitiesObserverInterface::State::FATAL_ERROR, CapabilitiesObserverInterface::Error::BAD_REQUEST);
            break;
        case MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2:
            setCapabilitiesState(
                CapabilitiesObserverInterface::State::RETRIABLE_ERROR,
                CapabilitiesObserverInterface::Error::SERVER_INTERNAL_ERROR);
            break;
        default:
            setCapabilitiesState(
                CapabilitiesObserverInterface::State::RETRIABLE_ERROR,
                CapabilitiesObserverInterface::Error::UNKNOWN_ERROR);
            break;
    }
}

bool CapabilitiesDelegate::updateEndpointConfigInStorage(
    const std::unordered_map<std::string, std::string>& addOrUpdateReportEndpoints,
    const std::unordered_map<std::string, std::string>& deleteReportEndpoints) {
    ACSDK_DEBUG5(LX(__func__));
    if (!m_capabilitiesDelegateStorage) {
        ACSDK_ERROR(LX("updateEndpointConfigInStorageLockedFailed").d("reason", "invalidStorage"));
        return false;
    }

    if (!m_capabilitiesDelegateStorage->store(addOrUpdateReportEndpoints)) {
        ACSDK_ERROR(LX("updateStorageFailed").d("reason", "storeFailed"));
        return false;
    }

    if (!m_capabilitiesDelegateStorage->erase(deleteReportEndpoints)) {
        ACSDK_ERROR(LX("updateStorageFailed").d("reason", "eraseFailed"));
        return false;
    }

    return true;
}

void CapabilitiesDelegate::addDiscoveryEventSender(
    const std::shared_ptr<DiscoveryEventSenderInterface>& discoveryEventSender) {
    ACSDK_DEBUG5(LX(__func__));
    if (!discoveryEventSender) {
        ACSDK_ERROR(LX("addDiscoveryEventSenderFailed").d("reason", "invalid sender"));
        return;
    }
    resetDiscoveryEventSender();
    std::lock_guard<std::mutex> lock_guard{m_discoveryEventSenderMutex};
    if (discoveryEventSender) {
        discoveryEventSender->addDiscoveryStatusObserver(shared_from_this());
    }
    m_currentDiscoveryEventSender = discoveryEventSender;
}

void CapabilitiesDelegate::resetDiscoveryEventSender() {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lockGuard{m_discoveryEventSenderMutex};
    if (m_currentDiscoveryEventSender) {
        m_currentDiscoveryEventSender->removeDiscoveryStatusObserver(shared_from_this());
    }
    m_currentDiscoveryEventSender.reset();
}

void CapabilitiesDelegate::onAVSGatewayChanged(const std::string& avsGateway) {
    ACSDK_DEBUG5(LX(__func__));
    invalidateCapabilities();
}

}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK
