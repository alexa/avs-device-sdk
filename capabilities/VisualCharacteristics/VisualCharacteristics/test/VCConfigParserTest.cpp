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

#include <vector>

#include <gtest/gtest.h>

#include <acsdk/VisualCharacteristicsInterfaces/VisualCharacteristicsInterface.h>

#include "acsdk/VisualCharacteristics/private/VCConfigParser.h"

namespace alexaClientSDK {
namespace visualCharacteristics {

using namespace ::testing;
using namespace visualCharacteristicsInterfaces;
using namespace avsCommon::utils::configuration;

/// Alias for JSON stream type used in @c ConfigurationNode initialization
using JSONStream = std::vector<std::shared_ptr<std::istream>>;

static const std::string INTERACTION_MODES = "interactionModes";
static const std::string TEMPLATES = "templates";

static const std::string CONFIG_INTERACTIONMODE = R"({
          "interactionModes": [
            {
              "id": "tv",
              "uiMode": "TV",
              "interactionDistance": {
                "unit": "INCHES",
                "value": 130
              },
              "touch": "UNSUPPORTED",
              "keyboard": "SUPPORTED",
              "video": "SUPPORTED",
              "dialog": "SUPPORTED"
            },
            {
              "id": "tv_overlay",
              "uiMode": "TV",
              "interactionDistance": {
                "unit": "INCHES",
                "value": 130
              },
              "touch": "UNSUPPORTED",
              "keyboard": "SUPPORTED",
              "video": "UNSUPPORTED",
              "dialog": "SUPPORTED"
            }
          ]
})";

/**
 * Expected objects on parsing CONFIG_INTERACTIONMODE specified above
 */
static const InteractionMode INTERACTION_MODE_STANDARD =
    {InteractionMode::UIMode::TV, "tv", InteractionMode::Unit::INCHES, 130, false, true, true, true};
static const InteractionMode INTERACTION_MODE_OVERLAY =
    {InteractionMode::UIMode::TV, "tv_overlay", InteractionMode::Unit::INCHES, 130, false, true, false, true};
static const std::vector<InteractionMode> EXPECTED_INTERACTION_MODES{INTERACTION_MODE_STANDARD,
                                                                     INTERACTION_MODE_OVERLAY};

static const std::string CONFIG_WINDOW = R"({
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
                      "value": {
                        "width": 1920,
                        "height": 1080
                      }
                    }
                  }
                ],
                "interactionModes": [
                  "tv"
                ]
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
                      "value": {
                        "width": 1920,
                        "height": 400
                      }
                    }
                  }
                ],
                "interactionModes": [
                  "tv_overlay"
                ]
              }
            }
          ]
})";

/**
 * Expected objects on parsing CONFIG_WINDOW specified above
 */
static const Dimension DIMENSION_FULL_SCREEN = {Dimension::Unit::PIXEL, 1920, 1080};
static const WindowSize SIZE_FULL_SCREEN = {"fullscreen",
                                            WindowSize::WindowSizeType::DISCRETE,
                                            DIMENSION_FULL_SCREEN,
                                            DIMENSION_FULL_SCREEN};
static const std::vector<WindowSize> SIZES_FULL_SCREEN{SIZE_FULL_SCREEN};
static const std::vector<std::string> INTERACTION_MODES_FULL_SCREEN{"tv"};
static const WindowTemplate TEMPLATE_FULL_SCREEN = {"tvFullscreen",
                                                    WindowTemplate::WindowType::STANDARD,
                                                    SIZES_FULL_SCREEN,
                                                    INTERACTION_MODES_FULL_SCREEN};

static const Dimension DIMENSION_OVERLAY = {Dimension::Unit::PIXEL, 1920, 400};
static const WindowSize SIZE_LANDSCAPE_PANEL = {"landscapePanel",
                                                WindowSize::WindowSizeType::DISCRETE,
                                                DIMENSION_OVERLAY,
                                                DIMENSION_OVERLAY};
static const std::vector<WindowSize> SIZES_OVERLAY{SIZE_LANDSCAPE_PANEL};
static const std::vector<std::string> INTERACTION_MODES_OVERLAY{"tv_overlay"};
static const WindowTemplate TEMPLATE_OVERLAY = {"tvOverlayLandscape",
                                                WindowTemplate::WindowType::OVERLAY,
                                                SIZES_OVERLAY,
                                                INTERACTION_MODES_OVERLAY};
static const std::vector<WindowTemplate> EXPECTED_TEMPLATES{TEMPLATE_FULL_SCREEN, TEMPLATE_OVERLAY};

