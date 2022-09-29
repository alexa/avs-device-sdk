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
#ifndef ALEXA_CLIENT_SDK_ALEXALIVEVIEWCONTROLLERINTERFACES_INCLUDE_ACSDK_ALEXALIVEVIEWCONTROLLERINTERFACES_LIVEVIEWCONTROLLERCONFIGURATION_H_
#define ALEXA_CLIENT_SDK_ALEXALIVEVIEWCONTROLLERINTERFACES_INCLUDE_ACSDK_ALEXALIVEVIEWCONTROLLERINTERFACES_LIVEVIEWCONTROLLERCONFIGURATION_H_

#include <set>
#include <string>

#include "LiveViewControllerTypes.h"

namespace alexaClientSDK {
namespace alexaLiveViewControllerInterfaces {

/**
 * Configuration object used in the Discovery Response
 */
struct Configuration {
    /// Display modes supported.
    std::set<DisplayMode> supportedDisplayModes;

    /// Overlay types supported.
    std::set<OverlayType> supportedOverlayTypes;

    /// Overlay positions supported.
    std::set<OverlayPosition> supportedOverlayPositions;

    /**
     * Constructor.
     *
     * @param supportedDisplayModes The set of supported display modes.
     * @param supportedOverlayTypes The set of supported overlay types.
     * @param supportedOverlayPositions The set of supported overlay positions.
     */
    Configuration(
        const std::set<DisplayMode>& supportedDisplayModes = {DisplayMode::FULL_SCREEN},
        const std::set<OverlayType>& supportedOverlayTypes = {OverlayType::NONE},
        const std::set<OverlayPosition>& supportedOverlayPositions = {OverlayPosition::TOP_RIGHT}) :
            supportedDisplayModes{std::move(supportedDisplayModes)},
            supportedOverlayTypes{std::move(supportedOverlayTypes)},
            supportedOverlayPositions{std::move(supportedOverlayPositions)} {
    }
};

}  // namespace alexaLiveViewControllerInterfaces
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ALEXALIVEVIEWCONTROLLERINTERFACES_INCLUDE_ACSDK_ALEXALIVEVIEWCONTROLLERINTERFACES_LIVEVIEWCONTROLLERCONFIGURATION_H_
