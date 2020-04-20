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

#include "SampleApp/SDKDiagnostics.h"

namespace alexaClientSDK {
namespace sampleApp {

using namespace alexaClientSDK::diagnostics;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::diagnostics;

/// String to identify log entries originating from this file.
static const std::string TAG("SDKDiagnostics");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<SDKDiagnostics> SDKDiagnostics::create() {
    ACSDK_DEBUG5(LX(__func__));

    std::shared_ptr<alexaClientSDK::diagnostics::DevicePropertyAggregator> deviceProperties;
    std::shared_ptr<alexaClientSDK::diagnostics::DeviceProtocolTracer> protocolTrace;
    std::shared_ptr<alexaClientSDK::diagnostics::FileBasedAudioInjector> audioInjector;

#ifdef DEVICE_PROPERTIES
    deviceProperties = alexaClientSDK::diagnostics::DevicePropertyAggregator::create();
    if (!deviceProperties) {
        ACSDK_ERROR(LX("Failed to create deviceProperties!"));
        return nullptr;
    }
#endif

#ifdef PROTOCOL_TRACE
    protocolTrace = alexaClientSDK::diagnostics::DeviceProtocolTracer::create();
    if (!protocolTrace) {
        ACSDK_ERROR(LX("Failed to create protocolTrace!"));
        return nullptr;
    }
#endif

#ifdef AUDIO_INJECTION
    audioInjector = std::make_shared<alexaClientSDK::diagnostics::FileBasedAudioInjector>();
    if (!audioInjector) {
        ACSDK_ERROR(LX("Failed to create audioInjector!"));
        return nullptr;
    }
#endif

    return std::unique_ptr<SDKDiagnostics>(new SDKDiagnostics(deviceProperties, protocolTrace, audioInjector));
}

SDKDiagnostics::SDKDiagnostics(
    std::shared_ptr<DevicePropertyAggregator> deviceProperties,
    std::shared_ptr<DeviceProtocolTracer> protocolTrace,
    std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::AudioInjectorInterface> audioInjector) :
        m_deviceProperties{deviceProperties},
        m_protocolTrace{protocolTrace},
        m_audioInjector{audioInjector} {
}

std::shared_ptr<DevicePropertyAggregatorInterface> SDKDiagnostics::getDevicePropertyAggregator() {
    ACSDK_DEBUG5(LX(__func__));

    return m_deviceProperties;
}

std::shared_ptr<ProtocolTracerInterface> SDKDiagnostics::getProtocolTracer() {
    ACSDK_DEBUG5(LX(__func__));

    return m_protocolTrace;
}

void SDKDiagnostics::setDiagnosticDependencies(
    std::shared_ptr<DirectiveSequencerInterface> sequencer,
    std::shared_ptr<AttachmentManagerInterface> attachmentManager) {
    ACSDK_DEBUG5(LX(__func__));
}

std::shared_ptr<AudioInjectorInterface> SDKDiagnostics::getAudioInjector() {
    ACSDK_DEBUG5(LX(__func__));
    return m_audioInjector;
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
