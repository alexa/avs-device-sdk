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
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <functional>

#include <acsdkAuthorization/LWA/LWAAuthorizationAdapter.h>
#include <acsdkAuthorizationInterfaces/LWA/CBLAuthorizationObserverInterface.h>
#include <acsdkAuthorizationInterfaces/LWA/LWAAuthorizationStorageInterface.h>
#include <acsdkAuthorization/LWA/LWAAuthorizationConfiguration.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/HTTP/HttpResponseCode.h>
#include <AVSCommon/Utils/LibcurlUtils/HTTPResponse.h>
#include <AVSCommon/Utils/LibcurlUtils/MockHttpGet.h>
#include <AVSCommon/Utils/LibcurlUtils/MockHttpPost.h>
#include <AVSCommon/Utils/WaitEvent.h>
#include <AVSCommon/Utils/Optional.h>
#include <acsdkAuthorization/LWA/test/StubStorage.h>

namespace alexaClientSDK {
namespace acsdkAuthorization {
namespace lwa {
namespace test {

using namespace ::testing;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::libcurlUtils;
using namespace avsCommon::utils::http;
using namespace avsCommon::utils::libcurlUtils::test;
using namespace acsdkAuthorizationInterfaces;
using namespace acsdkAuthorizationInterfaces::lwa;

/// An example user id that would be returned by the Customer Profile API.
static const std::string USER_ID = "test-user-id";

/// An example name that would be returned by the Customer Profile API.
static const std::string NAME = "Test User";

/// An example email that would be returned by the Customer Profile API.
static const std::string EMAIL = "test@user.com";

/// The Veritifcation URI.
static const std::string VERIFICATION_URI = "https://amazon.com/us/code";

/// An example CBL code to be returned from CBL auth.
static const std::string USER_CODE = "ABCDE";

/// An example device code to be returned from CBL auth.
static const std::string DEVICE_CODE = "deviceCode";

/// A test adapter id.
static const std::string ADAPTER_ID = "test-adapter-id";

/// Default adapter id.
const std::string DEFAULT_ADAPTER_ID = "lwa-adapter";

/// Timeout for test cases that require synchronization.
std::chrono::seconds TIMEOUT{2};

/// Long timeout for cases involving waiting for retries.
std::chrono::seconds LONG_TIMEOUT{20};

/// The config node for the LWAAuthorizationAdapter.
static const std::string CONFIG_ROOT_NODE = "lwaAuthorization";

/// The method name for authorizeUsingCBL.
static const std::string AUTHORIZE_USING_CBL = "authorizeUsingCBL";

/// The method name for authorizeUsingCBLWithCustomerProfile.
static const std::string AUTHORIZE_USING_CBL_WITH_CUSTOMER_PROFILE = "authorizeUsingCBLWithCustomerProfile";

/// Example responses from LWA.
const std::string EXPIRATION_S = "3600";
const std::string INTERVAL_S = "3600";
const std::string ACCESS_TOKEN = "myaccesstoken";
const std::string REFRESH_TOKEN = "myrefreshtoken";
const std::string TOKEN_TYPE = "bearer";
const std::string EXPIRATION = "3600";

/// The Config JSON.
// clang-format off
static const std::string CONFIG_JSON = R"(
{
    "deviceInfo" : {
        "clientId":"MyClientId",
        "productId":"MyProductId",
        "deviceSerialNumber":"0",
        "manufacturerName":"MyCompany",
        "description":"MyCommpany"
    },
    ")" + CONFIG_ROOT_NODE + R"(" : {}
}
)";

/// A default successful code pair response.
static const HTTPResponse CODE_PAIR_RESPONSE{
    HTTPResponseCode::SUCCESS_OK,
    R"(
        {
            "user_code": ")" + USER_CODE + R"(",
            "device_code": ")" + DEVICE_CODE + R"(",
            "verification_uri": ")" + VERIFICATION_URI +
                                                 R"(",
            "expires_in": )" + EXPIRATION_S + R"(,
            "interval": )" + INTERVAL_S + R"(
        })"};

/// A default successful token exchange response.
static const HTTPResponse TOKEN_EXCHANGE_RESPONSE{
    HTTPResponseCode::SUCCESS_OK,
    R"(
        {
            "access_token": ")" + ACCESS_TOKEN + R"(",
            "refresh_token": ")" + REFRESH_TOKEN + R"(",
            "token_type": ")" + TOKEN_TYPE + R"(",
            "expires_in": )" + EXPIRATION + R"(
        })"};

/// A defauilt successful customer profile response containing only the user id.
static const HTTPResponse CUSTOMER_PROFILE_SHORT_RESPONSE{
    HTTPResponseCode::SUCCESS_OK,
    R"(
        {
            "user_id": ")" + USER_ID + R"("
        })"};

/// A defauilt successful customer profile response.
static const HTTPResponse CUSTOMER_PROFILE_RESPONSE{
    HTTPResponseCode::SUCCESS_OK,
    R"(
        {
            "user_id": ")" + USER_ID + R"(",
            "name": ")" + NAME + R"(",
            "email": ")" + EMAIL + R"("
        })"};
// clang-format on

/// A mock observer.
class MockCBLObserver : public acsdkAuthorizationInterfaces::lwa::CBLAuthorizationObserverInterface {
public:
    MOCK_METHOD2(onRequestAuthorization, void(const std::string& url, const std::string& code));
    MOCK_METHOD0(onCheckingForAuthorization, void());
    MOCK_METHOD1(onCustomerProfileAvailable, void(const CustomerProfile& customerProfile));
};

/// A mock AuthorizationManager.
class MockAuthManager : public acsdkAuthorizationInterfaces::AuthorizationManagerInterface {
public:
    MOCK_METHOD3(
        reportStateChange,
        void(
            const avsCommon::sdkInterfaces::AuthObserverInterface::FullState& state,
            const std::string& authId,
            const std::string& userId));
    MOCK_METHOD1(
        add,
        void(const std::shared_ptr<acsdkAuthorizationInterfaces::AuthorizationAdapterInterface>& adapter));
};

/**
 * This class tests the internals of the LWAAuthorizationAdapter class.
 * Due to the complicated nature of the the APIs, whitebox testing is done.
 */
class LWAAuthorizationAdapterTest : public ::testing::TestWithParam<std::string> {
public:
    /// SetUp.
    void SetUp() override;

