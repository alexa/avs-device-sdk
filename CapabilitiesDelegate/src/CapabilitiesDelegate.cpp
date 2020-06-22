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

#include <functional>

#include "CapabilitiesDelegate/CapabilitiesDelegate.h"
#include "CapabilitiesDelegate/DiscoveryEventSender.h"
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

/**
 * Gets the @c EndpointIdentifiers from a map of EndpointIdentifier to configuration.
 *
 * @param endpoints The map of endpoint to configuration.
 * @return A vector of @c EndpointIdentifiers.
 */
static std::vector<EndpointIdentifier> getEndpointIdentifiers(
    const std::unordered_map<std::string, std::string>& endpoints) {
    std::vector<std::string> identifiers;
    if (!endpoints.empty()) {
        identifiers.reserve(endpoints.size());
        for (auto const& endpoint : endpoints) {
            identifiers.push_back(endpoint.first);
        }
    }

    return identifiers;
}

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
        m_capabilitiesDelegateStorage{capabilitiesDelegateStorage},
        m_isConnected{false},
        m_isShuttingDown{false} {
}

void CapabilitiesDelegate::doShutdown() {
    ACSDK_DEBUG5(LX(__func__));
    {
        std::lock_guard<std::mutex> lock(m_isShuttingDownMutex);
        m_isShuttingDown = true;
    }

    {
        std::lock_guard<std::mutex> lock(m_observerMutex);
        m_capabilitiesObservers.clear();
    }

    m_executor.shutdown();
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

    {
        std::lock_guard<std::mutex> lock(m_observerMutex);
        if (!m_capabilitiesObservers.insert(observer).second) {
            ACSDK_WARN(LX("addCapabilitiesObserverFailed").d("reason", "observerAlreadyAdded"));
            return;
        }
    }

    observer->onCapabilitiesStateChange(
        m_capabilitiesState, m_capabilitiesError, std::vector<std::string>{}, std::vector<std::string>{});
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
    const CapabilitiesObserverInterface::State newCapabilitiesState,
    const CapabilitiesObserverInterface::Error newCapabilitiesError,
    const std::vector<EndpointIdentifier>& addOrUpdateReportEndpoints,
    const std::vector<EndpointIdentifier>& deleteReportEndpoints) {
    ACSDK_DEBUG5(LX("setCapabilitiesState").d("newCapabilitiesState", newCapabilitiesState));

    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesObserverInterface>> capabilitiesObservers;
    {
        std::lock_guard<std::mutex> lock(m_observerMutex);
        capabilitiesObservers = m_capabilitiesObservers;
        m_capabilitiesState = newCapabilitiesState;
        m_capabilitiesError = newCapabilitiesError;
    }

    if (!capabilitiesObservers.empty()) {
        ACSDK_DEBUG9(
            LX("callingOnCapabilitiesStateChange").d("state", m_capabilitiesState).d("error", m_capabilitiesError));
        for (auto& observer : capabilitiesObservers) {
            observer->onCapabilitiesStateChange(
                newCapabilitiesState, newCapabilitiesError, addOrUpdateReportEndpoints, deleteReportEndpoints);
        }
    }
}

void CapabilitiesDelegate::setMessageSender(
    const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender) {
    ACSDK_DEBUG5(LX(__func__));
    if (!messageSender) {
        ACSDK_ERROR(LX("setMessageSenderFailed").d("reason", "Null messageSender"));
        return;
    }
    std::lock_guard<std::mutex> lock(m_messageSenderMutex);
    m_messageSender = messageSender;
}

void CapabilitiesDelegate::invalidateCapabilities() {
    ACSDK_DEBUG5(LX(__func__));
    if (!m_capabilitiesDelegateStorage->clearDatabase()) {
        ACSDK_ERROR(LX("invalidateCapabilitiesFailed").d("reason", "unable to clear database"));
    }
}

