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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_INTERACTIONMANAGERHANDLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_INTERACTIONMANAGERHANDLERINTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

/// The initiatorType json key in the message.
static const char AUDIO_INPUT_INITIATOR_TYPE_TAG[] = "initiatorType";

/// The captureState json key in the message.
static const char CAPTURE_STATE_TAG[] = "captureState";

/// The event json key in the message.
static const char EVENT_TAG[] = "event";

/**
 * This contract handles the user interaction events coming from client.
 */
class InteractionManagerHandlerInterface {
public:
    /**
     * Destructor.
     */
    virtual ~InteractionManagerHandlerInterface() = default;

    /**
     * Open or close the microphone to start an Alexa interaction via the SpeechRecognizer.Recognize event.
     * @see https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/speechrecognizer.html#recognize
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void recognizeSpeechRequest(const std::string& message) = 0;

    /**
     * Handle event the navigation event.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void navigationEvent(const std::string& message) = 0;

    /**
     * Handle GUI activity event message.
     *
     * @param message Incoming IPC message from the IPC client.
     */
    virtual void guiActivityEvent(const std::string& message) = 0;
};

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_HANDLERINTERFACES_INTERACTIONMANAGERHANDLERINTERFACE_H_