    static const HTTPResponse NULL_HTTP_RESPONSE;

    /**
     * Set expectations against expected responses from LWA using CBL using the following default responses:
     *
     * CODE_PAIR_RESPONSE
     * TOKEN_EXCHANGE_RESPONSE
     *
     * @param customerProfileResponse The customerProfileResponse to use.
     */
    void setCBLExpectations(HTTPResponse customerProfileResponse);

    /**
     * Set expectations against expected responses from LWA using CBL with the provided responses.
     * Passing in a value equal to NULL_HTTP_RESPONSE will cause the expectation to be omitted.
     *
     * @param codePairResponse Response from a code pair operation.
     * @param tokenExchangeResponse Response from a token exchange operation.
     * @param customerProfileResponse Response form a customer profile operation.
     */
    void setCBLExpectations(
        HTTPResponse codePairResponse,
        HTTPResponse tokenExchangeResponse,
        HTTPResponse customerProfileResponse);

    /**
     * Initialize the configuration node object.
     *
     * @return An initialized configuration node.
     */
    std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode> createConfig();

    /// Mock Objects
    /// @{
    MockHttpPost* m_httpPost;
    MockHttpGet* m_httpGet;
    std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode> m_configuration;
    std::shared_ptr<avsCommon::utils::DeviceInfo> m_deviceInfo;
    std::shared_ptr<acsdkAuthorizationInterfaces::lwa::LWAAuthorizationStorageInterface> m_storage;
    std::shared_ptr<MockCBLObserver> m_cblObserver;
    std::shared_ptr<MockAuthManager> m_manager;
    /// @}

    /// Create an instance to retrieve constants.
    std::unique_ptr<LWAAuthorizationConfiguration> m_lwaConfig;

    /// Object under test.
    std::shared_ptr<LWAAuthorizationAdapter> m_lwa;

