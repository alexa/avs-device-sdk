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

#ifndef ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_EQUALIZERRUNTIMESETUP_H_
#define ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_EQUALIZERRUNTIMESETUP_H_

#include <acsdkEqualizerInterfaces/EqualizerConfigurationInterface.h>
#include <acsdkEqualizerInterfaces/EqualizerModeControllerInterface.h>
#include <acsdkEqualizerInterfaces/EqualizerStorageInterface.h>
#include <acsdkEqualizerInterfaces/EqualizerInterface.h>
#include <acsdkEqualizerInterfaces/EqualizerControllerListenerInterface.h>
#include <acsdkEqualizerInterfaces/EqualizerRuntimeSetupInterface.h>

#include <list>
#include <memory>

namespace alexaClientSDK {
namespace defaultClient {

/**
 * Class containing references to implementations for all equalizer related interfaces.
 */
class EqualizerRuntimeSetup : public acsdkEqualizerInterfaces::EqualizerRuntimeSetupInterface {
public:
    /**
     * Factory method to create an instance of @c EqualizerRuntimeSetupInterface.
     *
     * @param equalizerConfiguration Equalizer configuration instance.
     * @param equalizerStorage Equalizer storage instance.
     * @param equalizerModeController Equalizer mode controller instance.
     * @return An enabled @c EqualizerRuntimeSetup if equalizer is enabled in the configuration instance; otherwise,
     * a disabled @c EqualizerRuntimeSetupInterface.
     */
    static std::shared_ptr<EqualizerRuntimeSetupInterface> createEqualizerRuntimeSetupInterface(
        const std::shared_ptr<acsdkEqualizerInterfaces::EqualizerConfigurationInterface>& equalizerConfiguration,
        const std::shared_ptr<acsdkEqualizerInterfaces::EqualizerStorageInterface>& equalizerStorage,
        const std::shared_ptr<acsdkEqualizerInterfaces::EqualizerModeControllerInterface>& equalizerModeController);

    /**
     * Constructor.
     *
     * @param isEnabled Whether equalizer is enabled; true by default.
     */
    EqualizerRuntimeSetup(bool isEnabled = true);

    /// @name EqualizerRuntimeSetupInterface functions
    /// @{
    std::shared_ptr<acsdkEqualizerInterfaces::EqualizerConfigurationInterface> getConfiguration() override;

    std::shared_ptr<acsdkEqualizerInterfaces::EqualizerStorageInterface> getStorage() override;

    std::shared_ptr<acsdkEqualizerInterfaces::EqualizerModeControllerInterface> getModeController() override;

    bool addEqualizer(std::shared_ptr<acsdkEqualizerInterfaces::EqualizerInterface> equalizer) override;

    bool addEqualizerControllerListener(
        std::shared_ptr<acsdkEqualizerInterfaces::EqualizerControllerListenerInterface> listener) override;

    std::list<std::shared_ptr<acsdkEqualizerInterfaces::EqualizerInterface>> getAllEqualizers() override;

    std::list<std::shared_ptr<acsdkEqualizerInterfaces::EqualizerControllerListenerInterface>>
    getAllEqualizerControllerListeners() override;

    bool isEnabled() override;
    ///@}

    /**
     * Set equalizer configuration instance.
     *
     * @param configuration Equalizer configuration instance.
     */
    void setConfiguration(std::shared_ptr<acsdkEqualizerInterfaces::EqualizerConfigurationInterface> configuration);

    /**
     * Set equalizer state storage instance.
     *
     * @param storage Equalizer state storage instance.
     */
    void setStorage(std::shared_ptr<acsdkEqualizerInterfaces::EqualizerStorageInterface> storage);

    /**
     * Set equalizer mode controller instance.
     *
     * @param modeController Equalizer mode controller instance.
     */
    void setModeController(std::shared_ptr<acsdkEqualizerInterfaces::EqualizerModeControllerInterface> modeController);

private:
    /// Equalizer configuration instance.
    std::shared_ptr<acsdkEqualizerInterfaces::EqualizerConfigurationInterface> m_configuration;

    /// Equalizer mode controller instance.
    std::shared_ptr<acsdkEqualizerInterfaces::EqualizerModeControllerInterface> m_modeController;

    /// Equalizer state storage instance.
    std::shared_ptr<acsdkEqualizerInterfaces::EqualizerStorageInterface> m_storage;

    /// List of equalizers to be used by the SDK.
    std::list<std::shared_ptr<acsdkEqualizerInterfaces::EqualizerInterface>> m_equalizers;

    /// List of listeners to be subscribed to @c EqualizerController.
    std::list<std::shared_ptr<acsdkEqualizerInterfaces::EqualizerControllerListenerInterface>>
        m_equalizerControllerListeners;

    /// Whether the equalizer is enabled.
    bool m_isEnabled;
};

}  // namespace defaultClient
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_EQUALIZERRUNTIMESETUP_H_
