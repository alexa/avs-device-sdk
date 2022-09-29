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

#ifndef ACSDK_AUDIOENCODERCOMPONENT_COMPONENTFACTORY_H_
#define ACSDK_AUDIOENCODERCOMPONENT_COMPONENTFACTORY_H_

#include <memory>

#include <acsdk/AudioEncoderInterfaces/AudioEncoderInterface.h>
#include <acsdkManufactory/Component.h>

namespace alexaClientSDK {
namespace audioEncoderComponent {

/**
 * Manufactory Component definition for an Opus @c AudioEncoderInterface.
 */
using AudioEncoderComponent =
    acsdkManufactory::Component<std::shared_ptr<audioEncoderInterfaces::AudioEncoderInterface>>;

/**
 * Get the Manufactory component for creating an Opus @c AudioEncoderInterface.
 *
 * @return The default @c Manufactory component for creating an Opus @c AudioEncoderInterface.
 */
AudioEncoderComponent getComponent();

}  // namespace audioEncoderComponent
}  // namespace alexaClientSDK

#endif  // ACSDK_AUDIOENCODERCOMPONENT_COMPONENTFACTORY_H_
