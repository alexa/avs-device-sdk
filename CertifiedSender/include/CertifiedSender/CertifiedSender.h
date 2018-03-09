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

#ifndef ALEXA_CLIENT_SDK_CERTIFIEDSENDER_INCLUDE_CERTIFIEDSENDER_CERTIFIEDSENDER_H_
#define ALEXA_CLIENT_SDK_CERTIFIEDSENDER_INCLUDE_CERTIFIEDSENDER_CERTIFIEDSENDER_H_

#include "CertifiedSender/MessageStorageInterface.h"

#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/AVS/AbstractConnection.h>
#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <RegistrationManager/CustomerDataHandler.h>
#include <RegistrationManager/CustomerDataManager.h>

#include <deque>
#include <memory>

namespace alexaClientSDK {
namespace certifiedSender {

/// The number of items we can store for sending without emitting a warning.
static const int CERTIFIED_SENDER_QUEUE_SIZE_WARN_LIMIT = 25;
/// The maximum number of items we can store for sending.
static const int CERTIFIED_SENDER_QUEUE_SIZE_HARD_LIMIT = 50;

/**
 * This class provides a guaranteed message delivery service to AVS.  Upon calling the single api,
 * @c sendJSONMessage, this class will persist the message and continually attempt sending it until it succeeds.
 * The persistence will work across application runs, dependent on the nature of the storage object provided.
 *
 * To avoid excessive memory usage, the maximum number of messages stored in this way is configurable via the
 * settings 'queueSizeWarnLimit' and 'queueSizeHardLimit', under the configuration root 'certifiedSender'.
 *
 * Similarly, the file path for the database storage is configured under the setting 'databaseFilePath'.
 *
 * This class maintains the ordering of messages passed to it.  For example, if @c sendJSONMessage is invoked with
 * messages A then B then C, then this class guarantees that the messages will be sent to AVS in the same order -
 * A then B then C.
 */
class CertifiedSender
        : public avsCommon::utils::RequiresShutdown
        , public avsCommon::sdkInterfaces::ConnectionStatusObserverInterface
        , public std::enable_shared_from_this<CertifiedSender>
        , public registrationManager::CustomerDataHandler {
public:
    /**
     * This function creates a new instance of a @c CertifiedSender. If it fails for any reason, @c nullptr is returned.
     *
     * @param messageSender The entity which is able to send @c MessageRequests to AVS.
     * @param connection The connection which may be observed to determine connection status.
     * @param storage The object which manages persistent storage of messages to be sent.
     * @param dataManager A dataManager object that will track the CustomerDataHandler.
     * @return A @c CertifiedSender object, or @c nullptr if there is any problem.
     */
    static std::shared_ptr<CertifiedSender> create(
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::avs::AbstractConnection> connection,
        std::shared_ptr<MessageStorageInterface> storage,
        std::shared_ptr<registrationManager::CustomerDataManager> dataManager);

    /**
     * Destructor.
     */
    virtual ~CertifiedSender();

    /**
     * Function to request a message be sent to AVS.  Since this class is strictly responsible for sending a message
     * to AVS, the parameter is explicitly described as being in JSON format.  This will be expected to take the
     * form of some kind of Event which AVS understands.  While the sending of the message is entirely asynchronous,
     * the future returned allows the caller to know if the request was successfully persisted.  Once the message
     * is persisted, the caller can expect the message to be sent to AVS at some point in the future by this class.
     *
     * @param jsonMessage The message to be sent to AVS.
     * @return A future expressing if the message was successfully persisted.
     */
    std::future<bool> sendJSONMessage(const std::string& jsonMessage);

    /**
     * Clear all messages that we are currently storing
     */
    void clearData() override;

private:
    /**
     * A utility class to manage interaction with the MessageSender.
     */
    class CertifiedMessageRequest : public avsCommon::avs::MessageRequest {
    public:
        /**
         * Constructor.
         *
         * @param jsonContent The JSON text to be sent to AVS.
         * @param dbId The database id associated with this @c MessageRequest.
         */
        CertifiedMessageRequest(const std::string& jsonContent, int dbId);

        void exceptionReceived(const std::string& exceptionMessage) override;

        void sendCompleted(
            avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status sendMessageStatus) override;

        /**
         * A blocking function which will return once the @c MessageSender has completed processing the message.
         *
         * @return The status returned by the @c MessageSender once it's handled the message (successfully or not).
         */
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status waitForCompletion();

        /**
         * Utility function to return the database id associated with this @c MessageRequest.
         *
         * @return The database id associated with this @c MessageRequest.
         */
        int getDbId();

        /**
         * A function that allows early exit of the message sending logic.
         */
        void shutdown();

    private:
        /// The status of whether the message was sent to AVS ok.
        avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status m_sendMessageStatus;
        /// Captures if the @c MessageRequest has been processed or not by AVS.
        bool m_responseReceived;
        /// Mutex used to enforce thread safety.
        std::mutex m_mutex;
        /// The condition variable used when waiting for the @c MessageRequest to be processed.
        std::condition_variable m_cv;
        /// The database id associated with this @c MessageRequest.
        int m_dbId;
        /// A control so we may allow the message to stop waiting to be sent.
        bool m_isShuttingDown;
    };

    /**
     * Constructor.
     *
     * @param messageSender The entity which is able to send @c MessageRequests to AVS.
     * @param connection The connection which may be observed to determine connection status.
     * @param storage The object which manages persistent storage of messages to be sent.
     * @param dataManager A dataManager object that will track the CustomerDataHandler.
     * @param queueSizeWarnLimit The number of items we can store for sending without emitting a warning.
     * @param queueSizeHardLimit The maximum number of items we can store for sending.
     */
    CertifiedSender(
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::avs::AbstractConnection> connection,
        std::shared_ptr<MessageStorageInterface> storage,
        std::shared_ptr<registrationManager::CustomerDataManager> dataManager,
        int queueSizeWarnLimit = CERTIFIED_SENDER_QUEUE_SIZE_WARN_LIMIT,
        int queueSizeHardLimit = CERTIFIED_SENDER_QUEUE_SIZE_HARD_LIMIT);

    void onConnectionStatusChanged(
        const avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status status,
        const avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason reason) override;

    /**
     * Initialize this object.
     *
     * @return Whether the object was initialized ok.
     */
    bool init();

    /**
     * The actual handling of the sendJSONMessage call by our internal executor.
     *
     * @param jsonMessage The message to be sent to AVS.
     * @return Whether the message was successfully persisted.
     */
    bool executeSendJSONMessage(std::string jsonMessage);

    void doShutdown() override;

    /**
     * The worker function which will process the queue, and send messages.
     */
    void mainloop();

    /// A queue size threshold, beyond which we will emit warnings if more items are added.
    int m_queueSizeWarnLimit;
    /// The maximum possible size of the queue.
    int m_queueSizeHardLimit;

    /// The thread that will actually handle the sending of messages.
    std::thread m_workerThread;
    /// A control so we may disable the worker thread on shutdown.
    bool m_isShuttingDown;
    /// Mutex to protect access to class data members.
    std::mutex m_mutex;
    /// A condition variable with which to notify the worker thread that a new item was added to the queue.
    std::condition_variable m_workerThreadCV;

    /// A variable to capture if we are currently connected to AVS.
    bool m_isConnected;

    /// Our queue of requests that should be sent.
    std::deque<std::shared_ptr<CertifiedMessageRequest>> m_messagesToSend;

    /// The entity which actually sends the messages to AVS.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /// The message currently being sent.
    std::shared_ptr<CertifiedMessageRequest> m_currentMessage;

    // The connection object we are observing.
    std::shared_ptr<avsCommon::avs::AbstractConnection> m_connection;

    /// Where we will store the messages we wish to send.
    std::shared_ptr<MessageStorageInterface> m_storage;

    /// Executor to decouple the public-facing api from possibly inefficient persistent storage implementations.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace certifiedSender
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CERTIFIEDSENDER_INCLUDE_CERTIFIEDSENDER_CERTIFIEDSENDER_H_
