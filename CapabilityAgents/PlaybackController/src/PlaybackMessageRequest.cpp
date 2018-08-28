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

#include "PlaybackController/PlaybackMessageRequest.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace playbackController {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;

PlaybackMessageRequest::PlaybackMessageRequest(
    const PlaybackCommand& command,
    const std::string& jsonContent,
    std::shared_ptr<PlaybackController> playbackController) :
        MessageRequest(jsonContent),
        m_playbackController{playbackController},
        m_command(command) {
}

void PlaybackMessageRequest::sendCompleted(MessageRequestObserverInterface::Status status) {
    m_playbackController->messageSent(m_command, status);
}

}  // namespace playbackController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
