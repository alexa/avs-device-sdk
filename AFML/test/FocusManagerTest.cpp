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

#include <memory>
#include <gtest/gtest.h>

#include <AVSCommon/AVS/FocusState.h>
#include <AVSCommon/SDKInterfaces/MockFocusManagerObserver.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include "AFML/FocusManager.h"

#include "InterruptModel/InterruptModel.h"

namespace alexaClientSDK {
namespace afml {
namespace test {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::avs;
using namespace alexaClientSDK::avsCommon::utils::configuration;
/// Alias for JSON stream type used in @c ConfigurationNode initialization
using JSONStream = std::vector<std::shared_ptr<std::istream>>;

/// Short time out for when callbacks are expected not to occur.
static const auto SHORT_TIMEOUT = std::chrono::milliseconds(50);

/// Long time out for the Channel observer to wait for the focus change callback (we should not reach this).
static const auto DEFAULT_TIMEOUT = std::chrono::seconds(15);

/// Time out to wait for ActivityUpdates.
static const auto NO_ACTIVITY_UPDATE_TIMEOUT = std::chrono::milliseconds(250);

/// The dialog Channel name used in initializing the FocusManager.
static const std::string DIALOG_CHANNEL_NAME = "Dialog";

/// The alerts Channel name used in initializing the FocusManager.
static const std::string ALERTS_CHANNEL_NAME = "Alert";

/// The content Channel name used in initializing the FocusManager.
static const std::string CONTENT_CHANNEL_NAME = "Content";

/// An incorrect Channel name that is never initialized as a Channel.
static const std::string INCORRECT_CHANNEL_NAME = "aksdjfl;aksdjfl;akdsjf";

/// The virtual Channel name used in initializing the FocusManager.
static const std::string VIRTUAL_CHANNEL_NAME = "VirtualChannel";

/// The priority of the virtual Channel used in initializing the FocusManager.
static const unsigned int VIRTUAL_CHANNEL_PRIORITY = 25;

/// The priority of the dialog Channel used in initializing the FocusManager.
static const unsigned int DIALOG_CHANNEL_PRIORITY = 10;

/// The priority of the alerts Channel used in initializing the FocusManager.
static const unsigned int ALERTS_CHANNEL_PRIORITY = 20;

/// The priority of the content Channel used in initializing the FocusManager.
static const unsigned int CONTENT_CHANNEL_PRIORITY = 30;

/// Sample dialog interface name.
static const std::string DIALOG_INTERFACE_NAME = "dialog";

/// Sample alerts interface name.
static const std::string ALERTS_INTERFACE_NAME = "alerts";

/// Sample content interface name.
static const std::string CONTENT_INTERFACE_NAME = "content";

/// Another sample content interface name.
static const std::string DIFFERENT_CONTENT_INTERFACE_NAME = "different content";

/// Another sample dialog interface name.
static const std::string DIFFERENT_DIALOG_INTERFACE_NAME = "different dialog";

/// Sample Virtual Channel interface name.
static const std::string VIRTUAL_INTERFACE_NAME = "virtual";

/// The root key for the InterruptModel in the @c InterruptModelConfiguration
static const std::string INTERRUPT_MODEL_ROOT_KEY = "interruptModel";

/// The Json that describes the InterruptModelConfiguration
static const std::string INTERRUPT_MODEL_CONFIG_JSON = R"({
"interruptModel" : {
                "Dialog" : {
                },
                "Communications" : {
                    "contentType":
                    {
                        "MIXABLE" : {
                            "incomingChannel" : {
                                "Dialog" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MAY_DUCK"
                                    }
                                }
                            }
                        },
                        "NONMIXABLE" : {
                            "incomingChannel" : {
                                "Dialog" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MAY_PAUSE"
                                    }
                                }
                            }
                        }
                    }
                },
                "Alert" : {
                    "contentType" :
                    {
                        "MIXABLE" : {
                            "incomingChannel" : {
                                "Dialog" : {
                                      "incomingContentType" : {
                                        "MIXABLE" : "MAY_DUCK"
                                    }
                                },
                                "Communications" : {
                                      "incomingContentType" : {
                                        "MIXABLE" : "MAY_DUCK",
                                        "NONMIXABLE" : "MAY_DUCK"
                                    }
                                }
                            }
                        }
                    }
                },
                "VirtualChannel" : {
                    "contentType" :
                    {
                        "MIXABLE" : {
                            "incomingChannel" : {
                                "Dialog" : {
                                      "incomingContentType" : {
                                        "MIXABLE" : "MAY_DUCK"
                                    }
                                },
                                "Communications" : {
                                      "incomingContentType" : {
                                        "MIXABLE" : "MAY_DUCK",
                                        "NONMIXABLE" : "MAY_DUCK"
                                    }
                                }
                            }
                        }
                    }
                },
                "Content" : {
                    "contentType" :
                    {
                        "MIXABLE" : {
                            "incomingChannel" : {
                                "Dialog" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MAY_DUCK"
                                    }
                                },
                                "Communications" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MAY_DUCK",
                                        "NONMIXABLE" : "MUST_PAUSE"
                                    }
                                },
                                "Alert" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MAY_DUCK"
                                    }
                                },
                                "VirtualChannel" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MAY_DUCK"
                                    }
                                }
                            }
                        },
                        "NONMIXABLE" : {
                            "incomingChannel" : {
                                "Dialog" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MUST_PAUSE"
                                    }
                                },
                                "Communications" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MUST_PAUSE",
                                        "NONMIXABLE" : "MUST_PAUSE"
                                    }
                                },
                                "Alert" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MUST_PAUSE"
                                    }
                                }
                            }
                        }
                    }
                }
            }
    }
)";

/// A test observer that mocks out the ChannelObserverInterface##onFocusChanged() call.
class TestClient
        : public ChannelObserverInterface
        , public std::enable_shared_from_this<TestClient> {
public:
    /**
     * Constructor.
     */
    TestClient(const std::string& channelName, const std::string& interfaceName) :
            m_channelName(channelName),
            m_interfaceName(interfaceName),
            m_focusState(FocusState::NONE),
            m_mixingBehavior(MixingBehavior::UNDEFINED),
            m_focusChangeCallbackInvoked(false),
            m_mixingBehaviorChanged(false) {
    }

    std::shared_ptr<FocusManagerInterface::Activity> createActivity(
        ContentType contentType = ContentType::NONMIXABLE,
        std::chrono::milliseconds patience = std::chrono::milliseconds(0)) {
        m_contentType = contentType;
        return FocusManagerInterface::Activity::create(m_interfaceName, shared_from_this(), patience, contentType);
    }

    const std::string& getChannelName() const {
        return m_channelName;
    }

    const std::string& getInterfaceName() const {
        return m_interfaceName;
    }

    /**
     * Implementation of the ChannelObserverInterface##onFocusChanged() callback.
     *
     * @param focusState The new focus state of the Channel observer.
     * @param behavior The new MixingBehavior of the Channel observer.
     */
    void onFocusChanged(FocusState focusState, MixingBehavior behavior) override {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_focusState = focusState;
        m_mixingBehaviorChanged = (m_mixingBehavior != behavior);
        m_mixingBehavior = behavior;
        m_focusChangeCallbackInvoked = true;
        m_focusChanged.notify_one();
    }

    struct testClientInfo {
        FocusState focusState;
        MixingBehavior mixingBehavior;
        bool focusChanged;
        bool mixingBehaviorChanged;
        testClientInfo(FocusState state, MixingBehavior behavior, bool focusChg, bool mixingBehaviorChg) :
                focusState{state},
                mixingBehavior{behavior},
                focusChanged{focusChg},
                mixingBehaviorChanged{mixingBehaviorChg} {
        }
        testClientInfo() :
                focusState{FocusState::NONE},
                mixingBehavior{MixingBehavior::UNDEFINED},
                focusChanged{false},
                mixingBehaviorChanged{false} {
        }
    };

    /**
     * Waits for the ChannelObserverInterface##onFocusChanged() callback.
     *
     * @param timeout The amount of time to wait for the callback.
     * @param focusChanged An output parameter that notifies the caller whether a callback occurred.
     * @return Returns @c true if the callback occured within the timeout period and @c false otherwise.
     */
    testClientInfo waitForFocusOrMixingBehaviorChange(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(m_mutex);
        auto success = m_focusChanged.wait_for(
            lock, timeout, [this]() { return m_focusChangeCallbackInvoked || m_mixingBehaviorChanged; });

        testClientInfo ret;
        if (!success) {
            ret.focusChanged = false;
            ret.mixingBehaviorChanged = false;
        } else {
            m_focusChangeCallbackInvoked = false;
            ret.focusChanged = true;
            ret.mixingBehaviorChanged = m_mixingBehaviorChanged;
            m_mixingBehaviorChanged = false;
        }

        ret.focusState = m_focusState;
        ret.mixingBehavior = m_mixingBehavior;

        return ret;
    }

private:
    std::string m_channelName;

    std::string m_interfaceName;

    /// The ContentType of the Activity.
    ContentType m_contentType;

    /// The focus state of the observer.
    FocusState m_focusState;

    /// The MixingBehavior of the observer.
    MixingBehavior m_mixingBehavior;

    /// A lock to guard against focus state changes.
    std::mutex m_mutex;

    /// A condition variable to wait for focus changes.
    std::condition_variable m_focusChanged;

    /// A boolean flag so that we can re-use the observer even after a callback has occurred.
    bool m_focusChangeCallbackInvoked;

    /// A re-usable boolean flag to indicate that the MixingBehavior changed
    bool m_mixingBehaviorChanged;
};

