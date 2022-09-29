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

#include <AVSCommon/SDKInterfaces/Storage/StubMiscStorage.h>
#include <AVSCommon/SDKInterfaces/MockAuthObserver.h>
#include <AVSCommon/Utils/WaitEvent.h>
#include <acsdkAuthorization/AuthorizationManager.h>
#include <acsdkAuthorizationInterfaces/AuthorizationAdapterInterface.h>
#include <RegistrationManager/RegistrationManagerInterface.h>
#include <RegistrationManager/MockCustomerDataManager.h>

namespace alexaClientSDK {
namespace acsdkAuthorization {
namespace test {

using namespace ::testing;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::sdkInterfaces::storage::test;
using namespace avsCommon::utils;
using namespace acsdkAuthorization;
using namespace acsdkAuthorizationInterfaces;
using namespace registrationManager;

/// Adapter Related Constants
/// @{
static const std::string ADAPTER_ID = "test-adapter";
static const std::string USER_ID = "test-user-id";
static const std::string AUTH_TOKEN = "supersecureauthotoken";
static const std::string ADAPTER_ID_2 = ADAPTER_ID + "2";
static const std::string USER_ID_2 = USER_ID + "2";
static const std::string AUTH_TOKEN_2 = AUTH_TOKEN + "2";
/// @}

/// Storage Constants
/// @{
static const std::string MISC_TABLE_COMPONENT_NAME = "AuthorizationManager";
static const std::string MISC_TABLE_TABLE_NAME = "authorizationState";
static const std::string MISC_TABLE_ADAPTER_ID_KEY = "authAdapterId";
static const std::string MISC_TABLE_USER_ID_KEY = "userId";
/// @}

/// Timeout for test cases that require synchronization.
std::chrono::milliseconds TIMEOUT{2000};

/// Class to implement AuthorizationInterface, which is used to retrieve the id.
class StubAuthorization : public acsdkAuthorizationInterfaces::AuthorizationInterface {
public:
    /**
     * Constructor.
     *
     * @param id The authorization id that identifies this authorization mechanism.
     */
    StubAuthorization(const std::string& id);

    /**
     * Returns the authorization id.
     *
     * @return The authorization id that identifies this authorization mechanism.
     */
    std::string getId() override;

private:
    /// The id.
    std::string m_id;
};

StubAuthorization::StubAuthorization(const std::string& id) : m_id{id} {
}

std::string StubAuthorization::getId() {
    return m_id;
}

/// Mock class implementing @c RegistrationManagerInterface
class MockRegistrationManager : public registrationManager::RegistrationManagerInterface {
public:
    MOCK_METHOD0(logout, void());
};

/// Mock class implementing the adapter.
class MockAuthorizationAdapter
        : public acsdkAuthorizationInterfaces::AuthorizationAdapterInterface
        , public acsdkAuthorizationInterfaces::AuthorizationInterface {
public:
    MockAuthorizationAdapter(std::string id = ADAPTER_ID);

    MOCK_METHOD0(getAuthToken, std::string());
    MOCK_METHOD0(reset, void());
    MOCK_METHOD1(onAuthFailure, void(const std::string&));
    MOCK_METHOD0(getState, avsCommon::sdkInterfaces::AuthObserverInterface::FullState());
    MOCK_METHOD0(getAuthorizationInterface, std::shared_ptr<AuthorizationInterface>());
    MOCK_METHOD1(
        onAuthorizationManagerReady,
        avsCommon::sdkInterfaces::AuthObserverInterface::FullState(
            const std::shared_ptr<AuthorizationManagerInterface>& manager));

    MOCK_METHOD0(getId, std::string());

private:
    /// The id.
    std::string m_id;
};

/// Mock adapter.
MockAuthorizationAdapter::MockAuthorizationAdapter(std::string id) : m_id{id} {
    ON_CALL(*this, getId()).WillByDefault(Return(m_id));
    ON_CALL(*this, getAuthorizationInterface()).WillByDefault(Return(std::make_shared<StubAuthorization>(m_id)));
}

class AuthorizationManagerTest : public ::testing::TestWithParam<std::pair<std::string, std::string>> {
public:
    /// SetUp.
    void SetUp() override;

