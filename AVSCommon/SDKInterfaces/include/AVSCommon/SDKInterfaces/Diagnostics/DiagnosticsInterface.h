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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_DIAGNOSTICS_DIAGNOSTICSINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_DIAGNOSTICS_DIAGNOSTICSINTERFACE_H_

#include <memory>

#include <AVSCommon/AVS/Attachment/AttachmentManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Diagnostics/AudioInjectorInterface.h>
#include <AVSCommon/SDKInterfaces/Diagnostics/DevicePropertyAggregatorInterface.h>
#include <AVSCommon/SDKInterfaces/Diagnostics/ProtocolTracerInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveSequencerInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace diagnostics {

/**
 * A @c DiagnosticsInterface which provides a suite of APIs to provide diagnostic insight into the SDK.
 */
class DiagnosticsInterface {
public:
    /// Destructor.
    virtual ~DiagnosticsInterface() = default;

    /**
     * An API to obtain state information about the SDK.
     *
     * @return The @c DevicePropertyAggregatorInterface. A nullptr will be return if unsupported.
     */
    virtual std::shared_ptr<DevicePropertyAggregatorInterface> getDevicePropertyAggregator() = 0;

    /**
     * An API to capture and store Directives and Events.
     *
     * @return The @c ProtocolTracerInterface. A nullptr will be return if unsupported.
     */
    virtual std::shared_ptr<ProtocolTracerInterface> getProtocolTracer() = 0;

    /**
     * An API to pass optional dependencies to the @c DiagnosticsInterface.
     *
     * @param sequencer An instance of the @c DirectiveSequencerInterface.
     * @param attachmentManager An instance of the @c AttachmentManager.
     */
    virtual void setDiagnosticDependencies(
        std::shared_ptr<DirectiveSequencerInterface> sequencer,
        std::shared_ptr<avs::attachment::AttachmentManagerInterface> attachmentManager) = 0;

    /**
     * An API to inject audio utterances into the SDK.
     *
     * @return The @c AudioInjectorInterface. A nullptr will be return if unsupported.
     */
    virtual std::shared_ptr<AudioInjectorInterface> getAudioInjector() = 0;
};

}  // namespace diagnostics
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_DIAGNOSTICS_DIAGNOSTICSINTERFACE_H_