/// A test observer that mocks out the ActivityTrackerInterface##notifyOfActivityUpdates() call.
class MockActivityTrackerInterface : public ActivityTrackerInterface {
public:
    /**
     * Constructor.
     */
    MockActivityTrackerInterface() : m_activityUpdatesOccurred{false} {
    }

    /// Structure of expected Channel::State result from tests.
    struct ExpectedChannelStateResult {
        /// The expected channel name.
        const std::string name;

        /// The expected interface name.
        const std::string interfaceName;

        /// The expected focus state.
        const FocusState focusState;
    };

    /**
     * Implementation of the ActivityTrackerInterface##notifyOfActivityUpdates() callback.
     *
     * @param channelStates A vector of @c Channel::State that has been updated.
     */
    void notifyOfActivityUpdates(const std::vector<Channel::State>& channelStates) override {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_updatedChannels.clear();
        for (auto& channel : channelStates) {
            m_updatedChannels[channel.interfaceName] = channel;
        }
        m_activityUpdatesOccurred = true;
        m_activityChanged.notify_one();
    }

    /**
     * Waits for the ActivityTrackerInterface##notifyOfActivityUpdates() callback.
     *
     * @param timeout The amount of time to wait for the callback.
     * @param expected The expected channel state results
     */
    void waitForActivityUpdates(
        std::chrono::milliseconds timeout,
        const std::vector<ExpectedChannelStateResult>& expected) {
        std::unique_lock<std::mutex> lock(m_mutex);
        bool success = m_activityChanged.wait_for(lock, timeout, [this, &expected]() {
            if (m_activityUpdatesOccurred) {
                EXPECT_EQ(m_updatedChannels.size(), expected.size());
                auto count = 0;
                for (auto& expectedChannel : expected) {
                    auto& channel = m_updatedChannels[expectedChannel.interfaceName];
                    EXPECT_EQ(channel.name, expectedChannel.name);
                    EXPECT_EQ(channel.interfaceName, expectedChannel.interfaceName);
                    EXPECT_EQ(channel.focusState, expectedChannel.focusState);
                    count++;
                }
            }
            return m_activityUpdatesOccurred;
        });

        if (success) {
            m_activityUpdatesOccurred = false;
        }
        ASSERT_TRUE(success);
    }

    /**
     * Waits for the if there's a ActivityTrackerInterface##notifyOfActivityUpdates() callback.
     *
     * @param timeout The amount of time to wait for the callback.
     */
    bool waitForNoActivityUpdates(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_activityChanged.wait_for(lock, timeout);
        return m_activityUpdatesOccurred;
    }

private:
    /// The focus state of the observer. The key is the interface name.
    std::unordered_map<std::string, Channel::State> m_updatedChannels;

    /// A lock to guard against activity changes.
    std::mutex m_mutex;

    /// A condition variable to wait for activity changes.
    std::condition_variable m_activityChanged;

    /// A boolean flag so that we can re-use the observer even after a callback has occurred.
    bool m_activityUpdatesOccurred;
};

/// Manages testing focus changes
class FocusChangeManager {
public:
    TestClient::testClientInfo getWaitResult(std::shared_ptr<TestClient> client) {
        return client->waitForFocusOrMixingBehaviorChange(DEFAULT_TIMEOUT);
    }

    /**
     * Checks that a focus change occurred and that the focus state received is the same as the expected focus
     * state.
     *
     * @param client The Channel observer.
     * @param expectedFocusState The expected focus state.
     */
    void assertFocusChange(std::shared_ptr<TestClient> client, FocusState expectedFocusState) {
        auto waitResult = getWaitResult(client);
        ASSERT_TRUE(waitResult.focusChanged);
        ASSERT_EQ(expectedFocusState, waitResult.focusState);
    }

    /**
     * Checks that a focus change does not occur by waiting for the timeout duration.
     *
     * @param client The Channel observer.
     */
    void assertNoFocusChange(std::shared_ptr<TestClient> client) {
        // Will wait for the short timeout duration before succeeding
        auto ret = client->waitForFocusOrMixingBehaviorChange(SHORT_TIMEOUT);
        ASSERT_FALSE(ret.focusChanged);
    }

    void assertMixingBehaviorChange(std::shared_ptr<TestClient> client, MixingBehavior behavior) {
        // Will wait for the short timeout duration before succeeding
        auto ret = client->waitForFocusOrMixingBehaviorChange(SHORT_TIMEOUT);
        ASSERT_TRUE(ret.mixingBehaviorChanged);
        ASSERT_EQ(ret.mixingBehavior, behavior);
    }

    void assertNoMixingBehaviorChange(std::shared_ptr<TestClient> client) {
        // Will wait for the short timeout duration before succeeding
        auto ret = client->waitForFocusOrMixingBehaviorChange(SHORT_TIMEOUT);
        ASSERT_FALSE(ret.mixingBehaviorChanged);
    }

    void assertNoMixingBehaviorOrFocusChange(std::shared_ptr<TestClient> client) {
        // Will wait for the short timeout duration before succeeding
        auto ret = client->waitForFocusOrMixingBehaviorChange(SHORT_TIMEOUT);
        ASSERT_FALSE(ret.mixingBehaviorChanged);
        ASSERT_FALSE(ret.focusChanged);
    }

    void assertMixingBehaviorAndFocusChange(
        std::shared_ptr<TestClient> client,
        FocusState expectedFocusState,
        MixingBehavior behavior) {
        // Will wait for the short timeout duration before succeeding
        auto ret = client->waitForFocusOrMixingBehaviorChange(SHORT_TIMEOUT);
        ASSERT_TRUE(ret.mixingBehaviorChanged);
        ASSERT_TRUE(ret.focusChanged);
        ASSERT_EQ(expectedFocusState, ret.focusState);
        ASSERT_EQ(behavior, ret.mixingBehavior);
    }
};