static const std::string CONFIG_DISPLAY = R"({
          "display": {
            "type": "PIXEL",
            "touch": [
              "UNSUPPORTED",
              "SINGLE"
            ],
            "shape": "RECTANGLE",
            "dimensions": {
              "resolution": {
                "unit": "PIXEL",
                "value": {
                  "width": 1920,
                  "height": 1080
                }
              },
              "physicalSize": {
                "unit": "INCHES",
                "value": {
                  "width": 56.7,
                  "height": 31.9
                }
              },
              "pixelDensity": {
                "unit": "DPI",
                "value": 320
              },
              "densityIndependentResolution": {
                "unit": "DP",
                "value": {
                  "width": 960,
                  "height": 540
                }
              }
            }
          }
})";

/**
 * Expected objects on parsing CONFIG_DISPLAY specified above
 */
static const Dimension RESOLUTION = {Dimension::Unit::PIXEL, 1920, 1080};
static const Dimension PHYSICAL_SIZE = {Dimension::Unit::INCHES, 56.7, 31.9};
static const Dimension DENSITY_INDEPENDENT_RESOLUTION = {Dimension::Unit::DP, 960, 540};
static const std::vector<DisplayCharacteristics::TouchType> TOUCH_TYPES{DisplayCharacteristics::TouchType::SINGLE,
                                                                        DisplayCharacteristics::TouchType::UNSUPPORTED};
static const DisplayCharacteristics EXPECTED_DISPLAY_CHARACTERISTICS = {DisplayCharacteristics::Type::PIXEL,
                                                                        TOUCH_TYPES,
                                                                        DisplayCharacteristics::Shape::RECTANGLE,
                                                                        320,
                                                                        RESOLUTION,
                                                                        DENSITY_INDEPENDENT_RESOLUTION,
                                                                        PHYSICAL_SIZE};

/// Test harness for @c VisualCharacteristics class.
class VCConfigParserTest : public ::testing::Test {
public:
    static bool areInteractionModesEqual(
        const InteractionMode& interactionMode1,
        const InteractionMode& interactionMode2);
    static bool areDimensionsEqual(const Dimension& dimension1, const Dimension& dimension2);
    static bool areWindowSizesEqual(const WindowSize& windowSize1, const WindowSize& windowSize2);
    static bool areWindowTemplatesEqual(const WindowTemplate& windowTemplate1, const WindowTemplate& windowTemplate2);
    static bool areDisplayCharacteristicsEqual(
        const DisplayCharacteristics& displayCharacteristics1,
        const DisplayCharacteristics& displayCharacteristics2);
};

bool VCConfigParserTest::areInteractionModesEqual(
    const InteractionMode& interactionMode1,
    const InteractionMode& interactionMode2) {
    bool result = interactionMode1.uiMode == interactionMode2.uiMode && interactionMode1.id == interactionMode2.id &&
                  interactionMode1.interactionDistanceUnit == interactionMode2.interactionDistanceUnit &&
                  interactionMode1.interactionDistanceValue == interactionMode2.interactionDistanceValue &&
                  interactionMode1.dialogSupported == interactionMode2.dialogSupported &&
                  interactionMode1.keyboardSupported == interactionMode2.keyboardSupported &&
                  interactionMode1.touchSupported == interactionMode2.touchSupported &&
                  interactionMode1.videoSupported == interactionMode2.videoSupported;
    return result;
}

bool VCConfigParserTest::areDimensionsEqual(const Dimension& dimension1, const Dimension& dimension2) {
    bool result = dimension1.unit == dimension2.unit && dimension1.height == dimension2.height &&
                  dimension1.width == dimension2.width;
    return result;
}

bool VCConfigParserTest::areWindowSizesEqual(const WindowSize& windowSize1, const WindowSize& windowSize2) {
    bool result = windowSize1.id == windowSize2.id && windowSize1.type == windowSize2.type &&
                  areDimensionsEqual(windowSize1.maximum, windowSize2.maximum) &&
                  areDimensionsEqual(windowSize1.minimum, windowSize2.minimum);
    return result;
}