bool CapabilitiesDelegate::addOrUpdateEndpoint(
    const AVSDiscoveryEndpointAttributes& endpointAttributes,
    const std::vector<CapabilityConfiguration>& capabilities) {
    ACSDK_DEBUG5(LX(__func__));

    if (!validateEndpointAttributes(endpointAttributes)) {
        ACSDK_ERROR(LX("addOrUpdateEndpointFailed").d("reason", "invalidAVSDiscoveryEndpointAttributes"));
        return false;
    }

    if (capabilities.empty()) {
        ACSDK_ERROR(LX("addOrUpdateEndpointFailed").d("reason", "invalidCapabilities"));
        return false;
    }

    for (auto& capability : capabilities) {
        if (!validateCapabilityConfiguration(capability)) {
            ACSDK_ERROR(LX("addOrUpdateEndpointFailed")
                            .d("reason", "invalidCapabilityConfiguration")
                            .d("capability", capability.interfaceName));
            return false;
        }
    }

    std::string endpointId = endpointAttributes.endpointId;
    {
        std::lock_guard<std::mutex> lock{m_endpointsMutex};

        // Check if this endpoint ID is already pending deletion.
        auto it = m_deleteEndpoints.pending.find(endpointId);
        if (m_deleteEndpoints.pending.end() != it) {
            ACSDK_ERROR(LX("addOrUpdateEndpointFailed")
                            .d("reason", "already pending deletion")
                            .sensitive("endpointId", endpointId));
            return false;
        }

        // Add endpoint to pending list.
        it = m_addOrUpdateEndpoints.pending.find(endpointId);
        if (m_addOrUpdateEndpoints.pending.end() != it) {
            ACSDK_ERROR(LX("addOrUpdateEndpointFailed")
                            .d("reason", "already pending addOrUpdate")
                            .sensitive("endpointId", endpointId));
            return false;
        }

        std::string endpointConfigJson = getEndpointConfigJson(endpointAttributes, capabilities);
        m_addOrUpdateEndpoints.pending.insert(std::make_pair(endpointId, endpointConfigJson));
    }

    if (!m_currentDiscoveryEventSender) {
        m_executor.submit([this] { executeSendPendingEndpoints(); });
    }

    return true;
}

bool CapabilitiesDelegate::deleteEndpoint(
    const avsCommon::avs::AVSDiscoveryEndpointAttributes& endpointAttributes,
    const std::vector<avsCommon::avs::CapabilityConfiguration>& capabilities) {
    ACSDK_DEBUG5(LX(__func__));

    if (!validateEndpointAttributes(endpointAttributes)) {
        ACSDK_ERROR(LX("deleteEndpointFailed").d("reason", "invalidAVSDiscoveryEndpointAttributes"));
        return false;
    }

    if (capabilities.empty()) {
        ACSDK_ERROR(LX("deleteEndpointFailed").d("reason", "invalidCapabilities"));
        return false;
    }

    for (auto& capability : capabilities) {
        if (!validateCapabilityConfiguration(capability)) {
            ACSDK_ERROR(LX("deleteEndpointFailed")
                            .d("reason", "invalidCapabilityConfiguration")
                            .d("capability", capability.interfaceName));
            return false;
        }
    }

    std::string endpointId = endpointAttributes.endpointId;

    {
        std::lock_guard<std::mutex> lock{m_endpointsMutex};

        // Check for duplicate in addOrUpdateEndpoints and report error if found.
        auto it = m_addOrUpdateEndpoints.pending.find(endpointId);
        if (m_addOrUpdateEndpoints.pending.end() != it) {
            ACSDK_ERROR(LX(__func__)
                            .d("deleteEndpointFailed", "already pending registration")
                            .sensitive("endpointId", endpointId));
            return false;
        }

        // If the endpoint to delete is not registered, return false and do not send Discovery event.
        it = m_endpoints.find(endpointId);
        if (m_endpoints.end() == it) {
            ACSDK_ERROR(
                LX("deleteEndpointFailed").d("reason", "endpoint not registered").sensitive("endpointId", endpointId));
            return false;
        }

        // Add endpoint to pending list.
        it = m_deleteEndpoints.pending.find(endpointId);
        if (m_deleteEndpoints.pending.end() != it) {
            ACSDK_ERROR(
                LX("deleteEndpointFailed").d("reason", "already pending deletion").sensitive("endpointId", endpointId));
            return false;
        }

        std::string endpointConfigJson = getEndpointConfigJson(endpointAttributes, capabilities);
        m_deleteEndpoints.pending.insert(std::make_pair(endpointId, endpointConfigJson));
    }

    if (!m_currentDiscoveryEventSender) {
        m_executor.submit([this] { executeSendPendingEndpoints(); });
    }

    return true;
}

