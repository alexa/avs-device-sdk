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

#include "acsdk/VisualCharacteristics/private/VisualCharacteristics.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/SDKInterfaces/Endpoints/MockEndpointCapabilitiesRegistrar.h>
#include <AVSCommon/SDKInterfaces/MockContextManager.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/Threading/Executor.h>

#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorStateObserverInterface.h>
#include <acsdk/VisualCharacteristicsInterfaces/VisualCharacteristicsInterface.h>

namespace alexaClientSDK {
namespace visualCharacteristics {

using namespace presentationOrchestratorInterfaces;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::threading;
using namespace visualCharacteristicsInterfaces;
using namespace rapidjson;
using namespace ::testing;

using avsCommon::sdkInterfaces::endpoints::test::MockEndpointCapabilitiesRegistrar;
using avsCommon::sdkInterfaces::test::MockContextManager;
using avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender;

/// Interaction mode configuration json
static const std::string INTERACTION_MODE_CONFIG = R"({
   "type": "AlexaInterface",
   "interface": "Alexa.InteractionMode",
   "version": "1.1",
   "configurations": {
     "interactionModes": [
       {
         "id": "tv",
         "uiMode": "TV",
         "interactionDistance": { "unit": "INCHES", "value": 130 },
         "touch": "UNSUPPORTED",
         "keyboard": "SUPPORTED",
         "video": "SUPPORTED",
         "dialog": "SUPPORTED"
       },
       {
         "id": "tv_overlay",
         "uiMode": "TV",
         "interactionDistance": { "unit": "INCHES", "value": 130 },
         "touch": "UNSUPPORTED",
         "keyboard": "SUPPORTED",
         "video": "UNSUPPORTED",
         "dialog": "SUPPORTED"
       }
     ]
   }
})";

/// Alexa.Display.Window configuration json
static const std::string WINDOW_CONFIG = R"({
   "type": "AlexaInterface",
   "interface": "Alexa.Display.Window",
   "version": "1.0",
   "configurations": {
     "templates": [
       {
         "id": "tvFullscreen",
         "type": "STANDARD",
         "configuration": {
           "sizes": [
             {
               "type": "DISCRETE",
               "id": "fullscreen",
               "value": {
                 "unit": "PIXEL",
                 "value": { "width": 1920, "height": 1080 }
               }
             }
           ],
           "interactionModes": ["tv"]
         }
       },
       {
         "id": "tvOverlayLandscape",
         "type": "OVERLAY",
         "configuration": {
           "sizes": [
             {
               "type": "DISCRETE",
               "id": "landscapePanel",
               "value": {
                 "unit": "PIXEL",
                 "value": { "width": 1920, "height": 400 }
               }
             }
           ],
           "interactionModes": ["tv_overlay"]
         }
       }
     ]
   }
})";

/// Alexa.Display.Window configuration json which specifies an invalid interaction mode
static const std::string INVALID_INTERACTION_MODE_WINDOW_CONFIG = R"({
   "type": "AlexaInterface",
   "interface": "Alexa.Display.Window",
   "version": "1.0",
   "configurations": {
     "templates": [
       {
         "id": "tvFullscreen",
         "type": "STANDARD",
         "configuration": {
           "sizes": [
             {
               "type": "DISCRETE",
               "id": "fullscreen",
               "value": {
                 "unit": "PIXEL",
                 "value": { "width": 1920, "height": 1080 }
               }
             }
           ],
           "interactionModes": ["potato", "tv"]
         }
       }
     ]
   }
})";

/// Alexa.Display configuration json
static const std::string DISPLAY_CONFIG = R"({
   "type": "AlexaInterface",
   "interface": "Alexa.Display",
   "version": "1.0",
   "configurations": {
     "display": {
       "type": "PIXEL",
       "touch": ["UNSUPPORTED"],
       "shape": "RECTANGLE",
       "dimensions": {
         "resolution": {
           "unit": "PIXEL",
           "value": { "width": 1920, "height": 1080 }
         },
         "physicalSize": {
           "unit": "INCHES",
           "value": { "width": 56.7, "height": 31.9 }
         },
         "pixelDensity": { "unit": "DPI", "value": 320 },
         "densityIndependentResolution": {
           "unit": "DP",
           "value": { "width": 960, "height": 540 }
         }
       }
     }
   }
})";