bool VCConfigParserTest::areWindowTemplatesEqual(
    const WindowTemplate& windowTemplate1,
    const WindowTemplate& windowTemplate2) {
    bool result = windowTemplate1.id == windowTemplate2.id && windowTemplate1.type == windowTemplate2.type &&
                  windowTemplate1.interactionModes == windowTemplate2.interactionModes;
    if (!result) {
        return result;
    }

    if (windowTemplate1.sizes.size() != windowTemplate2.sizes.size()) {
        return false;
    }

    size_t numberOfWindowSizes = windowTemplate1.sizes.size();
    for (size_t index = 0; index < numberOfWindowSizes; index++) {
        if (!areWindowSizesEqual(windowTemplate1.sizes[index], windowTemplate2.sizes[index])) {
            return false;
        }
    }
    return true;
}

bool VCConfigParserTest::areDisplayCharacteristicsEqual(
    const DisplayCharacteristics& displayCharacteristics1,
    const DisplayCharacteristics& displayCharacteristics2) {
    bool result = displayCharacteristics1.type == displayCharacteristics2.type &&
                  displayCharacteristics1.shape == displayCharacteristics2.shape &&
                  displayCharacteristics1.touch == displayCharacteristics2.touch &&
                  displayCharacteristics1.pixelDensity == displayCharacteristics2.pixelDensity &&
                  VCConfigParserTest::areDimensionsEqual(
                      displayCharacteristics1.resolution, displayCharacteristics2.resolution) &&
                  VCConfigParserTest::areDimensionsEqual(
                      displayCharacteristics1.densityIndependentResolution,
                      displayCharacteristics2.densityIndependentResolution) &&
                  VCConfigParserTest::areDimensionsEqual(
                      displayCharacteristics1.physicalSize, displayCharacteristics2.physicalSize);
    return result;
}

static ConfigurationNode getConfigurationNode(const std::string& jsonConfig) {
    ConfigurationNode::uninitialize();
    auto stream = std::shared_ptr<std::stringstream>(new std::stringstream(jsonConfig));
    JSONStream jsonStream({stream});
    ConfigurationNode::initialize(jsonStream);
    return ConfigurationNode::getRoot();
}

/**
 * Tests parsing logic for Interaction modes.
 */
TEST_F(VCConfigParserTest, testParseInteractionModes) {
    auto cfg = getConfigurationNode(CONFIG_INTERACTIONMODE);
    auto interactionModes = cfg.getArray(INTERACTION_MODES);
    for (size_t j = 0; j < interactionModes.getArraySize(); ++j) {
        InteractionMode interactionMode;
        ASSERT_TRUE(VCConfigParser::parseInteractionMode(interactionModes[j], interactionMode));
        ASSERT_TRUE(VCConfigParserTest::areInteractionModesEqual(EXPECTED_INTERACTION_MODES[j], interactionMode));
    }
}

/**
 * Tests parsing logic for Window Templates.
 */
TEST_F(VCConfigParserTest, testParseWindowTemplates) {
    auto cfg = getConfigurationNode(CONFIG_WINDOW);
    auto templates = cfg.getArray(TEMPLATES);
    for (size_t j = 0; j < templates.getArraySize(); ++j) {
        WindowTemplate windowTemplate;
        ASSERT_TRUE(VCConfigParser::parseWindowTemplate(templates[j], windowTemplate));
        ASSERT_TRUE(VCConfigParserTest::areWindowTemplatesEqual(EXPECTED_TEMPLATES[j], windowTemplate));
    }
}

/**
 * Tests parsing logic for display characteristics.
 */
TEST_F(VCConfigParserTest, testParseDisplayCharacteristics) {
    auto cfg = getConfigurationNode(CONFIG_DISPLAY);
    DisplayCharacteristics displayCharacteristics;
    ASSERT_TRUE(VCConfigParser::parseDisplayCharacteristics(cfg, displayCharacteristics));
    ASSERT_TRUE(
        VCConfigParserTest::areDisplayCharacteristicsEqual(EXPECTED_DISPLAY_CHARACTERISTICS, displayCharacteristics));
}

/**
 * Tests serialize logic for Interaction modes.
 */