void CapabilitiesDelegate::executeSendPendingEndpoints() {
    ACSDK_DEBUG5(LX(__func__));

    if (isShuttingDown()) {
        ACSDK_DEBUG5(LX(__func__).d("Skipped", "Shutting down"));
        return;
    }

    if (!m_isConnected) {
        ACSDK_DEBUG5(LX(__func__).d("Deferred", "Not connected"));
        return;
    }

    if (m_currentDiscoveryEventSender) {
        ACSDK_DEBUG5(LX(__func__).d("Deferred", "Discovery events already in-flight"));
        return;
    }

    if (m_addOrUpdateEndpoints.pending.empty() && m_deleteEndpoints.pending.empty()) {
        ACSDK_DEBUG5(LX(__func__).d("Skipped", "No endpoints to register or delete"));
        return;
    }

    std::unordered_map<std::string, std::string> addOrUpdateEndpointsToSend;
    std::unordered_map<std::string, std::string> deleteEndpointsToSend;
    {
        std::lock_guard<std::mutex> lock{m_endpointsMutex};

        // Move pending endpoints to in-flight, since we are going to send them.
        m_addOrUpdateEndpoints.inFlight = m_addOrUpdateEndpoints.pending;
        addOrUpdateEndpointsToSend = m_addOrUpdateEndpoints.inFlight;
        m_addOrUpdateEndpoints.pending.clear();

        m_deleteEndpoints.inFlight = m_deleteEndpoints.pending;
        deleteEndpointsToSend = m_deleteEndpoints.inFlight;
        m_deleteEndpoints.pending.clear();
    }

    ACSDK_DEBUG5(LX(__func__)
                     .d("num endpoints to add", addOrUpdateEndpointsToSend.size())
                     .d("num endpoints to delete", deleteEndpointsToSend.size()));

    auto discoveryEventSender =
        DiscoveryEventSender::create(addOrUpdateEndpointsToSend, deleteEndpointsToSend, m_authDelegate);

    if (!discoveryEventSender) {
        ACSDK_ERROR(LX("failedExecuteSendPendingEndpoints").d("reason", "failed to create DiscoveryEventSender"));
        moveInFlightEndpointsToPending();
        setCapabilitiesState(
            CapabilitiesObserverInterface::State::FATAL_ERROR,
            CapabilitiesObserverInterface::Error::UNKNOWN_ERROR,
            getEndpointIdentifiers(addOrUpdateEndpointsToSend),
            getEndpointIdentifiers(deleteEndpointsToSend));
        return;
    }

    addDiscoveryEventSender(discoveryEventSender);
    discoveryEventSender->sendDiscoveryEvents(m_messageSender);
}

void CapabilitiesDelegate::onAlexaEventProcessedReceived(const std::string& eventCorrelationToken) {
    ACSDK_DEBUG5(LX(__func__));

    std::shared_ptr<DiscoveryEventSenderInterface> currentEventSender;
    {
        std::lock_guard<std::mutex> lock{m_currentDiscoveryEventSenderMutex};
        currentEventSender = m_currentDiscoveryEventSender;
    }

    if (currentEventSender) {
        currentEventSender->onAlexaEventProcessedReceived(eventCorrelationToken);
    } else {
        ACSDK_ERROR(LX(__func__).m("invalidDiscoveryEventSender"));
    }
}