/// Dummy configuration for testing VC
static const std::string SETTINGS_CONFIG = R"({
    "visualCharacteristics": [)" + INTERACTION_MODE_CONFIG +
                                           "," + WINDOW_CONFIG + "," + DISPLAY_CONFIG + R"(]
})";

/// Invalid interactionmode configuration for testing VC
static const std::string INVALID_WINDOW_INTERACTIONMODE_SETTINGS_CONFIG = R"({
    "visualCharacteristics": [)" + INTERACTION_MODE_CONFIG + "," + INVALID_INTERACTION_MODE_WINDOW_CONFIG +
                                                                          "," + DISPLAY_CONFIG + R"(]
})";

/// Visual Context for the window states.
static const std::string WINDOW_STATE =
    R"({"defaultWindowId":"tvFullscreen","instances":[{"id":"tvFullscreen","templateId":"tvFullscreen","token":"token1","configuration":{"interactionMode":"tv","sizeConfigurationId":"fullscreen"}}]})";

/// Visual Context for the window states without a token.
static const std::string WINDOW_STATE_NO_TOKEN =
    R"({"defaultWindowId":"tvFullscreen","instances":[{"id":"tvFullscreen","templateId":"tvFullscreen","token":"","configuration":{"interactionMode":"tv","sizeConfigurationId":"fullscreen"}}]})";

/// Empty Visual Context for the window states.
static const std::string EMPTY_WINDOW_STATE = R"({"defaultWindowId":"tvFullscreen","instances":[]})";

/// Namespace for sending WindowState context from a device.
static const std::string ALEXA_DISPLAY_WINDOW_NAMESPACE{"Alexa.Display.Window"};

/// Tag for finding the device window state context information sent from the runtime as part of event context.
static const std::string WINDOW_STATE_NAME{"WindowState"};

/// Id of a window instance.
static const std::string WINDOW_ID{"tvFullscreen"};

/// Id of a window template used by the window instance.
static const std::string WINDOW_TEMPLATE_ID{"tvFullscreen"};

/// Id of a window template used by the window instance.
static const std::string WINDOW_TEMPLATE_ID_OVERLAY{"tvOverlayLandscape"};

/// Id of an interaction mode used by a window instance instance.
static const std::string INTERACTION_MODE{"tv"};

/// Id of a size configuration used by a window instance instance.
static const std::string SIZE_CONFIG_ID{"fullscreen"};

/// Presentation token of currently served by a presentation.
static const std::string TOKEN{"token1"};

/// Empty Presentation token.
static const std::string EMPTY_TOKEN{""};

/// Display interface holding a presentation.
static const std::string DISPLAY_INTERFACE{"Alexa.Presentation.APL"};

/// The endpoint holding this window.
static const std::string DEFAULT_ENDPOINT{""};

/// Count of the capability configuration
static const int CAPABILITY_CFG_COUNT = 3;

/// Count of the number of interaction modes
static const int INTERACTION_MODE_CFG_COUNT = 2;

/// Count of the number of window template
static const int WINDOW_TEMPLATE_CFG_COUNT = 2;

/// State request #0
static const int STATE_REQUEST_ZERO = 0;

/// State request #1
static const int STATE_REQUEST_ONE = 1;

/// State request #2
static const int STATE_REQUEST_TWO = 2;

/// The VisualCharacteristics context state signature.
static const avsCommon::avs::NamespaceAndName DEVICE_WINDOW_STATE{ALEXA_DISPLAY_WINDOW_NAMESPACE, WINDOW_STATE_NAME};

/// Mocked @c PresentationOrchestratorStateObserverInterface
class MockPresentationOrchestratorStateObserver : public PresentationOrchestratorStateObserverInterface {
public:
    MOCK_METHOD2(onStateChanged, void(const std::string& windowId, const PresentationMetadata& metadata));
};

/// Test harness for @c VisualCharacteristics class.
class VisualCharacteristicsTest : public ::testing::Test {
public:
    /// Set up the test harness for running a test.
    void SetUp() override;

    /// Clean up the test harness after running a test.
    void TearDown() override;

