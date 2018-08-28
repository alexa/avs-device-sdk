/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_PLAYBACKCONTROLLER_INCLUDE_PLAYBACKCONTROLLER_PLAYBACKMESSAGEREQUEST_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_PLAYBACKCONTROLLER_INCLUDE_PLAYBACKCONTROLLER_PLAYBACKMESSAGEREQUEST_H_

#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/SDKInterfaces/MessageRequestObserverInterface.h>

#include "PlaybackController.h"
#include "PlaybackCommand.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace playbackController {

/**
 * This class implements @c MessageRequests to alert observers upon completion of the message.
 */
class PlaybackMessageRequest : public avsCommon::avs::MessageRequest {
public:
    /**
     * @copyDoc avsCommon::avs::MessageRequest()
     *
     * Construct a @c MessageRequest while binding it to a @c PlaybackController and a @c Button.
     *
     * @param button The @c Button pressed.
     * @param jsonContent The JSON content to be sent to AVS.
     * @param playbackController A reference to a @c PlaybackController so that it can be notified when
     * @c onSendCompleted is invoked.
     */
    PlaybackMessageRequest(
        const PlaybackCommand& command,
        const std::string& jsonContent,
        std::shared_ptr<PlaybackController> playbackController);

    /// @name MessageRequest functions.
    /// @{
    void sendCompleted(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status status) override;
    /// @}

private:
    /// The @c PlaybackController to be notified when @c onSendCompleted is called.
    std::shared_ptr<PlaybackController> m_playbackController;

    /// The command associated with the @c Button pressed for this message request.
    const PlaybackCommand& m_command;
};

}  // namespace playbackController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_PLAYBACKCONTROLLER_INCLUDE_PLAYBACKCONTROLLER_PLAYBACKMESSAGEREQUEST_H_
