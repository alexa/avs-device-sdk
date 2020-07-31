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

#include <AVSCommon/AVS/ComponentConfiguration.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/SDKVersion.h>

#include "SDKComponent/SDKComponent.h"

namespace alexaClientSDK {
namespace applicationUtilities {
namespace SDKComponent {

using namespace avsCommon::utils::logger;

// Name of the Component to add to ComponentReporter
static const std::string SDK_COMPONENT_NAME = "com.amazon.sdk";

/// String to identify log entries originating from this file.
static const std::string TAG("SDKComponent");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * Helper function to get the SDK Component Configuration.
 *
 * @return shared_ptr to ComponentConfiguration of the SDK.
 */
static std::shared_ptr<avsCommon::avs::ComponentConfiguration> getSDKConfig() {
    return avsCommon::avs::ComponentConfiguration::createComponentConfiguration(
        SDK_COMPONENT_NAME, avsCommon::utils::sdkVersion::getCurrentVersion());
}

bool SDKComponent::registerComponent(
    std::shared_ptr<avsCommon::sdkInterfaces::ComponentReporterInterface> componentReporter) {
    if (!componentReporter) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullcomponentReporter"));
        return false;
    }

    if (!componentReporter->addConfiguration(getSDKConfig())) {
        ACSDK_ERROR(LX("addConfigurationFailed"));
        return false;
    };

    return true;
}

}  // namespace SDKComponent
}  // namespace applicationUtilities
}  // namespace alexaClientSDK
