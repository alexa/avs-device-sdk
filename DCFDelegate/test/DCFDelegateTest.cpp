/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <DCFDelegate/DCFDelegate.h>

#include <gtest/gtest.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpResponseCodes.h>
#include <Integration/TestAuthDelegate.h>
#include <Integration/TestCapabilityProvider.h>
#include <Integration/TestHttpPut.h>
#include <Integration/TestMiscStorage.h>

namespace alexaClientSDK {
namespace dcfDelegate {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::avs::initialization;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::libcurlUtils;
using namespace integration::test;

/// Auth token
static const std::string AUTH_TOKEN = "testAuthToken";
/// Client Id
static const std::string CLIENT_ID = "testClientId";
/// Client Id 2
static const std::string CLIENT_ID_TWO = "testClientId2";
/// Product Id
static const std::string PRODUCT_ID = "testProductId";
/// Product Id 2
static const std::string PRODUCT_ID_TWO = "testProductId2";
/// DSN
static const std::string DSN = "testDSN";
/// DSN 2
static const std::string DSN_TWO = "testDSN2";
/// Separator between the components that make up a capability key
static const std::string CAPABILITY_KEY_SEPARATOR = ".";
/// Capabilities key in message body
static const std::string CAPABILITIES_KEY = "capabilities";
/// Envelope version key in message body
static const std::string ENVELOPE_VERSION_KEY = "envelopeVersion";
/// Envelope version value in message body
static const std::string ENVELOPE_VERSION_VALUE = "20160207";
/// Envelope version value 2
static const std::string ENVELOPE_VERSION_VALUE_TWO = "201602072";
/// Interface type
static const std::string INTERFACE_TYPE = "testInterfaceType";
/// Interface version
static const std::string INTERFACE_VERSION = "testInterfaceVersion";
/// Interface configuration
static const std::string INTERFACE_CONFIG = "{\"config\":\"testConfig\"}";
/// Bad interface configuration
static const std::string INTERFACE_CONFIG_BAD = "thisIsNotJson";
/// Interface name one
static const std::string INTERFACE_NAME_ONE = "testInterfaceNameOne";
/// Interface name two
static const std::string INTERFACE_NAME_TWO = "testInterfaceNameTwo";
/// Interface name three
static const std::string INTERFACE_NAME_THREE = "testInterfaceNameThree";

/// Constants for the DCF publish message json
/// Content type header key
static const std::string CONTENT_TYPE_HEADER_KEY = "Content-Type";
/// Content type header value
static const std::string CONTENT_TYPE_HEADER_VALUE = "application/json";
/// Content length header key
static const std::string CONTENT_LENGTH_HEADER_KEY = "Content-Length";
/// Auth token header key
static const std::string AUTHORIZATION_HEADER_KEY = "x-amz-access-token";
/// Separator between header key and value
static const std::string HEADER_KEY_VALUE_SEPARATOR = ":";

/// Constituents of the DCF URL
/// DCF endpoint
static const std::string DCF_ENDPOINT = "https://api.amazonalexa.com";
/// Suffix before the device's place in the URL
static const std::string DCF_URL_PRE_DEVICE_SUFFIX = "/v1/devices/";
/// Suffix after the device's place in the URL
static const std::string DCF_URL_POST_DEVICE_SUFFIX = "/capabilities";
/// Device ID for the device that will show up in the URL
static const std::string SELF_DEVICE = "@self";

/// Constants for prefixes for DB storage
/// Endpoint key
static const std::string DB_KEY_ENDPOINT = "endpoint:";
/// Client id key
static const std::string DB_KEY_CLIENT_ID = "clientId:";
/// Product id key
static const std::string DB_KEY_PRODUCT_ID = "productId:";
/// Envelope version key
static const std::string DB_KEY_ENVELOPE_VERSION = "envelopeVersion:";
/// DSN key
static const std::string DB_KEY_DSN = "deviceSerialNumber:";
/// Message key
static const std::string DB_KEY_PUBLISH_MSG = "publishMsg:";
/// Separator between keys
static const std::string DB_KEY_SEPARATOR = ",";
/// Component key
static const std::string DB_KEY_COMPONENT = "component:";
/// Table key
static const std::string DB_KEY_TABLE = "table:";
/// Component name needed for Misc DB
static const std::string COMPONENT_NAME = "dcfDelegate";
/// DCF Publish message table
static const std::string DCF_PUBLISH_TABLE = "dcfPublishMessage";

// clang-format off
static const std::string DCF_CONFIG_JSON =
    "{"
      "\"dcfDelegate\":{"
        "\"randomKey\":\"randomValue\""
        "}"
    "}";
// clang-format on

/**
 * Test harness for @c DCFDelegate class.
 */
class DCFDelegateTest : public ::testing::Test {
public:
    /**
     * Constructor.
     */
    DCFDelegateTest();

