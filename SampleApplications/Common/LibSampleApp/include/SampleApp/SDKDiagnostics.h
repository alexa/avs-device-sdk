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
#ifndef SAMPLEAPP_SDKDIAGNOSTICS_H_
#define SAMPLEAPP_SDKDIAGNOSTICS_H_

#include <memory>

#include <AVSCommon/SDKInterfaces/Diagnostics/AudioInjectorInterface.h>
#include <AVSCommon/SDKInterfaces/Diagnostics/DiagnosticsInterface.h>
#include <Diagnostics/DevicePropertyAggregator.h>
#include <Diagnostics/DeviceProtocolTracer.h>
#include <Diagnostics/FileBasedAudioInjector.h>

namespace alexaClientSDK {
namespace sampleApp {

/**
 * An SDK implementation which implements the @c DiagnosticsInterface APIs.
 * This class depends on the underlying Diagnostic API objects for thread-safety.
 */
class SDKDiagnostics : public avsCommon::sdkInterfaces::diagnostics::DiagnosticsInterface {
public:
    /// @name Overridden DiagnosticsInterface methods.
    /// @{
    std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::DevicePropertyAggregatorInterface>
    getDevicePropertyAggregator() override;
    std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::ProtocolTracerInterface> getProtocolTracer() override;
    void setDiagnosticDependencies(
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> sequencer,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentManagerInterface> attachmentManager,
        std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> connectionManager) override;
    std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::AudioInjectorInterface> getAudioInjector() override;
    /// @}

    /**
     * Creates the @c SDKDiagnostics object.
     * @return An instance of the @c SDKDiagnostics object or a nullptr.
     */
    static std::unique_ptr<SDKDiagnostics> create();

private:
    /**
     * Constructor.
     *
     * @param deviceProperties The object implementing the @c DevicePropertyAggregatorInterface
     * to obtain deviceProperties from the SDK.
     * @param protocolTrace The object implementing the @c ProtocolTracerInterface to
     * capture directives and events.
     * @param audioInjector The object implementing the @c AudioInjectorInterface to inject audio into the SDK.
     */
    SDKDiagnostics(
        std::shared_ptr<diagnostics::DevicePropertyAggregator> deviceProperties,
        std::shared_ptr<diagnostics::DeviceProtocolTracer> protocolTrace,
        std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::AudioInjectorInterface> audioInjector);

    /// The object for obtaining device properties.
    std::shared_ptr<diagnostics::DevicePropertyAggregator> m_deviceProperties;

    /// The object for capturing directives and events.
    std::shared_ptr<diagnostics::DeviceProtocolTracer> m_protocolTrace;

    /// The object for injecting audio.
    std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::AudioInjectorInterface> m_audioInjector;
};

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // SAMPLEAPP_SDKDIAGNOSTICS_H_
