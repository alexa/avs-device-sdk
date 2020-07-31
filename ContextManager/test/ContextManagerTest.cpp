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

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/WaitEvent.h>

#include "ContextManager/ContextManager.h"

using namespace testing;

namespace alexaClientSDK {
namespace contextManager {
namespace test {

using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;

/// String to identify log entries originating from this file.
static const std::string TAG("ContextManagerTest");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Mock state provider.
class MockStateProvider : public StateProviderInterface {
public:
    MOCK_METHOD2(
        provideState,
        void(const avs::CapabilityTag& stateProviderName, const ContextRequestToken stateRequestToken));
    MOCK_METHOD0(hasReportableStateProperties, bool());
};

/// Mock legacy state provider.
struct MockLegacyStateProvider : public StateProviderInterface {
    MOCK_METHOD2(
        provideState,
        void(const avs::NamespaceAndName& stateProviderName, const ContextRequestToken stateRequestToken));
};

/// Mock context requester.
struct MockContextRequester : public ContextRequesterInterface {
    MOCK_METHOD3(
        onContextAvailable,
        void(const std::string& endpointId, const avs::AVSContext& endpointContext, ContextRequestToken requestToken));
    MOCK_METHOD2(onContextFailure, void(const ContextRequestError error, ContextRequestToken token));
};

/// Mock context observer.
struct MockContextObserver : public ContextManagerObserverInterface {
    MOCK_METHOD3(
        onStateChanged,
        void(const avs::CapabilityTag& identifier, const avs::CapabilityState& state, AlexaStateChangeCauseType cause));
};

/// Context Manager Test
class ContextManagerTest : public ::testing::Test {
protected:
    /**
     * Creates a @c ContextManager.
     */
    void SetUp() override;