/// Test fixture for testing FocusManager.
class FocusManagerTest
        : public ::testing::Test
        , public FocusChangeManager {
protected:
    /// The FocusManager.
    std::shared_ptr<FocusManager> m_focusManager;

    /// A client that acquires the dialog Channel.
    std::shared_ptr<TestClient> dialogClient;

    /// Another client that acquires the dialog Channel.
    std::shared_ptr<TestClient> anotherDialogClient;

    /// A client that acquires the alerts Channel.
    std::shared_ptr<TestClient> alertsClient;

    /// A client that acquires the content Channel.
    std::shared_ptr<TestClient> contentClient;

    /// A client that acquires the content Channel.
    std::shared_ptr<TestClient> anotherContentClient;

    /// A client that acquires the virtual Channel.
    std::shared_ptr<TestClient> virtualClient;

    /// Mock Activity Tracker
    std::shared_ptr<MockActivityTrackerInterface> m_activityTracker;

    /// Interrupt Model
    std::shared_ptr<interruptModel::InterruptModel> m_interruptModel;

    ConfigurationNode generateInterruptModelConfig() {
        auto stream = std::shared_ptr<std::istream>(new std::istringstream(INTERRUPT_MODEL_CONFIG_JSON));
        std::vector<std::shared_ptr<std::istream>> jsonStream({stream});
        ConfigurationNode::initialize(jsonStream);
        return ConfigurationNode::getRoot();
    }

    virtual void SetUp() {
        m_activityTracker = std::make_shared<MockActivityTrackerInterface>();

        FocusManager::ChannelConfiguration dialogChannelConfig{DIALOG_CHANNEL_NAME, DIALOG_CHANNEL_PRIORITY};
        FocusManager::ChannelConfiguration alertsChannelConfig{ALERTS_CHANNEL_NAME, ALERTS_CHANNEL_PRIORITY};
        FocusManager::ChannelConfiguration contentChannelConfig{CONTENT_CHANNEL_NAME, CONTENT_CHANNEL_PRIORITY};
        FocusManager::ChannelConfiguration virtualChannelConfig{VIRTUAL_CHANNEL_NAME, VIRTUAL_CHANNEL_PRIORITY};

        std::vector<FocusManager::ChannelConfiguration> channelConfigurations{
            dialogChannelConfig, alertsChannelConfig, contentChannelConfig};

        dialogClient = std::make_shared<TestClient>(DIALOG_CHANNEL_NAME, DIALOG_INTERFACE_NAME);
        alertsClient = std::make_shared<TestClient>(ALERTS_CHANNEL_NAME, ALERTS_INTERFACE_NAME);
        contentClient = std::make_shared<TestClient>(CONTENT_CHANNEL_NAME, CONTENT_INTERFACE_NAME);
        anotherContentClient = std::make_shared<TestClient>(CONTENT_CHANNEL_NAME, DIFFERENT_CONTENT_INTERFACE_NAME);
        anotherDialogClient = std::make_shared<TestClient>(DIALOG_CHANNEL_NAME, DIFFERENT_DIALOG_INTERFACE_NAME);
        virtualClient = std::make_shared<TestClient>(VIRTUAL_CHANNEL_NAME, VIRTUAL_INTERFACE_NAME);

        std::vector<FocusManager::ChannelConfiguration> virtualChannelConfigurations{virtualChannelConfig};

        m_interruptModel =
            interruptModel::InterruptModel::create(generateInterruptModelConfig()[INTERRUPT_MODEL_ROOT_KEY]);
        m_focusManager = std::make_shared<FocusManager>(
            channelConfigurations, m_activityTracker, virtualChannelConfigurations, m_interruptModel);
    }

    bool acquireChannelHelper(
        std::shared_ptr<TestClient> client,
        ContentType contentType = ContentType::NONMIXABLE,
        std::chrono::milliseconds patience = std::chrono::milliseconds(0)) {
        auto activity = client->createActivity(contentType, patience);
        return m_focusManager->acquireChannel(client->getChannelName(), activity);
    }
};

/// Tests acquireChannel with an invalid Channel name, expecting no focus changes to be made.
TEST_F(FocusManagerTest, test_acquireInvalidChannelName) {
    ASSERT_FALSE(m_focusManager->acquireChannel(INCORRECT_CHANNEL_NAME, dialogClient, DIALOG_INTERFACE_NAME));
}

/// Tests acquireChannel, expecting to get Foreground status since no other Channels are active.
TEST_F(FocusManagerTest, test_acquireChannelWithNoOtherChannelsActive) {
    ASSERT_TRUE(acquireChannelHelper(dialogClient, ContentType::MIXABLE));
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);
}

/**
 * Tests acquireChannel with two Channels. The lower priority Channel should get Background focus and the higher
 * priority Channel should get Foreground focus.
 */
TEST_F(FocusManagerTest, test_acquireLowerPriorityChannelWithOneHigherPriorityChannelTaken) {
    ASSERT_TRUE(acquireChannelHelper(dialogClient, ContentType::MIXABLE));
    ASSERT_TRUE(acquireChannelHelper(alertsClient, ContentType::MIXABLE));
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    assertMixingBehaviorAndFocusChange(alertsClient, FocusState::BACKGROUND, MixingBehavior::MAY_DUCK);
}

/**
 * Tests acquireChannel with three Channels. The two lowest priority Channels should get Background focus while the
 * highest priority Channel should be Foreground focused.
 */
TEST_F(FocusManagerTest, test_aquireLowerPriorityChannelWithTwoHigherPriorityChannelsTaken) {
    ASSERT_TRUE(acquireChannelHelper(dialogClient, ContentType::MIXABLE));
    ASSERT_TRUE(acquireChannelHelper(alertsClient, ContentType::MIXABLE));
    ASSERT_TRUE(acquireChannelHelper(contentClient, ContentType::MIXABLE));
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    assertMixingBehaviorAndFocusChange(alertsClient, FocusState::BACKGROUND, MixingBehavior::MAY_DUCK);
    assertMixingBehaviorAndFocusChange(contentClient, FocusState::BACKGROUND, MixingBehavior::MAY_DUCK);
}

/**
 * Tests acquireChannel virtual channel is working the same as other channels.  We test with three Channels (one
 * virtual). First we acquire a non-virtual channel and make sure it goes FOREGROUND.  Then we acquire a virtual channel
 * with higher prioirty and one non-virtual with lower priority and make sure the two lowest priority Channels should
 * get Background focus while the highest priority Channel should be Foreground focused.
 */
TEST_F(FocusManagerTest, acquireVirtualChannelWithTwoLowerPriorityChannelsTaken) {
    ASSERT_TRUE(acquireChannelHelper(dialogClient, ContentType::MIXABLE));
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);

    ASSERT_TRUE(acquireChannelHelper(contentClient, ContentType::MIXABLE));
    ASSERT_TRUE(acquireChannelHelper(virtualClient, ContentType::MIXABLE));
    assertMixingBehaviorAndFocusChange(virtualClient, FocusState::BACKGROUND, MixingBehavior::MAY_DUCK);
    assertMixingBehaviorAndFocusChange(contentClient, FocusState::BACKGROUND, MixingBehavior::MAY_DUCK);
}

/**
 * Tests acquireChannel with a high priority Channel while a low priority Channel is already taken. The lower priority
 * Channel should at first be Foreground focused and then get a change to Background focus while the higher priority
 * should be Foreground focused.
 */
TEST_F(FocusManagerTest, test_acquireHigherPriorityChannelWithOneLowerPriorityChannelTaken) {
    ASSERT_TRUE(acquireChannelHelper(contentClient, ContentType::MIXABLE));
    assertMixingBehaviorAndFocusChange(contentClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);

    ASSERT_TRUE(acquireChannelHelper(dialogClient, ContentType::MIXABLE));
    assertMixingBehaviorAndFocusChange(contentClient, FocusState::BACKGROUND, MixingBehavior::MAY_DUCK);
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);
}

/**
 * Tests acquireChannel with a single Channel. The original observer should be notified to stop and the new observer
 * should obtain Foreground focus.
 */
TEST_F(FocusManagerTest, test_kickOutActivityOnSameChannel) {
    ASSERT_TRUE(acquireChannelHelper(dialogClient));
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);

    ASSERT_TRUE(acquireChannelHelper(anotherDialogClient));
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::NONE, MixingBehavior::MUST_STOP);
    assertMixingBehaviorAndFocusChange(anotherDialogClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);
}

/**
 * Tests releaseChannel with a single Channel. The observer should be notified to stop.
 */
TEST_F(FocusManagerTest, test_simpleReleaseChannel) {
    ASSERT_TRUE(acquireChannelHelper(dialogClient));
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);

    ASSERT_TRUE(m_focusManager->releaseChannel(DIALOG_CHANNEL_NAME, dialogClient).get());
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::NONE, MixingBehavior::MUST_STOP);
}

/**
 * Tests releaseChannel on a Channel with an incorrect observer. The client should not receive any callback.
 */
TEST_F(FocusManagerTest, test_simpleReleaseChannelWithIncorrectObserver) {
    ASSERT_TRUE(acquireChannelHelper(dialogClient));
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);

    ASSERT_FALSE(m_focusManager->releaseChannel(CONTENT_CHANNEL_NAME, dialogClient).get());
    ASSERT_FALSE(m_focusManager->releaseChannel(DIALOG_CHANNEL_NAME, contentClient).get());

    assertNoMixingBehaviorOrFocusChange(dialogClient);
    assertNoMixingBehaviorOrFocusChange(contentClient);
}