    /**
     * Checks if the specified key exists in the table.
     *
     * @param key The key.
     * @return A boolean indicating whether the key exists.
     */
    bool storageHasKey(const std::string& key);

    /**
     * Checks if the specified key-value pair exists in the table.
     *
     * @param key The key.
     * @param value The value.
     * @return A boolean indicating whether the key exists.
     */
    bool storageHasKeyValue(const std::string& key, const std::string& value);

    /// Used for synchronization.
    WaitEvent m_wait;

    /// Mocks and Stubs.
    std::shared_ptr<MockRegistrationManager> m_mockRegMgr;
    std::shared_ptr<StubMiscStorage> m_storage;
    std::shared_ptr<MockAuthorizationAdapter> m_mockAdapter;
    std::shared_ptr<MockAuthorizationAdapter> m_mockAdapter2;
    std::shared_ptr<MockCustomerDataManager> m_mockCDM;
    std::shared_ptr<MockAuthObserver> m_mockAuthObsv;

    /// The object under test.
    std::shared_ptr<AuthorizationManager> m_authMgr;
};

void AuthorizationManagerTest::SetUp() {
    m_mockRegMgr = std::make_shared<NiceMock<MockRegistrationManager>>();
    ON_CALL(*m_mockRegMgr, logout()).WillByDefault(Invoke([this] { m_authMgr->clearData(); }));

    m_mockAdapter = std::make_shared<NiceMock<MockAuthorizationAdapter>>();
    m_mockAdapter2 = std::make_shared<NiceMock<MockAuthorizationAdapter>>(ADAPTER_ID_2);
    m_mockCDM = std::make_shared<NiceMock<MockCustomerDataManager>>();
    m_storage = StubMiscStorage::create();
    m_mockAuthObsv = std::make_shared<NiceMock<MockAuthObserver>>();
    m_authMgr = AuthorizationManager::create(m_storage, m_mockCDM);
    m_authMgr->add(m_mockAdapter);
    m_authMgr->addAuthObserver(m_mockAuthObsv);
    m_authMgr->setRegistrationManager(m_mockRegMgr);
}

bool AuthorizationManagerTest::storageHasKey(const std::string& key) {
    bool exists = false;
    EXPECT_TRUE(m_storage->tableEntryExists(MISC_TABLE_COMPONENT_NAME, MISC_TABLE_TABLE_NAME, key, &exists));

    return exists;
}

bool AuthorizationManagerTest::storageHasKeyValue(const std::string& key, const std::string& value) {
    std::string actualValue;
    EXPECT_TRUE(m_storage->get(MISC_TABLE_COMPONENT_NAME, MISC_TABLE_TABLE_NAME, key, &actualValue));

    return value == actualValue;
}

/// Test that create succeeds
TEST_F(AuthorizationManagerTest, test_create_Succeeds) {
    ASSERT_THAT(AuthorizationManager::create(m_storage, m_mockCDM), NotNull());
}

/// Test that create with null param results in nullptr.
TEST_F(AuthorizationManagerTest, test_createNullParam_Fails) {
    ASSERT_THAT(AuthorizationManager::create(nullptr, nullptr), IsNull());
    ASSERT_THAT(AuthorizationManager::create(m_storage, nullptr), IsNull());
    ASSERT_THAT(AuthorizationManager::create(nullptr, m_mockCDM), IsNull());
}

/// Test that in the AUTHORIZING state, auth and user id are not persisted.
TEST_F(AuthorizationManagerTest, test_authorizingState_DoesNotPersist) {
    EXPECT_CALL(
        *m_mockAuthObsv,
        onAuthStateChange(AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS))
        .WillOnce(InvokeWithoutArgs([this]() { m_wait.wakeUp(); }));

    m_authMgr->reportStateChange(
        {AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS}, ADAPTER_ID, USER_ID);

    EXPECT_TRUE(m_wait.wait(TIMEOUT));

    EXPECT_FALSE(storageHasKey(MISC_TABLE_ADAPTER_ID_KEY));
    EXPECT_FALSE(storageHasKey(MISC_TABLE_USER_ID_KEY));
}

/// Test that in the AUTHORIZING state, the getToken calls no-op.
TEST_F(AuthorizationManagerTest, test_authorizingState_NoToken) {
    ON_CALL(
        *m_mockAuthObsv,
        onAuthStateChange(AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS))
        .WillByDefault(InvokeWithoutArgs([this]() { m_wait.wakeUp(); }));
    EXPECT_CALL(*m_mockAdapter, getAuthToken()).Times(0);

    m_authMgr->reportStateChange(
        {AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS}, ADAPTER_ID, USER_ID);

    EXPECT_TRUE(m_wait.wait(TIMEOUT));
    ASSERT_THAT(m_authMgr->getAuthToken(), IsEmpty());
}

/// Check that in the REFRESHED state, the token is now valid as well as adapter id and user id are persisted.
TEST_F(AuthorizationManagerTest, test_refreshedState_Persisted) {
    EXPECT_CALL(
        *m_mockAuthObsv,
        onAuthStateChange(AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS));

    EXPECT_CALL(
        *m_mockAuthObsv,
        onAuthStateChange(AuthObserverInterface::State::REFRESHED, AuthObserverInterface::Error::SUCCESS))
        .WillOnce(InvokeWithoutArgs([this]() { m_wait.wakeUp(); }));

    EXPECT_CALL(*m_mockAdapter, getAuthToken()).WillOnce(Return(AUTH_TOKEN));

    m_authMgr->reportStateChange(
        {AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS}, ADAPTER_ID, USER_ID);
    m_authMgr->reportStateChange(
        {AuthObserverInterface::State::REFRESHED, AuthObserverInterface::Error::SUCCESS}, ADAPTER_ID, USER_ID);

    EXPECT_TRUE(m_wait.wait(TIMEOUT));

    EXPECT_EQ(AUTH_TOKEN, m_authMgr->getAuthToken());

    EXPECT_TRUE(storageHasKeyValue(MISC_TABLE_ADAPTER_ID_KEY, ADAPTER_ID));
    EXPECT_TRUE(storageHasKeyValue(MISC_TABLE_USER_ID_KEY, USER_ID));
}

/// Check the behavior if AuthorizationManager is in the UNRECOVERABLE_ERROR state.
TEST_F(AuthorizationManagerTest, test_unrecoverableError_Success) {
    {
        InSequence s;

        EXPECT_CALL(
            *m_mockAuthObsv,
            onAuthStateChange(AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS));

        EXPECT_CALL(
            *m_mockAuthObsv,
            onAuthStateChange(
                AuthObserverInterface::State::UNRECOVERABLE_ERROR, AuthObserverInterface::Error::UNKNOWN_ERROR))
            .WillOnce(InvokeWithoutArgs([this]() { m_wait.wakeUp(); }));
    }

    m_authMgr->reportStateChange(
        {AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS}, ADAPTER_ID, USER_ID);

    m_authMgr->reportStateChange(
        {AuthObserverInterface::State::UNRECOVERABLE_ERROR, AuthObserverInterface::Error::UNKNOWN_ERROR},
        ADAPTER_ID,
        USER_ID);

    EXPECT_TRUE(m_wait.wait(TIMEOUT));
}

/// Check invalid transitions.
TEST_F(AuthorizationManagerTest, test_invalidStateTransition_NoNotification) {
    m_authMgr->reportStateChange(
        {AuthObserverInterface::State::REFRESHED, AuthObserverInterface::Error::SUCCESS}, ADAPTER_ID, USER_ID);
    m_authMgr->reportStateChange(
        {AuthObserverInterface::State::EXPIRED, AuthObserverInterface::Error::SUCCESS}, ADAPTER_ID, USER_ID);
    // Call this to ensure any async requests have been processed.
    m_authMgr->doShutdown();

    EXPECT_CALL(*m_mockAuthObsv, onAuthStateChange(_, _)).Times(0);
}

/// Check that getState returns the correct thing.
TEST_F(AuthorizationManagerTest, test_getState) {
    auto waitAndReset = [this]() {
        EXPECT_TRUE(m_wait.wait(TIMEOUT));
        m_wait.reset();
    };

    EXPECT_CALL(*m_mockAuthObsv, onAuthStateChange(_, _)).WillRepeatedly(InvokeWithoutArgs([this]() {
        m_wait.wakeUp();
    }));
    ;

    std::vector<AuthObserverInterface::State> states{
        AuthObserverInterface::State::AUTHORIZING,
        AuthObserverInterface::State::REFRESHED,
        AuthObserverInterface::State::EXPIRED,
        // We don't check for UNRECOVERABLE_ERROR because it leads back to UNINITIALIZED
    };

    EXPECT_EQ(AuthObserverInterface::State::UNINITIALIZED, m_authMgr->getState());

    for (const auto& state : states) {
        m_authMgr->reportStateChange({state, AuthObserverInterface::Error::SUCCESS}, ADAPTER_ID, USER_ID);
        waitAndReset();
        EXPECT_EQ(state, m_authMgr->getState());
    }
}

/// Check that the active authorization returns the correct id.
TEST_F(AuthorizationManagerTest, test_activeAuthorization_Success) {
    EXPECT_CALL(
        *m_mockAuthObsv,
        onAuthStateChange(AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS));

    EXPECT_CALL(
        *m_mockAuthObsv,
        onAuthStateChange(AuthObserverInterface::State::REFRESHED, AuthObserverInterface::Error::SUCCESS))
        .WillOnce(InvokeWithoutArgs([this]() { m_wait.wakeUp(); }));

    m_authMgr->reportStateChange(
        {AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS}, ADAPTER_ID, USER_ID);
    m_authMgr->reportStateChange(
        {AuthObserverInterface::State::REFRESHED, AuthObserverInterface::Error::SUCCESS}, ADAPTER_ID, USER_ID);

    EXPECT_TRUE(m_wait.wait(TIMEOUT));

    EXPECT_EQ(ADAPTER_ID, m_authMgr->getActiveAuthorization());
}

/// Check that logout succeeds.
TEST_F(AuthorizationManagerTest, test_logout) {
    EXPECT_CALL(*m_mockRegMgr, logout()).WillOnce(InvokeWithoutArgs([this]() {
        m_authMgr->clearData();
        m_wait.wakeUp();
    }));

    m_authMgr->logout();

    EXPECT_TRUE(m_wait.wait(TIMEOUT));
}

/// Parameterized Tests to test combinations of mismatched adapter and user ids.
INSTANTIATE_TEST_CASE_P(
    Parameterized,
    AuthorizationManagerTest,
    ::testing::Values(
        std::pair<std::string, std::string>(ADAPTER_ID, USER_ID_2),
        std::pair<std::string, std::string>(ADAPTER_ID_2, USER_ID),
        std::pair<std::string, std::string>(ADAPTER_ID_2, USER_ID_2)));

/// Check the implict logout behavior if AuthorizationManager is in the REFRESHED state.
TEST_P(AuthorizationManagerTest, test_mismatchingIdRefreshed_AuthorizingRequest_ImplictLogout) {
    m_authMgr->add(m_mockAdapter2);

    std::string newAdapterId, newUserId;
    std::tie(newAdapterId, newUserId) = GetParam();

    {
        InSequence s;

        EXPECT_CALL(
            *m_mockAuthObsv,
            onAuthStateChange(AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS));

        EXPECT_CALL(
            *m_mockAuthObsv,
            onAuthStateChange(AuthObserverInterface::State::REFRESHED, AuthObserverInterface::Error::SUCCESS));

        EXPECT_CALL(
            *m_mockAuthObsv,
            onAuthStateChange(AuthObserverInterface::State::UNINITIALIZED, AuthObserverInterface::Error::SUCCESS));

        EXPECT_CALL(
            *m_mockAuthObsv,
            onAuthStateChange(AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS))
            .WillOnce(InvokeWithoutArgs([this]() { m_wait.wakeUp(); }));
    }

    EXPECT_CALL(*m_mockRegMgr, logout());
    EXPECT_CALL(*m_mockAdapter, reset());
    EXPECT_CALL(*m_mockAdapter2, reset()).Times(0);

    m_authMgr->reportStateChange(
        {AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS}, ADAPTER_ID, USER_ID);
    m_authMgr->reportStateChange(
        {AuthObserverInterface::State::REFRESHED, AuthObserverInterface::Error::SUCCESS}, ADAPTER_ID, USER_ID);
    m_authMgr->reportStateChange(
        {AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS}, newAdapterId, newUserId);

    EXPECT_TRUE(m_wait.wait(TIMEOUT));

    EXPECT_FALSE(storageHasKey(MISC_TABLE_ADAPTER_ID_KEY));
    EXPECT_FALSE(storageHasKey(MISC_TABLE_USER_ID_KEY));
}

/**
 * The new adapter will be allow to continue authorizing while we reset the current adapter
 * and logout the rest of the device.
 */
TEST_P(AuthorizationManagerTest, test_mismatchingIdAuthorizing_AuthorizingRequest_ImplictLogout) {
    std::string newAdapterId, newUserId;
    std::tie(newAdapterId, newUserId) = GetParam();

    {
        InSequence s;

        EXPECT_CALL(
            *m_mockAuthObsv,
            onAuthStateChange(AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS));

        EXPECT_CALL(
            *m_mockAuthObsv,
            onAuthStateChange(AuthObserverInterface::State::UNINITIALIZED, AuthObserverInterface::Error::SUCCESS));

        EXPECT_CALL(
            *m_mockAuthObsv,
            onAuthStateChange(AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS))
            .WillOnce(InvokeWithoutArgs([this]() { m_wait.wakeUp(); }));
    }

    EXPECT_CALL(*m_mockRegMgr, logout());
    EXPECT_CALL(*m_mockAdapter, reset());
    EXPECT_CALL(*m_mockAdapter2, reset()).Times(0);

    m_authMgr->add(m_mockAdapter2);

    m_authMgr->reportStateChange(
        {AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS}, ADAPTER_ID, USER_ID);
    m_authMgr->reportStateChange(
        {AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS}, newAdapterId, newUserId);

    EXPECT_TRUE(m_wait.wait(TIMEOUT));

    EXPECT_FALSE(storageHasKey(MISC_TABLE_ADAPTER_ID_KEY));
    EXPECT_FALSE(storageHasKey(MISC_TABLE_USER_ID_KEY));
}

/**
 * Another adapter has reported it is in REFRESHED state, meaning we have multiple authorizations.
 * To protect customer data, force a logout, and reset all authorizations.
 */
TEST_P(AuthorizationManagerTest, test_mismatchingId_RefreshingRequest_Logout) {
    std::string newAdapterId, newUserId;
    std::tie(newAdapterId, newUserId) = GetParam();

    {
        InSequence s;

        EXPECT_CALL(
            *m_mockAuthObsv,
            onAuthStateChange(AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS));

        EXPECT_CALL(
            *m_mockAuthObsv,
            onAuthStateChange(AuthObserverInterface::State::UNINITIALIZED, AuthObserverInterface::Error::SUCCESS))
            .WillOnce(InvokeWithoutArgs([this]() { m_wait.wakeUp(); }));
    }

    EXPECT_CALL(*m_mockRegMgr, logout());
    EXPECT_CALL(*m_mockAdapter, reset()).Times(AtLeast(1));
    // Only expect this adapter to be reset if it's reported REFRESHED.
    if (ADAPTER_ID_2 == newAdapterId) {
        EXPECT_CALL(*m_mockAdapter2, reset());
    }

    m_authMgr->add(m_mockAdapter2);

    m_authMgr->reportStateChange(
        {AuthObserverInterface::State::AUTHORIZING, AuthObserverInterface::Error::SUCCESS}, ADAPTER_ID, USER_ID);
    m_authMgr->reportStateChange(
        {AuthObserverInterface::State::REFRESHED, AuthObserverInterface::Error::SUCCESS}, newAdapterId, newUserId);

    EXPECT_TRUE(m_wait.wait(TIMEOUT));

    EXPECT_FALSE(storageHasKey(MISC_TABLE_ADAPTER_ID_KEY));
    EXPECT_FALSE(storageHasKey(MISC_TABLE_USER_ID_KEY));
}

}  // namespace test
}  // namespace acsdkAuthorization
}  // namespace alexaClientSDK