std::shared_ptr<PostConnectOperationInterface> CapabilitiesDelegate::createPostConnectOperation() {
    ACSDK_DEBUG5(LX(__func__));

    resetDiscoveryEventSender();

    /// We track the original pending endpoints in order to notify observers of their registration,
    /// even if CapabilitiesDelegate does not need to actually send the events since they are already
    /// registered with AVS.
    std::unordered_map<std::string, std::string> originalPendingAddOrUpdateEndpoints;

    /// The endpoints that need to be sent to AVS.
    std::unordered_map<std::string, std::string> addOrUpdateEndpointsToSend;
    std::unordered_map<std::string, std::string> deleteEndpointsToSend;
    {
        std::lock_guard<std::mutex> lock{m_endpointsMutex};

        /// If any endpoints were in-flight at the time of creating a post-connect operation, move them
        /// back to pending to be re-filtered and sent as part of the post-connect operation.
        moveInFlightEndpointsToPendingLocked();

        originalPendingAddOrUpdateEndpoints = m_addOrUpdateEndpoints.pending;

        std::unordered_map<std::string, std::string> storedEndpointConfig;
        if (!m_capabilitiesDelegateStorage->load(&storedEndpointConfig)) {
            ACSDK_ERROR(LX("createPostConnectOperationFailed").m("Could not load previous config from database."));
            return nullptr;
        }
        ACSDK_DEBUG5(LX(__func__).d("num endpoints stored", storedEndpointConfig.size()));

        /// If the database is empty, send any cached endpoints as part of this post-connect operation.
        if (storedEndpointConfig.empty()) {
            for (auto& endpoint : m_endpoints) {
                auto endpointId = endpoint.first;

                /// If the stored endpoint is pending deletion, do not include it in the addOrUpdateReport.
                auto it = m_deleteEndpoints.pending.find(endpointId);
                if (m_deleteEndpoints.pending.end() != it) {
                    continue;
                }

                /// If the registered endpoint does not have a pending change, add the stored configuration
                /// to the addOrUpdateReport. Otherwise, prefer the pending change as it's more recent.
                it = m_addOrUpdateEndpoints.pending.find(endpointId);
                if (m_addOrUpdateEndpoints.pending.end() == it) {
                    m_addOrUpdateEndpoints.pending[endpointId] = endpoint.second;
                }
            }
        } else {
            filterUnchangedPendingAddOrUpdateEndpointsLocked(&storedEndpointConfig);
            addStaleEndpointsToPendingDeleteLocked(&storedEndpointConfig);
        }

        /// Move endpoints from pending to in-flight, since they will now be sent.
        m_addOrUpdateEndpoints.inFlight = m_addOrUpdateEndpoints.pending;
        addOrUpdateEndpointsToSend = m_addOrUpdateEndpoints.inFlight;
        m_addOrUpdateEndpoints.pending.clear();

        m_deleteEndpoints.inFlight = m_deleteEndpoints.pending;
        deleteEndpointsToSend = m_deleteEndpoints.inFlight;
        m_deleteEndpoints.pending.clear();
    }

    /// Sometimes pending add/update endpoints do not need to be sent to AVS as they are already stored
    /// and registered with AVS. In this case, we should still notify observers that registration succeeded
    /// even though no Discovery event was sent. This logic is not required for pending delete endpoints, since
    /// calls to CapabilitiesDelegate.deleteEndpoint fail when the endpoint is not already cached with
    /// CapabilitiesDelegate.
    if (addOrUpdateEndpointsToSend.empty() && !originalPendingAddOrUpdateEndpoints.empty()) {
        setCapabilitiesState(
            CapabilitiesObserverInterface::State::SUCCESS,
            CapabilitiesObserverInterface::Error::SUCCESS,
            getEndpointIdentifiers(originalPendingAddOrUpdateEndpoints),
            std::vector<std::string>{});
    }

    if (addOrUpdateEndpointsToSend.empty() && deleteEndpointsToSend.empty()) {
        ACSDK_DEBUG5(LX(__func__).m("No change in Capabilities, skipping post connect step"));

        return nullptr;
    }

    ACSDK_DEBUG5(LX(__func__)
                     .d("num endpoints to add", addOrUpdateEndpointsToSend.size())
                     .d("num endpoints to delete", deleteEndpointsToSend.size()));

    std::shared_ptr<DiscoveryEventSenderInterface> currentEventSender =
        DiscoveryEventSender::create(addOrUpdateEndpointsToSend, deleteEndpointsToSend, m_authDelegate);

    if (currentEventSender) {
        addDiscoveryEventSender(currentEventSender);
    } else {
        ACSDK_ERROR(LX("createPostConnectOperationFailed").m("Could not create DiscoveryEventSender."));
        return nullptr;
    }

    auto instance = PostConnectCapabilitiesPublisher::create(currentEventSender);
    if (!instance) {
        resetDiscoveryEventSender();
    }
    return instance;
}