/**
 * Tests releaseChannel of the Foreground focused Channel while another Channel is taken. The originally Background
 * focused Channel should be notified to come to the Foreground while the originally Foreground focused Channel should
 * be notified to stop.
 */
TEST_F(FocusManagerTest, test_releaseForegroundChannelWhileBackgroundChannelTaken) {
    ASSERT_TRUE(acquireChannelHelper(dialogClient, ContentType::MIXABLE));
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);

    ASSERT_TRUE(acquireChannelHelper(contentClient, ContentType::MIXABLE));
    assertMixingBehaviorAndFocusChange(contentClient, FocusState::BACKGROUND, MixingBehavior::MAY_DUCK);
    assertNoMixingBehaviorOrFocusChange(dialogClient);

    ASSERT_TRUE(m_focusManager->releaseChannel(DIALOG_CHANNEL_NAME, dialogClient).get());
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::NONE, MixingBehavior::MUST_STOP);
    assertMixingBehaviorAndFocusChange(contentClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);
}

/**
 * Tests stopForegroundActivity with a single Channel. The observer should be notified to stop.
 */
TEST_F(FocusManagerTest, test_simpleNonTargetedStop) {
    ASSERT_TRUE(acquireChannelHelper(dialogClient, ContentType::MIXABLE));
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);

    m_focusManager->stopForegroundActivity();
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::NONE, MixingBehavior::MUST_STOP);
}

/**
 * Tests stopForegroundActivity with a three active Channels. The Foreground Channel observer should be notified to
 * stop each time and the next highest priority background Channel should be brought to the foreground each time.
 */
TEST_F(FocusManagerTest, test_threeNonTargetedStopsWithThreeActivitiesHappening) {
    ASSERT_TRUE(acquireChannelHelper(dialogClient, ContentType::MIXABLE));
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);

    ASSERT_TRUE(acquireChannelHelper(alertsClient, ContentType::MIXABLE));
    assertMixingBehaviorAndFocusChange(alertsClient, FocusState::BACKGROUND, MixingBehavior::MAY_DUCK);

    ASSERT_TRUE(acquireChannelHelper(contentClient, ContentType::MIXABLE));
    assertMixingBehaviorAndFocusChange(contentClient, FocusState::BACKGROUND, MixingBehavior::MAY_DUCK);
    assertNoMixingBehaviorOrFocusChange(alertsClient);

    m_focusManager->stopForegroundActivity();
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::NONE, MixingBehavior::MUST_STOP);
    assertMixingBehaviorAndFocusChange(alertsClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    // no change in mixingbehavior or focusstate for content, no activity updates either
    assertNoMixingBehaviorOrFocusChange(contentClient);

    m_focusManager->stopForegroundActivity();
    assertMixingBehaviorAndFocusChange(alertsClient, FocusState::NONE, MixingBehavior::MUST_STOP);
    assertMixingBehaviorAndFocusChange(contentClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);

    m_focusManager->stopForegroundActivity();
    assertMixingBehaviorAndFocusChange(contentClient, FocusState::NONE, MixingBehavior::MUST_STOP);
}

/**
 * Tests stopForegroundActivity with a single Channel. The next client to request a different Channel should be given
 * foreground focus.
 */
TEST_F(FocusManagerTest, test_stopForegroundActivityAndAcquireDifferentChannel) {
    ASSERT_TRUE(acquireChannelHelper(dialogClient));
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);

    m_focusManager->stopForegroundActivity();
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::NONE, MixingBehavior::MUST_STOP);

    ASSERT_TRUE(acquireChannelHelper(contentClient));
    assertMixingBehaviorAndFocusChange(contentClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);
}

/**
 * Tests stopForegroundActivity with a single Channel. The next client to request the same Channel should be given
 * foreground focus.
 */
TEST_F(FocusManagerTest, test_stopForegroundActivityAndAcquireSameChannel) {
    ASSERT_TRUE(acquireChannelHelper(dialogClient));
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);

    m_focusManager->stopForegroundActivity();
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::NONE, MixingBehavior::MUST_STOP);

    ASSERT_TRUE(acquireChannelHelper(dialogClient));
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);
}

/**
 * Test stopAllActivities with a single channel.
 * Expect focus change only on that channel and reacquiring the same channel should resulted in foreground focus.
 */
TEST_F(FocusManagerTest, test_stopAllActivitiesWithSingleChannel) {
    ASSERT_TRUE(acquireChannelHelper(dialogClient));
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);

    m_focusManager->stopAllActivities();
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::NONE, MixingBehavior::MUST_STOP);
    assertNoMixingBehaviorOrFocusChange(contentClient);
    assertNoMixingBehaviorOrFocusChange(alertsClient);

    ASSERT_TRUE(acquireChannelHelper(dialogClient));
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);
}

/**
 * Test stopAllActivities with three channels.
 * Expect focus change to none for all channels and a channel should resulted in foreground focus.
 */
TEST_F(FocusManagerTest, test_stopAllActivitiesWithThreeChannels) {
    ASSERT_TRUE(acquireChannelHelper(dialogClient, ContentType::MIXABLE));
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);

    ASSERT_TRUE(acquireChannelHelper(contentClient));
    assertMixingBehaviorAndFocusChange(contentClient, FocusState::BACKGROUND, MixingBehavior::MUST_PAUSE);

    ASSERT_TRUE(acquireChannelHelper(alertsClient, ContentType::MIXABLE));
    assertMixingBehaviorAndFocusChange(alertsClient, FocusState::BACKGROUND, MixingBehavior::MAY_DUCK);

    m_focusManager->stopAllActivities();

    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::NONE, MixingBehavior::MUST_STOP);
    assertMixingBehaviorAndFocusChange(contentClient, FocusState::NONE, MixingBehavior::MUST_STOP);
    assertMixingBehaviorAndFocusChange(alertsClient, FocusState::NONE, MixingBehavior::MUST_STOP);

    ASSERT_TRUE(acquireChannelHelper(dialogClient));
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);
}

/**
 * Test stopAllActivities with a single channel.
 * Expect focus change only on that channel and reacquiring the same channel should result in foreground focus.
 */
TEST_F(FocusManagerTest, stopAllActivitiesWithSingleChannel) {
    ASSERT_TRUE(acquireChannelHelper(dialogClient));
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);

    m_focusManager->stopAllActivities();
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::NONE, MixingBehavior::MUST_STOP);

    assertNoMixingBehaviorOrFocusChange(contentClient);
    assertNoMixingBehaviorOrFocusChange(alertsClient);

    ASSERT_TRUE(acquireChannelHelper(dialogClient));
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);
}

/**
 * Test stopAllActivities with three channels.
 * Expect focus change to none for all channels and a channel should result in foreground focus.
 */
TEST_F(FocusManagerTest, stopAllActivitiesWithThreeChannels) {
    ASSERT_TRUE(acquireChannelHelper(dialogClient, ContentType::MIXABLE));
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);

    ASSERT_TRUE(acquireChannelHelper(contentClient));
    assertMixingBehaviorAndFocusChange(contentClient, FocusState::BACKGROUND, MixingBehavior::MUST_PAUSE);

    ASSERT_TRUE(acquireChannelHelper(alertsClient, ContentType::MIXABLE));
    assertMixingBehaviorAndFocusChange(alertsClient, FocusState::BACKGROUND, MixingBehavior::MAY_DUCK);
    assertNoMixingBehaviorOrFocusChange(contentClient);

    m_focusManager->stopAllActivities();

    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::NONE, MixingBehavior::MUST_STOP);
    assertMixingBehaviorAndFocusChange(contentClient, FocusState::NONE, MixingBehavior::MUST_STOP);
    assertMixingBehaviorAndFocusChange(alertsClient, FocusState::NONE, MixingBehavior::MUST_STOP);

    ASSERT_TRUE(acquireChannelHelper(dialogClient));
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);
}

/**
 * Tests releaseChannel with the background Channel while there is a foreground Channel. The foreground Channel
 * should remain foregrounded while the background Channel's observer should be notified to stop.
 */
