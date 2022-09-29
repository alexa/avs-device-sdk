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
#ifndef ACSDK_VISUALCHARACTERISTICSINTERFACES_VISUALCHARACTERISTICSINTERFACE_H_
#define ACSDK_VISUALCHARACTERISTICSINTERFACES_VISUALCHARACTERISTICSINTERFACE_H_

#include <string>
#include <vector>

namespace alexaClientSDK {
namespace visualCharacteristicsInterfaces {

/**
 * The dimension configuration
 */
struct Dimension {
    /// Enum defining the units for dimension
    enum class Unit {
        /// Unit in pixels.
        PIXEL,

        /// Unit in pixels.
        DPI,

        /// Unit in dp
        DP,

        /// Unit in centimeter
        CENTIMETERS,

        /// Unit in inch
        INCHES
    };

    /// Unit for this dimension.
    Unit unit;

    /// Width in unit dimensions.
    double width;

    /// Height in unit dimensions.
    double height;

    /**
     * Convert enum to string
     * @param unit UNIT value
     * @return Text defining the enum
     */
    static const std::string unitToText(const Unit unit) {
        switch (unit) {
            case Unit::PIXEL:
                return "PIXEL";
            case Unit::DPI:
                return "DPI";
            case Unit::DP:
                return "DP";
            case Unit::CENTIMETERS:
                return "CENTIMETERS";
            case Unit::INCHES:
                return "INCHES";
        }

        return std::string();
    }
};

/**
 * The window size configuration
 */
struct WindowSize {
    /// Enum defining the Window Size types
    enum class WindowSizeType {
        /// Specify DISCRETE for a window size with fixed points
        DISCRETE,

        /// Specify CONTINUOUS to allow any window size under a specified maximum width and height
        CONTINUOUS
    };

    /// Unique identifier for the specified size configuration
    std::string id;

    /// Window size type
    WindowSizeType type;

    /// Dimension for the window
    /// For DISCRETE window types both minimum and maximum will be identical
    /// Unit must be PIXELS
    Dimension minimum;
    Dimension maximum;

    static const std::string windowSizeTypeToText(const WindowSizeType windowSizeType) {
        switch (windowSizeType) {
            case WindowSizeType::DISCRETE:
                return "DISCRETE";
            case WindowSizeType::CONTINUOUS:
                return "CONTINUOUS";
        }

        return std::string();
    }
};

/**
 * The window template for Alexa.Display.Window interface.
 * https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/display-window.html
 */
struct WindowTemplate {
    /// Enum defining the window types for the template
    enum class WindowType {
        /// Permanent, static window
        STANDARD,

        /// Temporary window that might share the screen with other experiences. Overlay windows could be transparent
        OVERLAY
    };

    /// Window template id.
    std::string id;

    /// Type of of the window template.
    WindowType type;

    /// Sizes supported by this window template.
    std::vector<WindowSize> sizes;

    /// Interaction mode supported by this window template.
    std::vector<std::string> interactionModes;

    /**
     * Converts enum to string
     * @param windowType WindowType enum value
     * @return Text defining the enum
     */
    static const std::string windowTypeToText(const WindowType windowType) {
        switch (windowType) {
            case WindowType::STANDARD:
                return "STANDARD";
            case WindowType::OVERLAY:
                return "OVERLAY";
        }

        return std::string();
    }
};

/**
 * The interaction mode for Alexa.InteractionMode interface.
 * https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/interaction-mode.html
 */
struct InteractionMode {
    /// Enum defining the UI modes of interaction mode.
    enum class UIMode {
        /// Auto UI mode
        AUTO,

        /// Hub UI mode
        HUB,

        /// Tv UI mode
        TV,

        /// Mobile UI mode
        MOBILE,

        /// Pc UI mode
        PC,

        /// Headless UI mode
        HEADLESS
    };

    /// Enum class defining the unit for the interaction mode
    enum class Unit {
        /// Unit is centimeters.
        CENTIMETERS,

        /// Unit is inches.
        INCHES
    };

    /// UI mode for this interaction mode.
    UIMode uiMode;

    /// ID of this interaction mode.
    std::string id;

    /// Unit of the interaction distance for this interaction mode.
    Unit interactionDistanceUnit;

    /// Value of the interaction distance for this interaction mode.
    int interactionDistanceValue;

    /// Touch support.
    bool touchSupported;

    /// Keyboard support.
    bool keyboardSupported;

    /// Video support.
    bool videoSupported;

    /// Dialog support.
    bool dialogSupported;

    /**
     * Converts enum to string
     * @param uiMode UIMode enum value
     * @return Text defining the enum
     */
    static const std::string uiModeToText(const UIMode uiMode) {
        switch (uiMode) {
            case UIMode::AUTO:
                return "AUTO";
            case UIMode::HUB:
                return "HUB";
            case UIMode::TV:
                return "TV";
            case UIMode::MOBILE:
                return "MOBILE";
            case UIMode::PC:
                return "PC";
            case UIMode::HEADLESS:
                return "HEADLESS";
        }

        return std::string();
    }