void CapabilitiesDelegate::onDiscoveryCompleted(
    const std::unordered_map<std::string, std::string>& addOrUpdateReportEndpoints,
    const std::unordered_map<std::string, std::string>& deleteReportEndpoints) {
    ACSDK_DEBUG5(LX(__func__));

    if (m_addOrUpdateEndpoints.inFlight != addOrUpdateReportEndpoints ||
        m_deleteEndpoints.inFlight != deleteReportEndpoints) {
        ACSDK_WARN(LX(__func__).m("Cached in-flight endpoints do not match endpoints registered to AVS"));
    }

    auto addOrUpdateReportEndpointIdentifiers = getEndpointIdentifiers(addOrUpdateReportEndpoints);
    auto deleteReportEndpointIdentifiers = getEndpointIdentifiers(deleteReportEndpoints);

    if (!updateEndpointConfigInStorage(addOrUpdateReportEndpoints, deleteReportEndpoints)) {
        ACSDK_ERROR(LX("publishCapabilitiesFailed").d("reason", "failed to save endpointConfig to database"));
        setCapabilitiesState(
            CapabilitiesObserverInterface::State::FATAL_ERROR,
            CapabilitiesObserverInterface::Error::UNKNOWN_ERROR,
            addOrUpdateReportEndpointIdentifiers,
            deleteReportEndpointIdentifiers);
        return;
    }

    moveInFlightEndpointsToRegisteredEndpoints();
    resetDiscoveryEventSender();

    setCapabilitiesState(
        CapabilitiesObserverInterface::State::SUCCESS,
        CapabilitiesObserverInterface::Error::SUCCESS,
        addOrUpdateReportEndpointIdentifiers,
        deleteReportEndpointIdentifiers);

    m_executor.submit([this] { executeSendPendingEndpoints(); });
}