    /**
     * Destructor.
     */
    ~DCFDelegateTest();

    /**
     * Set up the test harness for running a test.
     */
    void SetUp() override;

    /**
     * Tear down the test harness for running a test.
     */
    void TearDown() override;

    /**
     * Returns a capability's unique key.
     *
     * @param capabilityMap The capability map.
     * @return A key that uniquely represents a capability.
     */
    std::string getCapabilityKey(const std::unordered_map<std::string, std::string>& capabilityMap);

    /**
     * Returns a capability's unique key.
     *
     * @param interfaceType The interface type of the capability.
     * @param interfaceName The interface name of the capability.
     * @return A key that uniquely represents a capability.
     */
    std::string getCapabilityKey(const std::string& interfaceType, const std::string& interfaceName);

    /**
     * Returns the URL to which the DCF publish message is sent.
     *
     * @param deviceId The device Id.
     * @return The URL to which the DCF publish message is sent.
     */
    std::string getDcfUrl(const std::string& deviceId);

    /**
     * Returns the published envelope version given the published DCF message.
     *
     * @param publishedMsgStr The published message.
     * @return The published envelope version.
     */
    std::string getPublishedEnvelopeVersion(const std::string& publishedMsgStr);

    /**
     * Returns the published capabilities given the published DCF message.
     *
     * @param publishedMsgStr The published message.
     * @return A map of the capability key and the capability configuration.
     */
    std::unordered_map<std::string, std::shared_ptr<CapabilityConfiguration>> getPublishedConfigs(
        const std::string& publishedMsgStr);

protected:
    /// Auth delegate instance
    std::shared_ptr<TestAuthDelegate> m_authDelegate;
    /// Misc storage instance
    std::shared_ptr<TestMiscStorage> m_miscStorage;
    /// HTTP put handler
    std::shared_ptr<TestHttpPut> m_httpPut;
    /// Device Info instance
    std::shared_ptr<DeviceInfo> m_deviceInfo;
    /// Config instance
    ConfigurationNode m_configRoot;
    /// The DCF delegate instance
    std::shared_ptr<DCFDelegate> m_dcfDelegate;
};

DCFDelegateTest::DCFDelegateTest() :
        m_authDelegate{std::make_shared<TestAuthDelegate>()},
        m_miscStorage{std::make_shared<TestMiscStorage>()},
        m_httpPut{std::make_shared<TestHttpPut>()},
        m_deviceInfo{DeviceInfo::create(CLIENT_ID, PRODUCT_ID, DSN)} {
    auto inString = std::shared_ptr<std::istringstream>(new std::istringstream(DCF_CONFIG_JSON));
    AlexaClientSDKInit::initialize({inString});
    m_configRoot = ConfigurationNode::getRoot();

    m_authDelegate->setAuthToken(AUTH_TOKEN);
}

DCFDelegateTest::~DCFDelegateTest() {
    AlexaClientSDKInit::uninitialize();
}

void DCFDelegateTest::SetUp() {
    m_dcfDelegate = DCFDelegate::create(m_authDelegate, m_miscStorage, m_httpPut, m_configRoot, m_deviceInfo);
    ASSERT_NE(m_dcfDelegate, nullptr);
    m_dcfDelegate->onAuthStateChange(AuthObserverInterface::State::REFRESHED, AuthObserverInterface::Error::SUCCESS);
}

void DCFDelegateTest::TearDown() {
    m_dcfDelegate->shutdown();
}

std::string DCFDelegateTest::getDcfUrl(const std::string& deviceId) {
    return DCF_ENDPOINT + DCF_URL_PRE_DEVICE_SUFFIX + deviceId + DCF_URL_POST_DEVICE_SUFFIX;
}

std::string DCFDelegateTest::getCapabilityKey(const std::unordered_map<std::string, std::string>& capabilityMap) {
    auto interfaceTypeIterator = capabilityMap.find(CAPABILITY_INTERFACE_TYPE_KEY);
    if (interfaceTypeIterator == capabilityMap.end()) {
        return "";
    }
    auto interfaceType = interfaceTypeIterator->second;

    auto interfaceNameIterator = capabilityMap.find(CAPABILITY_INTERFACE_NAME_KEY);
    if (interfaceNameIterator == capabilityMap.end()) {
        return "";
    }
    auto interfaceName = interfaceNameIterator->second;

    return getCapabilityKey(interfaceType, interfaceName);
}

std::string DCFDelegateTest::getCapabilityKey(const std::string& interfaceType, const std::string& interfaceName) {
    return interfaceType + CAPABILITY_KEY_SEPARATOR + interfaceName;
}

std::string DCFDelegateTest::getPublishedEnvelopeVersion(const std::string& publishedMsgStr) {
    rapidjson::Document publishedMsgJson;
    publishedMsgJson.Parse(publishedMsgStr);

    return publishedMsgJson[ENVELOPE_VERSION_KEY].GetString();
}

std::unordered_map<std::string, std::shared_ptr<CapabilityConfiguration>> DCFDelegateTest::getPublishedConfigs(
    const std::string& publishedMsgStr) {
    std::unordered_map<std::string, std::shared_ptr<CapabilityConfiguration>> publishedConfigs;

    rapidjson::Document publishedMsgJson;
    publishedMsgJson.Parse(publishedMsgStr);

    for (rapidjson::SizeType capabilityIndx = 0; capabilityIndx < publishedMsgJson[CAPABILITIES_KEY].Size();
         capabilityIndx++) {
        const rapidjson::Value& capabilityJson = publishedMsgJson[CAPABILITIES_KEY][capabilityIndx];
        std::unordered_map<std::string, std::string> capabilityMap;

        std::string capabilityType = (capabilityJson.FindMember(CAPABILITY_INTERFACE_TYPE_KEY))->value.GetString();
        capabilityMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, capabilityType});

