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

#include "CertifiedSender/CertifiedSender.h"

#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <RegistrationManager/CustomerDataManager.h>

#include <queue>

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

/*
 * Retry times on subsequent retries for when a message could not be sent to the server over a valid AVS connection.
 * These numbers are based on the formula: 10 * 5^n, where n is the number of retries.
 */
static const std::vector<int> EXPONENTIAL_BACKOFF_RETRY_TABLE = {
    10000,   // Retry 1:  10s
    50000,   // Retry 2:  50s
    250000,  // Retry 3:  250s = 4min 10s
    1250000  // Retry 4:  1250s = 20min 50s
};

CertifiedSender::CertifiedMessageRequest::CertifiedMessageRequest(
    const std::string& jsonContent,
    int dbId,
    const std::string& uriPathExtension) :
        MessageRequest{jsonContent, uriPathExtension},
        m_responseReceived{false},
        m_dbId{dbId},
        m_isRequestShuttingDown{false} {
}

void CertifiedSender::CertifiedMessageRequest::exceptionReceived(const std::string& exceptionMessage) {
    std::lock_guard<std::mutex> lock(m_requestMutex);
    m_sendMessageStatus = MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2;
    m_responseReceived = true;
    m_requestCv.notify_all();
}

void CertifiedSender::CertifiedMessageRequest::sendCompleted(
    MessageRequestObserverInterface::Status sendMessageStatus) {
    std::lock_guard<std::mutex> lock(m_requestMutex);
    if (!m_responseReceived) {
        m_sendMessageStatus = sendMessageStatus;
        m_responseReceived = true;
        m_requestCv.notify_all();
    }
}

MessageRequestObserverInterface::Status CertifiedSender::CertifiedMessageRequest::waitForCompletion() {
    std::unique_lock<std::mutex> lock(m_requestMutex);
    m_requestCv.wait(lock, [this]() { return m_isRequestShuttingDown || m_responseReceived; });

    if (m_isRequestShuttingDown) {
        return MessageRequestObserverInterface::Status::TIMEDOUT;
    }

    return m_sendMessageStatus;
}

int CertifiedSender::CertifiedMessageRequest::getDbId() {
    return m_dbId;
}

void CertifiedSender::CertifiedMessageRequest::shutdown() {
    std::lock_guard<std::mutex> lock(m_requestMutex);
    m_isRequestShuttingDown = true;
    m_requestCv.notify_all();
}

std::shared_ptr<CertifiedSender> CertifiedSender::create(
    std::shared_ptr<MessageSenderInterface> messageSender,
    std::shared_ptr<AVSConnectionManagerInterface> connection,
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
    std::shared_ptr<AVSConnectionManagerInterface> connection,
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
        m_retryTimer(EXPONENTIAL_BACKOFF_RETRY_TABLE),
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
    m_backoffWaitCV.notify_one();

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

    /// Load stored messages from storage.
    std::queue<MessageStorageInterface::StoredMessage> storedMessages;
    if (!m_storage->load(&storedMessages)) {
        ACSDK_ERROR(LX("initFailed").m("Could not load messages from database file."));
        return false;
    }

    while (!storedMessages.empty()) {
        auto storedMessage = storedMessages.front();
        {
            std::lock_guard<std::mutex> lock{m_mutex};
            m_messagesToSend.push_back(std::make_shared<CertifiedMessageRequest>(
                storedMessage.message, storedMessage.id, storedMessage.uriPathExtension));
        }
        storedMessages.pop();
    }

    m_workerThread = std::thread(&CertifiedSender::mainloop, this);

    return true;
}

/**
 * A function to evaluate if the given status suggests that the client should retry sending the message.
 *
 * @param status The status being queried.
 * @return Whether the status means the client should keep retrying.
 */
