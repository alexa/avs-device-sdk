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

#include "acsdk/VisualCharacteristics/private/VCConfigParser.h"

#include <unordered_map>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace alexaClientSDK {
namespace visualCharacteristics {
using namespace visualCharacteristicsInterfaces;

/// String to identify log entries originating from this file.
static const std::string TAG{"VisualCharacteristicsConfigParser"};
/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

/// @{
/// String constants used for parsing visual characteristics
static const std::string ID = "id";
static const std::string UIMODE = "uiMode";
static const std::string INTERACTION_DISTANCE = "interactionDistance";
static const std::string UNIT = "unit";
static const std::string VALUE = "value";
static const std::string TOUCH = "touch";
static const std::string KEYBOARD = "keyboard";
static const std::string VIDEO = "video";
static const std::string DIALOG = "dialog";
static const std::string TYPE = "type";
static const std::string CONFIGURATION = "configuration";
static const std::string TEMPLATES = "templates";
static const std::string MINIMUM = "minimum";
static const std::string MAXIMUM = "maximum";
static const std::string SUPPORTED = "SUPPORTED";
static const std::string UNSUPPORTED = "UNSUPPORTED";
static const std::string WIDTH = "width";
static const std::string HEIGHT = "height";
static const std::string SIZES = "sizes";
static const std::string INTERACTION_MODES = "interactionModes";
static const std::string DISPLAY = "display";
static const std::string SHAPE = "shape";
static const std::string DIMENSIONS = "dimensions";
static const std::string RESOLUTION = "resolution";
static const std::string PHYSICAL_SIZE = "physicalSize";
static const std::string DENSITY_INDEPENDENT_RESOLUTION = "densityIndependentResolution";
static const std::string PIXEL_DENSITY = "pixelDensity";
/// @}

/// Mappings from string values to UIMode
static const std::unordered_map<std::string, InteractionMode::UIMode> UIMODE_MAPPING = {
    {"AUTO", InteractionMode::UIMode::AUTO},
    {"HUB", InteractionMode::UIMode::HUB},
    {"TV", InteractionMode::UIMode::TV},
    {"MOBILE", InteractionMode::UIMode::MOBILE},
    {"PC", InteractionMode::UIMode::PC},
    {"HEADLESS", InteractionMode::UIMode::HEADLESS}};

/// Mappings from string values to InteractionMode Unit
static const std::unordered_map<std::string, InteractionMode::Unit> INTERACTIONMODE_UNIT_MAPPING = {
    {"CENTIMETERS", InteractionMode::Unit::CENTIMETERS},
    {"INCHES", InteractionMode::Unit::INCHES}};

/// Mappings from string values to WindowType
static const std::unordered_map<std::string, WindowTemplate::WindowType> WINDOWTYPE_MAPPING = {
    {"OVERLAY", WindowTemplate::WindowType::OVERLAY},
    {"STANDARD", WindowTemplate::WindowType::STANDARD}};

/// Mappings from string values to WindowSizeType
static const std::unordered_map<std::string, WindowSize::WindowSizeType> WINDOWSIZETYPE_MAPPING = {
    {"DISCRETE", WindowSize::WindowSizeType::DISCRETE},
    {"CONTINUOUS", WindowSize::WindowSizeType::CONTINUOUS}};

/// Mappings from string values to Dimension Unit
static const std::unordered_map<std::string, Dimension::Unit> DIMENSION_UNIT_MAPPING = {
    {"INCHES", Dimension::Unit::INCHES},
    {"CENTIMETERS", Dimension::Unit::CENTIMETERS},
    {"DP", Dimension::Unit::DP},
    {"DPI", Dimension::Unit::DPI},
    {"PIXEL", Dimension::Unit::PIXEL}};

/// Mappings from string values to DisplayCharacteristics Type
static const std::unordered_map<std::string, DisplayCharacteristics::Type> DISPLAY_TYPE_MAPPING = {
    {"PIXEL", DisplayCharacteristics::Type::PIXEL}};

/// Mappings from string values to DisplayCharacteristics TouchType
static const std::unordered_map<std::string, DisplayCharacteristics::TouchType> TOUCH_MAPPING = {
    {"SINGLE", DisplayCharacteristics::TouchType::SINGLE},
    {"UNSUPPORTED", DisplayCharacteristics::TouchType::UNSUPPORTED},
};

/// Mappings from string values to DisplayCharacteristics Shape
static const std::unordered_map<std::string, DisplayCharacteristics::Shape> SHAPE_MAPPING = {
    {"RECTANGLE", DisplayCharacteristics::Shape::RECTANGLE},
    {"ROUND", DisplayCharacteristics::Shape::ROUND},
};

/**
 * Converts a boolean value to a SUPPORTED / UNSUPPORTED string
 * @param value the boolean value
 * @return the string representing the value
 */
constexpr static const std::string& isSupported(bool value) {
    return value ? SUPPORTED : UNSUPPORTED;
}

bool VCConfigParser::parseInteractionMode(
    const avsCommon::utils::configuration::ConfigurationNode& config,
    InteractionMode& interactionMode) {
    if (!stringFromConfig(config, ID, interactionMode.id)) return false;
    if (!enumFromConfig(config, UIMODE, UIMODE_MAPPING, interactionMode.uiMode)) return false;
    if (!enumFromConfig(
            config[INTERACTION_DISTANCE], UNIT, INTERACTIONMODE_UNIT_MAPPING, interactionMode.interactionDistanceUnit))
        return false;
    if (!intFromConfig(config[INTERACTION_DISTANCE], VALUE, interactionMode.interactionDistanceValue)) return false;
    if (!supportedBoolFromConfig(config, TOUCH, interactionMode.touchSupported)) return false;
    if (!supportedBoolFromConfig(config, KEYBOARD, interactionMode.keyboardSupported)) return false;
    if (!supportedBoolFromConfig(config, VIDEO, interactionMode.videoSupported)) return false;
    if (!supportedBoolFromConfig(config, DIALOG, interactionMode.dialogSupported)) return false;

    return true;
}

bool VCConfigParser::parseWindowTemplate(
    const avsCommon::utils::configuration::ConfigurationNode& config,
    WindowTemplate& windowTemplate) {
    if (!stringFromConfig(config, ID, windowTemplate.id)) return false;
    if (!enumFromConfig(config, TYPE, WINDOWTYPE_MAPPING, windowTemplate.type)) return false;

    auto configuration = config[CONFIGURATION];
    if (!configuration) {
        ACSDK_ERROR(LX(__func__).d("reason", "Missing configuration"));
        return false;
    }

    auto sizes = configuration.getArray(SIZES);
    for (size_t i = 0; i < sizes.getArraySize(); ++i) {
        WindowSize windowSize;
        auto sizeConfig = sizes[i];
        if (!enumFromConfig(sizeConfig, TYPE, WINDOWSIZETYPE_MAPPING, windowSize.type)) {
            ACSDK_ERROR(LX(__func__).d("reason", "invalid window size type"));
            return false;
        }
        if (!stringFromConfig(sizeConfig, ID, windowSize.id)) return false;

        if (windowSize.type == WindowSize::WindowSizeType::DISCRETE) {
            if (!dimensionFromConfig(sizeConfig, VALUE, windowSize.minimum)) return false;
            windowSize.maximum = windowSize.minimum;
        } else {
            if (!dimensionFromConfig(sizeConfig, MINIMUM, windowSize.minimum)) return false;
            if (!dimensionFromConfig(sizeConfig, MAXIMUM, windowSize.maximum)) return false;
        }

        if (windowSize.minimum.unit != Dimension::Unit::PIXEL) {
            ACSDK_ERROR(LX(__func__).d("reason", "invalid unit type, only PIXEL is valid"));
            return false;
        }
        windowTemplate.sizes.push_back(windowSize);
    }

    std::set<std::string> values;
    configuration.getStringValues(INTERACTION_MODES, &values);
    for (auto value : values) {
        windowTemplate.interactionModes.push_back(value);
    }
    return true;
}

bool VCConfigParser::parseDisplayCharacteristics(
    const avsCommon::utils::configuration::ConfigurationNode& config,
    DisplayCharacteristics& display) {
    auto displayConfig = config[DISPLAY];
    if (!displayConfig) {
        ACSDK_ERROR(LX(__func__).d("reason", "Missing display configuration item"));
        return false;
    }

    if (!enumFromConfig(displayConfig, TYPE, DISPLAY_TYPE_MAPPING, display.type)) return false;
    std::set<std::string> touchValues;
    if (!displayConfig.getStringValues(TOUCH, &touchValues)) {
        ACSDK_ERROR(LX(__func__).d("reason", "Missing touch configuration"));
        return false;
    }

    for (auto& value : touchValues) {
        DisplayCharacteristics::TouchType type;
        if (!stringToEnum(TOUCH_MAPPING, value, type)) return false;
        display.touch.push_back(type);
    }

    if (!enumFromConfig(displayConfig, SHAPE, SHAPE_MAPPING, display.shape)) return false;

    auto dimensionConfig = displayConfig[DIMENSIONS];
    if (!dimensionFromConfig(dimensionConfig, RESOLUTION, display.resolution)) return false;
    if (!dimensionFromConfig(dimensionConfig, PHYSICAL_SIZE, display.physicalSize)) return false;
    if (!dimensionFromConfig(dimensionConfig, DENSITY_INDEPENDENT_RESOLUTION, display.densityIndependentResolution))
        return false;
    if (!intFromConfig(dimensionConfig[PIXEL_DENSITY], VALUE, display.pixelDensity)) return false;

    return true;
}

bool VCConfigParser::serializeInteractionMode(
    const std::vector<InteractionMode>& interactionModes,
    std::string& configJson) {
    rapidjson::Document configurationJson(rapidjson::kObjectType);
    auto& alloc = configurationJson.GetAllocator();
    rapidjson::Value payload(rapidjson::kObjectType);
    rapidjson::Value interactionModesJson(rapidjson::kArrayType);

    for (auto& interactionMode : interactionModes) {
        rapidjson::Value interactionModeJson(rapidjson::kObjectType);

        interactionModeJson.AddMember(rapidjson::StringRef(ID), interactionMode.id, alloc);
        interactionModeJson.AddMember(
            rapidjson::StringRef(UIMODE), InteractionMode::uiModeToText(interactionMode.uiMode), alloc);

        rapidjson::Value interactionDistanceJson(rapidjson::kObjectType);
        interactionDistanceJson.AddMember(
            rapidjson::StringRef(UNIT), InteractionMode::unitToText(interactionMode.interactionDistanceUnit), alloc);
        interactionDistanceJson.AddMember(rapidjson::StringRef(VALUE), interactionMode.interactionDistanceValue, alloc);
        interactionModeJson.AddMember(rapidjson::StringRef(INTERACTION_DISTANCE), interactionDistanceJson, alloc);

        interactionModeJson.AddMember(rapidjson::StringRef(TOUCH), isSupported(interactionMode.touchSupported), alloc);
        interactionModeJson.AddMember(
            rapidjson::StringRef(KEYBOARD), isSupported(interactionMode.keyboardSupported), alloc);
        interactionModeJson.AddMember(rapidjson::StringRef(VIDEO), isSupported(interactionMode.videoSupported), alloc);
        interactionModeJson.AddMember(
            rapidjson::StringRef(DIALOG), isSupported(interactionMode.dialogSupported), alloc);

        interactionModesJson.PushBack(interactionModeJson, alloc);
    }

    payload.AddMember(rapidjson::StringRef(INTERACTION_MODES), interactionModesJson, alloc);

    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    bool returnValue = payload.Accept(writer);

    configJson = sb.GetString();
    ACSDK_DEBUG9(LX(__func__).d("Serialized Interaction Mode", configJson));

    return returnValue;
}

bool VCConfigParser::serializeWindowTemplate(
    const std::vector<WindowTemplate>& windowTemplates,
    std::string& configJson) {
    rapidjson::Document configurationsJson(rapidjson::kObjectType);
    auto& alloc = configurationsJson.GetAllocator();
    rapidjson::Value payload(rapidjson::kObjectType);
    rapidjson::Value windowTemplatesJson(rapidjson::kArrayType);

    for (auto& windowTemplate : windowTemplates) {
        rapidjson::Value windowTemplateJson(rapidjson::kObjectType);

        windowTemplateJson.AddMember(rapidjson::StringRef(ID), windowTemplate.id, alloc);
        windowTemplateJson.AddMember(
            rapidjson::StringRef(TYPE), WindowTemplate::windowTypeToText(windowTemplate.type), alloc);

        rapidjson::Value configurationJson(rapidjson::kObjectType);
        rapidjson::Value sizesJson(rapidjson::kArrayType);
        for (auto& size : windowTemplate.sizes) {
            rapidjson::Value sizeJson(rapidjson::kObjectType);
            sizeJson.AddMember(rapidjson::StringRef(TYPE), WindowSize::windowSizeTypeToText(size.type), alloc);
            sizeJson.AddMember(rapidjson::StringRef(ID), size.id, alloc);

            if (size.type == WindowSize::WindowSizeType::DISCRETE) {
                rapidjson::Value valueJson(rapidjson::kObjectType);
                valueJson.AddMember(rapidjson::StringRef(UNIT), Dimension::unitToText(size.minimum.unit), alloc);
                rapidjson::Value unitValueJson(rapidjson::kObjectType);
                unitValueJson.AddMember(rapidjson::StringRef(WIDTH), static_cast<int>(size.minimum.width), alloc);
                unitValueJson.AddMember(rapidjson::StringRef(HEIGHT), static_cast<int>(size.minimum.height), alloc);
                valueJson.AddMember(rapidjson::StringRef(VALUE), unitValueJson, alloc);
                sizeJson.AddMember(rapidjson::StringRef(VALUE), valueJson, alloc);
            } else if (size.type == WindowSize::WindowSizeType::CONTINUOUS) {
                // Serialize minimum values
                rapidjson::Value minValueJson(rapidjson::kObjectType);
                minValueJson.AddMember(rapidjson::StringRef(UNIT), Dimension::unitToText(size.minimum.unit), alloc);
                rapidjson::Value minUnitValueJson(rapidjson::kObjectType);
                minUnitValueJson.AddMember(rapidjson::StringRef(WIDTH), static_cast<int>(size.minimum.width), alloc);
                minUnitValueJson.AddMember(rapidjson::StringRef(HEIGHT), static_cast<int>(size.minimum.height), alloc);
                minValueJson.AddMember(rapidjson::StringRef(VALUE), minUnitValueJson, alloc);
                sizeJson.AddMember(rapidjson::StringRef(MINIMUM), minValueJson, alloc);
                // Serialize maximum values
                rapidjson::Value maxValueJson(rapidjson::kObjectType);
                maxValueJson.AddMember(rapidjson::StringRef(UNIT), Dimension::unitToText(size.maximum.unit), alloc);
                rapidjson::Value maxUnitValueJson(rapidjson::kObjectType);
                maxUnitValueJson.AddMember(rapidjson::StringRef(WIDTH), static_cast<int>(size.maximum.width), alloc);
                maxUnitValueJson.AddMember(rapidjson::StringRef(HEIGHT), static_cast<int>(size.maximum.height), alloc);
                maxValueJson.AddMember(rapidjson::StringRef(VALUE), maxUnitValueJson, alloc);
                sizeJson.AddMember(rapidjson::StringRef(MAXIMUM), maxValueJson, alloc);
            }

            sizesJson.PushBack(sizeJson, alloc);
        }

        configurationJson.AddMember(rapidjson::StringRef(SIZES), sizesJson, alloc);

        rapidjson::Value interactionModesJson(rapidjson::kArrayType);
        for (auto& mode : windowTemplate.interactionModes) {
            rapidjson::Value value;
            value.SetString(mode.c_str(), mode.length(), alloc);
            interactionModesJson.PushBack(value, alloc);
        }
        configurationJson.AddMember(rapidjson::StringRef(INTERACTION_MODES), interactionModesJson, alloc);

        windowTemplateJson.AddMember(rapidjson::StringRef(CONFIGURATION), configurationJson, alloc);
        windowTemplatesJson.PushBack(windowTemplateJson, alloc);
    }

    payload.AddMember(rapidjson::StringRef(TEMPLATES), windowTemplatesJson, alloc);

    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    bool returnValue = payload.Accept(writer);

    configJson = sb.GetString();
    ACSDK_DEBUG9(LX(__func__).d("Serialized Window Template", configJson));

    return returnValue;
}

bool VCConfigParser::serializeDisplayCharacteristics(const DisplayCharacteristics& display, std::string& configJson) {
    rapidjson::Document configurationsJson(rapidjson::kObjectType);
    auto& alloc = configurationsJson.GetAllocator();
    rapidjson::Value payload(rapidjson::kObjectType);
    rapidjson::Value displayJson(rapidjson::kObjectType);

    displayJson.AddMember(rapidjson::StringRef(TYPE), DisplayCharacteristics::typeToText(display.type), alloc);
    rapidjson::Value touchJson(rapidjson::kArrayType);
    for (auto& touch : display.touch) {
        rapidjson::Value value;
        std::string touchType = DisplayCharacteristics::touchTypeToText(touch);
        value.SetString(touchType.c_str(), touchType.length(), alloc);
        touchJson.PushBack(value, alloc);
    }
    displayJson.AddMember(rapidjson::StringRef(TOUCH), touchJson, alloc);
    displayJson.AddMember(rapidjson::StringRef(SHAPE), DisplayCharacteristics::shapeToText(display.shape), alloc);

    rapidjson::Value dimensionJson(rapidjson::kObjectType);

    rapidjson::Value resolutionJson(rapidjson::kObjectType);
    resolutionJson.AddMember(rapidjson::StringRef(UNIT), Dimension::unitToText(display.resolution.unit), alloc);
    rapidjson::Value resolutionUnitValueJson(rapidjson::kObjectType);
    resolutionUnitValueJson.AddMember(rapidjson::StringRef(WIDTH), static_cast<int>(display.resolution.width), alloc);
    resolutionUnitValueJson.AddMember(rapidjson::StringRef(HEIGHT), static_cast<int>(display.resolution.height), alloc);
    resolutionJson.AddMember(rapidjson::StringRef(VALUE), resolutionUnitValueJson, alloc);
    dimensionJson.AddMember(rapidjson::StringRef(RESOLUTION), resolutionJson, alloc);

    rapidjson::Value physicalSizeJson(rapidjson::kObjectType);
    physicalSizeJson.AddMember(rapidjson::StringRef(UNIT), Dimension::unitToText(display.physicalSize.unit), alloc);
    rapidjson::Value physicalSizeUnitValueJson(rapidjson::kObjectType);
    physicalSizeUnitValueJson.AddMember(rapidjson::StringRef(WIDTH), display.physicalSize.width, alloc);
    physicalSizeUnitValueJson.AddMember(rapidjson::StringRef(HEIGHT), display.physicalSize.height, alloc);
    physicalSizeJson.AddMember(rapidjson::StringRef(VALUE), physicalSizeUnitValueJson, alloc);
    dimensionJson.AddMember(rapidjson::StringRef(PHYSICAL_SIZE), physicalSizeJson, alloc);

    rapidjson::Value pixelDensityJson(rapidjson::kObjectType);
    pixelDensityJson.AddMember(rapidjson::StringRef(UNIT), Dimension::unitToText(Dimension::Unit::DPI), alloc);
    pixelDensityJson.AddMember(rapidjson::StringRef(VALUE), display.pixelDensity, alloc);
    dimensionJson.AddMember(rapidjson::StringRef(PIXEL_DENSITY), pixelDensityJson, alloc);

    rapidjson::Value densityIndependentResolutionJson(rapidjson::kObjectType);
    densityIndependentResolutionJson.AddMember(
        rapidjson::StringRef(UNIT), Dimension::unitToText(display.densityIndependentResolution.unit), alloc);
    rapidjson::Value densityIndependentResolutionUnitValueJson(rapidjson::kObjectType);
    densityIndependentResolutionUnitValueJson.AddMember(
        rapidjson::StringRef(WIDTH), static_cast<int>(display.densityIndependentResolution.width), alloc);
    densityIndependentResolutionUnitValueJson.AddMember(
        rapidjson::StringRef(HEIGHT), static_cast<int>(display.densityIndependentResolution.height), alloc);
    densityIndependentResolutionJson.AddMember(
        rapidjson::StringRef(VALUE), densityIndependentResolutionUnitValueJson, alloc);
    dimensionJson.AddMember(
        rapidjson::StringRef(DENSITY_INDEPENDENT_RESOLUTION), densityIndependentResolutionJson, alloc);

    displayJson.AddMember(rapidjson::StringRef(DIMENSIONS), dimensionJson, alloc);
    payload.AddMember(rapidjson::StringRef(DISPLAY), displayJson, alloc);

    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    bool returnValue = payload.Accept(writer);

    configJson = sb.GetString();
    ACSDK_DEBUG9(LX(__func__).d("Serialized Display Characteristics", configJson));

    return returnValue;
}

template <typename T>
bool VCConfigParser::enumFromConfig(
    const avsCommon::utils::configuration::ConfigurationNode& config,
    const std::string& configKey,
    const std::unordered_map<std::string, T>& enumMapping,
    T& out) {
    std::string value;
    if (!config.getString(configKey, &value, "")) {
        ACSDK_ERROR(LX(__func__).d("reason", "Missing configuration item").d("key", configKey));
        return false;
    }

    if (!stringToEnum(enumMapping, value, out)) {
        ACSDK_ERROR(LX(__func__).d("reason", "Invalid value").d("key", configKey).d("value", value));
        return false;
    }

    return true;
}

template <typename T>
bool VCConfigParser::stringToEnum(const std::unordered_map<std::string, T>& mapping, const std::string& in, T& out) {
    auto it = mapping.find(in);
    if (it != mapping.end()) {
        out = it->second;
        return true;
    }

    return false;
}

bool VCConfigParser::supportedBoolFromConfig(
    const avsCommon::utils::configuration::ConfigurationNode& config,
    const std::string& configKey,
    bool& out) {
    std::string value;
    if (!config.getString(configKey, &value)) {
        ACSDK_ERROR(LX(__func__).d("reason", "Missing configuration item").d("key", configKey));
        return false;
    }
    if (SUPPORTED == value) {
        out = true;
        return true;
    } else if (UNSUPPORTED == value) {
        out = false;
        return true;
    }

    ACSDK_ERROR(LX(__func__)
                    .d("reason", "Invalid value, expected SUPPORTED or UNSUPPORTED")
                    .d("key", configKey)
                    .d("value", value));
    return false;
}

bool VCConfigParser::stringFromConfig(
    const avsCommon::utils::configuration::ConfigurationNode& config,
    const std::string& configKey,
    std::string& out) {
    if (config.getString(configKey, &out)) {
        return true;
    }

    ACSDK_ERROR(LX(__func__).d("reason", "Missing configuration item").d("key", configKey));
    return false;
}

bool VCConfigParser::intFromConfig(
    const avsCommon::utils::configuration::ConfigurationNode& config,
    const std::string& configKey,
    int& out) {
    if (config.getInt(configKey, &out)) {
        return true;
    }

    ACSDK_ERROR(LX(__func__).d("reason", "Missing configuration item").d("key", configKey));
    return false;
}

bool VCConfigParser::doubleFromConfig(
    const avsCommon::utils::configuration::ConfigurationNode& config,
    const std::string& configKey,
    double& out) {
    if (config.getValue(configKey, &out, 0.0, &rapidjson::Value::IsNumber, &rapidjson::Value::GetDouble)) {
        return true;
    }

    ACSDK_ERROR(LX(__func__).d("reason", "Missing configuration item").d("key", configKey));
    return false;
}

bool VCConfigParser::dimensionFromConfig(
    const avsCommon::utils::configuration::ConfigurationNode& config,
    const std::string& configKey,
    Dimension& out) {
    auto configItem = config[configKey];
    if (!configItem) {
        ACSDK_ERROR(LX(__func__).d("reason", "Missing configuration item").d("key", configKey));
        return false;
    }
    if (!enumFromConfig(configItem, UNIT, DIMENSION_UNIT_MAPPING, out.unit)) return false;
    if (!doubleFromConfig(configItem[VALUE], WIDTH, out.width)) return false;
    if (!doubleFromConfig(configItem[VALUE], HEIGHT, out.height)) return false;
    return true;
}
}  // namespace visualCharacteristics
}  // namespace alexaClientSDK
