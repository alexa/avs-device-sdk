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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_INITIALIZATION_INITIALIZATIONPARAMETERS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_INITIALIZATION_INITIALIZATIONPARAMETERS_H_

#include <iostream>
#include <memory>
#include <vector>

#include <AVSCommon/SDKInterfaces/PowerResourceManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Timing/TimerDelegateFactoryInterface.h>
#include <AVSCommon/Utils/Timing/TimerDelegateFactory.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace initialization {

/**
 * A struct to contain the various parameters that are needed to initialize @c AlexaClientSDKInit.
 */
struct InitializationParameters {
    /**
     * Vector of @c istreams containing JSON documents from which
     * to parse configuration parameters. Streams are processed in the order they appear in the vector. When a
     * value appears in more than one JSON stream the last processed stream's value overwrites the previous value
     * (and a debug log entry will be created). This allows for specifying default settings (by providing them
     * first) and specifying the configuration from multiple sources (e.g. a separate stream for each component).
     * Documentation of the JSON configuration format and methods to access the resulting global configuration
     * can be found here: avsCommon::utils::configuration::ConfigurationNode.
     */
    std::shared_ptr<std::vector<std::shared_ptr<std::istream>>> jsonStreams;

    /// The @c PowerResourceManagerInterface. This will be used for power management.
    std::shared_ptr<sdkInterfaces::PowerResourceManagerInterface> powerResourceManager;

    /// The @c TimerDelegateFactoryInterface. This will be used to inject custom implementations of @c
    /// TimerDelegateInterface.
    std::shared_ptr<sdkInterfaces::timing::TimerDelegateFactoryInterface> timerDelegateFactory;
};

}  // namespace initialization
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_INITIALIZATION_INITIALIZATIONPARAMETERS_H_