void CapabilitiesDelegate::onDiscoveryFailure(MessageRequestObserverInterface::Status status) {
    ACSDK_DEBUG5(LX(__func__));

    if (status == MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalidStatus").d("status", status));
        return;
    }
    ACSDK_ERROR(LX(__func__).d("reason", status));

    auto addOrUpdateReportEndpointIdentifiers = getEndpointIdentifiers(m_addOrUpdateEndpoints.inFlight);
    auto deleteReportEndpointIdentifiers = getEndpointIdentifiers(m_deleteEndpoints.inFlight);

    switch (status) {
        case MessageRequestObserverInterface::Status::INVALID_AUTH:
            m_addOrUpdateEndpoints.inFlight.clear();
            m_deleteEndpoints.inFlight.clear();
            resetDiscoveryEventSender();

            setCapabilitiesState(
                CapabilitiesObserverInterface::State::FATAL_ERROR,
                CapabilitiesObserverInterface::Error::FORBIDDEN,
                addOrUpdateReportEndpointIdentifiers,
                deleteReportEndpointIdentifiers);
            break;
        case MessageRequestObserverInterface::Status::BAD_REQUEST:
            m_addOrUpdateEndpoints.inFlight.clear();
            m_deleteEndpoints.inFlight.clear();
            resetDiscoveryEventSender();

            setCapabilitiesState(
                CapabilitiesObserverInterface::State::FATAL_ERROR,
                CapabilitiesObserverInterface::Error::BAD_REQUEST,
                addOrUpdateReportEndpointIdentifiers,
                deleteReportEndpointIdentifiers);
            break;
        case MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2:
            if (isShuttingDown()) {
                resetDiscoveryEventSender();
            }

            setCapabilitiesState(
                CapabilitiesObserverInterface::State::RETRIABLE_ERROR,
                CapabilitiesObserverInterface::Error::SERVER_INTERNAL_ERROR,
                addOrUpdateReportEndpointIdentifiers,
                deleteReportEndpointIdentifiers);
            break;
        default:
            if (isShuttingDown()) {
                resetDiscoveryEventSender();
            }

            setCapabilitiesState(
                CapabilitiesObserverInterface::State::RETRIABLE_ERROR,
                CapabilitiesObserverInterface::Error::UNKNOWN_ERROR,
                addOrUpdateReportEndpointIdentifiers,
                deleteReportEndpointIdentifiers);
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

    std::lock_guard<std::mutex> lock{m_currentDiscoveryEventSenderMutex};
    discoveryEventSender->addDiscoveryStatusObserver(shared_from_this());
    m_currentDiscoveryEventSender = discoveryEventSender;
}

void CapabilitiesDelegate::resetDiscoveryEventSender() {
    ACSDK_DEBUG5(LX(__func__));
    std::shared_ptr<DiscoveryEventSenderInterface> currentDiscoveryEventSender;
    {
        std::lock_guard<std::mutex> lock{m_currentDiscoveryEventSenderMutex};
        currentDiscoveryEventSender = m_currentDiscoveryEventSender;
        m_currentDiscoveryEventSender.reset();
    }

    if (currentDiscoveryEventSender) {
        currentDiscoveryEventSender->removeDiscoveryStatusObserver(shared_from_this());
        currentDiscoveryEventSender->stop();
    }
}

void CapabilitiesDelegate::onAVSGatewayChanged(const std::string& avsGateway) {
    ACSDK_DEBUG5(LX(__func__));
    invalidateCapabilities();
}

void CapabilitiesDelegate::onConnectionStatusChanged(
    const ConnectionStatusObserverInterface::Status status,
    const ConnectionStatusObserverInterface::ChangedReason reason) {
    ACSDK_DEBUG5(LX(__func__).d("connectionStatus", status));

    {
        std::lock_guard<std::mutex> lock(m_isConnectedMutex);
        m_isConnected = (ConnectionStatusObserverInterface::Status::CONNECTED == status);
    }

    if (ConnectionStatusObserverInterface::Status::CONNECTED == status) {
        /// If newly connected, send Discovery events for any endpoints that may have been added or deleted
        /// during the post-connect stage.
        m_executor.submit([this] { executeSendPendingEndpoints(); });
    }
}

void CapabilitiesDelegate::moveInFlightEndpointsToRegisteredEndpoints() {
    std::lock_guard<std::mutex> lock(m_endpointsMutex);
    for (const auto& inFlight : m_addOrUpdateEndpoints.inFlight) {
        m_endpoints[inFlight.first] = inFlight.second;
    }
    for (const auto& inFlight : m_deleteEndpoints.inFlight) {
        m_endpoints.erase(inFlight.first);
    }
    m_addOrUpdateEndpoints.inFlight.clear();
    m_deleteEndpoints.inFlight.clear();
}

void CapabilitiesDelegate::addStaleEndpointsToPendingDeleteLocked(
    std::unordered_map<std::string, std::string>* storedEndpointConfig) {
    ACSDK_DEBUG5(LX(__func__));

    if (!storedEndpointConfig) {
        ACSDK_ERROR(LX("findEndpointsToDeleteLockedFailed").d("reason", "invalidStoredEndpointConfig"));
        return;
    }

    for (auto& it : *storedEndpointConfig) {
        if (m_endpoints.end() == m_endpoints.find(it.first) &&
            m_addOrUpdateEndpoints.pending.end() == m_addOrUpdateEndpoints.pending.find(it.first)) {
            ACSDK_DEBUG9(LX(__func__).d("step", "endpoint included in deleteReport").sensitive("endpointId", it.first));
            m_deleteEndpoints.pending.insert({it.first, getDeleteReportEndpointConfigJson(it.first)});
        }
    }
}

void CapabilitiesDelegate::filterUnchangedPendingAddOrUpdateEndpointsLocked(
    std::unordered_map<std::string, std::string>* storedEndpointConfig) {
    ACSDK_DEBUG5(LX(__func__));

    if (!storedEndpointConfig) {
        ACSDK_ERROR(
            LX("filterUnchangedPendingAddOrUpdateEndpointsLockedFailed").d("reason", "invalidStoredEndpointConfig"));
        return;
    }

    std::unordered_map<std::string, std::string> addOrUpdateEndpointIdToConfigPairs = m_addOrUpdateEndpoints.pending;

    /// Find the endpoints that are unchanged
    for (auto& endpointIdToConfigPair : addOrUpdateEndpointIdToConfigPairs) {
        auto storedEndpointConfigId = storedEndpointConfig->find(endpointIdToConfigPair.first);
        if (storedEndpointConfig->end() != storedEndpointConfigId) {
            if (endpointIdToConfigPair.second == storedEndpointConfigId->second) {
                ACSDK_DEBUG9(LX(__func__)
                                 .d("step", "endpoint not be included in addOrUpdateReport")
                                 .sensitive("endpointId", endpointIdToConfigPair.first));

                /// Endpoint has not changed, so it does not need to be updated.
                /// Remove it from pending list of endpoints to register.
                m_addOrUpdateEndpoints.pending.erase(endpointIdToConfigPair.first);

                /// Add endpoint to cached registered endpoints, in case this is the first time the stored endpoints
                /// have been checked (ie. this is the first post-connect operation since starting the client).
                m_endpoints[endpointIdToConfigPair.first] = endpointIdToConfigPair.second;

                /// Remove this endpoint from the stored endpoint list.
                storedEndpointConfig->erase(endpointIdToConfigPair.first);
            } else {
                ACSDK_DEBUG9(LX(__func__)
                                 .d("step", "endpoint included in addOrUpdateReport")
                                 .d("reason", "configuration changed")
                                 .sensitive("endpointId", endpointIdToConfigPair.first));
            }
        } else {
            ACSDK_DEBUG9(LX(__func__)
                             .d("step", "endpoint included in addOrUpdateReport")
                             .d("reason", "new")
                             .sensitive("endpointId", endpointIdToConfigPair.first));
        }
    }
}

void CapabilitiesDelegate::moveInFlightEndpointsToPendingLocked() {
    ACSDK_DEBUG5(LX(__func__));

    /// Move in-flight addOrUpdateEndpoints to pending.
    for (auto& inFlightEndpointIdToConfigPair : m_addOrUpdateEndpoints.inFlight) {
        if (m_addOrUpdateEndpoints.pending.end() !=
            m_addOrUpdateEndpoints.pending.find(inFlightEndpointIdToConfigPair.first)) {
            ACSDK_ERROR(LX(__func__)
                            .d("moveInFlightEndpointToPendingFailed", "Unexpected duplicate endpointId in pending")
                            .sensitive("endpointId", inFlightEndpointIdToConfigPair.first));
        } else {
            ACSDK_DEBUG9(LX(__func__)
                             .d("step", "willRetryInFlightAddOrUpdateEndpoint")
                             .sensitive("endpointId", inFlightEndpointIdToConfigPair.first)
                             .sensitive("configuration", inFlightEndpointIdToConfigPair.second));
            m_addOrUpdateEndpoints.pending.insert(inFlightEndpointIdToConfigPair);
        }
    }

    /// Move in-flight deleteEndpoints to pending.
    for (auto& inFlightEndpointIdToConfigPair : m_deleteEndpoints.inFlight) {
        auto inFlightEndpointIdToConfigPairId = m_deleteEndpoints.pending.find(inFlightEndpointIdToConfigPair.first);
        if (m_deleteEndpoints.pending.end() != inFlightEndpointIdToConfigPairId) {
            ACSDK_ERROR(LX(__func__)
                            .d("moveInFlightEndpointToPendingFailed", "Unexpected duplicate endpointId in pending")
                            .sensitive("endpointId", inFlightEndpointIdToConfigPair.first));
        } else {
            ACSDK_DEBUG9(LX(__func__)
                             .d("step", "willRetryDeletingInFlightEndpoint")
                             .sensitive("endpointId", inFlightEndpointIdToConfigPair.first));
            m_deleteEndpoints.pending.insert(inFlightEndpointIdToConfigPair);
        }
    }

    m_addOrUpdateEndpoints.inFlight.clear();
    m_deleteEndpoints.inFlight.clear();
}

void CapabilitiesDelegate::moveInFlightEndpointsToPending() {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock(m_endpointsMutex);
    moveInFlightEndpointsToPendingLocked();
}

bool CapabilitiesDelegate::isShuttingDown() {
    std::lock_guard<std::mutex> lock(m_isShuttingDownMutex);
    return m_isShuttingDown;
}

}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK
