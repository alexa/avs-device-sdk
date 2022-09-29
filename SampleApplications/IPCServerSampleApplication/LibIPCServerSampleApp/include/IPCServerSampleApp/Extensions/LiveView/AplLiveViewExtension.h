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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_EXTENSIONS_LIVEVIEW_APLLIVEVIEWEXTENSION_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_EXTENSIONS_LIVEVIEW_APLLIVEVIEWEXTENSION_H_

#include "AplLiveViewExtensionObserverInterface.h"

// Disable warnings from APLClient
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <APLClient/Extensions/AplCoreExtensionInterface.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace extensions {
namespace liveView {

using namespace APLClient::Extensions;

static const std::string URI = "aplext:liveview:10";

/**
 * An APL Extension designed for communication with a Camera @LiveView
 */
class AplLiveViewExtension : public AplCoreExtensionInterface {
public:
    /**
     * Constructor
     */
    explicit AplLiveViewExtension(std::shared_ptr<AplLiveViewExtensionObserverInterface> observer);

    ~AplLiveViewExtension() = default;

    /// @name AplCoreExtensionInterface Functions
    /// @{
    std::string getUri() override;

    apl::Object getEnvironment() override;

    std::list<apl::ExtensionCommandDefinition> getCommandDefinitions() override;

    std::list<apl::ExtensionEventHandler> getEventHandlers() override;

    std::unordered_map<std::string, apl::LiveObjectPtr> getLiveDataObjects() override;

    void applySettings(const apl::Object& settings) override;
    /// @}

    /// @name AplCoreExtensionEventCallbackInterface Functions
    /// @{
    void onExtensionEvent(
        const std::string& uri,
        const std::string& name,
        const apl::Object& source,
        const apl::Object& params,
        unsigned int event,
        std::shared_ptr<AplCoreExtensionEventCallbackResultInterface> resultCallback) override;
    /// @}

    /**
     * Informs the APL document of changes in camera state
     * @param cameraState enumerated @CameraState
     */
    void setCameraState(const std::string& cameraState);

    /**
     * Informs the APL document of changes in camera microphone state.
     * @param enabled true if camera microphone is enabled and unmuted.
     */
    void setCameraMicrophoneState(bool enabled);

    /**
     * Informs the APL document of the current ASR Profile for the device relative to audio input.
     * Used to determine UI state and display characteristics.
     * See: https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/audio-hardware-configurations.html
     * @param asrProfile the current asrProfile for the device.
     */
    void setAsrProfile(const std::string& asrProfile);

    /**
     * Informs the APL document that the first frame of the camera has rendered.
     */
    void onCameraFirstFrameRendered();

    /**
     * Informs the APL document that the camera has been cleared and is no longer displayed.
     */
    void onCameraCleared();

private:
    /// The @c AplLiveViewExtensionObserverInterface observer
    std::shared_ptr<AplLiveViewExtensionObserverInterface> m_observer;

    /// The document settings defined 'name' for the cameraState data object
    std::string m_cameraStateName;

    /// The @c apl::LiveMap for LiveView cameraState data.
    apl::LiveMapPtr m_cameraState;
};

using AplLiveViewExtensionPtr = std::shared_ptr<AplLiveViewExtension>;

}  // namespace liveView
}  // namespace extensions
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_EXTENSIONS_LIVEVIEW_APLLIVEVIEWEXTENSION_H_
