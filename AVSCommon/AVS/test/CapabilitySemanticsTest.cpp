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

#include <AVSCommon/AVS/CapabilitySemantics/CapabilitySemantics.h>

#include <gtest/gtest.h>
#include <rapidjson/document.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace test {

using namespace ::testing;
using namespace avsCommon::avs::capabilitySemantics;

/// Accepted action IDs.
/// @{
static std::string ACTION_OPEN = "Alexa.Actions.Open";
static std::string ACTION_CLOSE = "Alexa.Actions.Close";
static std::string ACTION_RAISE = "Alexa.Actions.Raise";
static std::string ACTION_LOWER = "Alexa.Actions.Lower";
/// @}

/// Accepted state IDs.
/// @{
static std::string STATE_OPEN = "Alexa.States.Open";
static std::string STATE_CLOSED = "Alexa.States.Closed";
/// @}

/// Sample directive names.
/// @{
static std::string DIRECTIVE_TURNOFF = "TurnOff";
static std::string DIRECTIVE_SETMODE = "SetMode";
static std::string DIRECTIVE_SETRANGE = "SetRangeValue";
static std::string DIRECTIVE_ADJUSTRANGE = "AdjustRangeValue";
/// @}

// clang-format off

/**
 * Sample 'semantics' object with multiple ActionsToDirectiveMappings with multiple actions.
 */
static std::string JSON_SEMANTICS_MULTIPLE_ACTIONS = R"({
"actionMappings": [
  {
    "@type": "ActionsToDirective",
    "actions": ["Alexa.Actions.Close", "Alexa.Actions.Lower"],
    "directive": {
        "name": "SetMode",
        "payload": {
            "mode": "Position.Down"
        }
    }
  },
  {
    "@type": "ActionsToDirective",
    "actions": ["Alexa.Actions.Open", "Alexa.Actions.Raise"],
    "directive": {
      "name": "SetMode",
      "payload": {
          "mode": "Position.Up"
      }
    }
  }
]
})";

/**
 * Sample 'semantics' object with 'actionMappings' and 'stateMappings'.
 */
static std::string JSON_SEMANTICS_COMPLETE = R"({
"actionMappings": [
  {
    "@type": "ActionsToDirective",
    "actions": [
      "Alexa.Actions.Close"
    ],
    "directive": {
      "name": "SetRangeValue",
      "payload": {
        "rangeValue": 0
      }
    }
  },
  {
    "@type": "ActionsToDirective",
    "actions": [
      "Alexa.Actions.Open"
    ],
    "directive": {
      "name": "SetRangeValue",
      "payload": {
        "rangeValue": 100
      }
    }
  },
  {
    "@type": "ActionsToDirective",
    "actions": [
      "Alexa.Actions.Lower"
    ],
    "directive": {
      "name": "AdjustRangeValue",
      "payload": {
        "rangeValueDelta": -10
      }
    }
  },
  {
    "@type": "ActionsToDirective",
    "actions": [
      "Alexa.Actions.Raise"
    ],
    "directive": {
      "name": "AdjustRangeValue",
      "payload": {
        "rangeValueDelta": 10
      }
    }
  }
],
"stateMappings": [
  {
    "@type": "StatesToValue",
    "states": [
      "Alexa.States.Closed"
    ],
    "value": 0
  },
  {
    "@type": "StatesToRange",
    "states": [
      "Alexa.States.Open"
    ],
    "range": {
      "minimumValue": 1,
      "maximumValue": 100
    }
  }
]
})";

/// Empty JSON object.
static std::string JSON_EMPTY_OBJECT = "{}";

// clang-format on

/**
 * Expects the provided JSON strings to be equal.
 */
void validateJson(const std::string& providedJson, const std::string& expectedJson) {
    rapidjson::Document providedStateParsed;
    providedStateParsed.Parse(providedJson);

    rapidjson::Document expectedStateParsed;
    expectedStateParsed.Parse(expectedJson);

    EXPECT_EQ(providedStateParsed, expectedStateParsed);
}

/**
 * The test harness for @c CapabilitySemantics.
 */
class CapabilitySemanticsTest : public Test {};

/**
 * Test that ActionsToDirectiveMapping::addAction() checks for an empty action.
 */
TEST_F(CapabilitySemanticsTest, test_actions_emptyAction) {
    ActionsToDirectiveMapping invalidMapping;
    ASSERT_FALSE(invalidMapping.addAction(""));
    ASSERT_FALSE(invalidMapping.isValid());
    ASSERT_EQ(invalidMapping.toJson(), JSON_EMPTY_OBJECT);
}