    /// @c ContextManager to test
    std::shared_ptr<ContextManagerInterface> m_contextManager;
};

void ContextManagerTest::SetUp() {
    auto deviceInfo = avsCommon::utils::DeviceInfo::create(
        "clientId", "productId", "1234", "manufacturer", "my device", "friendlyName", "deviceType");
    ASSERT_NE(deviceInfo, nullptr);

    m_contextManager = ContextManager::createContextManagerInterface(std::move(deviceInfo));
}

/**
 * Set the state with a @c StateRefreshPolicy @c ALWAYS for a @c StateProviderInterface that is registered with the
 * @c ContextManager. Expect @c SetStateResult @c SUCCESS is returned.
 */
TEST_F(ContextManagerTest, test_setStateForLegacyRegisteredProvider) {
    auto provider = std::make_shared<MockLegacyStateProvider>();
    auto capability = NamespaceAndName("Namespace", "Name");
    m_contextManager->setStateProvider(capability, provider);
    std::string payload{R"({"state":"value"})"};
    ASSERT_EQ(m_contextManager->setState(capability, payload, StateRefreshPolicy::ALWAYS), SetStateResult::SUCCESS);
}

/**
 * Set the state with a @c StateRefreshPolicy @c NEVER for a @c StateProviderInterface that is not registered with the
 * @c ContextManager. Expect @c SetStateResult @c SUCCESS is returned.
 */
TEST_F(ContextManagerTest, test_setStateForUnregisteredLegacyProvider) {
    auto provider = std::make_shared<MockLegacyStateProvider>();
    auto capability = NamespaceAndName("Namespace", "Name");
    std::string payload{R"({"state":"value"})"};
    ASSERT_EQ(m_contextManager->setState(capability, payload, StateRefreshPolicy::ALWAYS), SetStateResult::SUCCESS);
}

/**
 * Set the states with a @c StateRefreshPolicy @c ALWAYS for @c StateProviderInterfaces that are registered with the
 * @c ContextManager. Request for context by calling @c getContext. Expect that the context is returned within the
 * timeout period. Check the context that is returned by the @c ContextManager. Expect it should match the test value.
 */
TEST_F(ContextManagerTest, test_getContextLegacyProvider) {
    auto provider = std::make_shared<MockLegacyStateProvider>();
    auto capability = NamespaceAndName("Namespace", "Name");
    std::string payload{R"({"state":"value"})"};
    m_contextManager->setStateProvider(capability, provider);

    std::promise<ContextRequestToken> tokenPromise;
    EXPECT_CALL(*provider, provideState(_, _))
        .WillOnce(
            WithArg<1>(Invoke([&tokenPromise](const ContextRequestToken token) { tokenPromise.set_value(token); })));

    auto requester = std::make_shared<MockContextRequester>();
    std::promise<AVSContext::States> contextStatesPromise;
    EXPECT_CALL(*requester, onContextAvailable(_, _, _))
        .WillOnce(WithArg<1>(Invoke([&contextStatesPromise](const AVSContext& context) {
            contextStatesPromise.set_value(context.getStates());
        })));

    auto requestToken = m_contextManager->getContext(requester);

    const std::chrono::milliseconds timeout{100};
    auto expectedTokenFuture = tokenPromise.get_future();
    ASSERT_EQ(expectedTokenFuture.wait_for(timeout), std::future_status::ready);
    EXPECT_EQ(requestToken, expectedTokenFuture.get());

    ASSERT_EQ(
        m_contextManager->setState(capability, payload, StateRefreshPolicy::ALWAYS, requestToken),
        SetStateResult::SUCCESS);

    auto statesFuture = contextStatesPromise.get_future();
    ASSERT_EQ(statesFuture.wait_for(timeout), std::future_status::ready);
    EXPECT_EQ(statesFuture.get()[capability].valuePayload, payload);
}

/**
 * Respond to the same state request twice. The first one should succeed while the second one should fail.
 */
TEST_F(ContextManagerTest, test_setLegacyStateProviderSetStateTwiceShouldFail) {
    auto provider = std::make_shared<MockLegacyStateProvider>();
    auto capability = NamespaceAndName("Namespace", "Name");
    std::string payload{R"({"state":"value"})"};
    m_contextManager->setStateProvider(capability, provider);

    utils::WaitEvent provideStateEvent;
    EXPECT_CALL(*provider, provideState(_, _)).WillOnce((InvokeWithoutArgs([&provideStateEvent] {
        provideStateEvent.wakeUp();
    })));

    auto requester = std::make_shared<MockContextRequester>();
    utils::WaitEvent stateAvailableEvent;
    EXPECT_CALL(*requester, onContextAvailable(_, _, _)).WillOnce((InvokeWithoutArgs([&stateAvailableEvent] {
        stateAvailableEvent.wakeUp();
    })));

    auto requestToken = m_contextManager->getContext(requester);

    const std::chrono::milliseconds timeout{100};
    ASSERT_TRUE(provideStateEvent.wait(timeout));

    ASSERT_EQ(
        m_contextManager->setState(capability, payload, StateRefreshPolicy::ALWAYS, requestToken),
        SetStateResult::SUCCESS);

    ASSERT_TRUE(stateAvailableEvent.wait(timeout));
    EXPECT_EQ(
        m_contextManager->setState(capability, payload, StateRefreshPolicy::ALWAYS, requestToken),
        SetStateResult::STATE_TOKEN_OUTDATED);
}

/**
 * Register a @c StateProviderInterfaces with the @c ContextManager which responds slowly to @provideState requests.
 * Set the states with a @c StateRefreshPolicy @c ALWAYS for @c StateProviderInterfaces. Request for context by calling
 * @c getContext. Expect that failure occurs due to timeout.
 */
TEST_F(ContextManagerTest, test_provideStateTimeout) {
    auto provider = std::make_shared<MockLegacyStateProvider>();
    auto capability = NamespaceAndName("Namespace", "Name");
    std::string payload{R"({"state":"value"})"};
    m_contextManager->setStateProvider(capability, provider);

    utils::WaitEvent provideStateEvent;
    EXPECT_CALL(*provider, provideState(_, _)).WillOnce((InvokeWithoutArgs([&provideStateEvent] {
        provideStateEvent.wakeUp();
    })));

    const std::chrono::milliseconds timeout{100};
    auto requester = std::make_shared<MockContextRequester>();
    auto token = m_contextManager->getContext(requester, capability.endpointId, timeout);

    utils::WaitEvent stateFailureEvent;
    EXPECT_CALL(*requester, onContextFailure(ContextRequestError::STATE_PROVIDER_TIMEDOUT, token))
        .WillOnce((InvokeWithoutArgs([&stateFailureEvent] { stateFailureEvent.wakeUp(); })));

    ASSERT_TRUE(provideStateEvent.wait(timeout));
    ASSERT_TRUE(stateFailureEvent.wait(timeout + timeout));
}

/**
 * Request for context by calling @c getContextSet. Wait for context. Set the state for a @c StateProviderInterface
 * that is registered with the @c ContextManager with a wrong token value. Expect that
 * @c SetStateResult @c STATE_TOKEN_OUTDATED is returned.
 */
TEST_F(ContextManagerTest, test_incorrectToken) {
    auto provider = std::make_shared<MockLegacyStateProvider>();
    auto capability = NamespaceAndName("Namespace", "Name");
    std::string payload{R"({"state":"value"})"};
    m_contextManager->setStateProvider(capability, provider);

    utils::WaitEvent provideStateEvent;
    EXPECT_CALL(*provider, provideState(_, _)).WillOnce((InvokeWithoutArgs([&provideStateEvent] {
        provideStateEvent.wakeUp();
    })));

    auto requester = std::make_shared<MockContextRequester>();
    auto requestToken = m_contextManager->getContext(requester);
    const std::chrono::milliseconds timeout{100};
    ASSERT_TRUE(provideStateEvent.wait(timeout));

    EXPECT_EQ(
        m_contextManager->setState(capability, payload, StateRefreshPolicy::ALWAYS, requestToken + 1),
        SetStateResult::STATE_TOKEN_OUTDATED);
}

/**
 * Test that a @c StateProvider that has a refreshPolicy @c SOMETIMES is queried on call to @c getContext().
 * If the stateProvider provides a valid (non-empty) state, test that it is reflected in the corresponding context that
 * is generated.
 */
TEST_F(ContextManagerTest, test_sometimesProviderWithValidState) {
    auto sometimesProvider = std::make_shared<MockLegacyStateProvider>();
    auto sometimesCapability = NamespaceAndName("Namespace", "Name");
    std::string sometimesPayload{R"({"state":"value"})"};
    m_contextManager->setStateProvider(sometimesCapability, sometimesProvider);

    utils::WaitEvent provideStateEvent;
    EXPECT_CALL(*sometimesProvider, provideState(_, _)).WillOnce((InvokeWithoutArgs([&provideStateEvent] {
        provideStateEvent.wakeUp();
    })));

    auto requester = std::make_shared<MockContextRequester>();
    std::promise<AVSContext::States> contextStatesPromise;
    EXPECT_CALL(*requester, onContextAvailable(_, _, _))
        .WillOnce(WithArg<1>(Invoke([&contextStatesPromise](const AVSContext& context) {
            contextStatesPromise.set_value(context.getStates());
        })));

    auto requestToken = m_contextManager->getContext(requester);
    const std::chrono::milliseconds timeout{100};
    provideStateEvent.wait(timeout);
    ASSERT_EQ(
        m_contextManager->setState(sometimesCapability, sometimesPayload, StateRefreshPolicy::SOMETIMES, requestToken),
        SetStateResult::SUCCESS);

    auto statesFuture = contextStatesPromise.get_future();
    ASSERT_EQ(statesFuture.wait_for(timeout), std::future_status::ready);
    EXPECT_EQ(statesFuture.get()[sometimesCapability].valuePayload, sometimesPayload);
}

/**
 * Test that a @c StateProvider that has a refreshPolicy @c SOMETIMES is queried on call to @c getContext().
 * If the stateProvider provides an empty state, test that it is NOT reflected in the corresponding context that is
 * generated.
 */
TEST_F(ContextManagerTest, test_sometimesProviderWithEmptyState) {
    auto sometimesProvider = std::make_shared<MockLegacyStateProvider>();
    auto sometimesCapability = NamespaceAndName("Namespace", "Name");
    m_contextManager->setStateProvider(sometimesCapability, sometimesProvider);

    utils::WaitEvent provideStateEvent;
    EXPECT_CALL(*sometimesProvider, provideState(_, _)).WillOnce((InvokeWithoutArgs([&provideStateEvent] {
        provideStateEvent.wakeUp();
    })));

    auto requester = std::make_shared<MockContextRequester>();
    std::promise<AVSContext::States> contextStatesPromise;
    EXPECT_CALL(*requester, onContextAvailable(_, _, _))
        .WillOnce(WithArg<1>(Invoke([&contextStatesPromise](const AVSContext& context) {
            contextStatesPromise.set_value(context.getStates());
        })));

    auto requestToken = m_contextManager->getContext(requester);
    const std::chrono::milliseconds timeout{100};
    provideStateEvent.wait(timeout);
    ASSERT_EQ(
        m_contextManager->setState(sometimesCapability, "", StateRefreshPolicy::SOMETIMES, requestToken),
        SetStateResult::SUCCESS);

    auto statesFuture = contextStatesPromise.get_future();
    ASSERT_EQ(statesFuture.wait_for(timeout), std::future_status::ready);
    auto states = statesFuture.get();
    EXPECT_EQ(states.find(sometimesCapability), states.end());
}

/**
 * Test that a provider that sets its policy to NEVER is not queried but that the state is included in the context.
 */
TEST_F(ContextManagerTest, test_neverProvider) {
    auto neverProvider = std::make_shared<StrictMock<MockLegacyStateProvider>>();
    auto neverCapability = NamespaceAndName("Namespace", "Name");
    std::string neverPayload{R"({"state":"value"})"};
    m_contextManager->setStateProvider(neverCapability, neverProvider);

    ASSERT_EQ(
        m_contextManager->setState(neverCapability, neverPayload, StateRefreshPolicy::NEVER), SetStateResult::SUCCESS);

    auto requester = std::make_shared<MockContextRequester>();
    std::promise<AVSContext::States> contextStatesPromise;
    EXPECT_CALL(*requester, onContextAvailable(_, _, _))
        .WillOnce(WithArg<1>(Invoke([&contextStatesPromise](const AVSContext& context) {
            contextStatesPromise.set_value(context.getStates());
        })));

    m_contextManager->getContext(requester);
    const std::chrono::milliseconds timeout{100};

    auto statesFuture = contextStatesPromise.get_future();
    ASSERT_EQ(statesFuture.wait_for(timeout), std::future_status::ready);
    EXPECT_EQ(statesFuture.get()[neverCapability].valuePayload, neverPayload);
}

/// Test that only context relevant to the given endpoint is included in the getContext.
TEST_F(ContextManagerTest, test_getEndpointContextShouldIncludeOnlyRelevantStates) {
    // Capability that belongs to the target endpoint.
    auto providerForTargetEndpoint = std::make_shared<MockStateProvider>();
    auto capabilityForTarget = CapabilityTag("TargetNamespace", "TargetName", "TargetEndpointId");
    CapabilityState stateForTarget{R"({"state":"target"})"};
    EXPECT_CALL(*providerForTargetEndpoint, hasReportableStateProperties()).WillRepeatedly(Return(false));
    m_contextManager->setStateProvider(capabilityForTarget, providerForTargetEndpoint);

    // Capability that belongs to another endpoint.
    auto providerForOtherEndpoint = std::make_shared<StrictMock<MockStateProvider>>();
    auto capabilityForOther = CapabilityTag("OtherNamespace", "OtherName", "OtherEndpointId");
    EXPECT_CALL(*providerForOtherEndpoint, hasReportableStateProperties()).WillRepeatedly(Return(false));
    m_contextManager->setStateProvider(capabilityForOther, providerForOtherEndpoint);

    utils::WaitEvent provideStateEvent;
    EXPECT_CALL(*providerForTargetEndpoint, provideState(_, _)).WillOnce((InvokeWithoutArgs([&provideStateEvent] {
        provideStateEvent.wakeUp();
    })));

    auto requester = std::make_shared<MockContextRequester>();
    std::promise<AVSContext::States> contextStatesPromise;
    EXPECT_CALL(*requester, onContextAvailable(_, _, _))
        .WillOnce(WithArg<1>(Invoke([&contextStatesPromise](const AVSContext& context) {
            contextStatesPromise.set_value(context.getStates());
        })));

    // Get context for the target endpoint.
    auto requestToken = m_contextManager->getContext(requester, capabilityForTarget.endpointId);

    const std::chrono::milliseconds timeout{100};
    provideStateEvent.wait(timeout);
    m_contextManager->provideStateResponse(capabilityForTarget, stateForTarget, requestToken);

    auto statesFuture = contextStatesPromise.get_future();
    ASSERT_EQ(statesFuture.wait_for(timeout), std::future_status::ready);

    auto states = statesFuture.get();
    EXPECT_EQ(states[capabilityForTarget].valuePayload, stateForTarget.valuePayload);
    EXPECT_EQ(states.find(capabilityForOther), states.end());
}

/// Test that requester will get notified of a failure when one provider was not able to provide its state.
TEST_F(ContextManagerTest, test_getContextWhenStateAndCacheAreUnavailableShouldFail) {
    auto provider = std::make_shared<MockStateProvider>();
    auto capability = CapabilityTag("Namespace", "Name", "EndpointId");
    EXPECT_CALL(*provider, hasReportableStateProperties()).WillRepeatedly(Return(false));
    m_contextManager->setStateProvider(capability, provider);

    utils::WaitEvent provideStateEvent;
    EXPECT_CALL(*provider, provideState(_, _)).WillOnce((InvokeWithoutArgs([&provideStateEvent] {
        provideStateEvent.wakeUp();
    })));

    // Get context.
    auto requester = std::make_shared<MockContextRequester>();
    auto requestToken = m_contextManager->getContext(requester, capability.endpointId);

    // Expect failure.
    utils::WaitEvent contextFailureEvent;
    EXPECT_CALL(*requester, onContextFailure(ContextRequestError::BUILD_CONTEXT_ERROR, requestToken))
        .WillOnce(InvokeWithoutArgs([&contextFailureEvent] { contextFailureEvent.wakeUp(); }));

    // Respond that state is unavailable after state has been requested.
    const std::chrono::milliseconds timeout{100};
    ASSERT_TRUE(provideStateEvent.wait(timeout));
    m_contextManager->provideStateUnavailableResponse(capability, requestToken, false);

    // Wait for failure.
    EXPECT_TRUE(contextFailureEvent.wait(timeout));
}

/// Test that requester will get cached value when provider cannot provide latest state.
TEST_F(ContextManagerTest, test_getContextWhenStateUnavailableShouldReturnCache) {
    auto provider = std::make_shared<MockStateProvider>();
    auto capability = CapabilityTag("Namespace", "Name", "EndpointId");
    CapabilityState state{R"({"state":"target"})"};
    EXPECT_CALL(*provider, hasReportableStateProperties()).WillRepeatedly(Return(false));
    m_contextManager->setStateProvider(capability, provider);

    // Set value in the cache.
    m_contextManager->reportStateChange(capability, state, AlexaStateChangeCauseType::APP_INTERACTION);

    utils::WaitEvent provideStateEvent;
    EXPECT_CALL(*provider, provideState(_, _)).WillOnce((InvokeWithoutArgs([&provideStateEvent] {
        provideStateEvent.wakeUp();
    })));

    // Get context.
    auto requester = std::make_shared<MockContextRequester>();
    auto requestToken = m_contextManager->getContext(requester, capability.endpointId);

    std::promise<AVSContext::States> contextStatesPromise;
    EXPECT_CALL(*requester, onContextAvailable(_, _, _))
        .WillOnce(WithArg<1>(Invoke([&contextStatesPromise](const AVSContext& context) {
            contextStatesPromise.set_value(context.getStates());
        })));

    // Respond that state is unavailable after state has been requested.
    const std::chrono::milliseconds timeout{100};
    ASSERT_TRUE(provideStateEvent.wait(timeout));
    m_contextManager->provideStateUnavailableResponse(capability, requestToken, false);

    // Wait for failure.
    auto statesFuture = contextStatesPromise.get_future();
    ASSERT_EQ(statesFuture.wait_for(timeout), std::future_status::ready);
    EXPECT_EQ(statesFuture.get()[capability].valuePayload, state.valuePayload);
}

/// Test that report state change notifies the @c ContextManagerObserverInterface.
TEST_F(ContextManagerTest, test_reportStateChangeShouldNotifyObserver) {
    // Add provider.
    auto provider = std::make_shared<MockStateProvider>();
    auto capability = CapabilityTag("Namespace", "Name", "EndpointId");
    CapabilityState state{R"({"state":"target"})"};
    m_contextManager->setStateProvider(capability, provider);

    // Add observer.
    auto observer = std::make_shared<MockContextObserver>();
    m_contextManager->addContextManagerObserver(observer);

    utils::WaitEvent notificationEvent;
    auto cause = AlexaStateChangeCauseType::APP_INTERACTION;
    EXPECT_CALL(*observer, onStateChanged(capability, state, cause)).WillOnce(InvokeWithoutArgs([&notificationEvent] {
        notificationEvent.wakeUp();
    }));

    // Report change.
    m_contextManager->reportStateChange(capability, state, cause);

    const std::chrono::milliseconds timeout{100};
    EXPECT_TRUE(notificationEvent.wait(timeout));
}

/// Test that getContext can handle multiple getContext at the same time.
TEST_F(ContextManagerTest, test_getContextInParallelShouldSucceed) {
    // Capability that belongs to the first endpoint.
    auto providerForEndpoint1 = std::make_shared<MockStateProvider>();
    auto capabilityForEndpoint1 = CapabilityTag("Namespace", "Name", "EndpointId1");
    CapabilityState stateForEndpoint1{R"({"state":1})"};
    EXPECT_CALL(*providerForEndpoint1, hasReportableStateProperties()).WillRepeatedly(Return(false));
    m_contextManager->setStateProvider(capabilityForEndpoint1, providerForEndpoint1);

    // Capability that belongs to the second endpoint.
    auto providerForEndpoint2 = std::make_shared<MockStateProvider>();
    auto capabilityForEndpoint2 = CapabilityTag("Namespace", "Name", "EndpointId2");
    CapabilityState stateForEndpoint2{R"({"state":2})"};
    EXPECT_CALL(*providerForEndpoint2, hasReportableStateProperties()).WillRepeatedly(Return(false));
    m_contextManager->setStateProvider(capabilityForEndpoint2, providerForEndpoint2);

    // Expect both provide state calls
    utils::WaitEvent provideStateEvent1;
    EXPECT_CALL(*providerForEndpoint1, provideState(_, _)).WillOnce((InvokeWithoutArgs([&provideStateEvent1] {
        provideStateEvent1.wakeUp();
    })));
    utils::WaitEvent provideStateEvent2;
    EXPECT_CALL(*providerForEndpoint2, provideState(_, _)).WillOnce((InvokeWithoutArgs([&provideStateEvent2] {
        provideStateEvent2.wakeUp();
    })));

    // Expect both context to be available
    auto requester1 = std::make_shared<MockContextRequester>();
    std::promise<AVSContext::States> contextStatesPromise1;
    EXPECT_CALL(*requester1, onContextAvailable(_, _, _))
        .WillOnce(WithArg<1>(Invoke([&contextStatesPromise1](const AVSContext& context) {
            contextStatesPromise1.set_value(context.getStates());
        })));
    auto requester2 = std::make_shared<MockContextRequester>();
    std::promise<AVSContext::States> contextStatesPromise2;
    EXPECT_CALL(*requester2, onContextAvailable(_, _, _))
        .WillOnce(WithArg<1>(Invoke([&contextStatesPromise2](const AVSContext& context) {
            contextStatesPromise2.set_value(context.getStates());
        })));

    // Get context for both endpoints.
    auto requestToken1 = m_contextManager->getContext(requester1, capabilityForEndpoint1.endpointId);
    auto requestToken2 = m_contextManager->getContext(requester2, capabilityForEndpoint2.endpointId);

    const std::chrono::milliseconds timeout{100};
    ASSERT_TRUE(provideStateEvent1.wait(timeout));
    ASSERT_TRUE(provideStateEvent2.wait(timeout));
    m_contextManager->provideStateResponse(capabilityForEndpoint1, stateForEndpoint1, requestToken1);
    m_contextManager->provideStateResponse(capabilityForEndpoint2, stateForEndpoint2, requestToken2);

    // Validate that context for endpoint 1 only has its own capability state.
    auto statesFuture1 = contextStatesPromise1.get_future();
    ASSERT_EQ(statesFuture1.wait_for(timeout), std::future_status::ready);
    auto statesForEndpoint1 = statesFuture1.get();
    EXPECT_EQ(statesForEndpoint1[capabilityForEndpoint1].valuePayload, stateForEndpoint1.valuePayload);
    EXPECT_EQ(statesForEndpoint1.find(capabilityForEndpoint2), statesForEndpoint1.end());

    // Validate that context for endpoint 2 only has its own capability state.
    auto statesFuture2 = contextStatesPromise2.get_future();
    ASSERT_EQ(statesFuture2.wait_for(timeout), std::future_status::ready);
    auto statesForEndpoint2 = statesFuture2.get();
    EXPECT_EQ(statesForEndpoint2[capabilityForEndpoint2].valuePayload, stateForEndpoint2.valuePayload);
    EXPECT_EQ(statesForEndpoint2.find(capabilityForEndpoint1), statesForEndpoint2.end());
}

/// Test if the getContextWithoutReportableStateProperties() method skips state from @c StateProviders which have
/// reportable state properties.
TEST_F(ContextManagerTest, test_getContextWithoutReportableStateProperties) {
    auto providerWithReportableStateProperties = std::make_shared<MockStateProvider>();
    auto capability1 = CapabilityTag("Namespace", "Name1", "");
    CapabilityState state1{R"({"state1":"target1"})"};
    EXPECT_CALL(*providerWithReportableStateProperties, hasReportableStateProperties()).WillRepeatedly(Return(true));
    m_contextManager->setStateProvider(capability1, providerWithReportableStateProperties);

    auto providerWithoutReportableStateProperties = std::make_shared<MockStateProvider>();
    auto capability2 = CapabilityTag("Namespace", "Name2", "");
    CapabilityState state2{R"({"state2":"target2"})"};
    EXPECT_CALL(*providerWithoutReportableStateProperties, hasReportableStateProperties())
        .WillRepeatedly(Return(false));
    m_contextManager->setStateProvider(capability2, providerWithoutReportableStateProperties);

    EXPECT_CALL(*providerWithReportableStateProperties, provideState(_, _)).Times(0);

    utils::WaitEvent provideStateEvent;
    EXPECT_CALL(*providerWithoutReportableStateProperties, provideState(_, _))
        .WillOnce((InvokeWithoutArgs([&provideStateEvent] { provideStateEvent.wakeUp(); })));

    auto requester = std::make_shared<MockContextRequester>();
    std::promise<AVSContext::States> contextStatesPromise;
    EXPECT_CALL(*requester, onContextAvailable(_, _, _))
        .WillOnce(WithArg<1>(Invoke([&contextStatesPromise](const AVSContext& context) {
            contextStatesPromise.set_value(context.getStates());
        })));

    // Get context for the target endpoint.
    auto requestToken = m_contextManager->getContextWithoutReportableStateProperties(requester);

    const std::chrono::milliseconds timeout{100};
    provideStateEvent.wait(timeout);
    m_contextManager->provideStateResponse(capability2, state2, requestToken);

    auto statesFuture = contextStatesPromise.get_future();
    ASSERT_EQ(statesFuture.wait_for(timeout), std::future_status::ready);

    auto states = statesFuture.get();
    EXPECT_EQ(states[capability2].valuePayload, state2.valuePayload);
    EXPECT_EQ(states.find(capability1), states.end());
}

/// Test if the getContext() method includes state from @c StateProviders which have reportable state properties.
TEST_F(ContextManagerTest, test_getContextWithReportableStateProperties) {
    auto providerWithReportableStateProperties = std::make_shared<MockStateProvider>();
    auto capability1 = CapabilityTag("Namespace", "Name1", "");
    CapabilityState state1{R"({"state1":"target1"})"};
    EXPECT_CALL(*providerWithReportableStateProperties, hasReportableStateProperties()).WillRepeatedly(Return(true));
    m_contextManager->setStateProvider(capability1, providerWithReportableStateProperties);

    auto providerWithoutReportableStateProperties = std::make_shared<MockStateProvider>();
    auto capability2 = CapabilityTag("Namespace", "Name2", "");
    CapabilityState state2{R"({"state2":"target2"})"};
    EXPECT_CALL(*providerWithoutReportableStateProperties, hasReportableStateProperties())
        .WillRepeatedly(Return(false));
    m_contextManager->setStateProvider(capability2, providerWithoutReportableStateProperties);

    utils::WaitEvent provideStateEvent1;
    EXPECT_CALL(*providerWithReportableStateProperties, provideState(_, _))
        .WillOnce((InvokeWithoutArgs([&provideStateEvent1] { provideStateEvent1.wakeUp(); })));

    utils::WaitEvent provideStateEvent2;
    EXPECT_CALL(*providerWithoutReportableStateProperties, provideState(_, _))
        .WillOnce((InvokeWithoutArgs([&provideStateEvent2] { provideStateEvent2.wakeUp(); })));

    auto requester = std::make_shared<MockContextRequester>();
    std::promise<AVSContext::States> contextStatesPromise;
    EXPECT_CALL(*requester, onContextAvailable(_, _, _))
        .WillOnce(WithArg<1>(Invoke([&contextStatesPromise](const AVSContext& context) {
            contextStatesPromise.set_value(context.getStates());
        })));

    // Get context for the target endpoint.
    auto requestToken = m_contextManager->getContext(requester);

    const std::chrono::milliseconds timeout{100};
    provideStateEvent1.wait(timeout);
    provideStateEvent2.wait(timeout);
    m_contextManager->provideStateResponse(capability2, state2, requestToken);
    m_contextManager->provideStateResponse(capability1, state1, requestToken);

    auto statesFuture = contextStatesPromise.get_future();
    ASSERT_EQ(statesFuture.wait_for(timeout), std::future_status::ready);

    auto states = statesFuture.get();
    EXPECT_EQ(states[capability2].valuePayload, state2.valuePayload);
    EXPECT_EQ(states[capability1].valuePayload, state1.valuePayload);
}

}  // namespace test
}  // namespace contextManager
}  // namespace alexaClientSDK