        std::string capabilityName = (capabilityJson.FindMember(CAPABILITY_INTERFACE_NAME_KEY))->value.GetString();
        capabilityMap.insert({CAPABILITY_INTERFACE_NAME_KEY, capabilityName});

        std::string capabilityVersion =
            (capabilityJson.FindMember(CAPABILITY_INTERFACE_VERSION_KEY))->value.GetString();
        capabilityMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, capabilityVersion});

        if (capabilityJson.HasMember(CAPABILITY_INTERFACE_CONFIGURATIONS_KEY)) {
            std::string capabilityConfigs =
                (capabilityJson.FindMember(CAPABILITY_INTERFACE_CONFIGURATIONS_KEY))->value.GetString();
            capabilityMap.insert({CAPABILITY_INTERFACE_CONFIGURATIONS_KEY, capabilityConfigs});
        }

        std::string capabilityKey = getCapabilityKey(capabilityMap);
        publishedConfigs.insert({capabilityKey, std::make_shared<CapabilityConfiguration>(capabilityMap)});
    }

    return publishedConfigs;
}

/// Test publishing no capabilities
TEST_F(DCFDelegateTest, noCapability) {
    ASSERT_EQ(m_dcfDelegate->publishCapabilities(), DCFDelegate::DCFPublishReturnCode::FATAL_ERROR);
    ASSERT_TRUE(m_httpPut->getRequestData().empty());
    ASSERT_FALSE(m_miscStorage->dataExists(COMPONENT_NAME, DCF_PUBLISH_TABLE));
}

