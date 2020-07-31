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

#ifndef ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_SDKCOMPONENT_INCLUDE_SDKCOMPONENT_SDKCOMPONENT_H_
#define ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_SDKCOMPONENT_INCLUDE_SDKCOMPONENT_SDKCOMPONENT_H_

#include <memory>

#include <AVSCommon/AVS/ComponentConfiguration.h>
#include <AVSCommon/SDKInterfaces/ComponentReporterInterface.h>

namespace alexaClientSDK {
namespace applicationUtilities {
namespace SDKComponent {

/**
 * Component Representing the configurations for the whole SDK.
 * Reports the Version of the SDK through @c ComponentReporterInterface
 */
class SDKComponent {
public:
    /**
     * Creates a new @c SDKComponent instance.
     *
     * @param ComponentReporterInterface The ComponentReporterInterface to register componentConfigurations to.
     *
     * @return A @c std::unique_ptr to the new @c SDKComponent instance or nullptr if invalid arguments.
     */
    static bool registerComponent(
        std::shared_ptr<avsCommon::sdkInterfaces::ComponentReporterInterface> componentReporter);
};

}  // namespace SDKComponent
}  // namespace applicationUtilities
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_SDKCOMPONENT_INCLUDE_SDKCOMPONENT_SDKCOMPONENT_H_