    /// Default Constructor
    VisualCharacteristicsTest() = default;

protected:
    /// A pointer to an instance of the VisualCharacteristics that will be instantiated per test.
    std::shared_ptr<VisualCharacteristics> m_visualCharacteristics;

    /// A strict mock that allows the test to fetch context.
    std::shared_ptr<StrictMock<MockContextManager>> m_mockContextManager;

    /// A strict mock that allows reporting of exceptions during directive handling.
    std::shared_ptr<StrictMock<MockExceptionEncounteredSender>> m_mockExceptionEncounteredSender;

    /// Mock Executor.
    std::shared_ptr<avsCommon::utils::threading::Executor> m_executor;

    /// An endpoint capabilities registrar with which to register capabilities.
    std::shared_ptr<MockEndpointCapabilitiesRegistrar> m_mockEndpointCapabilitiesRegistrar;
};

static void setConfig() {
    ConfigurationNode::uninitialize();
    auto stream = std::shared_ptr<std::stringstream>(new std::stringstream(SETTINGS_CONFIG));
    std::vector<std::shared_ptr<std::istream>> jsonStream({stream});
    ConfigurationNode::initialize(jsonStream);
}

void VisualCharacteristicsTest::SetUp() {
    setConfig();

    m_mockContextManager = std::make_shared<StrictMock<MockContextManager>>();
    m_mockExceptionEncounteredSender = std::make_shared<StrictMock<MockExceptionEncounteredSender>>();
    m_mockEndpointCapabilitiesRegistrar = std::make_shared<NiceMock<MockEndpointCapabilitiesRegistrar>>();

    EXPECT_CALL(*m_mockContextManager, setStateProvider(_, _)).Times(Exactly(1));

    m_visualCharacteristics = std::dynamic_pointer_cast<VisualCharacteristics>(
        VisualCharacteristics::create(m_mockContextManager, m_mockExceptionEncounteredSender));

    EXPECT_TRUE(m_visualCharacteristics != nullptr);

    m_executor = std::make_shared<avsCommon::utils::threading::Executor>();

    m_visualCharacteristics->setExecutor(m_executor);
}

void VisualCharacteristicsTest::TearDown() {
    if (m_visualCharacteristics) {
        m_visualCharacteristics->shutdown();
        m_visualCharacteristics.reset();
    }

    ConfigurationNode::uninitialize();
}

/**
 * Tests that the VisualCharacteristics capability agent can successfully publish the four APIs in the config
 * file. Verify that other components are able to fetch the configurations.
 */
TEST_F(VisualCharacteristicsTest, testGetCapabilityConfigurations) {
    auto capabilityConfigurations = m_visualCharacteristics->getCapabilityConfigurations();

    EXPECT_TRUE(capabilityConfigurations.size() == CAPABILITY_CFG_COUNT);
    for (auto& cfg : capabilityConfigurations) {
        EXPECT_TRUE(cfg != nullptr);
    }
}

/**
 * Verify that other components will be able to fetch the VC configurations.
 */
TEST_F(VisualCharacteristicsTest, testVerifyGetConfigurations) {
    auto capabilityConfigurations = m_visualCharacteristics->getCapabilityConfigurations();

    EXPECT_TRUE(capabilityConfigurations.size() == CAPABILITY_CFG_COUNT);
    for (auto& cfg : capabilityConfigurations) {
        EXPECT_TRUE(cfg != nullptr);
    }

    EXPECT_TRUE(m_visualCharacteristics->getWindowTemplates().size() == WINDOW_TEMPLATE_CFG_COUNT);
    EXPECT_TRUE(m_visualCharacteristics->getInteractionModes().size() == INTERACTION_MODE_CFG_COUNT);
    EXPECT_TRUE(m_visualCharacteristics->getDisplayCharacteristics().type == DisplayCharacteristics::Type::PIXEL);
}
/**
 * Test if the VisualCharacteristics component is able to add window instance
 * and verify the state context.
 */