    /// Used to synchronize in multi-thread test cases.
    WaitEvent m_wait;
};

const HTTPResponse LWAAuthorizationAdapterTest::NULL_HTTP_RESPONSE{-1, ""};

void LWAAuthorizationAdapterTest::SetUp() {
    auto httpPost = std::unique_ptr<MockHttpPost>(new NiceMock<MockHttpPost>());
    auto httpGet = std::unique_ptr<MockHttpGet>(new NiceMock<MockHttpGet>());

    // Keep a pointer since we still need to set expectations.
    // This is safe since LWAAuthorizationAdapter keeps the reference alive until destruction.
    m_httpPost = httpPost.get();
    m_httpGet = httpGet.get();

    m_configuration = createConfig();
    m_deviceInfo = DeviceInfo::createFromConfiguration(m_configuration);
    m_storage = std::make_shared<StubStorage>();
    m_cblObserver = std::make_shared<NiceMock<MockCBLObserver>>();
    m_manager = std::make_shared<NiceMock<MockAuthManager>>();
    m_lwaConfig = LWAAuthorizationConfiguration::create(*m_configuration, m_deviceInfo, CONFIG_ROOT_NODE);

    m_lwa = LWAAuthorizationAdapter::create(
        m_configuration, std::move(httpPost), m_deviceInfo, m_storage, std::move(httpGet));
}

std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode> LWAAuthorizationAdapterTest::createConfig() {
    auto json = std::shared_ptr<std::stringstream>(new std::stringstream());
    *json << CONFIG_JSON;
    std::vector<std::shared_ptr<std::istream>> jsonStream;
    jsonStream.push_back(json);
    ConfigurationNode::initialize(jsonStream);

    return ConfigurationNode::createRoot();
}

void LWAAuthorizationAdapterTest::setCBLExpectations(
    HTTPResponse codePairResponse,
    HTTPResponse tokenExchangeResponse,
    HTTPResponse customerProfileResponse) {
    if (NULL_HTTP_RESPONSE.code != codePairResponse.code) {
        EXPECT_CALL(
            *m_httpPost,
            doPost(
                m_lwaConfig->getRequestCodePairUrl(),
                _,
                A<const std::vector<std::pair<std::string, std::string>>&>(),
                _))
            .Times(AnyNumber())
            .WillRepeatedly(InvokeWithoutArgs([codePairResponse] { return codePairResponse; }));
    }

    if (NULL_HTTP_RESPONSE.code != tokenExchangeResponse.code) {
        EXPECT_CALL(
            *m_httpPost,
            doPost(
                m_lwaConfig->getRequestTokenUrl(), _, A<const std::vector<std::pair<std::string, std::string>>&>(), _))
            .Times(AnyNumber())
            .WillRepeatedly(InvokeWithoutArgs([tokenExchangeResponse] { return tokenExchangeResponse; }));
    }

    if (NULL_HTTP_RESPONSE.code != customerProfileResponse.code) {
        EXPECT_CALL(*m_httpGet, doGet(HasSubstr("access_token=" + ACCESS_TOKEN), _))
            .Times(AnyNumber())
            .WillRepeatedly(InvokeWithoutArgs([customerProfileResponse] { return customerProfileResponse; }));
    }
}

void LWAAuthorizationAdapterTest::setCBLExpectations(HTTPResponse customerProfileResponse) {
    setCBLExpectations(CODE_PAIR_RESPONSE, TOKEN_EXCHANGE_RESPONSE, customerProfileResponse);
}

/// Test create with null ptr
TEST_F(LWAAuthorizationAdapterTest, test_create_NullParams_Nullptr) {
    auto httpPost = std::unique_ptr<MockHttpPost>(new NiceMock<MockHttpPost>());
    auto httpGet = std::unique_ptr<MockHttpGet>(new NiceMock<MockHttpGet>());

    EXPECT_THAT(
        LWAAuthorizationAdapter::create(nullptr, std::move(httpPost), m_deviceInfo, m_storage, std::move(httpGet)),
        IsNull());

    httpPost = std::unique_ptr<MockHttpPost>(new NiceMock<MockHttpPost>());
    httpGet = std::unique_ptr<MockHttpGet>(new NiceMock<MockHttpGet>());

    EXPECT_THAT(
        LWAAuthorizationAdapter::create(m_configuration, nullptr, m_deviceInfo, m_storage, std::move(httpGet)),
        IsNull());

    httpPost = std::unique_ptr<MockHttpPost>(new NiceMock<MockHttpPost>());
    httpGet = std::unique_ptr<MockHttpGet>(new NiceMock<MockHttpGet>());

    EXPECT_THAT(
        LWAAuthorizationAdapter::create(m_configuration, std::move(httpPost), nullptr, m_storage, std::move(httpGet)),
        IsNull());

    httpPost = std::unique_ptr<MockHttpPost>(new NiceMock<MockHttpPost>());
    httpGet = std::unique_ptr<MockHttpGet>(new NiceMock<MockHttpGet>());

    EXPECT_THAT(
        LWAAuthorizationAdapter::create(
            m_configuration, std::move(httpPost), m_deviceInfo, nullptr, std::move(httpGet)),
        IsNull());
}

/// Check the default value for the adapter id both directly and via getAuthorizationInterface.
TEST_F(LWAAuthorizationAdapterTest, test_id_DefaultValue) {
    EXPECT_EQ(DEFAULT_ADAPTER_ID, m_lwa->getId());
    EXPECT_EQ(DEFAULT_ADAPTER_ID, m_lwa->getAuthorizationInterface()->getId());
}

/// Check that no token is returned when not authorized.
TEST_F(LWAAuthorizationAdapterTest, test_getAuthTokenNoAuth_Fails) {
    EXPECT_THAT(m_lwa->getAuthToken(), IsEmpty());
}

/// Check the custom value for the adapter id both directly and via getAuthorizationInterface.
TEST_F(LWAAuthorizationAdapterTest, test_id_CustomValue) {
    auto httpPost = std::unique_ptr<NiceMock<MockHttpPost>>(new NiceMock<MockHttpPost>());
    auto httpGet = std::unique_ptr<NiceMock<MockHttpGet>>(new NiceMock<MockHttpGet>());

    const std::string NEW_ID = "new-id";

    auto lwa = LWAAuthorizationAdapter::create(
        m_configuration, std::move(httpPost), m_deviceInfo, m_storage, std::move(httpGet), NEW_ID);

    EXPECT_EQ(NEW_ID, lwa->getId());
    EXPECT_EQ(NEW_ID, lwa->getAuthorizationInterface()->getId());
}

/// Check that authorizing without an AuthManager fails.
TEST_F(LWAAuthorizationAdapterTest, test_authorize_NoAuthMgr_Fails) {
    EXPECT_FALSE(m_lwa->authorizeUsingCBL(m_cblObserver));
    EXPECT_FALSE(m_lwa->authorizeUsingCBLWithCustomerProfile(m_cblObserver));
}

/// Check that multiple authorization requests fail. Only one reportStateChange call for each state is expected.
TEST_F(LWAAuthorizationAdapterTest, test_multipleCBLAuthorization_Fails) {
    setCBLExpectations(CUSTOMER_PROFILE_RESPONSE);

    {
        InSequence s;

        EXPECT_CALL(
            *m_manager,
            reportStateChange(
                AuthObserverInterface::FullState(
                    AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS),
                DEFAULT_ADAPTER_ID,
                ""));

        EXPECT_CALL(
            *m_manager,
            reportStateChange(
                AuthObserverInterface::FullState(
                    AuthObserverInterface::State::REFRESHED, AuthObserverInterface::Error::SUCCESS),
                DEFAULT_ADAPTER_ID,
                USER_ID))
            .WillOnce(InvokeWithoutArgs([this] { m_wait.wakeUp(); }));
    }

    m_lwa->onAuthorizationManagerReady(m_manager);
    EXPECT_TRUE(m_lwa->authorizeUsingCBLWithCustomerProfile(m_cblObserver));
    EXPECT_TRUE(m_wait.wait(TIMEOUT));

    EXPECT_FALSE(m_lwa->authorizeUsingCBL(m_cblObserver));
    EXPECT_FALSE(m_lwa->authorizeUsingCBLWithCustomerProfile(m_cblObserver));
}

/// Check that the retry logic for CBL code pair requests is successful.
TEST_F(LWAAuthorizationAdapterTest, test_cblCodePairRetry_Succeeds) {
    setCBLExpectations(NULL_HTTP_RESPONSE, TOKEN_EXCHANGE_RESPONSE, CUSTOMER_PROFILE_SHORT_RESPONSE);

    EXPECT_CALL(
        *m_httpPost,
        doPost(
            m_lwaConfig->getRequestCodePairUrl(), _, A<const std::vector<std::pair<std::string, std::string>>&>(), _))
        .Times(AnyNumber())
        .WillOnce(InvokeWithoutArgs([] {
            auto response = HTTPResponse();
            response.code = HTTPResponseCode::SERVER_ERROR_INTERNAL;
            return response;
        }))
        .WillRepeatedly(InvokeWithoutArgs([this] {
            m_wait.wakeUp();
            return CODE_PAIR_RESPONSE;
        }));

    m_lwa->onAuthorizationManagerReady(m_manager);
    EXPECT_TRUE(m_lwa->authorizeUsingCBL(m_cblObserver));
    EXPECT_TRUE(m_wait.wait(TIMEOUT));
}

/// Check that the retry logic for CBL token exchange requests is successful.
TEST_F(LWAAuthorizationAdapterTest, test_cblTokenExchangeRetry_Succeeds) {
    setCBLExpectations(CODE_PAIR_RESPONSE, NULL_HTTP_RESPONSE, CUSTOMER_PROFILE_SHORT_RESPONSE);

    EXPECT_CALL(
        *m_httpPost,
        doPost(m_lwaConfig->getRequestTokenUrl(), _, A<const std::vector<std::pair<std::string, std::string>>&>(), _))
        .Times(AnyNumber())
        .WillOnce(InvokeWithoutArgs([] { return HTTPResponse(HTTPResponseCode::SERVER_ERROR_INTERNAL, ""); }))
        .WillRepeatedly(InvokeWithoutArgs([this] {
            m_wait.wakeUp();
            return TOKEN_EXCHANGE_RESPONSE;
        }));

    m_lwa->onAuthorizationManagerReady(m_manager);
    EXPECT_TRUE(m_lwa->authorizeUsingCBL(m_cblObserver));
    EXPECT_TRUE(m_wait.wait(LONG_TIMEOUT));
}

/// Check that on auth failure, a retry is triggered.
TEST_F(LWAAuthorizationAdapterTest, test_authFailure_TriggersRetry) {
    WaitEvent tokenExchangeRequestWait, onAuthFailureProcessedWait;
    {
        InSequence is;

        EXPECT_CALL(
            *m_httpPost,
            doPost(
                m_lwaConfig->getRequestCodePairUrl(),
                _,
                A<const std::vector<std::pair<std::string, std::string>>&>(),
                _))
            .Times(AnyNumber())
            .WillRepeatedly(InvokeWithoutArgs([] { return CODE_PAIR_RESPONSE; }));

        EXPECT_CALL(
            *m_httpPost,
            doPost(
                m_lwaConfig->getRequestTokenUrl(), _, A<const std::vector<std::pair<std::string, std::string>>&>(), _))
            .Times(AnyNumber())
            .WillOnce(InvokeWithoutArgs([&tokenExchangeRequestWait] {
                tokenExchangeRequestWait.wakeUp();
                return TOKEN_EXCHANGE_RESPONSE;
            }))
            // This second expectation is in response to an onAuthFailure() call.
            .WillRepeatedly(InvokeWithoutArgs([&onAuthFailureProcessedWait] {
                onAuthFailureProcessedWait.wakeUp();
                return TOKEN_EXCHANGE_RESPONSE;
            }));
    }

    m_lwa->onAuthorizationManagerReady(m_manager);
    EXPECT_TRUE(m_lwa->authorizeUsingCBL(m_cblObserver));
    EXPECT_TRUE(tokenExchangeRequestWait.wait(TIMEOUT));
    m_lwa->onAuthFailure(ACCESS_TOKEN);
    EXPECT_TRUE(onAuthFailureProcessedWait.wait(TIMEOUT));
}

/// Check that getState returns the correct state.
TEST_F(LWAAuthorizationAdapterTest, test_getState_Succeeds) {
    setCBLExpectations(CUSTOMER_PROFILE_SHORT_RESPONSE);

    {
        InSequence s;

        EXPECT_CALL(
            *m_manager,
            reportStateChange(
                AuthObserverInterface::FullState(
                    AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS),
                DEFAULT_ADAPTER_ID,
                ""));

        EXPECT_CALL(
            *m_manager,
            reportStateChange(
                AuthObserverInterface::FullState(
                    AuthObserverInterface::State::REFRESHED, AuthObserverInterface::Error::SUCCESS),
                DEFAULT_ADAPTER_ID,
                USER_ID))
            .WillRepeatedly(InvokeWithoutArgs([this] { m_wait.wakeUp(); }));

        EXPECT_THAT(
            m_lwa->getState(),
            AuthObserverInterface::FullState(
                AuthObserverInterface::State::UNINITIALIZED, AuthObserverInterface::Error::SUCCESS));
    }

    m_lwa->onAuthorizationManagerReady(m_manager);
    EXPECT_TRUE(m_lwa->authorizeUsingCBL(m_cblObserver));

    EXPECT_TRUE(m_wait.wait(TIMEOUT));

    EXPECT_THAT(
        m_lwa->getState(),
        AuthObserverInterface::FullState(
            AuthObserverInterface::State::REFRESHED, AuthObserverInterface::Error::SUCCESS));
}

/// Check that reset operations are successful.
TEST_F(LWAAuthorizationAdapterTest, test_reset_Succeeds) {
    setCBLExpectations(CUSTOMER_PROFILE_SHORT_RESPONSE);

    {
        InSequence s;

        EXPECT_CALL(
            *m_manager,
            reportStateChange(
                AuthObserverInterface::FullState(
                    AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS),
                DEFAULT_ADAPTER_ID,
                ""));

        EXPECT_CALL(
            *m_manager,
            reportStateChange(
                AuthObserverInterface::FullState(
                    AuthObserverInterface::State::REFRESHED, AuthObserverInterface::Error::SUCCESS),
                DEFAULT_ADAPTER_ID,
                USER_ID))
            .WillOnce(InvokeWithoutArgs([this] { m_wait.wakeUp(); }));

        EXPECT_CALL(
            *m_manager,
            reportStateChange(
                AuthObserverInterface::FullState(
                    AuthObserverInterface::State::UNINITIALIZED, AuthObserverInterface::Error::SUCCESS),
                DEFAULT_ADAPTER_ID,
                ""))
            .WillOnce(InvokeWithoutArgs([this] { m_wait.wakeUp(); }));
    }

    m_lwa->onAuthorizationManagerReady(m_manager);
    EXPECT_TRUE(m_lwa->authorizeUsingCBL(m_cblObserver));
    EXPECT_TRUE(m_wait.wait(TIMEOUT));
    m_wait.reset();

    m_lwa->reset();
    EXPECT_TRUE(m_wait.wait(TIMEOUT));

    std::string temp;
    EXPECT_FALSE(m_storage->getRefreshToken(&temp));
    EXPECT_FALSE(m_storage->getUserId(&temp));
}

/// Check that the LWA token refreshing logic is successful.
TEST_F(LWAAuthorizationAdapterTest, test_refreshing_succeeds) {
    setCBLExpectations(CODE_PAIR_RESPONSE, LWAAuthorizationAdapterTest::NULL_HTTP_RESPONSE, CUSTOMER_PROFILE_RESPONSE);

    EXPECT_CALL(
        *m_httpPost,
        doPost(m_lwaConfig->getRequestTokenUrl(), _, A<const std::vector<std::pair<std::string, std::string>>&>(), _))
        .Times(AtLeast(2))
        .WillOnce(InvokeWithoutArgs([] {
            // Create a response that will expire immediately to force a refresh.
            auto response = HTTPResponse();
            response.code = HTTPResponseCode::SUCCESS_OK;
            // clang-format off
            response.body = R"(
                {
                    "access_token": ")" +
                            ACCESS_TOKEN + R"(",
                    "refresh_token": ")" +
                            REFRESH_TOKEN + R"(",
                    "token_type": ")" +
                            TOKEN_TYPE + R"(",
                    "expires_in": )" +
                            "1" + R"(
                })";
            // clang-format on
            return response;
        }))
        .WillRepeatedly(InvokeWithoutArgs([] { return TOKEN_EXCHANGE_RESPONSE; }));

