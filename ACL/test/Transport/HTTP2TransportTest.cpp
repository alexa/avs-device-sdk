/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <future>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <ACL/Transport/HTTP2Transport.h>
#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/AVS/Attachment/AttachmentUtils.h>
#include <AVSCommon/Utils/PromiseFuturePair.h>
#include <AVSCommon/Utils/HTTP/HttpResponseCode.h>
#include <AVSCommon/Utils/HTTP2/HTTP2RequestConfig.h>

#include "MockAuthDelegate.h"
#include "MockHTTP2Connection.h"
#include "MockHTTP2Request.h"
#include "MockMessageConsumer.h"
#include "MockPostConnect.h"
#include "MockPostConnectFactory.h"
#include "MockTransportObserver.h"

namespace alexaClientSDK {
namespace acl {
namespace transport {
namespace test {

using namespace acl::test;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::http;
using namespace avsCommon::utils::http2;
using namespace avsCommon::utils::http2::test;
using namespace ::testing;

/// Test endpoint.
static const std::string TEST_AVS_ENDPOINT_STRING = "http://avs-alexa-na.amazon.com";

/// Expected Downchannel URL sent on requests.
static const std::string AVS_DOWNCHANNEL_URL_PATH_EXTENSION = "/v20160207/directives";

/// Expected ping URL sent on requests.
static const std::string AVS_PING_URL_PATH_EXTENSION = "/ping";

/// Expected Full Downchannel URL sent on requests.
static const std::string FULL_DOWNCHANNEL_URL = TEST_AVS_ENDPOINT_STRING + AVS_DOWNCHANNEL_URL_PATH_EXTENSION;

/// Expected Full ping URL sent on requests.
static const std::string FULL_PING_URL = TEST_AVS_ENDPOINT_STRING + AVS_PING_URL_PATH_EXTENSION;

/// A 100 millisecond delay used in tests.
static const auto ONE_HUNDRED_MILLISECOND_DELAY = std::chrono::milliseconds(100);

/// A 10 millisecond delay used in tests.
static const auto TEN_MILLISECOND_DELAY = std::chrono::milliseconds(10);

/// A short delay used in tests.
static const auto SHORT_DELAY = std::chrono::seconds(1);

/// Typical Timeout used in waiting for responses.
static const auto RESPONSE_TIMEOUT = std::chrono::seconds(5);

/// A longer timeout used in waiting for responses.
static const auto LONG_RESPONSE_TIMEOUT = std::chrono::seconds(10);

/// HTTP Authorization header.
static const std::string HTTP_AUTHORIZATION_HEADER_BEARER = "Authorization: Bearer";

/// Authorization Token.
static const std::string CBL_AUTHORIZATION_TOKEN = "AUTH_TOKEN";

/// A test AttachmentId string.
static const std::string TEST_ATTACHMENT_ID_STRING_ONE = "testAttachmentId_1";

// Test message string to be sent.
static const std::string TEST_MESSAGE = "aaabbccc";

// Test attachment string.
static const std::string TEST_ATTACHMENT_MESSAGE = "MY_A_T_T_ACHMENT";

// Test attachment field.
static const std::string TEST_ATTACHMENT_FIELD = "ATTACHMENT";

// A constant that means no call as a response
static const long NO_CALL = -2;

// Non-MIME payload
static const std::string NON_MIME_PAYLOAD = "A_NON_MIME_PAYLOAD";

// A test directive.
static const std::string DIRECTIVE1 =
    "{\"namespace:\"SpeechSynthesizer\",name:\"Speak\",messageId:\"351df0ff-8041-4891-a925-136f52d54da1\","
    "dialogRequestId:\"58352bb2-7d07-4ba2-944b-10e6df25d193\"}";

// Another test directive.
static const std::string DIRECTIVE2 =
    "{\"namespace:\"Alerts\",name:\"SetAlert\",messageId:\"ccc005b8-ca8f-4c34-aeb5-73a8dbbd8d37\",dialogRequestId:"
    "\"dca0bece-16a7-44f3-b940-e6c4ecc2b1f5\"}";

// Test MIME Boundary.
static const std::string MIME_BOUNDARY = "thisisaboundary";

// MIME encoded DIRECTIVE1 with start and end MIME boundaries.
static const std::string MIME_BODY_DIRECTIVE1 = "--" + MIME_BOUNDARY + "\r\nContent-Type: application/json" +
                                                "\r\n\r\n" + DIRECTIVE1 + "\r\n--" + MIME_BOUNDARY + "--\r\n";
;

// MIME encoded DIRECTIVE2 with start MIME boundary but no end MIME boundary.
static const std::string MIME_BODY_DIRECTIVE2 = "--" + MIME_BOUNDARY + "\r\nContent-Type: application/json" +
                                                "\r\n\r\n" + DIRECTIVE2 + "\r\n--" + MIME_BOUNDARY + "\r\n";

// HTTP header to specify MIME boundary and content type.
static const std::string HTTP_BOUNDARY_HEADER =
    "Content-Type: multipart/related; boundary=" + MIME_BOUNDARY + "; type=application/json";

// The maximum dedicated number of ping streams in HTTP2Transport
static const unsigned MAX_PING_STREAMS = 1;

// The maximum dedicated number of downchannel streams in HTTP2Transport
static const unsigned MAX_DOWNCHANNEL_STREAMS = 1;

// The maximum number of HTTP2 requests that can be enqueued at a time waiting for response completion
static const unsigned MAX_AVS_STREAMS = 10;

// Maximum allowed of POST streams
static const unsigned MAX_POST_STREAMS = MAX_AVS_STREAMS - MAX_DOWNCHANNEL_STREAMS - MAX_PING_STREAMS;

/// Test harness for @c HTTP2Transport class.
class HTTP2TransportTest : public Test {
public:
    /// Initial setup for tests.
    void SetUp() override;

    /// Cleanup method.
    void TearDown() override;

protected:
    /**
     * Setup the handlers for the mocked methods @c AuthDelegateInterface::addAuthObserver(), @c
     * PostConnectFactoryInterface::createPostConnect() , @c  PostConnectInterface::doPostConnect() , @c
     * TransportObserverInterface::onConnected().
     *
     * @param sendOnPostConnected A boolean to specify whether to send onPostConnected() event when @c
     * PostConnectInterface::doPostConnect() is called.
     * @param expectConnected Specify that a call to TransportObserverInterface::onConnected() is expected.
     */
    void setupHandlers(bool sendOnPostConnected, bool expectConnected);