inline bool shouldRetryTransmission(MessageRequestObserverInterface::Status status) {
    switch (status) {
        case MessageRequestObserverInterface::Status::SUCCESS:
        case MessageRequestObserverInterface::Status::SUCCESS_ACCEPTED:
        case MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT:
        case MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2:
        case MessageRequestObserverInterface::Status::CANCELED:
        case MessageRequestObserverInterface::Status::SERVER_OTHER_ERROR:
        case MessageRequestObserverInterface::Status::BAD_REQUEST:
            return false;
        case MessageRequestObserverInterface::Status::THROTTLED:
        case MessageRequestObserverInterface::Status::PENDING:
        case MessageRequestObserverInterface::Status::NOT_CONNECTED:
        case MessageRequestObserverInterface::Status::NOT_SYNCHRONIZED:
        case MessageRequestObserverInterface::Status::TIMEDOUT:
        case MessageRequestObserverInterface::Status::PROTOCOL_ERROR:
        case MessageRequestObserverInterface::Status::INTERNAL_ERROR:
        case MessageRequestObserverInterface::Status::REFUSED:
        case MessageRequestObserverInterface::Status::INVALID_AUTH:
            return true;
    }
    return false;
}

void CertifiedSender::mainloop() {
    int failedSendRetryCount = 0;
    while (true) {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_currentMessage.reset();

        if ((!m_isConnected || m_messagesToSend.empty()) && !m_isShuttingDown) {
            m_workerThreadCV.wait(
                lock, [this]() { return ((m_isConnected && !m_messagesToSend.empty()) || m_isShuttingDown); });
        }

        if (m_isShuttingDown) {
            ACSDK_DEBUG9(LX("CertifiedSender worker thread done.  Exiting mainloop."));
            return;
        }

        m_currentMessage = m_messagesToSend.front();

        lock.unlock();

        // We have a message to send - send it!
        m_messageSender->sendMessage(m_currentMessage);

        auto status = m_currentMessage->waitForCompletion();

        if (shouldRetryTransmission(status)) {
            // If we couldn't send the message ok, let's push a fresh instance to the front of the deque.  This allows
            // ACL to continue interacting with the old instance (for example, if it is involved in a complex flow
            // of exception / onCompleted handling), and allows us to safely try sending the new instance.
            lock.lock();
            m_messagesToSend.pop_front();
            m_messagesToSend.push_front(std::make_shared<CertifiedMessageRequest>(
                m_currentMessage->getJsonContent(),
                m_currentMessage->getDbId(),
                m_currentMessage->getUriPathExtension()));
            lock.unlock();

            // Ensures that we do not DDOS the AVS endpoint, just in case we have a valid AVS connection but
            // the server is returning some non-server HTTP error.
            auto timeout = m_retryTimer.calculateTimeToRetry(failedSendRetryCount);
            ACSDK_DEBUG5(LX(__func__).d("failedSendRetryCount", failedSendRetryCount).d("timeout", timeout.count()));

            failedSendRetryCount++;

            lock.lock();
            m_backoffWaitCV.wait_for(lock, timeout, [this]() { return m_isShuttingDown; });
            if (m_isShuttingDown) {
                ACSDK_DEBUG9(LX("CertifiedSender worker thread done.  Exiting mainloop."));
                return;
            }
            lock.unlock();
        } else {
            /*
             * We should not retry to send the message (either because it was sent successfully or because trying again
             * is not expected to solve the issue)
             */
            lock.lock();

            if (!m_storage->erase(m_currentMessage->getDbId())) {
                ACSDK_ERROR(LX("mainloop : could not erase message from storage."));
            }

            m_messagesToSend.pop_front();
            lock.unlock();
            // Resetting the fail count.
            failedSendRetryCount = 0;
        }
    }
}

void CertifiedSender::onConnectionStatusChanged(
    ConnectionStatusObserverInterface::Status status,
    ConnectionStatusObserverInterface::ChangedReason reason) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_isConnected = (ConnectionStatusObserverInterface::Status::CONNECTED == status);
    lock.unlock();
    m_workerThreadCV.notify_one();
}

std::future<bool> CertifiedSender::sendJSONMessage(
    const std::string& jsonMessage,
    const std::string& uriPathExtension) {
    return m_executor.submit(
        [this, jsonMessage, uriPathExtension]() { return executeSendJSONMessage(jsonMessage, uriPathExtension); });
}

bool CertifiedSender::executeSendJSONMessage(std::string jsonMessage, const std::string& uriPathExtension) {
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
    if (!m_storage->store(jsonMessage, uriPathExtension, &messageId)) {
        ACSDK_ERROR(LX("executeSendJSONMessage").m("Could not store message."));
        return false;
    }

    m_messagesToSend.push_back(std::make_shared<CertifiedMessageRequest>(jsonMessage, messageId, uriPathExtension));

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