TEST_F(FocusManagerTest, test_releaseBackgroundChannelWhileTwoChannelsTaken) {
    ASSERT_TRUE(acquireChannelHelper(dialogClient, ContentType::MIXABLE));
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);

    ASSERT_TRUE(acquireChannelHelper(contentClient));
    assertMixingBehaviorAndFocusChange(contentClient, FocusState::BACKGROUND, MixingBehavior::MUST_PAUSE);

    ASSERT_TRUE(m_focusManager->releaseChannel(CONTENT_CHANNEL_NAME, contentClient).get());
    assertMixingBehaviorAndFocusChange(contentClient, FocusState::NONE, MixingBehavior::MUST_STOP);

    assertNoMixingBehaviorOrFocusChange(dialogClient);
}

/**
 * Tests acquireChannel of an already active foreground Channel while another Channel is also active. The original
 * observer of the foreground be notified to stop and the new observer of the Channel will be notified that it has
 * Foreground focus. The originally backgrounded Channel should not change focus.
 */
TEST_F(FocusManagerTest, test_kickOutActivityOnSameChannelWhileOtherChannelsActive) {
    ASSERT_TRUE(acquireChannelHelper(dialogClient, ContentType::MIXABLE));
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);

    ASSERT_TRUE(acquireChannelHelper(contentClient));
    assertMixingBehaviorAndFocusChange(contentClient, FocusState::BACKGROUND, MixingBehavior::MUST_PAUSE);

    ASSERT_TRUE(acquireChannelHelper(anotherDialogClient, ContentType::MIXABLE));
    assertMixingBehaviorAndFocusChange(dialogClient, FocusState::NONE, MixingBehavior::MUST_STOP);
    assertMixingBehaviorAndFocusChange(anotherDialogClient, FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    assertNoMixingBehaviorOrFocusChange(contentClient);
}

/// Tests that multiple observers can be added, and that they are notified of all focus changes.
TEST_F(FocusManagerTest, test_addObserver) {
    // These are all the observers that will be added.
    std::vector<std::shared_ptr<MockFocusManagerObserver>> observers;
    observers.push_back(std::make_shared<MockFocusManagerObserver>());
    observers.push_back(std::make_shared<MockFocusManagerObserver>());

    for (auto& observer : observers) {
        m_focusManager->addObserver(observer);
    }

    // Focus change on DIALOG channel.
    for (auto& observer : observers) {
        observer->expectFocusChange(DIALOG_CHANNEL_NAME, FocusState::FOREGROUND);
    }
    ASSERT_TRUE(acquireChannelHelper(dialogClient));

    // Focus change on CONTENT channel.
    for (auto& observer : observers) {
        observer->expectFocusChange(CONTENT_CHANNEL_NAME, FocusState::BACKGROUND);
    }
    ASSERT_TRUE(acquireChannelHelper(contentClient));

    // Wait for all pending changes to complete.
    for (auto& observer : observers) {
        ASSERT_TRUE(observer->waitForFocusChanges(DEFAULT_TIMEOUT));
    }

    // Drop foreground focus.
    for (auto& observer : observers) {
        observer->expectFocusChange(DIALOG_CHANNEL_NAME, FocusState::NONE);
        observer->expectFocusChange(CONTENT_CHANNEL_NAME, FocusState::FOREGROUND);
    }
    m_focusManager->stopForegroundActivity();

    for (auto& observer : observers) {
        ASSERT_TRUE(observer->waitForFocusChanges(DEFAULT_TIMEOUT));
    }
}

/// Tests that observers can be removed, and that they are no longer notified of focus changes after removal.
TEST_F(FocusManagerTest, test_removeObserver) {
    // These are all the observers that will ever be added.
    std::vector<std::shared_ptr<MockFocusManagerObserver>> allObservers;

    // Note: StrictMock here so that we fail on unexpected observer callbacks.
    allObservers.push_back(std::make_shared<testing::StrictMock<MockFocusManagerObserver>>());
    allObservers.push_back(std::make_shared<testing::StrictMock<MockFocusManagerObserver>>());

    for (auto& observer : allObservers) {
        m_focusManager->addObserver(observer);
    }

    // These are the observers which are currently added.
    auto activeObservers = allObservers;

    // One focus change with all observers added.
    for (auto& observer : activeObservers) {
        observer->expectFocusChange(DIALOG_CHANNEL_NAME, FocusState::FOREGROUND);
    }
    ASSERT_TRUE(acquireChannelHelper(dialogClient));

    // Wait for all pending changes to complete.
    for (auto& observer : allObservers) {
        ASSERT_TRUE(observer->waitForFocusChanges(DEFAULT_TIMEOUT));
    }

    // Remove an observer.
    m_focusManager->removeObserver(activeObservers.back());
    activeObservers.pop_back();

    // Now another focus change with the removed observer.
    for (auto& observer : activeObservers) {
        observer->expectFocusChange(CONTENT_CHANNEL_NAME, FocusState::BACKGROUND);
    }
    ASSERT_TRUE(acquireChannelHelper(contentClient));

    // Wait for all pending changes to complete.
    for (auto& observer : allObservers) {
        ASSERT_TRUE(observer->waitForFocusChanges(DEFAULT_TIMEOUT));
    }

    // Remove all remaining observers.
    for (auto& observer : activeObservers) {
        m_focusManager->removeObserver(observer);
    }
    activeObservers.clear();

    // And a final focus change with no observers.
    m_focusManager->stopForegroundActivity();

    for (auto& observer : allObservers) {
        ASSERT_TRUE(observer->waitForFocusChanges(DEFAULT_TIMEOUT));
    }
}

/**
 * Tests activityTracker with three Channels and make sure notifyOfActivityUpdates() is called correctly.
 */
