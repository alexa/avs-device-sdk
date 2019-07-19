/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_SAMPLEEQUALIZERMODECONTROLLER_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_SAMPLEEQUALIZERMODECONTROLLER_H_

#include <list>
#include <memory>
#include <mutex>

#include <AVSCommon/SDKInterfaces/Audio/EqualizerModeControllerInterface.h>

namespace alexaClientSDK {
namespace sampleApp {

/**
 * Sample implementation of @c EqualizerModeControllerInterface that prints the mode requested to console.
 */
class SampleEqualizerModeController : public avsCommon::sdkInterfaces::audio::EqualizerModeControllerInterface {
public:
    /**
     * Factory method.
     *
     * @return An instance of @c SampleEqualizerModeController.
     */
    static std::shared_ptr<SampleEqualizerModeController> create();

    /// @name EqualizerModeControllerInterface methods
    /// @{

    bool setEqualizerMode(avsCommon::sdkInterfaces::audio::EqualizerMode mode) override;

    ///@}

private:
    /**
     * Constructor.
     */
    SampleEqualizerModeController() = default;
};

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_SAMPLEEQUALIZERMODECONTROLLER_H_
