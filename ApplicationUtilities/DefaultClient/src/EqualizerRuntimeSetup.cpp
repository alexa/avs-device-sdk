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

#include <AVSCommon/Utils/Logger/Logger.h>
#include <DefaultClient/EqualizerRuntimeSetup.h>

#include "DefaultClient/EqualizerRuntimeSetup.h"

namespace alexaClientSDK {
namespace defaultClient {

using namespace alexaClientSDK::acsdkEqualizerInterfaces;

/// String to identify log entries originating from this file.
#define TAG "EqualizerRuntimeSetup"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<EqualizerRuntimeSetupInterface> EqualizerRuntimeSetup::createEqualizerRuntimeSetupInterface(
    const std::shared_ptr<acsdkEqualizerInterfaces::EqualizerConfigurationInterface>& equalizerConfiguration,
    const std::shared_ptr<acsdkEqualizerInterfaces::EqualizerStorageInterface>& equalizerStorage,
    const std::shared_ptr<acsdkEqualizerInterfaces::EqualizerModeControllerInterface>& equalizerModeController) {
    if (equalizerConfiguration && equalizerConfiguration->isEnabled()) {
        auto equalizerRuntimeSetup = std::make_shared<defaultClient::EqualizerRuntimeSetup>();
        equalizerRuntimeSetup->setStorage(equalizerStorage);
        equalizerRuntimeSetup->setConfiguration(equalizerConfiguration);
        equalizerRuntimeSetup->setModeController(equalizerModeController);

        return equalizerRuntimeSetup;
    }

    return std::make_shared<defaultClient::EqualizerRuntimeSetup>(false);
}

void EqualizerRuntimeSetup::setConfiguration(std::shared_ptr<EqualizerConfigurationInterface> configuration) {
    m_configuration = configuration;
}

std::shared_ptr<EqualizerConfigurationInterface> EqualizerRuntimeSetup::getConfiguration() {
    return m_configuration;
}

std::shared_ptr<EqualizerStorageInterface> EqualizerRuntimeSetup::getStorage() {
    return m_storage;
}

void EqualizerRuntimeSetup::setStorage(std::shared_ptr<EqualizerStorageInterface> storage) {
    m_storage = storage;
}

void EqualizerRuntimeSetup::setModeController(std::shared_ptr<EqualizerModeControllerInterface> modeController) {
    m_modeController = modeController;
}

std::shared_ptr<EqualizerModeControllerInterface> EqualizerRuntimeSetup::getModeController() {
    return m_modeController;
}

bool EqualizerRuntimeSetup::addEqualizer(std::shared_ptr<EqualizerInterface> equalizer) {
    if (!isEnabled()) {
        ACSDK_ERROR(LX("addEqualizerFailed").m("Equalizer not enabled"));
        return false;
    }
    m_equalizers.push_back(equalizer);
    return true;
}

std::list<std::shared_ptr<EqualizerInterface>> EqualizerRuntimeSetup::getAllEqualizers() {
    return m_equalizers;
}

bool EqualizerRuntimeSetup::addEqualizerControllerListener(
    std::shared_ptr<EqualizerControllerListenerInterface> listener) {
    if (!isEnabled()) {
        ACSDK_ERROR(LX("addEqualizerControllerListenerFailed").m("Equalizer not enabled"));
        return false;
    }
    m_equalizerControllerListeners.push_back(listener);
    return true;
}

std::list<std::shared_ptr<EqualizerControllerListenerInterface>> EqualizerRuntimeSetup::
    getAllEqualizerControllerListeners() {
    return m_equalizerControllerListeners;
}

bool EqualizerRuntimeSetup::isEnabled() {
    return m_isEnabled;
}

EqualizerRuntimeSetup::EqualizerRuntimeSetup(bool isEnabled) {
    m_isEnabled = isEnabled;
}

}  // namespace defaultClient
}  // namespace alexaClientSDK