TEST_F(FocusManagerTest, test_activityTracker) {
    // Acquire Content channel and expect notifyOfActivityUpdates() to notify activities on the Content channel.
    const std::vector<MockActivityTrackerInterface::ExpectedChannelStateResult> test1 = {
        {CONTENT_CHANNEL_NAME, CONTENT_INTERFACE_NAME, FocusState::FOREGROUND}};
    ASSERT_TRUE(acquireChannelHelper(contentClient));
    m_activityTracker->waitForActivityUpdates(DEFAULT_TIMEOUT, test1);

    // Acquire Alert channel and expect notifyOfActivityUpdates() to notify activities to both Content and Alert
    // channels.
    const std::vector<MockActivityTrackerInterface::ExpectedChannelStateResult> test2 = {
        {CONTENT_CHANNEL_NAME, CONTENT_INTERFACE_NAME, FocusState::BACKGROUND},
        {ALERTS_CHANNEL_NAME, ALERTS_INTERFACE_NAME, FocusState::FOREGROUND}};
    ASSERT_TRUE(acquireChannelHelper(alertsClient, ContentType::MIXABLE));
    m_activityTracker->waitForActivityUpdates(DEFAULT_TIMEOUT, test2);

    // Acquire Dialog channel and expect notifyOfActivityUpdates() to notify activities to both Alert and Dialog
    // channels.
    const std::vector<MockActivityTrackerInterface::ExpectedChannelStateResult> test3 = {
        {ALERTS_CHANNEL_NAME, ALERTS_INTERFACE_NAME, FocusState::BACKGROUND},
        {DIALOG_CHANNEL_NAME, DIALOG_INTERFACE_NAME, FocusState::FOREGROUND}};
    ASSERT_TRUE(acquireChannelHelper(dialogClient, ContentType::MIXABLE));
    m_activityTracker->waitForActivityUpdates(DEFAULT_TIMEOUT, test3);

    // Release Content channel and expect notifyOfActivityUpdates() to notify activities to Content channel.
    const std::vector<MockActivityTrackerInterface::ExpectedChannelStateResult> test4 = {
        {CONTENT_CHANNEL_NAME, CONTENT_INTERFACE_NAME, FocusState::NONE}};
    ASSERT_TRUE(m_focusManager->releaseChannel(CONTENT_CHANNEL_NAME, contentClient).get());
    m_activityTracker->waitForActivityUpdates(DEFAULT_TIMEOUT, test4);

    // Acquire Dialog channel with a different interface and expect notifyOfActivityUpdates() to notify activities to
    // Dialog channels first with focus change on the previous one, and then later with the updated interface.
    const std::vector<MockActivityTrackerInterface::ExpectedChannelStateResult> test5 = {
        {DIALOG_CHANNEL_NAME, DIALOG_INTERFACE_NAME, FocusState::NONE},
        {DIALOG_CHANNEL_NAME, DIFFERENT_DIALOG_INTERFACE_NAME, FocusState::FOREGROUND}};
    ASSERT_TRUE(acquireChannelHelper(anotherDialogClient));
    m_activityTracker->waitForActivityUpdates(DEFAULT_TIMEOUT, test5);

    // Release Dialog channel and expect notifyOfActivityUpdates() to notify activities to both Dialog and Alerts
    // channels.
    const std::vector<MockActivityTrackerInterface::ExpectedChannelStateResult> test6 = {
        {ALERTS_CHANNEL_NAME, ALERTS_INTERFACE_NAME, FocusState::FOREGROUND},
        {DIALOG_CHANNEL_NAME, DIFFERENT_DIALOG_INTERFACE_NAME, FocusState::NONE}};
    ASSERT_TRUE(m_focusManager->releaseChannel(DIALOG_CHANNEL_NAME, anotherDialogClient).get());
    m_activityTracker->waitForActivityUpdates(DEFAULT_TIMEOUT, test6);

    // Release Alerts channel and expect notifyOfActivityUpdates() to notify activities to Alerts channel.
    const std::vector<MockActivityTrackerInterface::ExpectedChannelStateResult> test7 = {
        {ALERTS_CHANNEL_NAME, ALERTS_INTERFACE_NAME, FocusState::NONE}};
    ASSERT_TRUE(m_focusManager->releaseChannel(ALERTS_CHANNEL_NAME, alertsClient).get());
    m_activityTracker->waitForActivityUpdates(DEFAULT_TIMEOUT, test7);

    // acquire Virtual channel and expect no notifyOfActivityUpdates().
    ASSERT_TRUE(acquireChannelHelper(virtualClient));
    ASSERT_FALSE(m_activityTracker->waitForNoActivityUpdates(NO_ACTIVITY_UPDATE_TIMEOUT));

    // release Virtual channel and expect no notifyOfActivityUpdates().
    ASSERT_TRUE(m_focusManager->releaseChannel(VIRTUAL_CHANNEL_NAME, virtualClient).get());
    ASSERT_FALSE(m_activityTracker->waitForNoActivityUpdates(NO_ACTIVITY_UPDATE_TIMEOUT));

    // Acquire Content channel and expect notifyOfActivityUpdates() to notify activities on the Content channel.
    const std::vector<MockActivityTrackerInterface::ExpectedChannelStateResult> test8 = {
        {CONTENT_CHANNEL_NAME, CONTENT_INTERFACE_NAME, FocusState::FOREGROUND}};
    ASSERT_TRUE(acquireChannelHelper(contentClient));
    m_activityTracker->waitForActivityUpdates(DEFAULT_TIMEOUT, test8);

    // acquire Virtual channel and expect no updates to virtual channel but Content channel to go to BACKGROUND.
    const std::vector<MockActivityTrackerInterface::ExpectedChannelStateResult> test9 = {
        {CONTENT_CHANNEL_NAME, CONTENT_INTERFACE_NAME, FocusState::BACKGROUND}};
    ASSERT_TRUE(acquireChannelHelper(virtualClient));
    m_activityTracker->waitForActivityUpdates(DEFAULT_TIMEOUT, test9);
}

/// Test fixture for testing Channel.
class ChannelTest
        : public ::testing::Test
        , public FocusChangeManager {
protected:
    /// observer A for the Content MultiActivity Channel
    std::shared_ptr<TestClient> clientA;

    /// observer B for the Content MultiActivity Channel
    std::shared_ptr<TestClient> clientB;

    /// observer C for the Content MultiActivity Channel
    std::shared_ptr<TestClient> clientC;

    /// A test Channel.
    std::shared_ptr<Channel> testChannel;

    /// Content Channel.
    std::shared_ptr<Channel> contentChannel;

    virtual void SetUp() {
        clientA = std::make_shared<TestClient>(CONTENT_CHANNEL_NAME, "ClientA_Interface");
        clientB = std::make_shared<TestClient>(CONTENT_CHANNEL_NAME, "ClientB_Interface");
        clientC = std::make_shared<TestClient>(CONTENT_CHANNEL_NAME, "ClientC_Interface");
        testChannel = std::make_shared<Channel>(DIALOG_CHANNEL_NAME, DIALOG_CHANNEL_PRIORITY);
        contentChannel = std::make_shared<Channel>(CONTENT_CHANNEL_NAME, CONTENT_CHANNEL_PRIORITY);
    }

    struct ActivityUpdateElem {
        std::string m_interfaceName;
        FocusState m_focusState;
        ActivityUpdateElem(std::string interfaceName, FocusState focus) :
                m_interfaceName{interfaceName},
                m_focusState{focus} {
        }
    };

    void checkActivityUpdates(std::shared_ptr<Channel> channel, std::vector<ActivityUpdateElem>& incoming) {
        auto activityUpdates = channel->getActivityUpdates();
        ASSERT_EQ(incoming.size(), activityUpdates.size());
        for (size_t i = 0; i < incoming.size(); i++) {
            ASSERT_EQ(activityUpdates.at(i).interfaceName, incoming.at(i).m_interfaceName);
            ASSERT_EQ(activityUpdates.at(i).focusState, incoming.at(i).m_focusState);
        }

        incoming.clear();
    }
};

/// Tests that the getName method of Channel works properly.
TEST_F(ChannelTest, test_getName) {
    ASSERT_EQ(testChannel->getName(), DIALOG_CHANNEL_NAME);
}

/// Tests that the getPriority method of Channel works properly.
TEST_F(ChannelTest, test_getPriority) {
    ASSERT_EQ(testChannel->getPriority(), DIALOG_CHANNEL_PRIORITY);
}

/// Tests that the observer properly gets notified of focus changes.
TEST_F(ChannelTest, test_setObserverThenSetFocus) {
    auto Activity_A = clientA->createActivity();
    testChannel->setPrimaryActivity(Activity_A);

    ASSERT_TRUE(testChannel->setFocus(FocusState::FOREGROUND, MixingBehavior::PRIMARY));
    assertMixingBehaviorAndFocusChange(clientA, FocusState::FOREGROUND, MixingBehavior::PRIMARY);

    ASSERT_TRUE(testChannel->setFocus(FocusState::BACKGROUND, MixingBehavior::MUST_PAUSE));
    assertMixingBehaviorAndFocusChange(clientA, FocusState::BACKGROUND, MixingBehavior::MUST_PAUSE);

    ASSERT_TRUE(testChannel->setFocus(FocusState::NONE, MixingBehavior::MUST_STOP));
    assertMixingBehaviorAndFocusChange(clientA, FocusState::NONE, MixingBehavior::MUST_STOP);

    ASSERT_FALSE(testChannel->setFocus(FocusState::NONE, MixingBehavior::MUST_STOP));
}

/// Tests that Channels are compared properly
TEST_F(ChannelTest, test_priorityComparison) {
    std::shared_ptr<Channel> lowerPriorityChannel =
        std::make_shared<Channel>(CONTENT_CHANNEL_NAME, CONTENT_CHANNEL_PRIORITY);
    ASSERT_TRUE(*testChannel > *lowerPriorityChannel);
    ASSERT_FALSE(*lowerPriorityChannel > *testChannel);
}

/// Tests that a Channel correctly reports whether it is active.
TEST_F(ChannelTest, test_isChannelActive) {
    // initially channel is not active
    ASSERT_FALSE(testChannel->isActive());

    // add Activity_A to the channel
    auto Activity_A = clientA->createActivity();
    testChannel->setPrimaryActivity(Activity_A);
    ASSERT_TRUE(testChannel->isActive());

    // add Activity_B to the channel
    auto Activity_B = clientB->createActivity(avsCommon::avs::ContentType::NONMIXABLE, std::chrono::seconds(2));
    testChannel->setPrimaryActivity(Activity_B);
    ASSERT_TRUE(testChannel->isActive());

    // release Activity_A
    testChannel->releaseActivity(clientA);
    ASSERT_TRUE(testChannel->isActive());

    // release Activity_B
    testChannel->releaseActivity(clientB);
    ASSERT_FALSE(testChannel->isActive());
}

/*
 * Tests that the timeAtIdle only gets updated when channel goes to idle and not when channel goes to Foreground or
 * Background.
 */
