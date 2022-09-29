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
#ifndef ALEXA_CLIENT_SDK_ALEXALIVEVIEWCONTROLLERINTERFACES_INCLUDE_ACSDK_ALEXALIVEVIEWCONTROLLERINTERFACES_LIVEVIEWCONTROLLEROBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_ALEXALIVEVIEWCONTROLLERINTERFACES_INCLUDE_ACSDK_ALEXALIVEVIEWCONTROLLERINTERFACES_LIVEVIEWCONTROLLEROBSERVERINTERFACE_H_

#include "LiveViewControllerTypes.h"

namespace alexaClientSDK {
namespace alexaLiveViewControllerInterfaces {
/**
 * This interface is used to observe changes in the application related to live view camera.
 */
class LiveViewControllerObserverInterface {
public:
    /**
     * Destructor.
     */
    virtual ~LiveViewControllerObserverInterface() = default;

    /**
     * Notifies the change in the microphone state of the application.
     *
     * @param microphoneState Audio state of the microphone.
     */
    virtual void onMicrophoneStateChanged(alexaLiveViewControllerInterfaces::AudioState microphoneState) = 0;

    /**
     * Notifies that application has cleared the live view camera.
     */
    virtual void onLiveViewCleared() = 0;
};
}  // namespace alexaLiveViewControllerInterfaces
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ALEXALIVEVIEWCONTROLLERINTERFACES_INCLUDE_ACSDK_ALEXALIVEVIEWCONTROLLERINTERFACES_LIVEVIEWCONTROLLEROBSERVERINTERFACE_H_