    /**
     * Helper function to send @c Refreshed Auth State to the @c HTTP2Transport observer.
     * It also checks that a proper Auth observer has been registered by @c HTTP2Transport.
     */
    void sendAuthStateRefreshed();

    /**
     * Helper function to put the @c HTTP2Transport into connected state.
     */
    void authorizeAndConnect();

    /// The HTTP2Transport instance to be tested.
    std::shared_ptr<HTTP2Transport> m_http2Transport;

    /// The mock @c AuthDelegateInterface.
    std::shared_ptr<MockAuthDelegate> m_mockAuthDelegate;

    /// The mock @c HTTP2ConnectionInterface.
    std::shared_ptr<MockHTTP2Connection> m_mockHttp2Connection;

    /// The mock @c MessageConsumerInterface.
    std::shared_ptr<MockMessageConsumer> m_mockMessageConsumer;

    /// An instance of the @c AttachmentManager.
    std::shared_ptr<AttachmentManager> m_attachmentManager;

    /// The mock @c TransportObserverInterface.
    std::shared_ptr<MockTransportObserver> m_mockTransportObserver;

    /// The mock @c PostConnectFactoryInterface
    std::shared_ptr<MockPostConnectFactory> m_mockPostConnectFactory;

    /// The mock @c PostConnectInterface.
    std::shared_ptr<MockPostConnect> m_mockPostConnect;

    /// A promise that the Auth Observer will be set.
    PromiseFuturePair<std::shared_ptr<AuthObserverInterface>> m_authObserverSet;

    /// A promise that @c PostConnectFactoryInterface::createPostConnect() will be called).
    PromiseFuturePair<void> m_createPostConnectCalled;

    /// A promise that @c PostConnectInterface:doPostConnect() will be called).
    PromiseFuturePair<std::shared_ptr<HTTP2Transport>> m_doPostConnected;

    /// A promise that the @c TransportObserver.onConnected() will be called.
    PromiseFuturePair<void> m_transportConnected;
};

/**
 *  A @c MessageRequestObserverInterface implementation used in this test.
 */
class TestMessageRequestObserver : public avsCommon::sdkInterfaces::MessageRequestObserverInterface {
public:
    /*
     * Called when a message request has been processed by AVS.
     */
    void onSendCompleted(MessageRequestObserverInterface::Status status) {
        m_status.setValue(status);
    }

    /*
     * Called when an exception is thrown when trying to send a message to AVS.
     */
    void onExceptionReceived(const std::string& exceptionMessage) {
        m_exception.setValue(exceptionMessage);
    }

    /// A promise that @c MessageRequestObserverInterface::onSendCompleted() will be called  with a @c
    /// MessageRequestObserverInterface::Status value
    PromiseFuturePair<MessageRequestObserverInterface::Status> m_status;