TEST_F(ChannelTest, test_getTimeAtIdle) {
    auto startTime = testChannel->getState().timeAtIdle;
    ASSERT_TRUE(testChannel->setFocus(FocusState::FOREGROUND, MixingBehavior::PRIMARY));
    auto afterForegroundTime = testChannel->getState().timeAtIdle;
    ASSERT_EQ(startTime, afterForegroundTime);

    ASSERT_TRUE(testChannel->setFocus(FocusState::BACKGROUND, MixingBehavior::MUST_PAUSE));
    auto afterBackgroundTime = testChannel->getState().timeAtIdle;
    ASSERT_EQ(afterBackgroundTime, afterForegroundTime);

    ASSERT_TRUE(testChannel->setFocus(FocusState::NONE, MixingBehavior::MUST_STOP));
    auto afterNoneTime = testChannel->getState().timeAtIdle;
    ASSERT_GT(afterNoneTime, afterBackgroundTime);
}

TEST_F(ChannelTest, test_MultiActivity_NewActivityKicksExistingActivity) {
    std::vector<ActivityUpdateElem> expectedUpdates;

    // client A acquires Content Channel
    auto Activity_A = clientA->createActivity();
    contentChannel->setPrimaryActivity(Activity_A);
    ASSERT_TRUE(contentChannel->getState().interfaceName == Activity_A->getInterface());
    contentChannel->setFocus(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    expectedUpdates.push_back({ActivityUpdateElem(clientA->getInterfaceName(), FocusState::FOREGROUND)});
    checkActivityUpdates(contentChannel, expectedUpdates);

    // client B acquires Content Channel : it must kick out client A
    auto Activity_B = clientB->createActivity();
    contentChannel->setPrimaryActivity(Activity_B);
    contentChannel->setFocus(FocusState::FOREGROUND, MixingBehavior::PRIMARY, true);
    expectedUpdates.push_back({ActivityUpdateElem(clientA->getInterfaceName(), FocusState::NONE)});
    expectedUpdates.push_back({ActivityUpdateElem(clientB->getInterfaceName(), FocusState::FOREGROUND)});
    checkActivityUpdates(contentChannel, expectedUpdates);
    ASSERT_TRUE(contentChannel->getState().interfaceName == Activity_B->getInterface());
    ASSERT_EQ(contentChannel->getActivity(Activity_A->getInterface()), nullptr);

    // when client B releases the channel , activity list becomes empty
    contentChannel->releaseActivity(clientB->getInterfaceName());
    contentChannel->setFocus(FocusState::NONE, MixingBehavior::MUST_STOP);
    expectedUpdates.push_back({ActivityUpdateElem(clientB->getInterfaceName(), FocusState::NONE)});
    checkActivityUpdates(contentChannel, expectedUpdates);
    ASSERT_EQ(contentChannel->getActivity(Activity_B->getInterface()), nullptr);
}

TEST_F(ChannelTest, test_MultiActivity_IncomingActivityWithPatience1) {
    std::vector<ActivityUpdateElem> expectedUpdates;

    // client A acquires Content Channel
    auto Activity_A = clientA->createActivity();
    contentChannel->setPrimaryActivity(Activity_A);
    ASSERT_TRUE(contentChannel->getState().interfaceName == Activity_A->getInterface());
    contentChannel->setFocus(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    expectedUpdates.push_back({ActivityUpdateElem(clientA->getInterfaceName(), FocusState::FOREGROUND)});
    checkActivityUpdates(contentChannel, expectedUpdates);

    // client B acquires Content Channel (with a patience parameter)
    // activityUpdate must show client A with FocusState::NONE
    // however client A is not kicked out
    auto Activity_B = clientB->createActivity(ContentType::MIXABLE, std::chrono::seconds(5));
    contentChannel->setPrimaryActivity(Activity_B);
    contentChannel->setFocus(FocusState::FOREGROUND, MixingBehavior::PRIMARY, true);
    expectedUpdates.push_back({ActivityUpdateElem(clientA->getInterfaceName(), FocusState::NONE)});
    expectedUpdates.push_back({ActivityUpdateElem(clientB->getInterfaceName(), FocusState::FOREGROUND)});
    checkActivityUpdates(contentChannel, expectedUpdates);
    ASSERT_TRUE(contentChannel->getState().interfaceName == Activity_B->getInterface());
    ASSERT_NE(contentChannel->getActivity(Activity_A->getInterface()), nullptr);

    // when client B activity is released , activity list becomes just client A
    contentChannel->releaseActivity(clientB->getInterfaceName());
    expectedUpdates.push_back({ActivityUpdateElem(clientB->getInterfaceName(), FocusState::NONE)});
    expectedUpdates.push_back({ActivityUpdateElem(clientA->getInterfaceName(), FocusState::FOREGROUND)});
    checkActivityUpdates(contentChannel, expectedUpdates);
    ASSERT_EQ(contentChannel->getActivity(Activity_B->getInterface()), nullptr);
    ASSERT_TRUE(contentChannel->getState().interfaceName == Activity_A->getInterface());

    // when client A activity is released , activity list becomes empty
    contentChannel->releaseActivity(clientA->getInterfaceName());
    expectedUpdates.push_back({ActivityUpdateElem(clientA->getInterfaceName(), FocusState::NONE)});
    checkActivityUpdates(contentChannel, expectedUpdates);
    ASSERT_EQ(contentChannel->getActivity(Activity_A->getInterface()), nullptr);
}

TEST_F(ChannelTest, test_MultiActivity_IncomingActivityWithPatience2) {
    std::vector<ActivityUpdateElem> expectedUpdates;

    // client A acquires Content Channel
    auto Activity_A = clientA->createActivity();
    contentChannel->setPrimaryActivity(Activity_A);
    ASSERT_TRUE(contentChannel->getState().interfaceName == Activity_A->getInterface());
    contentChannel->setFocus(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    expectedUpdates.push_back({ActivityUpdateElem(clientA->getInterfaceName(), FocusState::FOREGROUND)});
    checkActivityUpdates(contentChannel, expectedUpdates);

    // client B acquires Content Channel (with a patience parameter)
    // activityUpdate must show client A with FocusState::NONE
    // however client A is not kicked out
    auto Activity_B = clientB->createActivity(ContentType::MIXABLE, std::chrono::seconds(5));
    contentChannel->setPrimaryActivity(Activity_B);
    contentChannel->setFocus(FocusState::FOREGROUND, MixingBehavior::PRIMARY, true);
    expectedUpdates.push_back({ActivityUpdateElem(clientA->getInterfaceName(), FocusState::NONE)});
    expectedUpdates.push_back({ActivityUpdateElem(clientB->getInterfaceName(), FocusState::FOREGROUND)});
    checkActivityUpdates(contentChannel, expectedUpdates);
    ASSERT_TRUE(contentChannel->getState().interfaceName == Activity_B->getInterface());
    ASSERT_NE(contentChannel->getActivity(Activity_A->getInterface()), nullptr);

    // when client A activity is released , activity list becomes just client B
    contentChannel->releaseActivity(clientA->getInterfaceName());
    contentChannel->setFocus(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    expectedUpdates.push_back({ActivityUpdateElem(clientB->getInterfaceName(), FocusState::FOREGROUND)});
    checkActivityUpdates(contentChannel, expectedUpdates);
    ASSERT_EQ(contentChannel->getActivity(Activity_A->getInterface()), nullptr);
    ASSERT_TRUE(contentChannel->getState().interfaceName == Activity_B->getInterface());

    // when client B activity is released , activity list becomes empty
    contentChannel->releaseActivity(clientB->getInterfaceName());
    expectedUpdates.push_back({ActivityUpdateElem(clientB->getInterfaceName(), FocusState::NONE)});
    checkActivityUpdates(contentChannel, expectedUpdates);
    ASSERT_EQ(contentChannel->getActivity(Activity_B->getInterface()), nullptr);
}

TEST_F(ChannelTest, test_MultiActivity_IncomingActivityWithPatience3) {
    std::vector<ActivityUpdateElem> expectedUpdates;

    // client A acquires Content Channel
    auto Activity_A = clientA->createActivity();
    contentChannel->setPrimaryActivity(Activity_A);
    ASSERT_TRUE(contentChannel->getState().interfaceName == Activity_A->getInterface());
    contentChannel->setFocus(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    expectedUpdates.push_back({ActivityUpdateElem(clientA->getInterfaceName(), FocusState::FOREGROUND)});
    checkActivityUpdates(contentChannel, expectedUpdates);

    // client B acquires Content Channel (with a patience parameter)
    // activityUpdate must show client A with FocusState::NONE
    // however client A is not kicked out
    auto Activity_B = clientB->createActivity(ContentType::MIXABLE, std::chrono::seconds(1));
    contentChannel->setPrimaryActivity(Activity_B);
    contentChannel->setFocus(FocusState::FOREGROUND, MixingBehavior::PRIMARY, true);
    expectedUpdates.push_back({ActivityUpdateElem(clientA->getInterfaceName(), FocusState::NONE)});
    expectedUpdates.push_back({ActivityUpdateElem(clientB->getInterfaceName(), FocusState::FOREGROUND)});
    checkActivityUpdates(contentChannel, expectedUpdates);
    ASSERT_TRUE(contentChannel->getState().interfaceName == Activity_B->getInterface());
    ASSERT_NE(contentChannel->getActivity(Activity_A->getInterface()), nullptr);

    // When patience is elapsed, Activity A is released and B is the interface of the channel.
    std::this_thread::sleep_for(std::chrono::seconds(2));
    ASSERT_EQ(contentChannel->getActivity(Activity_A->getInterface()), nullptr);
    ASSERT_TRUE(contentChannel->getState().interfaceName == Activity_B->getInterface());

    // when client B activity is released , activity list becomes empty
    contentChannel->releaseActivity(clientB->getInterfaceName());
    expectedUpdates.push_back({ActivityUpdateElem(clientB->getInterfaceName(), FocusState::NONE)});
    checkActivityUpdates(contentChannel, expectedUpdates);
    ASSERT_EQ(contentChannel->getActivity(Activity_B->getInterface()), nullptr);
}

TEST_F(ChannelTest, test_MultiActivity_IncomingActivityWithPatience4) {
    std::vector<ActivityUpdateElem> expectedUpdates;

    // client A acquires Content Channel
    auto Activity_A = clientA->createActivity();
    contentChannel->setPrimaryActivity(Activity_A);
    ASSERT_TRUE(contentChannel->getState().interfaceName == Activity_A->getInterface());
    contentChannel->setFocus(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    expectedUpdates.push_back({ActivityUpdateElem(clientA->getInterfaceName(), FocusState::FOREGROUND)});
    checkActivityUpdates(contentChannel, expectedUpdates);

    // client B acquires Content Channel (with a patience parameter)
    // activityUpdate must show client A with FocusState::NONE
    // however client A is not kicked out
    auto Activity_B = clientB->createActivity(ContentType::MIXABLE, std::chrono::seconds(5));
    contentChannel->setPrimaryActivity(Activity_B);
    contentChannel->setFocus(FocusState::FOREGROUND, MixingBehavior::PRIMARY, true);
    expectedUpdates.push_back({ActivityUpdateElem(clientA->getInterfaceName(), FocusState::NONE)});
    expectedUpdates.push_back({ActivityUpdateElem(clientB->getInterfaceName(), FocusState::FOREGROUND)});
    checkActivityUpdates(contentChannel, expectedUpdates);
    ASSERT_TRUE(contentChannel->getState().interfaceName == Activity_B->getInterface());
    ASSERT_NE(contentChannel->getActivity(Activity_A->getInterface()), nullptr);

    // client C acquires Content Channel (with a patience parameter)
    // activityUpdate must show client B with FocusState::NONE
    // activity B is released already and already was assigned FocusState::NONE
    auto Activity_C = clientC->createActivity(ContentType::MIXABLE, std::chrono::seconds(5));
    contentChannel->setPrimaryActivity(Activity_C);
    contentChannel->setFocus(FocusState::FOREGROUND, MixingBehavior::PRIMARY, true);
    expectedUpdates.push_back({ActivityUpdateElem(clientB->getInterfaceName(), FocusState::NONE)});
    expectedUpdates.push_back({ActivityUpdateElem(clientC->getInterfaceName(), FocusState::FOREGROUND)});
    checkActivityUpdates(contentChannel, expectedUpdates);
    ASSERT_TRUE(contentChannel->getState().interfaceName == Activity_C->getInterface());
    // Activity A is released.
    ASSERT_EQ(contentChannel->getActivity(Activity_A->getInterface()), nullptr);
    // Activity B is not kicked out.
    ASSERT_NE(contentChannel->getActivity(Activity_B->getInterface()), nullptr);

    // when client C activity is released , activity list becomes empty
    contentChannel->releaseActivity(clientC->getInterfaceName());
    expectedUpdates.push_back({ActivityUpdateElem(clientC->getInterfaceName(), FocusState::NONE)});
    expectedUpdates.push_back({ActivityUpdateElem(clientB->getInterfaceName(), FocusState::FOREGROUND)});
    checkActivityUpdates(contentChannel, expectedUpdates);
    // Activity A/C are released.
    ASSERT_EQ(contentChannel->getActivity(Activity_A->getInterface()), nullptr);
    ASSERT_EQ(contentChannel->getActivity(Activity_C->getInterface()), nullptr);
    ASSERT_TRUE(contentChannel->getState().interfaceName == Activity_B->getInterface());
}

TEST_F(ChannelTest, test_MultiActivity_IncomingActivityWithPatience5) {
    std::vector<ActivityUpdateElem> expectedUpdates;

    // client A acquires Content Channel
    auto Activity_A = clientA->createActivity();
    contentChannel->setPrimaryActivity(Activity_A);
    ASSERT_TRUE(contentChannel->getState().interfaceName == Activity_A->getInterface());
    contentChannel->setFocus(FocusState::FOREGROUND, MixingBehavior::PRIMARY);
    expectedUpdates.push_back({ActivityUpdateElem(clientA->getInterfaceName(), FocusState::FOREGROUND)});
    checkActivityUpdates(contentChannel, expectedUpdates);

    // client B acquires Content Channel (with a patience parameter)
    // activityUpdate must show client A with FocusState::NONE
    // however client A is not kicked out
    auto Activity_B = clientB->createActivity(ContentType::MIXABLE, std::chrono::seconds(5));
    contentChannel->setPrimaryActivity(Activity_B);
    contentChannel->setFocus(FocusState::FOREGROUND, MixingBehavior::PRIMARY, true);
    expectedUpdates.push_back({ActivityUpdateElem(clientA->getInterfaceName(), FocusState::NONE)});
    expectedUpdates.push_back({ActivityUpdateElem(clientB->getInterfaceName(), FocusState::FOREGROUND)});
    checkActivityUpdates(contentChannel, expectedUpdates);
    ASSERT_TRUE(contentChannel->getState().interfaceName == Activity_B->getInterface());
    ASSERT_NE(contentChannel->getActivity(Activity_A->getInterface()), nullptr);

    // client C acquires Content Channel (with a no patience parameter)
    // With no patience, both activity must be released.
    auto Activity_C = clientC->createActivity(ContentType::MIXABLE, std::chrono::seconds(0));
    contentChannel->setPrimaryActivity(Activity_C);
    contentChannel->setFocus(FocusState::FOREGROUND, MixingBehavior::PRIMARY, true);
    expectedUpdates.push_back({ActivityUpdateElem(clientB->getInterfaceName(), FocusState::NONE)});
    expectedUpdates.push_back({ActivityUpdateElem(clientC->getInterfaceName(), FocusState::FOREGROUND)});
    checkActivityUpdates(contentChannel, expectedUpdates);
    ASSERT_TRUE(contentChannel->getState().interfaceName == Activity_C->getInterface());
    // Activity A/B is released.
    ASSERT_EQ(contentChannel->getActivity(Activity_A->getInterface()), nullptr);
    ASSERT_EQ(contentChannel->getActivity(Activity_B->getInterface()), nullptr);

    // when client C activity is released , activity list becomes empty
    contentChannel->releaseActivity(clientC->getInterfaceName());
    expectedUpdates.push_back({ActivityUpdateElem(clientC->getInterfaceName(), FocusState::NONE)});
    checkActivityUpdates(contentChannel, expectedUpdates);
    ASSERT_EQ(contentChannel->getActivity(Activity_C->getInterface()), nullptr);
}

}  // namespace test
}  // namespace afml
}  // namespace alexaClientSDK
