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

#ifndef ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_EQUALIZERRUNTIMESETUP_H_
#define ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_EQUALIZERRUNTIMESETUP_H_

#include <AVSCommon/SDKInterfaces/Audio/EqualizerConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/Audio/EqualizerModeControllerInterface.h>
#include <AVSCommon/SDKInterfaces/Audio/EqualizerStorageInterface.h>
#include <AVSCommon/SDKInterfaces/Audio/EqualizerInterface.h>
#include <AVSCommon/SDKInterfaces/Audio/EqualizerControllerListenerInterface.h>

#include <list>
#include <memory>

namespace alexaClientSDK {
namespace defaultClient {

/**
 * Class containing references to implementations for all equalizer related interfaces.
 */
class EqualizerRuntimeSetup {
public:
    /**
     * Constructor.
     */
    EqualizerRuntimeSetup();

    /**
     * Set equalizer configuration instance.
     *
     * @param configuration Equalizer configuration instance.
     */
    void setConfiguration(
        std::shared_ptr<avsCommon::sdkInterfaces::audio::EqualizerConfigurationInterface> configuration);

    /**
     * Returns equalizer configuration instance.
     *
     * @return Equalizer configuration instance.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::audio::EqualizerConfigurationInterface> getConfiguration();

    /**
     * Set equalizer state storage instance.
     *
     * @param storage Equalizer state storage instance.
     */
    void setStorage(std::shared_ptr<avsCommon::sdkInterfaces::audio::EqualizerStorageInterface> storage);

    /**
     * Returns equalizer state storage instance.
     *
     * @return Equalizer state storage instance.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::audio::EqualizerStorageInterface> getStorage();

    /**
     * Set equalizer mode controller instance.
     *
     * @param modeController Equalizer mode controller instance.
     */
    void setModeController(
        std::shared_ptr<avsCommon::sdkInterfaces::audio::EqualizerModeControllerInterface> modeController);

    /**
     * Returns equalizer mode controller instance.
     *
     * @return Equalizer mode controller instance.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::audio::EqualizerModeControllerInterface> getModeController();

    /**
     * Adds @c EqualizerInterface instance to be used by the SDK.
     *
     * @param equalizer @c EqualizerInterface instance to be used by the SDK.
     */
    void addEqualizer(std::shared_ptr<avsCommon::sdkInterfaces::audio::EqualizerInterface> equalizer);

    /**
     * Adds @c EqualizerControllerListenerInterface instance to be used by the SDK.
     *
     * @param listener @c EqualizerControllerListenerInterface instance to be used by the SDK.
     */
    void addEqualizerControllerListener(
        std::shared_ptr<avsCommon::sdkInterfaces::audio::EqualizerControllerListenerInterface> listener);

    /**
     * Returns a list of all equalizers that are going to be used by the SDK.
     *
     * @return List of all equalizers that are going to be used by the SDK.
     */
    std::list<std::shared_ptr<avsCommon::sdkInterfaces::audio::EqualizerInterface>> getAllEqualizers();

    /**
     * Returns a list of all equalizer controller listeners that are going to be used by the SDK.
     *
     * @return List of all equalizer controller listeners that are going to be used by the SDK.
     */
    std::list<std::shared_ptr<avsCommon::sdkInterfaces::audio::EqualizerControllerListenerInterface>>
    getAllEqualizerControllerListeners();

private:
    /// Equalizer configuration instance.
    std::shared_ptr<avsCommon::sdkInterfaces::audio::EqualizerConfigurationInterface> m_configuration;

    /// Equalizer mode controller instance.
    std::shared_ptr<avsCommon::sdkInterfaces::audio::EqualizerModeControllerInterface> m_modeController;

    /// Equalizer state storage instance.
    std::shared_ptr<avsCommon::sdkInterfaces::audio::EqualizerStorageInterface> m_storage;

    /// List of equalizers to be used by the SDK.
    std::list<std::shared_ptr<avsCommon::sdkInterfaces::audio::EqualizerInterface>> m_equalizers;

    /// List of listeners to be subscribed to @c EqualizerController.
    std::list<std::shared_ptr<avsCommon::sdkInterfaces::audio::EqualizerControllerListenerInterface>>
        m_equalizerControllerListeners;
};

}  // namespace defaultClient
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_EQUALIZERRUNTIMESETUP_H_