/// Test publishing capabilities with no errors
TEST_F(DCFDelegateTest, withCapabilitiesHappyCase) {
    std::shared_ptr<TestCapabilityProvider> capabilityProviderOne = std::make_shared<TestCapabilityProvider>();
    capabilityProviderOne->addCapabilityConfiguration(INTERFACE_TYPE, INTERFACE_NAME_ONE, INTERFACE_VERSION);
    capabilityProviderOne->addCapabilityConfiguration(INTERFACE_TYPE, INTERFACE_NAME_THREE, INTERFACE_VERSION);

    std::shared_ptr<TestCapabilityProvider> capabilityProviderTwo = std::make_shared<TestCapabilityProvider>();
    capabilityProviderTwo->addCapabilityConfiguration(
        INTERFACE_TYPE, INTERFACE_NAME_TWO, INTERFACE_VERSION, INTERFACE_CONFIG);

    ASSERT_TRUE(m_dcfDelegate->registerCapability(capabilityProviderOne));
    ASSERT_TRUE(m_dcfDelegate->registerCapability(capabilityProviderTwo));
    m_httpPut->setResponseCode(HTTPResponseCode::SUCCESS_NO_CONTENT);  /// Success
    ASSERT_EQ(m_dcfDelegate->publishCapabilities(), DCFDelegate::DCFPublishReturnCode::SUCCESS);
    ASSERT_TRUE(m_miscStorage->dataExists(COMPONENT_NAME, DCF_PUBLISH_TABLE));

    /// Check URL
    ASSERT_EQ(m_httpPut->getRequestUrl(), getDcfUrl(SELF_DEVICE));

    /// Check the body of the HTTP request
    const std::string publishedMsg = m_httpPut->getRequestData();
    std::unordered_map<std::string, std::shared_ptr<CapabilityConfiguration>> publishedConfigs =
        getPublishedConfigs(publishedMsg);
    std::string publishedEnvelopeVersion = getPublishedEnvelopeVersion(publishedMsg);

    /// Check envelope version
    ASSERT_EQ(publishedEnvelopeVersion, ENVELOPE_VERSION_VALUE);

    /// Check capabilities
    /// Total of 3 capabilities = 2 capabilties from capabilityProviderOne and 1 from capabilityProviderTwo
    size_t numOfCapabilties = 3;
    ASSERT_EQ(publishedConfigs.size(), numOfCapabilties);

    /// Check if all capabilities in the publish message are what we sent out
    /// First capability
    std::string capabilityKey = getCapabilityKey(INTERFACE_TYPE, INTERFACE_NAME_ONE);
    auto capabilityIterator = publishedConfigs.find(capabilityKey);
    ASSERT_NE(capabilityIterator, publishedConfigs.end());
    std::unordered_map<std::string, std::string> capabilityConfigMap =
        (capabilityIterator->second)->capabilityConfiguration;
    auto capabilityConfigIterator = capabilityConfigMap.find(CAPABILITY_INTERFACE_VERSION_KEY);
    ASSERT_NE(capabilityConfigIterator, capabilityConfigMap.end());
    ASSERT_EQ(capabilityConfigIterator->second, INTERFACE_VERSION);

    /// Second capability
    capabilityKey = getCapabilityKey(INTERFACE_TYPE, INTERFACE_NAME_TWO);
    capabilityIterator = publishedConfigs.find(capabilityKey);
    ASSERT_NE(capabilityIterator, publishedConfigs.end());
    capabilityConfigMap = (capabilityIterator->second)->capabilityConfiguration;
    capabilityConfigIterator = capabilityConfigMap.find(CAPABILITY_INTERFACE_VERSION_KEY);
    ASSERT_NE(capabilityConfigIterator, capabilityConfigMap.end());
    ASSERT_EQ(capabilityConfigIterator->second, INTERFACE_VERSION);
    capabilityConfigIterator = capabilityConfigMap.find(CAPABILITY_INTERFACE_CONFIGURATIONS_KEY);
    ASSERT_NE(capabilityConfigIterator, capabilityConfigMap.end());
    ASSERT_EQ(capabilityConfigIterator->second, INTERFACE_CONFIG);

    /// Third capability
    capabilityKey = getCapabilityKey(INTERFACE_TYPE, INTERFACE_NAME_THREE);
    capabilityIterator = publishedConfigs.find(capabilityKey);
    ASSERT_NE(capabilityIterator, publishedConfigs.end());
    capabilityConfigMap = (capabilityIterator->second)->capabilityConfiguration;
    capabilityConfigIterator = capabilityConfigMap.find(CAPABILITY_INTERFACE_VERSION_KEY);
    ASSERT_NE(capabilityConfigIterator, capabilityConfigMap.end());
    ASSERT_EQ(capabilityConfigIterator->second, INTERFACE_VERSION);

    /// Check the HTTP headers
    std::vector<std::string> headers = m_httpPut->getRequestHeaders();
    std::unordered_set<std::string> headersSet(headers.begin(), headers.end());
    std::string headerToCheck = CONTENT_TYPE_HEADER_KEY + HEADER_KEY_VALUE_SEPARATOR + CONTENT_TYPE_HEADER_VALUE;
    ASSERT_NE(headersSet.find(headerToCheck), headersSet.end());
    headerToCheck = CONTENT_LENGTH_HEADER_KEY + HEADER_KEY_VALUE_SEPARATOR + std::to_string(publishedMsg.size());
    ASSERT_NE(headersSet.find(headerToCheck), headersSet.end());
    headerToCheck = AUTHORIZATION_HEADER_KEY + HEADER_KEY_VALUE_SEPARATOR + AUTH_TOKEN;
    ASSERT_NE(headersSet.find(headerToCheck), headersSet.end());
}