TEST_F(VCConfigParserTest, testSerializeInteractionModes) {
    std::string payloadJson;
    InteractionMode tvInteractionMode;
    tvInteractionMode.uiMode = InteractionMode::UIMode::TV;
    tvInteractionMode.id = "tv";
    tvInteractionMode.interactionDistanceUnit = InteractionMode::Unit::INCHES;
    tvInteractionMode.interactionDistanceValue = 130;
    tvInteractionMode.touchSupported = false;
    tvInteractionMode.keyboardSupported = true;
    tvInteractionMode.videoSupported = true;
    tvInteractionMode.dialogSupported = true;

    InteractionMode overlayInteractionMode;
    overlayInteractionMode.uiMode = InteractionMode::UIMode::TV;
    overlayInteractionMode.id = "tv_overlay";
    overlayInteractionMode.interactionDistanceUnit = InteractionMode::Unit::INCHES;
    overlayInteractionMode.interactionDistanceValue = 130;
    overlayInteractionMode.touchSupported = false;
    overlayInteractionMode.keyboardSupported = true;
    overlayInteractionMode.videoSupported = false;
    overlayInteractionMode.dialogSupported = true;

    std::vector<InteractionMode> interactionModes{tvInteractionMode, overlayInteractionMode};
    ASSERT_TRUE(VCConfigParser::serializeInteractionMode(interactionModes, payloadJson));

    // verify that same interaction modes are recreated from the serialized json
    auto cfg = getConfigurationNode(payloadJson);
    auto interactionModesFromJson = cfg.getArray(INTERACTION_MODES);
    for (size_t j = 0; j < interactionModesFromJson.getArraySize(); ++j) {
        InteractionMode interactionModeRecreated;
        ASSERT_TRUE(VCConfigParser::parseInteractionMode(interactionModesFromJson[j], interactionModeRecreated));
        ASSERT_TRUE(VCConfigParserTest::areInteractionModesEqual(interactionModes[j], interactionModeRecreated));
    }
}

/**
 * Tests serialize logic for Window Templates.
 */
TEST_F(VCConfigParserTest, testSerializeWindowTemplates) {
    std::string payloadJson;
    Dimension fullscreenDimension{Dimension::Unit::PIXEL, 1920, 1080};
    Dimension partialDimension{Dimension::Unit::PIXEL, 1920, 400};
    WindowSize fullscreenSize{
        "fullscreen", WindowSize::WindowSizeType::DISCRETE, fullscreenDimension, fullscreenDimension};
    WindowSize partialSize{"landscapePanel", WindowSize::WindowSizeType::DISCRETE, partialDimension, partialDimension};
    WindowTemplate templateFullscreen{"tvFullscreen", WindowTemplate::WindowType::STANDARD, {fullscreenSize}, {"tv"}};
    WindowTemplate templatePartial{
        "tvOverlayLandscape", WindowTemplate::WindowType::OVERLAY, {partialSize}, {"tv_overlay"}};

    std::vector<WindowTemplate> windowTemplates = {templateFullscreen, templatePartial};
    ASSERT_TRUE(VCConfigParser::serializeWindowTemplate(windowTemplates, payloadJson));

    // verify that same window templates are recreated from the serialized json
    auto cfg = getConfigurationNode(payloadJson);
    auto templatesFromJson = cfg.getArray(TEMPLATES);
    for (size_t j = 0; j < templatesFromJson.getArraySize(); ++j) {
        WindowTemplate windowTemplateRecreated;
        ASSERT_TRUE(VCConfigParser::parseWindowTemplate(templatesFromJson[j], windowTemplateRecreated));
        ASSERT_TRUE(VCConfigParserTest::areWindowTemplatesEqual(windowTemplates[j], windowTemplateRecreated));
    }
}

/**
 * Tests serialize logic for display characteristics.
 */
TEST_F(VCConfigParserTest, testSerializeDisplayCharacteristics) {
    std::string payloadJson;
    DisplayCharacteristics displayCharacteristics;
    displayCharacteristics.type = DisplayCharacteristics::Type::PIXEL;
    displayCharacteristics.touch = {DisplayCharacteristics::TouchType::UNSUPPORTED};
    displayCharacteristics.shape = DisplayCharacteristics::RECTANGLE;
    displayCharacteristics.resolution = {Dimension::Unit::PIXEL, 1920, 1080};
    displayCharacteristics.physicalSize = {Dimension::Unit::INCHES, 56.7, 31.9};
    displayCharacteristics.pixelDensity = 320;
    displayCharacteristics.densityIndependentResolution = {Dimension::Unit::DP, 960, 540};
    ASSERT_TRUE(VCConfigParser::serializeDisplayCharacteristics(displayCharacteristics, payloadJson));

    // verify that same display characteristics are recreated from the serialized json
    auto cfg = getConfigurationNode(payloadJson);
    DisplayCharacteristics displayCharacteristicsRecreated;
    ASSERT_TRUE(VCConfigParser::parseDisplayCharacteristics(cfg, displayCharacteristicsRecreated));
    ASSERT_TRUE(areDisplayCharacteristicsEqual(displayCharacteristics, displayCharacteristicsRecreated));
}

}  // namespace visualCharacteristics
}  // namespace alexaClientSDK
