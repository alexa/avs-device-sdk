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

#include "CertifiedSender/CertifiedSender.h"

#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <RegistrationManager/CustomerDataManager.h>

namespace alexaClientSDK {
namespace certifiedSender {

using namespace avsCommon::utils::logger;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::avs;
using namespace avsCommon::utils::configuration;

/// String to identify log entries originating from this file.
static const std::string TAG("CertifiedSender");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

CertifiedSender::CertifiedMessageRequest::CertifiedMessageRequest(const std::string& jsonContent, int dbId) :
        MessageRequest{jsonContent},
        m_responseReceived{false},
        m_dbId{dbId},
        m_isShuttingDown{false} {
}

void CertifiedSender::CertifiedMessageRequest::exceptionReceived(const std::string& exceptionMessage) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sendMessageStatus = MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2;
    m_responseReceived = true;
    m_cv.notify_all();
}

void CertifiedSender::CertifiedMessageRequest::sendCompleted(
    MessageRequestObserverInterface::Status sendMessageStatus) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_responseReceived) {
        m_sendMessageStatus = sendMessageStatus;
        m_responseReceived = true;
        m_cv.notify_all();
    }
}

MessageRequestObserverInterface::Status CertifiedSender::CertifiedMessageRequest::waitForCompletion() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait(lock, [this]() { return m_isShuttingDown || m_responseReceived; });

    if (m_isShuttingDown) {
        return MessageRequestObserverInterface::Status::TIMEDOUT;
    }

    return m_sendMessageStatus;
}

int CertifiedSender::CertifiedMessageRequest::getDbId() {
    return m_dbId;
}

void CertifiedSender::CertifiedMessageRequest::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isShuttingDown = true;
    m_cv.notify_all();
}

std::shared_ptr<CertifiedSender> CertifiedSender::create(
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<AbstractConnection> connection,
    std::shared_ptr<MessageStorageInterface> storage,
    std::shared_ptr<registrationManager::CustomerDataManager> dataManager) {
    auto certifiedSender =
        std::shared_ptr<CertifiedSender>(new CertifiedSender(messageSender, connection, storage, dataManager));

    if (!certifiedSender->init()) {
        ACSDK_ERROR(LX("createFailed").m("Could not initialize certifiedSender."));
        return nullptr;
    }

    connection->addConnectionStatusObserver(certifiedSender);

    return certifiedSender;
}

CertifiedSender::CertifiedSender(
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<AbstractConnection> connection,
    std::shared_ptr<MessageStorageInterface> storage,
    std::shared_ptr<registrationManager::CustomerDataManager> dataManager,
    int queueSizeWarnLimit,
    int queueSizeHardLimit) :
        RequiresShutdown("CertifiedSender"),
        CustomerDataHandler(dataManager),
        m_queueSizeWarnLimit{queueSizeWarnLimit},
        m_queueSizeHardLimit{queueSizeHardLimit},
        m_isShuttingDown{false},
        m_isConnected{false},
        m_messageSender{messageSender},
        m_connection{connection},
        m_storage{storage} {
}

CertifiedSender::~CertifiedSender() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_isShuttingDown = true;
    if (m_currentMessage) {
        m_currentMessage->shutdown();
    }
    lock.unlock();

    m_workerThreadCV.notify_one();

    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

bool CertifiedSender::init() {
    if (m_queueSizeWarnLimit < 0 || m_queueSizeHardLimit <= 0 || m_queueSizeHardLimit < m_queueSizeWarnLimit) {
        ACSDK_ERROR(LX("initFailed")
                        .d("warnSizeLimit", m_queueSizeWarnLimit)
                        .d("hardSizeLimit", m_queueSizeHardLimit)
                        .m("Limit values are invalid."));
        return false;
    }

    if (!m_storage->open()) {
        ACSDK_INFO(LX("init : Database file does not exist.  Creating."));
        if (!m_storage->createDatabase()) {
            ACSDK_ERROR(LX("initFailed").m("Could not create database file."));
            return false;
        }
    }

    m_workerThread = std::thread(&CertifiedSender::mainloop, this);

    return true;
}

void CertifiedSender::mainloop() {
    while (true) {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_currentMessage.reset();

        if ((!m_isConnected || m_messagesToSend.empty()) && !m_isShuttingDown) {
            m_workerThreadCV.wait(
                lock, [this]() { return ((m_isConnected && !m_messagesToSend.empty()) || m_isShuttingDown); });
        }

        if (m_isShuttingDown) {
            ACSDK_DEBUG9(LX("CertifiedSender worker thread done.  exiting mainloop."));
            return;
        }

        m_currentMessage = m_messagesToSend.front();

        lock.unlock();

        // We have a message to send - send it!
        m_messageSender->sendMessage(m_currentMessage);

        auto status = m_currentMessage->waitForCompletion();

        if (MessageRequest::isServerStatus(status)) {
            lock.lock();

            if (!m_storage->erase(m_currentMessage->getDbId())) {
                ACSDK_ERROR(LX("mainloop : could not erase message from storage."));
            }

            m_messagesToSend.pop_front();
            lock.unlock();
        } else {
            // If we couldn't send the message ok, let's push a fresh instance to the front of the deque.  This allows
            // ACL to continue interacting with the old instance (for example, if it is involved in a complex flow
            // of exception / onCompleted handling), and allows us to safely try sending the new instance.
            lock.lock();
            m_messagesToSend.pop_front();
            m_messagesToSend.push_front(std::make_shared<CertifiedMessageRequest>(
                m_currentMessage->getJsonContent(), m_currentMessage->getDbId()));
            lock.unlock();
        }
    }
}

void CertifiedSender::onConnectionStatusChanged(
    ConnectionStatusObserverInterface::Status status,
    ConnectionStatusObserverInterface::ChangedReason reason) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isConnected = (ConnectionStatusObserverInterface::Status::CONNECTED == status);
    m_workerThreadCV.notify_one();
}

std::future<bool> CertifiedSender::sendJSONMessage(const std::string& jsonMessage) {
    return m_executor.submit([this, jsonMessage]() { return executeSendJSONMessage(jsonMessage); });
}

bool CertifiedSender::executeSendJSONMessage(std::string jsonMessage) {
    std::unique_lock<std::mutex> lock(m_mutex);

    int queueSize = static_cast<int>(m_messagesToSend.size());

    if (queueSize >= m_queueSizeHardLimit) {
        ACSDK_ERROR(LX("executeSendJSONMessage").m("Queue size is at max limit.  Cannot add message to send."));
        return false;
    }

    if (queueSize >= m_queueSizeWarnLimit) {
        ACSDK_WARN(LX("executeSendJSONMessage").m("Warning : queue size has exceeded the warn limit."));
    }

    int messageId = 0;
    if (!m_storage->store(jsonMessage, &messageId)) {
        ACSDK_ERROR(LX("executeSendJSONMessage").m("Could not store message."));
        return false;
    }

    m_messagesToSend.push_back(std::make_shared<CertifiedMessageRequest>(jsonMessage, messageId));

    lock.unlock();

    m_workerThreadCV.notify_one();

    return true;
}

void CertifiedSender::doShutdown() {
    m_connection->removeConnectionStatusObserver(shared_from_this());
}

void CertifiedSender::clearData() {
    auto result = m_executor.submit([this]() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_messagesToSend.clear();
        m_storage->clearDatabase();
    });
    result.wait();
}

}  // namespace certifiedSender
}  // namespace alexaClientSDK