    {
        InSequence s;

        EXPECT_CALL(
            *m_manager,
            reportStateChange(
                AuthObserverInterface::FullState(
                    AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS),
                DEFAULT_ADAPTER_ID,
                ""));

        EXPECT_CALL(
            *m_manager,
            reportStateChange(
                AuthObserverInterface::FullState(
                    AuthObserverInterface::State::REFRESHED, AuthObserverInterface::Error::SUCCESS),
                DEFAULT_ADAPTER_ID,
                USER_ID))
            .Times(AtLeast(1))
            .WillRepeatedly(InvokeWithoutArgs([this] { m_wait.wakeUp(); }));
    }

    m_lwa->onAuthorizationManagerReady(m_manager);
    EXPECT_TRUE(m_lwa->authorizeUsingCBL(m_cblObserver));
    EXPECT_TRUE(m_wait.wait(TIMEOUT));
}

/// Test resetting while waiting for code pair requests.
TEST_F(LWAAuthorizationAdapterTest, test_reset_CodePair) {
    EXPECT_CALL(
        *m_httpPost,
        doPost(
            m_lwaConfig->getRequestCodePairUrl(), _, A<const std::vector<std::pair<std::string, std::string>>&>(), _))
        .Times(AnyNumber())
        .WillRepeatedly(InvokeWithoutArgs([this] {
            auto response = HTTPResponse();
            response.code = HTTPResponseCode::SERVER_UNAVAILABLE;

            m_wait.wakeUp();

            return response;
        }));

    {
        InSequence s;

        EXPECT_CALL(
            *m_manager,
            reportStateChange(
                AuthObserverInterface::FullState(
                    AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS),
                DEFAULT_ADAPTER_ID,
                ""));

        EXPECT_CALL(
            *m_manager,
            reportStateChange(
                AuthObserverInterface::FullState(
                    AuthObserverInterface::State::UNINITIALIZED, AuthObserverInterface::Error::SUCCESS),
                DEFAULT_ADAPTER_ID,
                ""))
            .Times(AtLeast(1))
            .WillRepeatedly(InvokeWithoutArgs([this] { m_wait.wakeUp(); }));
    }

    m_lwa->onAuthorizationManagerReady(m_manager);
    EXPECT_TRUE(m_lwa->authorizeUsingCBL(m_cblObserver));
    EXPECT_TRUE(m_wait.wait(TIMEOUT));
    m_wait.reset();
    m_lwa->reset();
    EXPECT_TRUE(m_wait.wait(TIMEOUT));
}

/// Test resetting while waiting for token exchange requests.
TEST_F(LWAAuthorizationAdapterTest, test_reset_TokenExchange) {
    EXPECT_CALL(
        *m_httpPost,
        doPost(
            m_lwaConfig->getRequestCodePairUrl(), _, A<const std::vector<std::pair<std::string, std::string>>&>(), _))
        .Times(AnyNumber())
        .WillRepeatedly(InvokeWithoutArgs([this] {
            m_wait.wakeUp();

            return CODE_PAIR_RESPONSE;
        }));

    EXPECT_CALL(
        *m_httpPost,
        doPost(m_lwaConfig->getRequestTokenUrl(), _, A<const std::vector<std::pair<std::string, std::string>>&>(), _))
        .Times(AnyNumber())
        .WillOnce(InvokeWithoutArgs([] {
            auto response = HTTPResponse();
            response.code = HTTPResponseCode::SERVER_UNAVAILABLE;

            return response;
        }));

    {
        InSequence s;

        EXPECT_CALL(
            *m_manager,
            reportStateChange(
                AuthObserverInterface::FullState(
                    AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS),
                DEFAULT_ADAPTER_ID,
                ""));

        EXPECT_CALL(
            *m_manager,
            reportStateChange(
                AuthObserverInterface::FullState(
                    AuthObserverInterface::State::UNINITIALIZED, AuthObserverInterface::Error::SUCCESS),
                DEFAULT_ADAPTER_ID,
                ""))
            .Times(AtLeast(1))
            .WillRepeatedly(InvokeWithoutArgs([this] { m_wait.wakeUp(); }));
    }

    m_lwa->onAuthorizationManagerReady(m_manager);
    EXPECT_TRUE(m_lwa->authorizeUsingCBL(m_cblObserver));
    EXPECT_TRUE(m_wait.wait(TIMEOUT));
    m_wait.reset();
    m_lwa->reset();
    EXPECT_TRUE(m_wait.wait(TIMEOUT));
}

/// Parameterized Tests Exercising both authorizeUsingCBL and authorizeUsingCBLWithCustomerProfile
INSTANTIATE_TEST_CASE_P(
    Parameterized,
    LWAAuthorizationAdapterTest,
    ::testing::Values(AUTHORIZE_USING_CBL, AUTHORIZE_USING_CBL_WITH_CUSTOMER_PROFILE));

/// Test that the happy case succeeds.
TEST_P(LWAAuthorizationAdapterTest, test_cblAuthorize_Succeeds) {
    const std::string cblMethod = GetParam();

    EXPECT_CALL(*m_cblObserver, onRequestAuthorization(VERIFICATION_URI, USER_CODE));
    EXPECT_CALL(*m_cblObserver, onCheckingForAuthorization()).Times(AtLeast(1));

    std::function<bool(
        const std::shared_ptr<acsdkAuthorizationInterfaces::lwa::CBLAuthorizationObserverInterface>& observer)>
        cblAuthFunc;

    if (AUTHORIZE_USING_CBL == cblMethod) {
        setCBLExpectations(CUSTOMER_PROFILE_SHORT_RESPONSE);

        cblAuthFunc = std::bind(&LWAAuthorizationAdapter::authorizeUsingCBL, m_lwa, std::placeholders::_1);
    } else if (AUTHORIZE_USING_CBL_WITH_CUSTOMER_PROFILE == cblMethod) {
        EXPECT_CALL(
            *m_cblObserver, onCustomerProfileAvailable(CBLAuthorizationObserverInterface::CustomerProfile(NAME, EMAIL)))
            .Times(AtLeast(1));
        setCBLExpectations(CUSTOMER_PROFILE_RESPONSE);

        cblAuthFunc =
            std::bind(&LWAAuthorizationAdapter::authorizeUsingCBLWithCustomerProfile, m_lwa, std::placeholders::_1);
    } else {
        FAIL();
    }

    {
        InSequence s;

        EXPECT_CALL(
            *m_manager,
            reportStateChange(
                AuthObserverInterface::FullState(
                    AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS),
                DEFAULT_ADAPTER_ID,
                ""));
        EXPECT_CALL(
            *m_manager,
            reportStateChange(
                AuthObserverInterface::FullState(
                    AuthObserverInterface::State::REFRESHED, AuthObserverInterface::Error::SUCCESS),
                DEFAULT_ADAPTER_ID,
                USER_ID))
            .WillOnce(InvokeWithoutArgs([this] { m_wait.wakeUp(); }));
    }

    m_lwa->onAuthorizationManagerReady(m_manager);

    // Function succeds and token is retrievable.
    EXPECT_TRUE(cblAuthFunc(m_cblObserver));
    EXPECT_TRUE(m_wait.wait(TIMEOUT));
    EXPECT_EQ(m_lwa->getAuthToken(), ACCESS_TOKEN);

    // Check that the storage states are correct.
    std::string token;
    EXPECT_TRUE(m_storage->getRefreshToken(&token));
    EXPECT_EQ(REFRESH_TOKEN, token);

    std::string id;
    EXPECT_TRUE(m_storage->getUserId(&id));
    EXPECT_EQ(USER_ID, id);
}

/// Check that authorizeUsingCBL<WithCustomerProfile> is requesting with the correct scopes.
TEST_P(LWAAuthorizationAdapterTest, test_cblAuthorize_CorrectScopes) {
    const std::string cblMethod = GetParam();

    std::string scopes;
    std::function<bool(
        const std::shared_ptr<acsdkAuthorizationInterfaces::lwa::CBLAuthorizationObserverInterface>& observer)>
        cblAuthFunc;

    if (AUTHORIZE_USING_CBL == cblMethod) {
        setCBLExpectations(NULL_HTTP_RESPONSE, TOKEN_EXCHANGE_RESPONSE, CUSTOMER_PROFILE_SHORT_RESPONSE);

        cblAuthFunc = std::bind(&LWAAuthorizationAdapter::authorizeUsingCBL, m_lwa, std::placeholders::_1);
        scopes = "alexa:all profile:user_id";
    } else if (AUTHORIZE_USING_CBL_WITH_CUSTOMER_PROFILE == cblMethod) {
        setCBLExpectations(NULL_HTTP_RESPONSE, TOKEN_EXCHANGE_RESPONSE, CUSTOMER_PROFILE_RESPONSE);

        cblAuthFunc =
            std::bind(&LWAAuthorizationAdapter::authorizeUsingCBLWithCustomerProfile, m_lwa, std::placeholders::_1);
        scopes = "alexa:all profile";
    } else {
        FAIL();
    }

    EXPECT_CALL(
        *m_httpPost,
        doPost(
            m_lwaConfig->getRequestCodePairUrl(), _, A<const std::vector<std::pair<std::string, std::string>>&>(), _))
        .Times(AnyNumber())
        .WillRepeatedly(Invoke([this, scopes](
                                   const std::string& url,
                                   const std::vector<std::string> headerLines,
                                   const std::vector<std::pair<std::string, std::string>>& data,
                                   std::chrono::seconds timeout) {
            auto expectedScopes = std::pair<std::string, std::string>("scope", scopes);
            EXPECT_THAT(data, Contains(expectedScopes));

            m_wait.wakeUp();
            return CODE_PAIR_RESPONSE;
        }));

    m_lwa->onAuthorizationManagerReady(m_manager);
    EXPECT_TRUE(cblAuthFunc(m_cblObserver));
    EXPECT_TRUE(m_wait.wait(TIMEOUT));
}

}  // namespace test
}  // namespace lwa
}  // namespace acsdkAuthorization
}  // namespace alexaClientSDK
