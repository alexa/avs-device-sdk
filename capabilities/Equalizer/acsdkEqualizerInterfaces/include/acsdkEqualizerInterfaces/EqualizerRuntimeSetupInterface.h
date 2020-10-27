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

#ifndef ACSDKEQUALIZERINTERFACES_EQUALIZERRUNTIMESETUPINTERFACE_H_
#define ACSDKEQUALIZERINTERFACES_EQUALIZERRUNTIMESETUPINTERFACE_H_

#include <list>
#include <memory>

#include <acsdkEqualizerInterfaces/EqualizerConfigurationInterface.h>
#include <acsdkEqualizerInterfaces/EqualizerModeControllerInterface.h>
#include <acsdkEqualizerInterfaces/EqualizerStorageInterface.h>
#include <acsdkEqualizerInterfaces/EqualizerInterface.h>
#include <acsdkEqualizerInterfaces/EqualizerControllerListenerInterface.h>

namespace alexaClientSDK {
namespace acsdkEqualizerInterfaces {

/**
 * Interface for equalizer runtime setup.
 */
class EqualizerRuntimeSetupInterface {
public:
    /**
     * Destructor.
     */
    virtual ~EqualizerRuntimeSetupInterface() = default;

    /**
     * Returns equalizer configuration instance.
     *
     * @return Equalizer configuration instance.
     */
    virtual std::shared_ptr<acsdkEqualizerInterfaces::EqualizerConfigurationInterface> getConfiguration() = 0;

    /**
     * Returns equalizer state storage instance.
     *
     * @return Equalizer state storage instance.
     */
    virtual std::shared_ptr<acsdkEqualizerInterfaces::EqualizerStorageInterface> getStorage() = 0;

    /**
     * Returns equalizer mode controller instance.
     *
     * @return Equalizer mode controller instance.
     */
    virtual std::shared_ptr<acsdkEqualizerInterfaces::EqualizerModeControllerInterface> getModeController() = 0;

    /**
     * Adds @c EqualizerInterface instance to be used by the SDK.
     *
     * @param equalizer @c EqualizerInterface instance to be used by the SDK.
     * @return Whether the equalizer was added.
     */
    virtual bool addEqualizer(std::shared_ptr<acsdkEqualizerInterfaces::EqualizerInterface> equalizer) = 0;

    /**
     * Adds @c EqualizerControllerListenerInterface instance to be used by the SDK.
     *
     * @param listener @c EqualizerControllerListenerInterface instance to be used by the SDK.
     * @return Whether the listener was added.
     */
    virtual bool addEqualizerControllerListener(
        std::shared_ptr<acsdkEqualizerInterfaces::EqualizerControllerListenerInterface> listener) = 0;

    /**
     * Returns a list of all equalizers that are going to be used by the SDK.
     *
     * @return List of all equalizers that are going to be used by the SDK.
     */
    virtual std::list<std::shared_ptr<acsdkEqualizerInterfaces::EqualizerInterface>> getAllEqualizers() = 0;

    /**
     * Returns a list of all equalizer controller listeners that are going to be used by the SDK.
     *
     * @return List of all equalizer controller listeners that are going to be used by the SDK.
     */
    virtual std::list<std::shared_ptr<acsdkEqualizerInterfaces::EqualizerControllerListenerInterface>>
    getAllEqualizerControllerListeners() = 0;

    /**
     * Whether the equalizer is enabled.
     *
     * @return Whether the equalizer is enabled.
     */
    virtual bool isEnabled() = 0;
};

}  // namespace acsdkEqualizerInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKEQUALIZERINTERFACES_EQUALIZERRUNTIMESETUPINTERFACE_H_
