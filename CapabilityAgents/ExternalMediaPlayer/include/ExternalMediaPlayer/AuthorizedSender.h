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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_EXTERNALMEDIAPLAYER_INCLUDE_EXTERNALMEDIAPLAYER_AUTHORIZEDSENDER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_EXTERNALMEDIAPLAYER_INCLUDE_EXTERNALMEDIAPLAYER_AUTHORIZEDSENDER_H_

#include <mutex>
#include <string>
#include <unordered_set>

#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace externalMediaPlayer {

/**
 * If an adapter/player is not authorized, it is not allowed to send events or be mentioned
 * in the Context. This class parses the @c MessageRequest JSON and sends the message if
 * the sender has an authorized @c playerId. This also means that this class will block
 * events that do not have a @c playerId field in the payload. By default, no players
 * are authorized.
 */
class AuthorizedSender : public avsCommon::sdkInterfaces::MessageSenderInterface {
public:
    ~AuthorizedSender() override;

    /**
     * Creates an instance of the @c AuthorizedSender.
     *
     * @param messageSender Sends messages to the cloud.
     * @return A valid instance if creation was successful, else a nullptr.
     */
    static std::shared_ptr<AuthorizedSender> create(
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender);

    /// @name MessageSenderInterface Functions.
    /// @{
    void sendMessage(std::shared_ptr<avsCommon::avs::MessageRequest> request) override;
    /// @}

    /**
     * Update the list of authorized players that are allowed to send messages.
     *
     * @param playerIds The authorized players.
     */
    void updateAuthorizedPlayers(const std::unordered_set<std::string>& playerIds);

private:
    /**
     * Constructor.
     *
     * @param messageSender Sends messages to the cloud.
     * @return A constructed @c AuthorizedSender object.
     */
    AuthorizedSender(std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender);

    /// Mutex to protect @c m_playersIds.
    std::mutex m_updatePlayersMutex;

    /// Object to send messages.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /// Holds the authorized playerIds.
    std::unordered_set<std::string> m_authorizedPlayerIds;
};

}  // namespace externalMediaPlayer
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_EXTERNALMEDIAPLAYER_INCLUDE_EXTERNALMEDIAPLAYER_AUTHORIZEDSENDER_H_