    /**
     * Converts enum to string
     * @param unit Unit enum value
     * @return Text defining the enum
     */
    static const std::string unitToText(const Unit unit) {
        switch (unit) {
            case Unit::CENTIMETERS:
                return "CENTIMETERS";
            case Unit::INCHES:
                return "INCHES";
        }

        return std::string();
    }
};

/**
 * The display characteristics for Alexa.Display interface.
 * https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/display.html
 */
struct DisplayCharacteristics {
    /// Enum defining the type of the Display
    enum Type {
        /// Pixel display type
        PIXEL
    };

    /// Enum defining the touch type supported by display
    enum TouchType {
        /// Single display type
        SINGLE,

        /// Unsupported display type
        UNSUPPORTED
    };

    /// Enum defining the shape of the display
    enum Shape {
        /// Rectangle shape
        RECTANGLE,

        /// Round shape
        ROUND
    };

    /// Type of the display.
    Type type;

    /// Touch types supported by the display.
    std::vector<TouchType> touch;

    /// Shape of the display.
    Shape shape;

    /// The pixel density, unit is DPI
    int pixelDensity;

    /// The display resolution, unit must be PIXEL
    Dimension resolution;

    /// The density independent resolution, unit must be DP
    Dimension densityIndependentResolution;

    /// The physical size of the display, unit must be CENTIMETERS or INCHES
    Dimension physicalSize;

    /**
     * Converts enum to string
     * @param type Display type enum value
     * @return Text defining the enum
     */
    static const std::string typeToText(const Type type) {
        switch (type) {
            case Type::PIXEL:
                return "PIXEL";
        }

        return std::string();
    }

    /**
     * Converts enum to string
     * @param touchType Display touch type enum value
     * @return Text defining the enum
     */
    static const std::string touchTypeToText(const TouchType touchType) {
        switch (touchType) {
            case TouchType::SINGLE:
                return "SINGLE";
            case TouchType::UNSUPPORTED:
                return "UNSUPPORTED";
        }

        return std::string();
    }

    /**
     * Converts enum to string
     * @param shape Display shape enum value
     * @return Text defining the enum
     */
    static const std::string shapeToText(const Shape shape) {
        switch (shape) {
            case Shape::RECTANGLE:
                return "RECTANGLE";
            case Shape::ROUND:
                return "ROUND";
        }

        return std::string();
    }
};

/**
 * Struct defining an instance of a window.
 */
struct WindowInstance {
    std::string id;
    std::string templateId;
    std::string interactionMode;
    std::string sizeConfigurationId;
};

/**
 * Struct defining an instance of a window managed by orchestrator.
 */
struct OrchestratorWindowInstance : public WindowInstance {
    bool includeInWindowState;
    bool supportsBackgroundPresentations;
    int display;
    int zOrderIndex;
};

/**
 * Struct defining all the visual characteristics configurations
 */
struct VisualCharacteristicsConfiguration {
    std::vector<InteractionMode> interactionModes;
    std::vector<WindowTemplate> windowTemplates;
    DisplayCharacteristics displayCharacteristics;
};

/**
 * Interface contract for @c VisualCharacteristics capability agent.
 */
class VisualCharacteristicsInterface {
public:
    /**
     * Destructor
     */
    virtual ~VisualCharacteristicsInterface() = default;

    /**
     * Gets the window template configuration
     * @return vector containing the window templates
     */
    virtual std::vector<WindowTemplate> getWindowTemplates() = 0;

    /**
     * Retrieve the interaction mode configuration
     * @return vector containing the interaction modes
     */
    virtual std::vector<InteractionMode> getInteractionModes() = 0;

    /**
     * Get the display characteristics
     * @return the display characteristics object
     */
    virtual DisplayCharacteristics getDisplayCharacteristics() = 0;

    /**
     * Sets the window instances to be reported in WindowState.
     * Replaces any windows in the existing WindowState set.
     * @param instances The vector of window instances to aad/report in WindowState
     * @param defaultWindowInstanceId The default window id to report in WindowState
     */
    virtual void setWindowInstances(
        const std::vector<WindowInstance>& instances,
        const std::string& defaultWindowInstanceId) = 0;

    /**
     * Adds a window instance to be reported in WindowState
     * @param instance The window instance to add, the templateId, interactionMode and sizeConfigurationId must match
     * existing Ids which are reported by VisualCharacteristics, the window ID must be unique
     * @return true if the instance was successfully added, false otherwise
     */
    virtual bool addWindowInstance(const WindowInstance& instance) = 0;

    /**
     * Remove an existing window instance, at least one window must exist at all times
     * @param windowInstanceId The id of the window to remove
     * @return true if the instance was removed, false otherwise
     */
    virtual bool removeWindowInstance(const std::string& windowInstanceId) = 0;

    /**
     * Updates an already existing window instance
     * @param instace The updated window instance, the window ID must match an already existing window
     */
    virtual void updateWindowInstance(const WindowInstance& instance) = 0;

    /**
     * Sets the default window instance
     * @param windowInstanceId The id of window to set as the default, this window id must already exist
     * @return true if the default has been set, false otherwise
     */
    virtual bool setDefaultWindowInstance(const std::string& windowInstanceId) = 0;
};
}  // namespace visualCharacteristicsInterfaces
}  // namespace alexaClientSDK
#endif  // ACSDK_VISUALCHARACTERISTICSINTERFACES_VISUALCHARACTERISTICSINTERFACE_H_
