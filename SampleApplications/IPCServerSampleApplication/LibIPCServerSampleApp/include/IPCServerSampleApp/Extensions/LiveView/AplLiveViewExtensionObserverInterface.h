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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_EXTENSIONS_LIVEVIEW_APLLIVEVIEWEXTENSIONOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_EXTENSIONS_LIVEVIEW_APLLIVEVIEWEXTENSIONOBSERVERINTERFACE_H_

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace extensions {
namespace liveView {

/**
 * This class allows a @c AplLiveViewExtensionObserverInterface observer to be notified of changes in the
 * @c AplLiveViewExtension.
 * The observer should control the state of the corresponding LiveView.
 */
class AplLiveViewExtensionObserverInterface {
public:
    /**
     * Destructor
     */
    virtual ~AplLiveViewExtensionObserverInterface() = default;

    /**
     * Used to notify the observer when the extension has issued a request to exit the camera.
     * The observer should stop the active camera.
     */
    virtual void handleCameraExitRequest() = 0;

    /**
     * Used to notify the observer when the extension has issued a request to open the camera microphone.
     * The observer should open the microphone for communication over the camera.
     * @param enabled true if requesting to enable the microphone.
     */
    virtual void handleChangeCameraMicStateRequest(bool enabled) = 0;
};

}  // namespace liveView
}  // namespace extensions
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_EXTENSIONS_LIVEVIEW_APLLIVEVIEWEXTENSIONOBSERVERINTERFACE_H_