/**
 * Test that ActionsToDirectiveMapping::setDirective() checks for an empty name.
 */
TEST_F(CapabilitySemanticsTest, test_actions_emptyDirectiveName) {
    ActionsToDirectiveMapping invalidMapping;
    ASSERT_FALSE(invalidMapping.setDirective("", JSON_EMPTY_OBJECT));
    ASSERT_FALSE(invalidMapping.isValid());
    ASSERT_EQ(invalidMapping.toJson(), JSON_EMPTY_OBJECT);
}

/**
 * Test that ActionsToDirectiveMapping::addAction() skips duplicate actions.
 */
TEST_F(CapabilitySemanticsTest, test_actions_duplicateAction) {
    ActionsToDirectiveMapping invalidMapping;
    ASSERT_TRUE(invalidMapping.setDirective(DIRECTIVE_TURNOFF, JSON_EMPTY_OBJECT));
    ASSERT_TRUE(invalidMapping.addAction(ACTION_CLOSE));
    ASSERT_TRUE(invalidMapping.isValid());
    ASSERT_FALSE(invalidMapping.addAction(ACTION_CLOSE));
    ASSERT_FALSE(invalidMapping.isValid());
    ASSERT_EQ(invalidMapping.toJson(), JSON_EMPTY_OBJECT);
}

/**
 * Test that ActionsToDirectiveMapping without actions is invalid.
 */
TEST_F(CapabilitySemanticsTest, test_actions_noActions) {
    ActionsToDirectiveMapping invalidMapping;
    ASSERT_TRUE(invalidMapping.setDirective(DIRECTIVE_TURNOFF, JSON_EMPTY_OBJECT));
    ASSERT_FALSE(invalidMapping.isValid());
    ASSERT_EQ(invalidMapping.toJson(), JSON_EMPTY_OBJECT);
}

/**
 * Test that ActionsToDirectiveMapping without a directive is invalid.
 */
TEST_F(CapabilitySemanticsTest, test_actions_noDirective) {
    ActionsToDirectiveMapping invalidMapping;
    ASSERT_TRUE(invalidMapping.addAction(ACTION_CLOSE));
    ASSERT_FALSE(invalidMapping.isValid());
    ASSERT_EQ(invalidMapping.toJson(), JSON_EMPTY_OBJECT);
}

/**
 * Test that StatesToValueMapping::addState() checks for an empty state.
 */
TEST_F(CapabilitySemanticsTest, test_statesValue_emptyState) {
    StatesToValueMapping invalidMapping;
    ASSERT_FALSE(invalidMapping.addState(""));
    ASSERT_FALSE(invalidMapping.isValid());
    ASSERT_EQ(invalidMapping.toJson(), JSON_EMPTY_OBJECT);
}

/**
 * Test that StatesToValueMapping::addState() skips duplicate states.
 */
TEST_F(CapabilitySemanticsTest, test_statesValue_duplicateState) {
    StatesToValueMapping invalidMapping;
    ASSERT_TRUE(invalidMapping.setValue("Position.Down"));
    ASSERT_TRUE(invalidMapping.addState(STATE_CLOSED));
    ASSERT_TRUE(invalidMapping.isValid());
    ASSERT_FALSE(invalidMapping.addState(STATE_CLOSED));
    ASSERT_FALSE(invalidMapping.isValid());
    ASSERT_EQ(invalidMapping.toJson(), JSON_EMPTY_OBJECT);
}

/**
 * Test that StatesToValueMapping without states is invalid.
 */
TEST_F(CapabilitySemanticsTest, test_statesValue_noStates) {
    StatesToValueMapping invalidMapping;
    ASSERT_TRUE(invalidMapping.setValue(0));
    ASSERT_FALSE(invalidMapping.isValid());
    ASSERT_EQ(invalidMapping.toJson(), JSON_EMPTY_OBJECT);
}

/**
 * Test that StatesToValueMapping without a value is invalid.
 */
TEST_F(CapabilitySemanticsTest, test_statesValue_noValue) {
    StatesToValueMapping invalidMapping;
    ASSERT_TRUE(invalidMapping.addState(STATE_CLOSED));
    ASSERT_FALSE(invalidMapping.isValid());
    ASSERT_EQ(invalidMapping.toJson(), JSON_EMPTY_OBJECT);
}

