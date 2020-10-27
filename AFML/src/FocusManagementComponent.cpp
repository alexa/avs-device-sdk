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

#include <acsdkManufactory/ComponentAccumulator.h>

#include "AFML/AudioActivityTracker.h"
#include "AFML/FocusManagementComponent.h"
#include "AFML/FocusManager.h"

namespace alexaClientSDK {
namespace afml {

using AudioFocusAnnotation = avsCommon::sdkInterfaces::AudioFocusAnnotation;

/// String to identify log entries originating from this file.
static const std::string TAG("FocusManagementComponent");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Key for audio channel array configurations in configuration node.
static const std::string AUDIO_CHANNEL_CONFIG_KEY = "audioChannels";

acsdkManufactory::Annotated<AudioFocusAnnotation, avsCommon::sdkInterfaces::FocusManagerInterface>
createAudioFocusManager(
    acsdkManufactory::Annotated<AudioFocusAnnotation, ActivityTrackerInterface> annotatedActivityTracker,
    std::shared_ptr<interruptModel::InterruptModel> interruptModel) {
    // Read audioChannels configuration from config file
    std::vector<afml::FocusManager::ChannelConfiguration> audioVirtualChannelConfiguration;
    if (!afml::FocusManager::ChannelConfiguration::readChannelConfiguration(
            AUDIO_CHANNEL_CONFIG_KEY, &audioVirtualChannelConfiguration)) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToReadAudioChannelConfiguration"));
        return nullptr;
    }

    std::shared_ptr<afml::ActivityTrackerInterface> activityTracker = annotatedActivityTracker;

    auto focusManager = std::make_shared<afml::FocusManager>(
        afml::FocusManager::getDefaultAudioChannels(),
        activityTracker,
        audioVirtualChannelConfiguration,
        interruptModel);

    return acsdkManufactory::Annotated<AudioFocusAnnotation, avsCommon::sdkInterfaces::FocusManagerInterface>(
        focusManager);
}

FocusManagementComponent getComponent() {
    return acsdkManufactory::ComponentAccumulator<>()
        .addRetainedFactory(AudioActivityTracker::createAudioActivityTrackerInterface)
        .addRequiredFactory(createAudioFocusManager);
}

}  // namespace afml
}  // namespace alexaClientSDK