/// Test publishing capabilities that returns a fatal error
TEST_F(DCFDelegateTest, publishFatalError) {
    std::shared_ptr<TestCapabilityProvider> capabilityProvider = std::make_shared<TestCapabilityProvider>();
    capabilityProvider->addCapabilityConfiguration(INTERFACE_TYPE, INTERFACE_NAME_ONE, INTERFACE_VERSION);

    ASSERT_TRUE(m_dcfDelegate->registerCapability(capabilityProvider));
    m_httpPut->setResponseCode(HTTPResponseCode::BAD_REQUEST);  /// Fatal error
    ASSERT_EQ(m_dcfDelegate->publishCapabilities(), DCFDelegate::DCFPublishReturnCode::FATAL_ERROR);
    ASSERT_FALSE(m_httpPut->getRequestData().empty());
    ASSERT_FALSE(m_miscStorage->dataExists(COMPONENT_NAME, DCF_PUBLISH_TABLE));
}

/// Test publishing capabilities that returns a retriable error
TEST_F(DCFDelegateTest, publishRetriableError) {
    std::shared_ptr<TestCapabilityProvider> capabilityProvider = std::make_shared<TestCapabilityProvider>();
    capabilityProvider->addCapabilityConfiguration(INTERFACE_TYPE, INTERFACE_NAME_ONE, INTERFACE_VERSION);

    ASSERT_TRUE(m_dcfDelegate->registerCapability(capabilityProvider));
    m_httpPut->setResponseCode(HTTPResponseCode::SERVER_INTERNAL_ERROR);  /// Retriable error
    ASSERT_EQ(m_dcfDelegate->publishCapabilities(), DCFDelegate::DCFPublishReturnCode::RETRIABLE_ERROR);
    ASSERT_FALSE(m_httpPut->getRequestData().empty());
    ASSERT_FALSE(m_miscStorage->dataExists(COMPONENT_NAME, DCF_PUBLISH_TABLE));
}