/**
 * Test that StatesToRangeMapping::addState() checks for an empty state.
 */
TEST_F(CapabilitySemanticsTest, test_statesRange_emptyState) {
    StatesToRangeMapping invalidMapping;
    ASSERT_FALSE(invalidMapping.addState(""));
    ASSERT_FALSE(invalidMapping.isValid());
    ASSERT_EQ(invalidMapping.toJson(), JSON_EMPTY_OBJECT);
}

/**
 * Test that StatesToRangeMapping::addState() skips duplicate states.
 */
TEST_F(CapabilitySemanticsTest, test_statesRange_duplicateState) {
    StatesToRangeMapping invalidMapping;
    ASSERT_TRUE(invalidMapping.setRange(0, 50));
    ASSERT_TRUE(invalidMapping.addState(STATE_OPEN));
    ASSERT_TRUE(invalidMapping.isValid());
    ASSERT_FALSE(invalidMapping.addState(STATE_OPEN));
    ASSERT_FALSE(invalidMapping.isValid());
    ASSERT_EQ(invalidMapping.toJson(), JSON_EMPTY_OBJECT);
}

/**
 * Test that StatesToRangeMapping without states is invalid.
 */
TEST_F(CapabilitySemanticsTest, test_statesRange_noStates) {
    StatesToRangeMapping invalidMapping;
    ASSERT_TRUE(invalidMapping.setRange(0, 50));
    ASSERT_FALSE(invalidMapping.isValid());
    ASSERT_EQ(invalidMapping.toJson(), JSON_EMPTY_OBJECT);
}

/**
 * Test that StatesToRangeMapping without a range is invalid.
 */
TEST_F(CapabilitySemanticsTest, test_statesRange_noRange) {
    StatesToRangeMapping invalidMapping;
    ASSERT_TRUE(invalidMapping.addState(STATE_CLOSED));
    ASSERT_FALSE(invalidMapping.isValid());
    ASSERT_EQ(invalidMapping.toJson(), JSON_EMPTY_OBJECT);
}

/**
 * Test that StatesToRangeMapping with min > max is invalid.
 */
TEST_F(CapabilitySemanticsTest, test_statesRange_invertedRange) {
    StatesToRangeMapping invalidMapping;
    ASSERT_TRUE(invalidMapping.addState(STATE_OPEN));
    ASSERT_FALSE(invalidMapping.setRange(100, 1));
    ASSERT_FALSE(invalidMapping.isValid());
    ASSERT_EQ(invalidMapping.toJson(), JSON_EMPTY_OBJECT);
}

/**
 * Test that CapabilitySemantics without mappings is invalid.
 */
TEST_F(CapabilitySemanticsTest, test_semantics_noMappings) {
    CapabilitySemantics invalidSemantics;
    ASSERT_FALSE(invalidSemantics.isValid());
    validateJson(invalidSemantics.toJson(), JSON_EMPTY_OBJECT);
}

/**
 * Test that CapabilitySemantics with an invalid ActionsToDirectiveMapping is invalid.
 */
TEST_F(CapabilitySemanticsTest, test_semantics_invalidActionsDirectiveMapping) {
    ActionsToDirectiveMapping invalidMapping;
    CapabilitySemantics invalidSemantics;
    ASSERT_FALSE(invalidSemantics.addActionsToDirectiveMapping(invalidMapping));
    ASSERT_FALSE(invalidSemantics.isValid());
    validateJson(invalidSemantics.toJson(), JSON_EMPTY_OBJECT);
}

/**
 * Test that CapabilitySemantics with an invalid StatesToValueMapping is invalid.
 */
TEST_F(CapabilitySemanticsTest, test_semantics_invalidStatesValueMapping) {
    StatesToValueMapping invalidMapping;
    CapabilitySemantics invalidSemantics;
    ASSERT_FALSE(invalidSemantics.addStatesToValueMapping(invalidMapping));
    ASSERT_FALSE(invalidSemantics.isValid());
    validateJson(invalidSemantics.toJson(), JSON_EMPTY_OBJECT);
}

/**
 * Test that CapabilitySemantics with an invalid StatesToRangeMapping is invalid.
 */
TEST_F(CapabilitySemanticsTest, test_semantics_invalidStatesRangeMapping) {
    StatesToRangeMapping invalidMapping;
    CapabilitySemantics invalidSemantics;
    ASSERT_FALSE(invalidSemantics.addStatesToRangeMapping(invalidMapping));
    ASSERT_FALSE(invalidSemantics.isValid());
    validateJson(invalidSemantics.toJson(), JSON_EMPTY_OBJECT);
}

