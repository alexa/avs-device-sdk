/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <string>
#include <rapidjson/document.h>

#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "System/SoftwareInfoSender.h"
#include "System/SoftwareInfoSendRequest.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

using namespace acl;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::softwareInfo;
using namespace avsCommon::utils::json;
using namespace rapidjson;

/// String to identify log entries originating from this file.
static const std::string TAG{"SoftwareInfoSender"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

static const std::string NAMESPACE_SYSTEM = "System";

/// Namespace, Name pair for System.ReportSoftwareInfo.
static const NamespaceAndName REPORT_SOFTWARE_INFO(NAMESPACE_SYSTEM, "ReportSoftwareInfo");

std::shared_ptr<SoftwareInfoSender> SoftwareInfoSender::create(
    FirmwareVersion firmwareVersion,
    bool sendSoftwareInfoUponConnect,
    std::shared_ptr<avsCommon::sdkInterfaces::SoftwareInfoSenderObserverInterface> observer,
    std::shared_ptr<AVSConnectionManagerInterface> connection,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionEncounteredSender) {
    ACSDK_DEBUG5(LX("create"));

    if (firmwareVersion <= 0) {
        ACSDK_ERROR(LX("createFailed").d("reason", "invalidFirmwareVersion").d("firmwareVersion", firmwareVersion));
        return nullptr;
    }

    if (!connection) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullConnection"));
        return nullptr;
    }

    if (!messageSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMessageSender"));
        return nullptr;
    }

    if (!exceptionEncounteredSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionEncounteredSender"));
        return nullptr;
    }

    auto result = std::shared_ptr<SoftwareInfoSender>(new SoftwareInfoSender(
        firmwareVersion, sendSoftwareInfoUponConnect, observer, connection, messageSender, exceptionEncounteredSender));

    connection->addConnectionStatusObserver(result);

    return result;
}

bool SoftwareInfoSender::setFirmwareVersion(FirmwareVersion firmwareVersion) {
    ACSDK_DEBUG5(LX("setFirmwareVersion").d("firmwareVersion", firmwareVersion));

    if (!isValidFirmwareVersion(firmwareVersion)) {
        ACSDK_ERROR(
            LX("setFirmwareVersion").d("reason", "invalidFirmwareVersion").d("firmwareVersion", firmwareVersion));
        return false;
    }

    // We don't want to trigger @c ~SoftwareInfoSenderRequest() while holding @c m_mutex, so we use
    // a variable outside the scope of the lock to hold our reference until @c m_mutex is released.
    std::shared_ptr<SoftwareInfoSendRequest> previousSendRequest;

    // We don't want to @c send() while holding @c m_mutex, so we use a variable outside of the
    // scope of the lock to remember if there is a new request.
    std::shared_ptr<SoftwareInfoSendRequest> newSendRequest;

    bool result = true;

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (firmwareVersion == m_firmwareVersion) {
            return result;
        }
        m_firmwareVersion = firmwareVersion;

        if (ConnectionStatusObserverInterface::Status::CONNECTED == m_connectionStatus) {
            newSendRequest = SoftwareInfoSendRequest::create(m_firmwareVersion, m_messageSender, shared_from_this());
            if (newSendRequest) {
                previousSendRequest = m_clientInitiatedSendRequest;
                m_clientInitiatedSendRequest = newSendRequest;
            } else {
                result = false;
            }
        } else {
            m_sendSoftwareInfoUponConnect = true;
            previousSendRequest = m_clientInitiatedSendRequest;
            m_clientInitiatedSendRequest.reset();
        }
    }

    if (previousSendRequest) {
        ACSDK_INFO(LX("cancellingPreviousClientInitiatedSendRequest"));
        previousSendRequest->shutdown();
    }

    if (newSendRequest) {
        newSendRequest->send();
    }

    return result;
}

avsCommon::avs::DirectiveHandlerConfiguration SoftwareInfoSender::getConfiguration() const {
    ACSDK_DEBUG5(LX("getConfiguration"));

    static avsCommon::avs::DirectiveHandlerConfiguration configuration = {
        {REPORT_SOFTWARE_INFO, BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false)}};
    return configuration;
}

void SoftwareInfoSender::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    handleDirective(std::make_shared<CapabilityAgent::DirectiveInfo>(directive, nullptr));
}

void SoftwareInfoSender::preHandleDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    // Nothing to do.
}

void SoftwareInfoSender::handleDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    if (!info) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullInfo"));
        return;
    }

    ACSDK_DEBUG5(LX("handleDirective").d("messageId", info->directive->getMessageId()));

    if (info->directive->getNamespace() != REPORT_SOFTWARE_INFO.nameSpace ||
        info->directive->getName() != REPORT_SOFTWARE_INFO.name) {
        sendExceptionEncounteredAndReportFailed(
            info, "Unsupported operation", ExceptionErrorType::UNSUPPORTED_OPERATION);
        return;
    }

    // We don't want to trigger @c ~SoftwareInfoSenderRequest() while holding @c m_mutex, so we use
    // a variable outside the scope of the lock to hold our reference until @c m_mutex is released.
    std::shared_ptr<SoftwareInfoSendRequest> previousSendRequest;

    // We don't want to @c send() while holding @c m_mutex, so we use a variable outside of the
    // scope of the lock to remember if there is a new request.
    std::shared_ptr<SoftwareInfoSendRequest> newSendRequest;

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_firmwareVersion == INVALID_FIRMWARE_VERSION) {
            sendExceptionEncounteredAndReportFailed(info, "NoFirmwareVersion", ExceptionErrorType::INTERNAL_ERROR);
            return;
        }

        newSendRequest = SoftwareInfoSendRequest::create(m_firmwareVersion, m_messageSender, shared_from_this());

        if (newSendRequest) {
            previousSendRequest = m_handleDirectiveSendRequest;
            m_handleDirectiveSendRequest = newSendRequest;
        } else {
            sendExceptionEncounteredAndReportFailed(
                info, "sendFirmwareVersionFailed", ExceptionErrorType::INTERNAL_ERROR);
        }
    }

    if (previousSendRequest) {
        ACSDK_INFO(LX("cancellingPreviousHandleDirectiveSendRequest"));
        previousSendRequest->shutdown();
    }

    if (newSendRequest) {
        newSendRequest->send();
    }

    if (info->result) {
        info->result->setCompleted();
    }

    removeDirective(info);
}