/// Test republishing capabilities
TEST_F(DCFDelegateTest, republish) {
    std::shared_ptr<TestCapabilityProvider> capabilityProvider = std::make_shared<TestCapabilityProvider>();
    capabilityProvider->addCapabilityConfiguration(INTERFACE_TYPE, INTERFACE_NAME_ONE, INTERFACE_VERSION);

    /// Publish succeeds the first time
    ASSERT_TRUE(m_dcfDelegate->registerCapability(capabilityProvider));
    m_httpPut->setResponseCode(HTTPResponseCode::SUCCESS_NO_CONTENT);  /// Success
    ASSERT_EQ(m_dcfDelegate->publishCapabilities(), DCFDelegate::DCFPublishReturnCode::SUCCESS);
    ASSERT_FALSE(m_httpPut->getRequestData().empty());
    ASSERT_TRUE(m_miscStorage->dataExists(COMPONENT_NAME, DCF_PUBLISH_TABLE));

    /// Republish with same data will not send again
    m_httpPut->reset();
    m_httpPut->setResponseCode(HTTPResponseCode::SUCCESS_NO_CONTENT);  /// Success
    ASSERT_EQ(m_dcfDelegate->publishCapabilities(), DCFDelegate::DCFPublishReturnCode::SUCCESS);
    ASSERT_TRUE(m_httpPut->getRequestData().empty());

    const std::string dbKeysPrefix = DB_KEY_ENDPOINT + DCF_ENDPOINT + DB_KEY_SEPARATOR;
    /// Change client id to republish
    m_httpPut->reset();
    m_httpPut->setResponseCode(HTTPResponseCode::SUCCESS_NO_CONTENT);  /// Success
    std::string dbKey = dbKeysPrefix + DB_KEY_CLIENT_ID;
    m_miscStorage->update(COMPONENT_NAME, DCF_PUBLISH_TABLE, dbKey, CLIENT_ID_TWO);
    ASSERT_EQ(m_dcfDelegate->publishCapabilities(), DCFDelegate::DCFPublishReturnCode::SUCCESS);
    ASSERT_FALSE(m_httpPut->getRequestData().empty());

    /// Change product id to republish
    m_httpPut->reset();
    m_httpPut->setResponseCode(HTTPResponseCode::SUCCESS_NO_CONTENT);  /// Success
    dbKey = dbKeysPrefix + DB_KEY_PRODUCT_ID;
    m_miscStorage->update(COMPONENT_NAME, DCF_PUBLISH_TABLE, dbKey, PRODUCT_ID_TWO);
    ASSERT_EQ(m_dcfDelegate->publishCapabilities(), DCFDelegate::DCFPublishReturnCode::SUCCESS);
    ASSERT_FALSE(m_httpPut->getRequestData().empty());

    /// Change DSN to republish
    m_httpPut->reset();
    m_httpPut->setResponseCode(HTTPResponseCode::SUCCESS_NO_CONTENT);  /// Success
    dbKey = dbKeysPrefix + DB_KEY_DSN;
    m_miscStorage->update(COMPONENT_NAME, DCF_PUBLISH_TABLE, dbKey, DSN_TWO);
    ASSERT_EQ(m_dcfDelegate->publishCapabilities(), DCFDelegate::DCFPublishReturnCode::SUCCESS);
    ASSERT_FALSE(m_httpPut->getRequestData().empty());

    /// Change envelope version to republish
    m_httpPut->reset();
    m_httpPut->setResponseCode(HTTPResponseCode::SUCCESS_NO_CONTENT);  /// Success
    dbKey = dbKeysPrefix + DB_KEY_ENVELOPE_VERSION;
    m_miscStorage->update(COMPONENT_NAME, DCF_PUBLISH_TABLE, dbKey, ENVELOPE_VERSION_VALUE_TWO);
    ASSERT_EQ(m_dcfDelegate->publishCapabilities(), DCFDelegate::DCFPublishReturnCode::SUCCESS);
    ASSERT_FALSE(m_httpPut->getRequestData().empty());

    /// Change capabilities to republish
    m_httpPut->reset();
    m_httpPut->setResponseCode(HTTPResponseCode::SUCCESS_NO_CONTENT);  /// Success
    std::shared_ptr<TestCapabilityProvider> capabilityProviderTwo = std::make_shared<TestCapabilityProvider>();
    capabilityProviderTwo->addCapabilityConfiguration(INTERFACE_TYPE, INTERFACE_NAME_TWO, INTERFACE_VERSION);
    ASSERT_TRUE(m_dcfDelegate->registerCapability(capabilityProviderTwo));
    ASSERT_EQ(m_dcfDelegate->publishCapabilities(), DCFDelegate::DCFPublishReturnCode::SUCCESS);
    ASSERT_FALSE(m_httpPut->getRequestData().empty());
}