TEST_F(VisualCharacteristicsTest, testAddWindowInstance) {
    std::string context;
    EXPECT_CALL(*(m_mockContextManager.get()), setState(DEVICE_WINDOW_STATE, _, _, STATE_REQUEST_ZERO))
        .Times(1)
        .WillOnce(DoAll(SaveArg<1>(&context), Return(SetStateResult::SUCCESS)));
    m_visualCharacteristics->addWindowInstance({WINDOW_ID, WINDOW_TEMPLATE_ID, INTERACTION_MODE, SIZE_CONFIG_ID});
    m_visualCharacteristics->setDefaultWindowInstance(WINDOW_ID);
    m_visualCharacteristics->onStateChanged(WINDOW_ID, {DEFAULT_ENDPOINT, DISPLAY_INTERFACE, TOKEN});
    m_visualCharacteristics->provideState(DEVICE_WINDOW_STATE, STATE_REQUEST_ZERO);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(context == WINDOW_STATE);

    EXPECT_CALL(*(m_mockContextManager.get()), setState(DEVICE_WINDOW_STATE, _, _, STATE_REQUEST_ONE)).Times(1);
    m_visualCharacteristics->onStateChanged(WINDOW_ID, {DEFAULT_ENDPOINT, DISPLAY_INTERFACE, EMPTY_TOKEN});
    m_visualCharacteristics->removeWindowInstance(WINDOW_ID);
    m_executor->waitForSubmittedTasks();
    m_visualCharacteristics->provideState(DEVICE_WINDOW_STATE, STATE_REQUEST_ONE);
    m_executor->waitForSubmittedTasks();
}

/**
 * Test if the VisualCharacteristics component is able to update window instance
 * and verify the state context.
 */
TEST_F(VisualCharacteristicsTest, testUpdateWindowInstance) {
    std::string context;
    EXPECT_CALL(*(m_mockContextManager.get()), setState(DEVICE_WINDOW_STATE, _, _, STATE_REQUEST_ZERO))
        .Times(1)
        .WillOnce(DoAll(SaveArg<1>(&context), Return(SetStateResult::SUCCESS)));
    m_visualCharacteristics->addWindowInstance({WINDOW_ID, WINDOW_TEMPLATE_ID, INTERACTION_MODE, SIZE_CONFIG_ID});
    m_visualCharacteristics->setDefaultWindowInstance(WINDOW_ID);
    m_visualCharacteristics->onStateChanged(WINDOW_ID, {DEFAULT_ENDPOINT, DISPLAY_INTERFACE, TOKEN});
    m_visualCharacteristics->provideState(DEVICE_WINDOW_STATE, STATE_REQUEST_ZERO);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(context == WINDOW_STATE);

    context.clear();
    EXPECT_CALL(*(m_mockContextManager.get()), setState(DEVICE_WINDOW_STATE, _, _, STATE_REQUEST_ONE))
        .Times(1)
        .WillOnce(DoAll(SaveArg<1>(&context), Return(SetStateResult::SUCCESS)));
    m_visualCharacteristics->updateWindowInstance(
        {WINDOW_ID, WINDOW_TEMPLATE_ID_OVERLAY, INTERACTION_MODE, SIZE_CONFIG_ID});

    m_visualCharacteristics->onStateChanged(WINDOW_ID, {DEFAULT_ENDPOINT, DISPLAY_INTERFACE, TOKEN});
    m_visualCharacteristics->provideState(DEVICE_WINDOW_STATE, STATE_REQUEST_ONE);
    m_executor->waitForSubmittedTasks();
    ASSERT_FALSE(context.find(WINDOW_TEMPLATE_ID_OVERLAY) == std::string::npos);

    context.clear();
    EXPECT_CALL(*(m_mockContextManager.get()), setState(DEVICE_WINDOW_STATE, _, _, STATE_REQUEST_TWO))
        .Times(1)
        .WillOnce(DoAll(SaveArg<1>(&context), Return(SetStateResult::SUCCESS)));
    m_visualCharacteristics->onStateChanged(WINDOW_ID, {DEFAULT_ENDPOINT, DISPLAY_INTERFACE, EMPTY_TOKEN});
    m_visualCharacteristics->removeWindowInstance(WINDOW_ID);
    m_executor->waitForSubmittedTasks();

    m_visualCharacteristics->provideState(DEVICE_WINDOW_STATE, STATE_REQUEST_TWO);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(context == EMPTY_WINDOW_STATE);
}

/**
 * Test if the VisualCharacteristics component is able to remove window instance
 * and verify the state context.
 */
