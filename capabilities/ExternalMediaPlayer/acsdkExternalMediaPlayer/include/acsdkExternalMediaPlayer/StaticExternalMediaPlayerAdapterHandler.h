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

#ifndef ALEXA_CLIENT_SDK_ACSDKEXTERNALMEDIAPLAYER_INCLUDE_ACSDKEXTERNALMEDIAPLAYER_STATICEXTERNALMEDIAPLAYERADAPTERHANDLER_H_
#define ALEXA_CLIENT_SDK_ACSDKEXTERNALMEDIAPLAYER_INCLUDE_ACSDKEXTERNALMEDIAPLAYER_STATICEXTERNALMEDIAPLAYERADAPTERHANDLER_H_

#include <unordered_map>

#include "acsdkExternalMediaPlayerInterfaces/ExternalMediaAdapterHandlerInterface.h"

namespace alexaClientSDK {
namespace acsdkExternalMediaPlayer {

class StaticExternalMediaPlayerAdapterHandler
        : public acsdkExternalMediaPlayerInterfaces::ExternalMediaAdapterHandlerInterface {
public:
    /// Constructor
    StaticExternalMediaPlayerAdapterHandler();

    /// Destructor
    ~StaticExternalMediaPlayerAdapterHandler() override = default;

    /**
     * Adds an adapter implementing the ExternalMediaAdapterInterface to the list of adapters and associates it with the
     * provided localPlayerId
     * @param localPlayerId the local player id
     * @param adapter An adapter implementing ExternalMediaAdapterInterface
     */
    void addAdapter(
        const std::string& localPlayerId,
        std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaAdapterInterface> adapter);

    /// alexaClientSDK::avsCommon::utils::RequiresShutdown
    virtual void doShutdown() override;

    /// @c ExternalMediaPlayerAdapterHandlerInterface functions
    /// @{
    std::vector<acsdkExternalMediaPlayerInterfaces::PlayerInfo> updatePlayerInfo(
        const std::vector<acsdkExternalMediaPlayerInterfaces::PlayerInfo>& playerList) override;
    bool login(
        const std::string& localPlayerId,
        const std::string& accessToken,
        const std::string& userName,
        bool forceLogin,
        std::chrono::milliseconds tokenRefreshInterval) override;
    bool logout(const std::string& localPlayerId) override;
    bool play(const PlayParams& params) override;
    bool playControl(
        const std::string& localPlayerId,
        acsdkExternalMediaPlayerInterfaces::RequestType requestType,
        const std::string& playbackTarget) override;
    bool seek(const std::string& localPlayerId, std::chrono::milliseconds offset) override;
    bool adjustSeek(const std::string& localPlayerId, std::chrono::milliseconds deltaOffset) override;
    acsdkExternalMediaPlayerInterfaces::AdapterState getAdapterState(const std::string& localPlayerId) override;
    std::vector<acsdkExternalMediaPlayerInterfaces::AdapterState> getAdapterStates() override;
    std::chrono::milliseconds getOffset(const std::string& localPlayerId) override;
    void setExternalMediaPlayer(const std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerInterface>
                                    externalMediaPlayer) override;
    /// @}

private:
    /**
     * Helper method to get an adapter by localPlayerId.
     *
     * @param localPlayerId The local player id associated with a player.
     *
     * @return An instance of the adapter if found, else a nullptr.
     */
    std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaAdapterInterface> getAdapterByLocalPlayerId(
        const std::string& localPlayerId);

    /// The @c m_adapters Map of @c localPlayerId (business names) to adapters.
    std::unordered_map<std::string, std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaAdapterInterface>>
        m_adapters;

    /// Mutex to serialize access to @c m_adapters.
    std::mutex m_adaptersMutex;
};

}  // namespace acsdkExternalMediaPlayer
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_ACSDKEXTERNALMEDIAPLAYER_INCLUDE_ACSDKEXTERNALMEDIAPLAYER_STATICEXTERNALMEDIAPLAYERADAPTERHANDLER_H_
