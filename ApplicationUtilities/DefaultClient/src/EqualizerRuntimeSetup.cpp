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

#include <AVSCommon/Utils/Logger/Logger.h>
#include <DefaultClient/EqualizerRuntimeSetup.h>

#include "DefaultClient/EqualizerRuntimeSetup.h"

namespace alexaClientSDK {
namespace defaultClient {

using namespace alexaClientSDK::avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::audio;

/// String to identify log entries originating from this file.
static const std::string TAG("EqualizerRuntimeSetup");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

void EqualizerRuntimeSetup::setConfiguration(std::shared_ptr<EqualizerConfigurationInterface> configuration) {
    m_configuration = configuration;
}

std::shared_ptr<EqualizerConfigurationInterface> EqualizerRuntimeSetup::getConfiguration() {
    return m_configuration;
}

std::shared_ptr<avsCommon::sdkInterfaces::audio::EqualizerStorageInterface> EqualizerRuntimeSetup::getStorage() {
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

void EqualizerRuntimeSetup::addEqualizer(std::shared_ptr<EqualizerInterface> equalizer) {
    m_equalizers.push_back(equalizer);
}

std::list<std::shared_ptr<EqualizerInterface>> EqualizerRuntimeSetup::getAllEqualizers() {
    return m_equalizers;
}

void EqualizerRuntimeSetup::addEqualizerControllerListener(
    std::shared_ptr<EqualizerControllerListenerInterface> listener) {
    m_equalizerControllerListeners.push_back(listener);
}

std::list<std::shared_ptr<EqualizerControllerListenerInterface>> EqualizerRuntimeSetup::
    getAllEqualizerControllerListeners() {
    return m_equalizerControllerListeners;
}

EqualizerRuntimeSetup::EqualizerRuntimeSetup() {
}

}  // namespace defaultClient
}  // namespace alexaClientSDK