/// Tests with registering capabilities
TEST_F(DCFDelegateTest, registerTests) {
    std::shared_ptr<TestCapabilityProvider> capabilityProvider = std::make_shared<TestCapabilityProvider>();
    std::unordered_map<std::string, std::string> capabilityConfigurationMap;

    /// nullptr register
    ASSERT_FALSE(m_dcfDelegate->registerCapability(nullptr));

    /// No capability in config
    ASSERT_FALSE(m_dcfDelegate->registerCapability(capabilityProvider));

    /// Empty interface type
    capabilityProvider->clearCapabilityConfigurations();
    capabilityProvider->addCapabilityConfiguration("", INTERFACE_NAME_ONE, INTERFACE_VERSION);
    ASSERT_FALSE(m_dcfDelegate->registerCapability(capabilityProvider));

    /// Empty interface name
    capabilityProvider->clearCapabilityConfigurations();
    capabilityProvider->addCapabilityConfiguration(INTERFACE_TYPE, "", INTERFACE_VERSION);
    ASSERT_FALSE(m_dcfDelegate->registerCapability(capabilityProvider));

    /// Empty interface version
    capabilityProvider->clearCapabilityConfigurations();
    capabilityProvider->addCapabilityConfiguration(INTERFACE_TYPE, INTERFACE_NAME_ONE, "");
    ASSERT_FALSE(m_dcfDelegate->registerCapability(capabilityProvider));

    /// Bad interface config
    capabilityProvider->clearCapabilityConfigurations();
    capabilityProvider->addCapabilityConfiguration(
        INTERFACE_TYPE, INTERFACE_NAME_ONE, INTERFACE_VERSION, INTERFACE_CONFIG_BAD);
    ASSERT_FALSE(m_dcfDelegate->registerCapability(capabilityProvider));

    /// Null interface type
    capabilityConfigurationMap.clear();
    capabilityConfigurationMap.insert({CAPABILITY_INTERFACE_NAME_KEY, INTERFACE_NAME_ONE});
    capabilityConfigurationMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, INTERFACE_VERSION});
    auto capabilityConfiguration = std::make_shared<CapabilityConfiguration>(capabilityConfigurationMap);
    capabilityProvider->clearCapabilityConfigurations();
    capabilityProvider->addCapabilityConfiguration(capabilityConfiguration);
    ASSERT_FALSE(m_dcfDelegate->registerCapability(capabilityProvider));

    /// Null interface name
    capabilityConfigurationMap.clear();
    capabilityConfigurationMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, INTERFACE_TYPE});
    capabilityConfigurationMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, INTERFACE_VERSION});
    capabilityConfiguration = std::make_shared<CapabilityConfiguration>(capabilityConfigurationMap);
    capabilityProvider->clearCapabilityConfigurations();
    capabilityProvider->addCapabilityConfiguration(capabilityConfiguration);
    ASSERT_FALSE(m_dcfDelegate->registerCapability(capabilityProvider));

    /// Null interface version
    capabilityConfigurationMap.clear();
    capabilityConfigurationMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, INTERFACE_TYPE});
    capabilityConfigurationMap.insert({CAPABILITY_INTERFACE_NAME_KEY, INTERFACE_NAME_ONE});
    capabilityConfiguration = std::make_shared<CapabilityConfiguration>(capabilityConfigurationMap);
    capabilityProvider->clearCapabilityConfigurations();
    capabilityProvider->addCapabilityConfiguration(capabilityConfiguration);
    ASSERT_FALSE(m_dcfDelegate->registerCapability(capabilityProvider));

    /// Some random entry in otherwise ok map
    capabilityConfigurationMap.clear();
    capabilityConfigurationMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, INTERFACE_TYPE});
    capabilityConfigurationMap.insert({CAPABILITY_INTERFACE_NAME_KEY, INTERFACE_NAME_ONE});
    capabilityConfigurationMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, INTERFACE_VERSION});
    capabilityConfigurationMap.insert({"randomKey", "randomValue"});
    capabilityConfiguration = std::make_shared<CapabilityConfiguration>(capabilityConfigurationMap);
    capabilityProvider->clearCapabilityConfigurations();
    capabilityProvider->addCapabilityConfiguration(capabilityConfiguration);
    ASSERT_FALSE(m_dcfDelegate->registerCapability(capabilityProvider));

    /// Successful entries
    capabilityProvider->clearCapabilityConfigurations();
    capabilityProvider->addCapabilityConfiguration(INTERFACE_TYPE, INTERFACE_NAME_ONE, INTERFACE_VERSION);
    capabilityProvider->addCapabilityConfiguration(
        INTERFACE_TYPE, INTERFACE_NAME_TWO, INTERFACE_VERSION, INTERFACE_CONFIG);
    ASSERT_TRUE(m_dcfDelegate->registerCapability(capabilityProvider));

    /// Can't register same capability again
    std::shared_ptr<TestCapabilityProvider> capabilityProviderTwo = std::make_shared<TestCapabilityProvider>();
    capabilityProvider->addCapabilityConfiguration(INTERFACE_TYPE, INTERFACE_NAME_ONE, INTERFACE_VERSION);
    ASSERT_FALSE(m_dcfDelegate->registerCapability(capabilityProvider));
}

}  // namespace test
}  // namespace dcfDelegate
}  // namespace alexaClientSDK