void SoftwareInfoSender::cancelDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    if (!info) {
        ACSDK_ERROR(LX("cancelDirectiveFailed").d("reason", "nullInfo"));
        return;
    }

    ACSDK_DEBUG5(LX("cancelDirective").d("messageId", info->directive->getMessageId()));

    std::shared_ptr<SoftwareInfoSendRequest> outstandingRequest;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_handleDirectiveSendRequest) {
            return;
        }
        outstandingRequest = m_handleDirectiveSendRequest;
        m_handleDirectiveSendRequest.reset();
    }

    if (outstandingRequest) {
        outstandingRequest->shutdown();
    }
}

void SoftwareInfoSender::onConnectionStatusChanged(
    ConnectionStatusObserverInterface::Status status,
    ConnectionStatusObserverInterface::ChangedReason reason) {
    ACSDK_DEBUG5(LX("onConnectionStatusChanged").d("status", status).d("reason", reason));

    // We don't want to @c send() while holding @c m_mutex, so we use a variable outside of the
    // scope of the lock to remember if there is a new request.
    std::shared_ptr<SoftwareInfoSendRequest> newSendRequest;

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (status == m_connectionStatus) {
            return;
        }

        m_connectionStatus = status;

        if (!m_sendSoftwareInfoUponConnect || status != ConnectionStatusObserverInterface::Status::CONNECTED) {
            return;
        }

        newSendRequest = SoftwareInfoSendRequest::create(m_firmwareVersion, m_messageSender, shared_from_this());
        m_sendSoftwareInfoUponConnect = false;
        if (newSendRequest) {
            m_clientInitiatedSendRequest = newSendRequest;
        } else {
            ACSDK_ERROR(LX("onConnectionStatusChangedFailed").d("reason", "failedToCreateOnConnectSendRequest"));
        }
    }

    if (newSendRequest) {
        newSendRequest->send();
    }
}

void SoftwareInfoSender::doShutdown() {
    ACSDK_DEBUG5(LX("shutdown"));

    // Local shared_ptrs used to prevent deletion of objects pointed to by members until m_mutex is released.

    std::shared_ptr<SoftwareInfoSenderObserverInterface> localObserver;
    std::shared_ptr<AVSConnectionManagerInterface> localConnection;
    std::shared_ptr<MessageSenderInterface> localMessageSender;
    std::shared_ptr<ExceptionEncounteredSenderInterface> localExceptionEncounteredSender;
    std::shared_ptr<SoftwareInfoSendRequest> localClientInitiatedSendRequest;
    std::shared_ptr<SoftwareInfoSendRequest> localHandleDirectiveSendRequest;

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_sendSoftwareInfoUponConnect = false;

        // Swap these shared_ptr values with nullptr.  This allows us to release
        // the reference from our member data while holding the lock, but do the
        // (potentially) final release of a shared_ptr to these instances while
        // NOT holding the lock (i.e. when these locals go out of scope).
        std::swap(m_observer, localObserver);
        std::swap(m_connection, localConnection);
        std::swap(m_messageSender, localMessageSender);
        std::swap(m_exceptionEncounteredSender, localExceptionEncounteredSender);
        std::swap(m_clientInitiatedSendRequest, localClientInitiatedSendRequest);
        std::swap(m_handleDirectiveSendRequest, localHandleDirectiveSendRequest);
    }

    localConnection->removeConnectionStatusObserver(shared_from_this());

    if (localClientInitiatedSendRequest) {
        localClientInitiatedSendRequest->shutdown();
    }

    if (localHandleDirectiveSendRequest) {
        localHandleDirectiveSendRequest->shutdown();
    }
}

void SoftwareInfoSender::onFirmwareVersionAccepted(FirmwareVersion firmwareVersion) {
    std::shared_ptr<SoftwareInfoSenderObserverInterface> localObserver;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::swap(m_observer, localObserver);
    }
    if (localObserver) {
        localObserver->onFirmwareVersionAccepted(firmwareVersion);
    }
}

SoftwareInfoSender::SoftwareInfoSender(
    FirmwareVersion firmwareVersion,
    bool sendSoftwareInfoUponConnect,
    std::shared_ptr<avsCommon::sdkInterfaces::SoftwareInfoSenderObserverInterface> observer,
    std::shared_ptr<AVSConnectionManagerInterface> connection,
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender) :
        CapabilityAgent{NAMESPACE_SYSTEM, exceptionEncounteredSender},
        RequiresShutdown{"SoftwareInfoSender"},
        m_firmwareVersion{firmwareVersion},
        m_sendSoftwareInfoUponConnect{sendSoftwareInfoUponConnect},
        m_observer{observer},
        m_connection{connection},
        m_messageSender{messageSender},
        m_exceptionEncounteredSender{exceptionEncounteredSender},
        m_connectionStatus{ConnectionStatusObserverInterface::Status::DISCONNECTED} {
    ACSDK_DEBUG5(LX("SoftwareInfoSender"));
}

void SoftwareInfoSender::removeDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX("removeDirective"));
    if (info && info->result) {
        CapabilityAgent::removeDirective(info->directive->getMessageId());
    }
}

}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