    /// A promise that @c MessageRequestObserverInterface::onExceptionReceived() will be called  with an exception
    /// message
    PromiseFuturePair<std::string> m_exception;
};

void HTTP2TransportTest::SetUp() {
    m_mockAuthDelegate = std::make_shared<NiceMock<MockAuthDelegate>>();
    m_mockHttp2Connection = std::make_shared<NiceMock<MockHTTP2Connection>>(FULL_DOWNCHANNEL_URL, FULL_PING_URL);
    m_mockMessageConsumer = std::make_shared<NiceMock<MockMessageConsumer>>();
    m_attachmentManager = std::make_shared<AttachmentManager>(AttachmentManager::AttachmentType::IN_PROCESS);
    m_mockTransportObserver = std::make_shared<NiceMock<MockTransportObserver>>();
    m_mockPostConnectFactory = std::make_shared<NiceMock<MockPostConnectFactory>>();
    m_mockPostConnect = std::make_shared<NiceMock<MockPostConnect>>();
    m_mockAuthDelegate->setAuthToken(CBL_AUTHORIZATION_TOKEN);
    m_http2Transport = HTTP2Transport::create(
        m_mockAuthDelegate,
        TEST_AVS_ENDPOINT_STRING,
        m_mockHttp2Connection,
        m_mockMessageConsumer,
        m_attachmentManager,
        m_mockTransportObserver,
        m_mockPostConnectFactory);

    ASSERT_NE(m_http2Transport, nullptr);
}

void HTTP2TransportTest::TearDown() {
    m_http2Transport->shutdown();
}

void HTTP2TransportTest::setupHandlers(bool sendOnPostConnected, bool expectConnected) {
    Sequence s1, s2;

    // Enforced ordering of mock method calls:
    // addAuthObserver should be before onConnected
    // createPostConnect should be before doPostConnect

    // Handle AuthDelegateInterface::addAuthObserver() when called.
    EXPECT_CALL(*m_mockAuthDelegate, addAuthObserver(_))
        .InSequence(s1)
        .WillOnce(Invoke([this](std::shared_ptr<avsCommon::sdkInterfaces::AuthObserverInterface> argAuthObserver) {
            m_authObserverSet.setValue(argAuthObserver);
        }));

    {
        InSequence dummy;

        // Handle PostConnectFactoryInterface::createPostConnect() when called.
        EXPECT_CALL(*m_mockPostConnectFactory, createPostConnect()).WillOnce(InvokeWithoutArgs([this] {
            m_createPostConnectCalled.setValue();
            return m_mockPostConnect;
        }));

        // Handle PostConnectInterface::doPostConnect() when called.
        EXPECT_CALL(*m_mockPostConnect, doPostConnect(_))
            .InSequence(s2)
            .WillOnce(Invoke([this, sendOnPostConnected](std::shared_ptr<HTTP2Transport> transport) {
                m_doPostConnected.setValue(transport);
                if (sendOnPostConnected) {
                    transport->onPostConnected();
                }
                return true;
            })

            );
    }

    if (expectConnected) {
        // Handle TransportObserverInterface::onConnected() when called.
        EXPECT_CALL(*m_mockTransportObserver, onConnected(_))
            .InSequence(s1, s2)
            .WillOnce(Invoke([this](std::shared_ptr<TransportInterface> transport) { m_transportConnected.setValue(); })

            );
    }
}

void HTTP2TransportTest::sendAuthStateRefreshed() {
    std::shared_ptr<AuthObserverInterface> authObserver;

    // Wait for HTTP2Transport Auth server registration.
    ASSERT_TRUE(m_authObserverSet.waitFor(RESPONSE_TIMEOUT));

    authObserver = m_authObserverSet.getValue();

    // Check HTTP2Transport registered itself as the authObserver.
    ASSERT_TRUE(authObserver != nullptr);
    ASSERT_EQ(authObserver.get(), m_http2Transport.get());

    // Send REFRESHED Auth State to HTTP2Transport.
    authObserver->onAuthStateChange(AuthObserverInterface::State::REFRESHED, AuthObserverInterface::Error::SUCCESS);
}

void HTTP2TransportTest::authorizeAndConnect() {
    setupHandlers(true, true);

    // Call connect().
    m_http2Transport->connect();

    // Deliver a 'REFRESHED' status to observers of AuthDelegateInterface.
    sendAuthStateRefreshed();

    // The Mock HTTP2Request replies to any downchannel request with a 200.
    ASSERT_TRUE(m_mockHttp2Connection->respondToDownchannelRequests(
        static_cast<long>(HTTPResponseCode::SUCCESS_OK), false, RESPONSE_TIMEOUT));

    // Wait for PostConnectInterface:doPostConnect() call.
    ASSERT_TRUE(m_doPostConnected.waitFor(RESPONSE_TIMEOUT));

    // Wait for a long time (up to 5 second), terminating the wait when the mock of
    // TransportObserverInterface::onConnected() is called.
    ASSERT_TRUE(m_transportConnected.waitFor(LONG_RESPONSE_TIMEOUT));
}

/**
 * Test non-authorization on empty auth token.
 */
TEST_F(HTTP2TransportTest, emptyAuthToken) {
    // Send an empty Auth token.
    m_mockAuthDelegate->setAuthToken("");

    setupHandlers(false, false);

    m_http2Transport->connect();

    // Give m_http2Transport a chance to misbehave and send a request w/o authorization.
    std::this_thread::sleep_for(SHORT_DELAY);

    // Check that no HTTP requests created and sent at this point.
    ASSERT_TRUE(m_mockHttp2Connection->isRequestQueueEmpty());

    sendAuthStateRefreshed();

    // Should not send any HTTP2 Request.
    ASSERT_EQ(m_mockHttp2Connection->waitForRequest(ONE_HUNDRED_MILLISECOND_DELAY), nullptr);
}

/**
 * Test waiting for AuthDelegateInterface.
 */
TEST_F(HTTP2TransportTest, waitAuthDelegateInterface) {
    setupHandlers(false, false);

    m_http2Transport->connect();

    // Give m_http2Transport a chance to misbehave and send a request w/o authorization.
    std::this_thread::sleep_for(SHORT_DELAY);

    // Check that no HTTP requests created and sent at this point.
    ASSERT_TRUE(m_mockHttp2Connection->isRequestQueueEmpty());

    sendAuthStateRefreshed();

    // Wait for HTTP2 Request.
    ASSERT_NE(m_mockHttp2Connection->waitForRequest(RESPONSE_TIMEOUT), nullptr);
    auto request = m_mockHttp2Connection->dequeRequest();
    ASSERT_EQ(request->getUrl(), FULL_DOWNCHANNEL_URL);
}

/**
 * Test verifying the proper inclusion of bearer token in requests.
 */
TEST_F(HTTP2TransportTest, bearerTokenInRequest) {
    setupHandlers(false, false);

    m_mockHttp2Connection->setWaitRequestHeader(HTTP_AUTHORIZATION_HEADER_BEARER);

    m_http2Transport->connect();

    sendAuthStateRefreshed();

    // Wait for HTTP2 Request with Authorization: Bearer in header.
    ASSERT_TRUE(m_mockHttp2Connection->waitForRequestWithHeader(RESPONSE_TIMEOUT));
}

/**
 * Test creation and triggering of post-connect object.
 */
TEST_F(HTTP2TransportTest, triggerPostConnectObject) {
    setupHandlers(false, false);

    // Don't expect TransportObserverInterface::onConnected() will be called.
    EXPECT_CALL(*m_mockTransportObserver, onConnected(_)).Times(0);

    m_http2Transport->connect();

    sendAuthStateRefreshed();

    // The Mock HTTP2Request replies to any downchannel request with 200.
    ASSERT_TRUE(m_mockHttp2Connection->respondToDownchannelRequests(
        static_cast<long>(HTTPResponseCode::SUCCESS_OK), false, RESPONSE_TIMEOUT));

    // Wait for a long time (up to 5 second), terminating the wait when the mock of
    // PostConnectFactoryInterface::createPostConnect() is called.
    ASSERT_TRUE(m_createPostConnectCalled.waitFor(RESPONSE_TIMEOUT));

    // Wait for a long time (up to 5 second), terminating the wait when the mock of
    // PostConnectInterface::doPostConnect() is called.
    ASSERT_TRUE(m_doPostConnected.waitFor(RESPONSE_TIMEOUT));
}

/**
 * Test delay of connection status until post-connect object created / notifies success.
 */
TEST_F(HTTP2TransportTest, connectionStatusOnPostConnect) {
    setupHandlers(true, true);

    // Call connect().
    m_http2Transport->connect();

    // Deliver a 'REFRESHED' status to observers of AuthDelegateInterface.
    sendAuthStateRefreshed();

    // The Mock HTTP2Request replies to any downchannel request with a 200.
    ASSERT_TRUE(m_mockHttp2Connection->respondToDownchannelRequests(
        static_cast<long>(HTTPResponseCode::SUCCESS_OK), false, RESPONSE_TIMEOUT));

    // Wait for PostConnectInterface:doPostConnect() call.
    ASSERT_TRUE(m_doPostConnected.waitFor(RESPONSE_TIMEOUT));

    // Wait for a long time (up to 5 second), terminating the wait when the mock of
    // TransportObserverInterface::onConnected() is called.
    ASSERT_TRUE(m_transportConnected.waitFor(LONG_RESPONSE_TIMEOUT));
}

/**
 * Test retry upon failed downchannel connection.
 */
TEST_F(HTTP2TransportTest, retryOnDownchannelConnectionFailure) {
    setupHandlers(false, false);

    EXPECT_CALL(*m_mockTransportObserver, onConnected(_)).Times(0);

    // Call connect().
    m_http2Transport->connect();

    // Deliver a 'REFRESHED' status to observers of AuthDelegateInterface.
    sendAuthStateRefreshed();

    // The Mock HTTP2Request replies to any downchannel request with a 500.
    ASSERT_TRUE(m_mockHttp2Connection->respondToDownchannelRequests(
        static_cast<long>(HTTPResponseCode::SERVER_ERROR_INTERNAL), false, RESPONSE_TIMEOUT));

    // Wait for a long time (up to 10 second), terminating the wait when the mock of HTTP2Connection receives a second
    // attempt to create a downchannel request.
    m_mockHttp2Connection->waitForRequest(LONG_RESPONSE_TIMEOUT, 2);
}

/**
 * Test sending of MessageRequest content.
 */
TEST_F(HTTP2TransportTest, messageRequestContent) {
    setupHandlers(false, false);

    // Call connect().
    m_http2Transport->connect();

    // Deliver a 'REFRESHED' status to observers of AuthDelegateInterface.
    sendAuthStateRefreshed();

    ASSERT_TRUE(m_mockHttp2Connection->respondToDownchannelRequests(
        static_cast<long>(HTTPResponseCode::SUCCESS_OK), false, RESPONSE_TIMEOUT));

    // Wait for doPostConnect().
    ASSERT_TRUE(m_doPostConnected.waitFor(RESPONSE_TIMEOUT));

    // Send post connect message.
    std::shared_ptr<MessageRequest> messageReq = std::make_shared<MessageRequest>(TEST_MESSAGE, "");
    m_http2Transport->sendPostConnectMessage(messageReq);

    // Wait for the postConnect message to become HTTP message request and HTTP body to be fully reassembled.
    auto postMessage = m_mockHttp2Connection->waitForPostRequest(LONG_RESPONSE_TIMEOUT);
    ASSERT_NE(postMessage, nullptr);

    // The number of MIME parts decoded should just be 1.
    ASSERT_EQ(postMessage->getMimeResponseSink()->getCountOfMimeParts(), 1u);

    // Get the first MIME part message.
    auto mimeMessage = postMessage->getMimeResponseSink()->getMimePart(0);
    std::string mimeMessageString(mimeMessage.begin(), mimeMessage.end());

    // Check the MIME part is the message sent.
    ASSERT_EQ(TEST_MESSAGE, mimeMessageString);
}

/**
 * Test sending of MessageRequest with attachment data.
 */
TEST_F(HTTP2TransportTest, messageRequestWithAttachment) {
    // Create an attachment reader.
    std::vector<char> attachment(TEST_ATTACHMENT_MESSAGE.begin(), TEST_ATTACHMENT_MESSAGE.end());
    std::shared_ptr<AttachmentReader> attachmentReader =
        avsCommon::avs::attachment::AttachmentUtils::createAttachmentReader(attachment);
    ASSERT_NE(attachmentReader, nullptr);

    setupHandlers(false, false);

    // Call connect().
    m_http2Transport->connect();

    // Deliver a 'REFRESHED' status to observers of AuthDelegateInterface.
    sendAuthStateRefreshed();

    m_mockHttp2Connection->respondToDownchannelRequests(
        static_cast<long>(HTTPResponseCode::SUCCESS_OK), false, RESPONSE_TIMEOUT);

    // Wait for doPostConnect().
    ASSERT_TRUE(m_doPostConnected.waitFor(RESPONSE_TIMEOUT));

    // Send post connect message with attachment.
    std::shared_ptr<MessageRequest> messageReq = std::make_shared<MessageRequest>(TEST_MESSAGE, "");
    messageReq->addAttachmentReader(TEST_ATTACHMENT_FIELD, attachmentReader);
    m_http2Transport->sendPostConnectMessage(messageReq);

    // Wait for the postConnect message to become HTTP message request and HTTP body to be fully reassembled.
    auto postMessage = m_mockHttp2Connection->waitForPostRequest(LONG_RESPONSE_TIMEOUT);
    ASSERT_NE(postMessage, nullptr);

    // The number of MIME parts decoded should just be 2.
    ASSERT_EQ(postMessage->getMimeResponseSink()->getCountOfMimeParts(), 2u);

    // Get the first MIME part message and check it is the message sent.
    auto mimeMessage = postMessage->getMimeResponseSink()->getMimePart(0);
    std::string mimeMessageString(mimeMessage.begin(), mimeMessage.end());
    ASSERT_EQ(TEST_MESSAGE, mimeMessageString);

    // Get the second MIME part message and check it is the attachement sent.
    auto mimeAttachment = postMessage->getMimeResponseSink()->getMimePart(1);
    std::string mimeAttachmentString(mimeAttachment.begin(), mimeAttachment.end());
    ASSERT_EQ(TEST_ATTACHMENT_MESSAGE, mimeAttachmentString);
}

/**
 * Test pause of sending message when attachment buffer (SDS) empty but not closed.
 */
TEST_F(HTTP2TransportTest, pauseSendWhenSDSEmpty) {
    setupHandlers(false, false);

    // Call connect().
    m_http2Transport->connect();

    // Deliver a 'REFRESHED' status to observers of AuthDelegateInterface.
    sendAuthStateRefreshed();

    m_mockHttp2Connection->respondToDownchannelRequests(
        static_cast<long>(HTTPResponseCode::SUCCESS_OK), false, RESPONSE_TIMEOUT);

    // Wait for doPostConnect().
    ASSERT_TRUE(m_doPostConnected.waitFor(RESPONSE_TIMEOUT));

    // Send post connect message with attachment.
    std::shared_ptr<MessageRequest> messageReq = std::make_shared<MessageRequest>(TEST_MESSAGE, "");
    AttachmentManager attMgr(AttachmentManager::AttachmentType::IN_PROCESS);
    std::vector<char> attachment(TEST_ATTACHMENT_MESSAGE.begin(), TEST_ATTACHMENT_MESSAGE.end());
    std::shared_ptr<AttachmentReader> attachmentReader =
        attMgr.createReader(TEST_ATTACHMENT_ID_STRING_ONE, avsCommon::utils::sds::ReaderPolicy::NONBLOCKING);
    ASSERT_NE(attachmentReader, nullptr);
    messageReq->addAttachmentReader(TEST_ATTACHMENT_FIELD, attachmentReader);
    m_http2Transport->sendPostConnectMessage(messageReq);

    // Send the attachment in chunks in another thread.
    std::thread writerThread([this, &attMgr, &attachment]() {
        // Number of chunks the attachment will be divided into
        const unsigned chunks = 4;
        // the size of each chunk in bytes, this is the ceiling of (attachment.size / chunks)
        unsigned int chunkSize = (attachment.size() + chunks - 1) / chunks;
        auto writer = attMgr.createWriter(TEST_ATTACHMENT_ID_STRING_ONE, avsCommon::utils::sds::WriterPolicy::BLOCKING);
        AttachmentWriter::WriteStatus writeStatus = AttachmentWriter::WriteStatus::OK;
        unsigned int lastChunkSize = (attachment.size() % chunks == 0) ? chunkSize : attachment.size() % chunks;
        for (unsigned chunk = 0; chunk < chunks; chunk++) {
            writer->write(
                &attachment[chunk * chunkSize], (chunk == chunks - 1) ? lastChunkSize : chunkSize, &writeStatus);
            ASSERT_EQ(writeStatus, AttachmentWriter::WriteStatus::OK);
            ASSERT_TRUE(m_mockHttp2Connection->isPauseOnSendReceived(ONE_HUNDRED_MILLISECOND_DELAY));
        }
        writer->close();
    });

    // Wait for the postConnect message to become HTTP message request and HTTP body to be fully reassembled.
    auto postMessage = m_mockHttp2Connection->waitForPostRequest(LONG_RESPONSE_TIMEOUT);
    ASSERT_NE(postMessage, nullptr);

    // The number of MIME parts decoded should just be 2.
    ASSERT_EQ(postMessage->getMimeResponseSink()->getCountOfMimeParts(), 2u);

    // Get the first MIME part message and check it is the message sent.
    auto mimeMessage = postMessage->getMimeResponseSink()->getMimePart(0);
    std::string mimeMessageString(mimeMessage.begin(), mimeMessage.end());
    ASSERT_EQ(TEST_MESSAGE, mimeMessageString);

    // Get the second MIME part message and check it is the attachment sent.
    auto mimeAttachment = postMessage->getMimeResponseSink()->getMimePart(1);
    std::string mimeAttachmentString(mimeAttachment.begin(), mimeAttachment.end());
    ASSERT_EQ(TEST_ATTACHMENT_MESSAGE, mimeAttachmentString);

    writerThread.join();
}

/**
 * Test queuing MessageRequests until a response code has been received for any outstanding MessageRequest
 */
TEST_F(HTTP2TransportTest, messageRequestsQueuing) {
    authorizeAndConnect();

    // Send 5 messages.
    std::vector<std::shared_ptr<TestMessageRequestObserver>> messageObservers;
    const unsigned messagesCount = 5;  // number of test messages to Send
    for (unsigned messageNum = 0; messageNum < messagesCount; messageNum++) {
        std::shared_ptr<MessageRequest> messageReq = std::make_shared<MessageRequest>(TEST_MESSAGE, "");
        auto messageObserver = std::make_shared<TestMessageRequestObserver>();
        messageObservers.push_back(messageObserver);
        messageReq->addObserver(messageObserver);
        m_http2Transport->send(messageReq);
    }

    // Give m_http2Transport a chance to misbehave and send more than a single request before receiving a response.
    std::this_thread::sleep_for(SHORT_DELAY);

    // Check that only 1 out of the 5 POST messages have been in the outgoing send queue.
    ASSERT_EQ(m_mockHttp2Connection->getPostRequestsNum(), 1u);

    // Delayed 200 response for each POST request.
    unsigned int postsRequestsCount = 0;
    while (postsRequestsCount < messagesCount) {
        auto request = m_mockHttp2Connection->waitForPostRequest(RESPONSE_TIMEOUT);
        if (request) {
            postsRequestsCount++;
            // Give m_http2Transport a chance to misbehave and send requests before receiving a response.
            std::this_thread::sleep_for(SHORT_DELAY);
            request->getSink()->onReceiveResponseCode(HTTPResponseCode::SUCCESS_OK);
        } else {
            break;
        }
    }

    // Make sure HTTP2Transport sends out the 5 POST requests.
    ASSERT_EQ(postsRequestsCount, messagesCount);

    // On disconnect, send CANCELED response for each POST REQUEST.
    EXPECT_CALL(*m_mockHttp2Connection, disconnect()).WillOnce(Invoke([this]() {
        while (true) {
            auto request = m_mockHttp2Connection->dequePostRequest();
            if (!request) break;

            request->getSink()->onResponseFinished(HTTP2ResponseFinishedStatus::CANCELLED);
        };
    }));

    m_http2Transport->shutdown();

    // Count the number of messages that received CANCELED or NOT_CONNECTED event.
    unsigned messagesCanceled = 0;
    for (unsigned messageNum = 0; messageNum < messagesCount; messageNum++) {
        if (messageObservers[messageNum]->m_status.waitFor(RESPONSE_TIMEOUT)) {
            switch (messageObservers[messageNum]->m_status.getValue()) {
                case MessageRequestObserverInterface::Status::CANCELED:
                case MessageRequestObserverInterface::Status::NOT_CONNECTED:
                    messagesCanceled++;
                default:
                    break;
            }
        }
    }

    ASSERT_EQ(messagesCanceled, messagesCount);
}

/**
 * Test notification of onSendCompleted (check mapping of all cases and their mapping to
 * MessageRequestObserverInterface::Status).
 */
TEST_F(HTTP2TransportTest, onSendCompletedNotification) {
    // Contains the mapping of HTTPResponseCode, HTTP2ResponseFinishedStatus and the expected
    // MessageRequestObserverInterface::Status.
    const std::vector<std::tuple<long, HTTP2ResponseFinishedStatus, MessageRequestObserverInterface::Status>>
        messageResponseMap = {
            {std::make_tuple(
                NO_CALL,
                HTTP2ResponseFinishedStatus::INTERNAL_ERROR,
                MessageRequestObserverInterface::Status::INTERNAL_ERROR)},
            {std::make_tuple(
                NO_CALL, HTTP2ResponseFinishedStatus::CANCELLED, MessageRequestObserverInterface::Status::CANCELED)},
            {std::make_tuple(
                NO_CALL, HTTP2ResponseFinishedStatus::TIMEOUT, MessageRequestObserverInterface::Status::TIMEDOUT)},
            {std::make_tuple(
                NO_CALL,
                static_cast<HTTP2ResponseFinishedStatus>(-1),
                MessageRequestObserverInterface::Status::INTERNAL_ERROR)},
            {std::make_tuple(
                HTTPResponseCode::HTTP_RESPONSE_CODE_UNDEFINED,
                HTTP2ResponseFinishedStatus::INTERNAL_ERROR,
                MessageRequestObserverInterface::Status::INTERNAL_ERROR)},
            {std::make_tuple(
                HTTPResponseCode::SUCCESS_OK,
                HTTP2ResponseFinishedStatus::CANCELLED,
                MessageRequestObserverInterface::Status::CANCELED)},
            {std::make_tuple(
                HTTPResponseCode::REDIRECTION_START_CODE,
                HTTP2ResponseFinishedStatus::TIMEOUT,
                MessageRequestObserverInterface::Status::TIMEDOUT)},
            {std::make_tuple(
                HTTPResponseCode::CLIENT_ERROR_BAD_REQUEST,
                static_cast<HTTP2ResponseFinishedStatus>(-1),
                MessageRequestObserverInterface::Status::INTERNAL_ERROR)},
            {std::make_tuple(
                HTTPResponseCode::HTTP_RESPONSE_CODE_UNDEFINED,
                HTTP2ResponseFinishedStatus::COMPLETE,
                MessageRequestObserverInterface::Status::INTERNAL_ERROR)},
            {std::make_tuple(
                HTTPResponseCode::SUCCESS_OK,
                HTTP2ResponseFinishedStatus::COMPLETE,
                MessageRequestObserverInterface::Status::SUCCESS)},
            {std::make_tuple(
                HTTPResponseCode::SUCCESS_NO_CONTENT,
                HTTP2ResponseFinishedStatus::COMPLETE,
                MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT)},
            {std::make_tuple(
                HTTPResponseCode::REDIRECTION_START_CODE,
                HTTP2ResponseFinishedStatus::COMPLETE,
                MessageRequestObserverInterface::Status::SERVER_OTHER_ERROR)},
            {std::make_tuple(
                HTTPResponseCode::REDIRECTION_END_CODE,
                HTTP2ResponseFinishedStatus::COMPLETE,
                MessageRequestObserverInterface::Status::SERVER_OTHER_ERROR)},
            {std::make_tuple(
                HTTPResponseCode::CLIENT_ERROR_BAD_REQUEST,
                HTTP2ResponseFinishedStatus::COMPLETE,
                MessageRequestObserverInterface::Status::BAD_REQUEST)},
            {std::make_tuple(
                HTTPResponseCode::CLIENT_ERROR_FORBIDDEN,
                HTTP2ResponseFinishedStatus::COMPLETE,
                MessageRequestObserverInterface::Status::INVALID_AUTH)},
            {std::make_tuple(
                HTTPResponseCode::SERVER_ERROR_INTERNAL,
                HTTP2ResponseFinishedStatus::COMPLETE,
                MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2)},
            {std::make_tuple(
                -1,
                HTTP2ResponseFinishedStatus::COMPLETE,
                MessageRequestObserverInterface::Status::SERVER_OTHER_ERROR)},
        };

    authorizeAndConnect();

    // Send a message for each test case defined in the messageResponseMap.
    std::vector<std::shared_ptr<TestMessageRequestObserver>> messageObservers;
    unsigned messagesCount = messageResponseMap.size();  // number of test messages to send
    for (unsigned messageNum = 0; messageNum < messagesCount; messageNum++) {
        std::shared_ptr<MessageRequest> messageReq = std::make_shared<MessageRequest>(TEST_MESSAGE, "");
        auto messageObserver = std::make_shared<TestMessageRequestObserver>();
        messageObservers.push_back(messageObserver);
        messageReq->addObserver(messageObserver);
        m_http2Transport->send(messageReq);
    }

    // Send the response code for each POST request.
    unsigned int postsRequestsCount = 0;
    for (unsigned int i = 0; i < messagesCount; i++) {
        auto request = m_mockHttp2Connection->waitForPostRequest(RESPONSE_TIMEOUT);
        if (request) {
            m_mockHttp2Connection->dequePostRequest();
            postsRequestsCount++;
            auto responseCode = std::get<0>(messageResponseMap[i]);
            if (responseCode != NO_CALL) {
                request->getSink()->onReceiveResponseCode(responseCode);
            }
            auto responseFinished = std::get<1>(messageResponseMap[i]);
            request->getSink()->onResponseFinished(static_cast<HTTP2ResponseFinishedStatus>(responseFinished));

        } else {
            break;
        }
    }

    // Check if we got all the POST requests.
    ASSERT_EQ(postsRequestsCount, messagesCount);

    // Check that we got the right onSendCompleted notifications.
    for (unsigned messageNum = 0; messageNum < messagesCount; messageNum++) {
        if (messageObservers[messageNum]->m_status.waitFor(RESPONSE_TIMEOUT)) {
            auto expectedMessageObserverStatus = std::get<2>(messageResponseMap[messageNum]);
            ASSERT_EQ(messageObservers[messageNum]->m_status.getValue(), expectedMessageObserverStatus);
        }
    }
}

/**
 * Test onExceptionReceived() notification for non 200 content.
 */
TEST_F(HTTP2TransportTest, onExceptionReceivedNon200Content) {
    authorizeAndConnect();

    // Send a message.
    std::shared_ptr<MessageRequest> messageReq = std::make_shared<MessageRequest>(TEST_MESSAGE, "");
    auto messageObserver = std::make_shared<TestMessageRequestObserver>();
    messageReq->addObserver(messageObserver);
    m_http2Transport->send(messageReq);

    auto request = m_mockHttp2Connection->waitForPostRequest(RESPONSE_TIMEOUT);
    ASSERT_NE(request, nullptr);
    request->getSink()->onReceiveResponseCode(HTTPResponseCode::SERVER_ERROR_INTERNAL);
    request->getSink()->onReceiveData(NON_MIME_PAYLOAD.c_str(), NON_MIME_PAYLOAD.size());
    request->getSink()->onResponseFinished(HTTP2ResponseFinishedStatus::COMPLETE);

    ASSERT_TRUE(messageObserver->m_exception.waitFor(RESPONSE_TIMEOUT));
    ASSERT_EQ(messageObserver->m_exception.getValue(), NON_MIME_PAYLOAD);
    ASSERT_TRUE(messageObserver->m_status.waitFor(RESPONSE_TIMEOUT));
    ASSERT_EQ(messageObserver->m_status.getValue(), MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2);
}

/**
 * Test MessageConsumerInterface receipt of directives on Downchannel and Event streams.
 */
TEST_F(HTTP2TransportTest, messageConsumerReceiveDirective) {
    PromiseFuturePair<void> messagesAreConsumed;
    unsigned int consumedMessageCount = 0;
    std::vector<std::string> messages;

    authorizeAndConnect();

    EXPECT_CALL(*m_mockMessageConsumer, consumeMessage(_, _))
        .WillRepeatedly(Invoke([&messagesAreConsumed, &consumedMessageCount, &messages](
                                   const std::string& contextId, const std::string& message) {
            consumedMessageCount++;
            messages.push_back(message);
            if (consumedMessageCount == 2u) {
                messagesAreConsumed.setValue();
            }
        })

        );

    // Send a message.
    std::shared_ptr<MessageRequest> messageReq = std::make_shared<MessageRequest>(TEST_MESSAGE, "");
    auto messageObserver = std::make_shared<TestMessageRequestObserver>();
    messageReq->addObserver(messageObserver);
    m_http2Transport->send(messageReq);

    auto eventStream = m_mockHttp2Connection->waitForPostRequest(RESPONSE_TIMEOUT);
    ASSERT_NE(eventStream, nullptr);
    eventStream->getSink()->onReceiveResponseCode(HTTPResponseCode::SUCCESS_OK);
    eventStream->getSink()->onReceiveHeaderLine(HTTP_BOUNDARY_HEADER);
    eventStream->getSink()->onReceiveData(MIME_BODY_DIRECTIVE1.c_str(), MIME_BODY_DIRECTIVE1.size());
    eventStream->getSink()->onResponseFinished(HTTP2ResponseFinishedStatus::COMPLETE);

    auto downchannelStream = m_mockHttp2Connection->getDownchannelRequest();
    ASSERT_NE(downchannelStream, nullptr);
    downchannelStream->getSink()->onReceiveResponseCode(HTTPResponseCode::SUCCESS_OK);
    downchannelStream->getSink()->onReceiveHeaderLine(HTTP_BOUNDARY_HEADER);
    downchannelStream->getSink()->onReceiveData(MIME_BODY_DIRECTIVE2.c_str(), MIME_BODY_DIRECTIVE2.size());

    ASSERT_TRUE(messagesAreConsumed.waitFor(RESPONSE_TIMEOUT));
    ASSERT_EQ(messages[0], DIRECTIVE1);
    ASSERT_EQ(messages[1], DIRECTIVE2);
}

/**
 * Test broadcast onServerSideDisconnect() upon closure of successfully opened downchannel.
 */
TEST_F(HTTP2TransportTest, onServerSideDisconnectOnDownchannelClosure) {
    authorizeAndConnect();

    // Send a message.
    std::shared_ptr<MessageRequest> messageReq = std::make_shared<MessageRequest>(TEST_MESSAGE, "");
    m_http2Transport->send(messageReq);

    PromiseFuturePair<void> gotOnServerSideDisconnect;
    auto setGotOnServerSideDisconnect = [&gotOnServerSideDisconnect] { gotOnServerSideDisconnect.setValue(); };

    PromiseFuturePair<void> gotOnDisconnected;
    auto setGotOnDisconnected = [&gotOnDisconnected] { gotOnDisconnected.setValue(); };

    // Expect disconnect events later when downchannel receives a COMPLETE finished response.
    {
        InSequence dummy;
        EXPECT_CALL(*m_mockTransportObserver, onServerSideDisconnect(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(setGotOnServerSideDisconnect));
        EXPECT_CALL(*m_mockTransportObserver, onDisconnected(_, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(setGotOnDisconnected));
    }

    // Upon receiving the message, HTTP2Connection/request will reply to the down-channel request with
    // onResponseFinished(COMPLETE).
    auto eventStream = m_mockHttp2Connection->waitForPostRequest(RESPONSE_TIMEOUT);
    ASSERT_NE(eventStream, nullptr);
    auto downchannelStream = m_mockHttp2Connection->getDownchannelRequest();
    downchannelStream->getSink()->onResponseFinished(HTTP2ResponseFinishedStatus::COMPLETE);

    // Wait for onResponseFinished() to be handled.
    ASSERT_TRUE(gotOnServerSideDisconnect.waitFor(RESPONSE_TIMEOUT));
    ASSERT_TRUE(gotOnDisconnected.waitFor(RESPONSE_TIMEOUT));
}

/**
 * Test detection of MessageRequest timeout and trigger of ping request.
 */
TEST_F(HTTP2TransportTest, messageRequestTimeoutPingRequest) {
    authorizeAndConnect();

    // Send a message.
    std::shared_ptr<MessageRequest> messageReq = std::make_shared<MessageRequest>(TEST_MESSAGE, "");
    m_http2Transport->send(messageReq);

    // Upon receiving the message, the mock HTTP2Connection/request will reply to the request with
    // onResponseFinished(TIMEOUT).
    auto eventStream = m_mockHttp2Connection->waitForPostRequest(RESPONSE_TIMEOUT);
    ASSERT_NE(eventStream, nullptr);
    eventStream->getSink()->onResponseFinished(HTTP2ResponseFinishedStatus::TIMEOUT);

    // Wait for a long time (up to 5 seconds) for the mock HTTP2Connection to receive a ping request.
    ASSERT_NE(m_mockHttp2Connection->waitForPingRequest(RESPONSE_TIMEOUT), nullptr);
}

/**
 * Test detection of network inactivity and trigger of ping request and continued ping for continued inactivity.
 */
TEST_F(HTTP2TransportTest, networkInactivityPingRequest) {
    // Short time to wait for inactivity before sending a ping.
    static const std::chrono::seconds testInactivityTimeout = SHORT_DELAY;
    // This test just checks that a second and third ping will be sen
    static const unsigned expectedInactivityPingCount{3u};
    // How long until pings should be sent plus some extra time to allow notifications to be processed.
    static const std::chrono::seconds testInactivityTime = {testInactivityTimeout * expectedInactivityPingCount +
                                                            SHORT_DELAY};

    // Setup HTTP2Transport with shorter ping inactivity timeout.
    HTTP2Transport::Configuration cfg;
    cfg.inactivityTimeout = testInactivityTimeout;
    m_http2Transport = HTTP2Transport::create(
        m_mockAuthDelegate,
        TEST_AVS_ENDPOINT_STRING,
        m_mockHttp2Connection,
        m_mockMessageConsumer,
        m_attachmentManager,
        m_mockTransportObserver,
        m_mockPostConnectFactory,
        cfg);

    authorizeAndConnect();

    PromiseFuturePair<void> gotPings;

    // Respond 204 to ping requests.
    std::thread pingResponseThread([this, &gotPings]() {
        unsigned pingCount{0};
        while (pingCount < expectedInactivityPingCount) {
            auto pingRequest = m_mockHttp2Connection->waitForPingRequest(RESPONSE_TIMEOUT);
            if (!pingRequest) {
                continue;
            }
            m_mockHttp2Connection->dequePingRequest();
            pingRequest->getSink()->onReceiveResponseCode(HTTPResponseCode::SUCCESS_NO_CONTENT);
            pingRequest->getSink()->onResponseFinished(HTTP2ResponseFinishedStatus::COMPLETE);
            pingCount++;
        }
        gotPings.setValue();
    });

    ASSERT_TRUE(gotPings.waitFor(testInactivityTime));

    pingResponseThread.join();
}

/**
 * Test connection tear down for ping timeout.
 */
TEST_F(HTTP2TransportTest, tearDownPingTimeout) {
    // Short time to wait for inactivity before sending a ping.
    const std::chrono::seconds testInactivityTimeout = SHORT_DELAY;

    // Setup HTTP2Transport with shorter ping inactivity timeout.
    HTTP2Transport::Configuration cfg;
    cfg.inactivityTimeout = testInactivityTimeout;
    m_http2Transport = HTTP2Transport::create(
        m_mockAuthDelegate,
        TEST_AVS_ENDPOINT_STRING,
        m_mockHttp2Connection,
        m_mockMessageConsumer,
        m_attachmentManager,
        m_mockTransportObserver,
        m_mockPostConnectFactory,
        cfg);

    authorizeAndConnect();

    PromiseFuturePair<void> gotOnDisconnected;
    auto setGotOnDisconnected = [&gotOnDisconnected] { gotOnDisconnected.setValue(); };

    EXPECT_CALL(*m_mockTransportObserver, onDisconnected(_, _))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(setGotOnDisconnected));

    // Reply to a ping request.
    std::thread pingThread([this]() {
        auto pingRequest = m_mockHttp2Connection->waitForPingRequest(RESPONSE_TIMEOUT);
        ASSERT_TRUE(pingRequest);
        m_mockHttp2Connection->dequePingRequest();
        pingRequest->getSink()->onResponseFinished(HTTP2ResponseFinishedStatus::TIMEOUT);
    });

    ASSERT_TRUE(gotOnDisconnected.waitFor(RESPONSE_TIMEOUT));

    pingThread.join();
}

/**
 * Test connection tear down for ping failure.
 */
TEST_F(HTTP2TransportTest, tearDownPingFailure) {
    // Short time to wait for inactivity before sending a ping.
    const std::chrono::seconds testInactivityTimeout = SHORT_DELAY;

    // Setup HTTP2Transport with shorter ping inactivity timeout.
    HTTP2Transport::Configuration cfg;
    cfg.inactivityTimeout = testInactivityTimeout;
    m_http2Transport = HTTP2Transport::create(
        m_mockAuthDelegate,
        TEST_AVS_ENDPOINT_STRING,
        m_mockHttp2Connection,
        m_mockMessageConsumer,
        m_attachmentManager,
        m_mockTransportObserver,
        m_mockPostConnectFactory,
        cfg);

    authorizeAndConnect();

    PromiseFuturePair<void> gotOnDisconnected;
    auto setGotOnDisconnected = [&gotOnDisconnected] { gotOnDisconnected.setValue(); };

    EXPECT_CALL(*m_mockTransportObserver, onDisconnected(_, _))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(setGotOnDisconnected));

    // Reply to a ping request.
    std::thread pingThread([this]() {
        auto pingRequest = m_mockHttp2Connection->waitForPingRequest(RESPONSE_TIMEOUT);
        ASSERT_TRUE(pingRequest);
        m_mockHttp2Connection->dequePingRequest();
        pingRequest->getSink()->onReceiveResponseCode(HTTPResponseCode::CLIENT_ERROR_BAD_REQUEST);
        pingRequest->getSink()->onResponseFinished(HTTP2ResponseFinishedStatus::COMPLETE);
    });

    ASSERT_TRUE(gotOnDisconnected.waitFor(RESPONSE_TIMEOUT));

    pingThread.join();
}

/**
 * Test limiting use of AVS streams to 10.
 */
TEST_F(HTTP2TransportTest, avsStreamsLimit) {
    // Number of test messages to send for this test.  This number is much larger than MAX_POST_STREAMS
    // to assure that this test exercises the case where more requests are outstanding than are allowed
    // to be sent at one time, forcing HTTP2Transport to queue the requests until some requests complete.
    const unsigned messagesCount = MAX_POST_STREAMS * 2;

    authorizeAndConnect();

    m_mockHttp2Connection->setResponseToPOSTRequests(HTTPResponseCode::SUCCESS_OK);

    // Send the 20 messages.
    std::vector<std::shared_ptr<TestMessageRequestObserver>> messageObservers;

    for (unsigned messageNum = 0; messageNum < messagesCount; messageNum++) {
        std::shared_ptr<MessageRequest> messageReq =
            std::make_shared<MessageRequest>(TEST_MESSAGE + std::to_string(messageNum), "");
        auto messageObserver = std::make_shared<TestMessageRequestObserver>();
        messageObservers.push_back(messageObserver);
        messageReq->addObserver(messageObserver);
        m_http2Transport->send(messageReq);
    }

    // Check that there was a downchannel request sent out.
    ASSERT_TRUE(m_mockHttp2Connection->getDownchannelRequest(RESPONSE_TIMEOUT));

    // Check the messages we sent were limited.
    ASSERT_EQ(m_mockHttp2Connection->getPostRequestsNum(), MAX_POST_STREAMS);

    unsigned int completed = 0;
    std::shared_ptr<MockHTTP2Request> request;
    while ((request = m_mockHttp2Connection->dequePostRequest(RESPONSE_TIMEOUT)) != nullptr &&
           completed < messagesCount) {
        request->getSink()->onResponseFinished(HTTP2ResponseFinishedStatus::COMPLETE);
        completed++;
        // Give m_http2Transport a little time to misbehave.
        std::this_thread::sleep_for(TEN_MILLISECOND_DELAY);
    }

    // Check all the POST requests have been enqueued
    ASSERT_EQ(completed, messagesCount);

    // Check that the maximum number of enqueued messages at any time has been limited.
    ASSERT_EQ(m_mockHttp2Connection->getMaxPostRequestsEnqueud(), MAX_POST_STREAMS);
}

}  // namespace test
}  // namespace transport
}  // namespace acl
}  // namespace alexaClientSDK