TEST_F(VisualCharacteristicsTest, testRemoveWindowInstance) {
    std::string context;
    EXPECT_CALL(*(m_mockContextManager.get()), setState(DEVICE_WINDOW_STATE, _, _, STATE_REQUEST_ZERO))
        .WillOnce(DoAll(SaveArg<1>(&context), Return(SetStateResult::SUCCESS)));
    m_visualCharacteristics->addWindowInstance({WINDOW_ID, WINDOW_TEMPLATE_ID, INTERACTION_MODE, SIZE_CONFIG_ID});
    m_visualCharacteristics->setDefaultWindowInstance(WINDOW_ID);
    m_visualCharacteristics->onStateChanged(WINDOW_ID, {DEFAULT_ENDPOINT, DISPLAY_INTERFACE, TOKEN});
    m_visualCharacteristics->provideState(DEVICE_WINDOW_STATE, STATE_REQUEST_ZERO);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(context == WINDOW_STATE);

    context.clear();
    EXPECT_CALL(*(m_mockContextManager.get()), setState(DEVICE_WINDOW_STATE, _, _, STATE_REQUEST_ONE))
        .Times(1)
        .WillOnce(DoAll(SaveArg<1>(&context), Return(SetStateResult::SUCCESS)));
    m_visualCharacteristics->onStateChanged(WINDOW_ID, {DEFAULT_ENDPOINT, DISPLAY_INTERFACE, EMPTY_TOKEN});
    m_visualCharacteristics->removeWindowInstance(WINDOW_ID);
    m_executor->waitForSubmittedTasks();

    m_visualCharacteristics->provideState(DEVICE_WINDOW_STATE, STATE_REQUEST_ONE);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(context == EMPTY_WINDOW_STATE);
}

TEST_F(VisualCharacteristicsTest, testValidateConfigurationUnknownInteractionMode) {
    // Reset the config with an invalid one
    ConfigurationNode::uninitialize();
    auto stream =
        std::shared_ptr<std::stringstream>(new std::stringstream(INVALID_WINDOW_INTERACTIONMODE_SETTINGS_CONFIG));
    std::vector<std::shared_ptr<std::istream>> jsonStream({stream});
    ConfigurationNode::initialize(jsonStream);

    auto visualCharacteristics = VisualCharacteristics::create(m_mockContextManager, m_mockExceptionEncounteredSender);

    ASSERT_TRUE(visualCharacteristics == nullptr);
}

/**
 * Test if the VisualCharacteristics component clears the token when an empty state is received
 */
TEST_F(VisualCharacteristicsTest, testClearToken) {
    std::string context;
    std::string context2;
    EXPECT_CALL(*(m_mockContextManager.get()), setState(DEVICE_WINDOW_STATE, _, _, STATE_REQUEST_ZERO))
        .WillRepeatedly(DoAll(SaveArg<1>(&context), Return(SetStateResult::SUCCESS)));
    EXPECT_CALL(*(m_mockContextManager.get()), setState(DEVICE_WINDOW_STATE, _, _, STATE_REQUEST_ONE))
        .WillOnce(DoAll(SaveArg<1>(&context2), Return(SetStateResult::SUCCESS)));
    m_visualCharacteristics->addWindowInstance({WINDOW_ID, WINDOW_TEMPLATE_ID, INTERACTION_MODE, SIZE_CONFIG_ID});
    m_visualCharacteristics->setDefaultWindowInstance(WINDOW_ID);
    m_visualCharacteristics->onStateChanged(WINDOW_ID, {DEFAULT_ENDPOINT, DISPLAY_INTERFACE, TOKEN});
    m_visualCharacteristics->provideState(DEVICE_WINDOW_STATE, STATE_REQUEST_ZERO);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(context == WINDOW_STATE);

    m_visualCharacteristics->onStateChanged(WINDOW_ID, {});
    m_visualCharacteristics->provideState(DEVICE_WINDOW_STATE, STATE_REQUEST_ONE);
    m_executor->waitForSubmittedTasks();
    ASSERT_TRUE(context2 == WINDOW_STATE_NO_TOKEN);
}
}  // namespace visualCharacteristics
}  // namespace alexaClientSDK