/**
 * Test the JSON result of an ActionsToDirectiveMapping with multiple actions.
 */
TEST_F(CapabilitySemanticsTest, test_validateJson_semanticsMultipleActionMappings) {
    ActionsToDirectiveMapping setModeDownMapping;
    ASSERT_TRUE(setModeDownMapping.addAction(ACTION_CLOSE));
    ASSERT_TRUE(setModeDownMapping.addAction(ACTION_LOWER));
    ASSERT_TRUE(setModeDownMapping.setDirective(DIRECTIVE_SETMODE, "{\"mode\": \"Position.Down\"}"));
    ASSERT_TRUE(setModeDownMapping.isValid());

    ActionsToDirectiveMapping setModeUpMapping;
    ASSERT_TRUE(setModeUpMapping.addAction(ACTION_OPEN));
    ASSERT_TRUE(setModeUpMapping.addAction(ACTION_RAISE));
    ASSERT_TRUE(setModeUpMapping.setDirective(DIRECTIVE_SETMODE, "{\"mode\": \"Position.Up\"}"));
    ASSERT_TRUE(setModeUpMapping.isValid());

    CapabilitySemantics semantics;
    ASSERT_TRUE(semantics.addActionsToDirectiveMapping(setModeDownMapping));
    ASSERT_TRUE(semantics.addActionsToDirectiveMapping(setModeUpMapping));
    ASSERT_TRUE(semantics.isValid());
    validateJson(semantics.toJson(), JSON_SEMANTICS_MULTIPLE_ACTIONS);
}

/**
 * Test the JSON result of a CapabilitySemantics with all mapping types.
 */
TEST_F(CapabilitySemanticsTest, test_validateJson_semanticsComplete) {
    ActionsToDirectiveMapping closeMapping;
    ASSERT_TRUE(closeMapping.addAction(ACTION_CLOSE));
    ASSERT_TRUE(closeMapping.setDirective(DIRECTIVE_SETRANGE, "{\"rangeValue\" : 0}"));
    ASSERT_TRUE(closeMapping.isValid());

    ActionsToDirectiveMapping openMapping;
    ASSERT_TRUE(openMapping.addAction(ACTION_OPEN));
    ASSERT_TRUE(openMapping.setDirective(DIRECTIVE_SETRANGE, "{\"rangeValue\" : 100}"));
    ASSERT_TRUE(openMapping.isValid());

    ActionsToDirectiveMapping lowerMapping;
    ASSERT_TRUE(lowerMapping.addAction(ACTION_LOWER));
    ASSERT_TRUE(lowerMapping.setDirective(DIRECTIVE_ADJUSTRANGE, "{\"rangeValueDelta\" : -10}"));
    ASSERT_TRUE(lowerMapping.isValid());

    ActionsToDirectiveMapping raiseMapping;
    ASSERT_TRUE(raiseMapping.addAction(ACTION_RAISE));
    ASSERT_TRUE(raiseMapping.setDirective(DIRECTIVE_ADJUSTRANGE, "{\"rangeValueDelta\" : 10}"));
    ASSERT_TRUE(raiseMapping.isValid());

    StatesToValueMapping closedMapping;
    ASSERT_TRUE(closedMapping.addState(STATE_CLOSED));
    ASSERT_TRUE(closedMapping.setValue(0));
    ASSERT_TRUE(closedMapping.isValid());

    StatesToRangeMapping openedMapping;
    ASSERT_TRUE(openedMapping.addState(STATE_OPEN));
    ASSERT_TRUE(openedMapping.setRange(1, 100));
    ASSERT_TRUE(openedMapping.isValid());

    CapabilitySemantics semantics;
    ASSERT_TRUE(semantics.addActionsToDirectiveMapping(closeMapping));
    ASSERT_TRUE(semantics.addActionsToDirectiveMapping(openMapping));
    ASSERT_TRUE(semantics.addActionsToDirectiveMapping(lowerMapping));
    ASSERT_TRUE(semantics.addActionsToDirectiveMapping(raiseMapping));
    ASSERT_TRUE(semantics.addStatesToValueMapping(closedMapping));
    ASSERT_TRUE(semantics.addStatesToRangeMapping(openedMapping));

    ASSERT_TRUE(semantics.isValid());
    validateJson(semantics.toJson(), JSON_SEMANTICS_COMPLETE);
}

}  // namespace test
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
