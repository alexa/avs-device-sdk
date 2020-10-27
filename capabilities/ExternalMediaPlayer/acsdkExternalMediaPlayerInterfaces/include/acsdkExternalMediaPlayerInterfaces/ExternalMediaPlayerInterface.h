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

#ifndef ACSDKEXTERNALMEDIAPLAYERINTERFACES_EXTERNALMEDIAPLAYERINTERFACE_H_
#define ACSDKEXTERNALMEDIAPLAYERINTERFACES_EXTERNALMEDIAPLAYERINTERFACE_H_

#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/PlaybackHandlerInterface.h>
#include <AVSCommon/SDKInterfaces/PlaybackRouterInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerInterface.h>

#include "acsdkExternalMediaPlayerInterfaces/ExternalMediaAdapterHandlerInterface.h"
#include "acsdkExternalMediaPlayerInterfaces/ExternalMediaPlayerCommon.h"
#include "acsdkExternalMediaPlayerInterfaces/ExternalMediaPlayerObserverInterface.h"

namespace alexaClientSDK {
namespace acsdkExternalMediaPlayerInterfaces {

/**
 * This class provides an interface to the @c ExternalMediaPlayer.
 * Currently it provides an interface for adapters to set the player in focus
 * when they acquire focus.
 */
class ExternalMediaPlayerInterface {
public:
    /**
     * Destructor
     */
    virtual ~ExternalMediaPlayerInterface() = default;

    /**
     * Method to set which player is currently active and which should be the target for playback control by the
     * ExternalMediaPlayer. This is not related to AFML focus.
     *
     * @param playerInFocus The business name of the adapter that has currently
     * acquired focus.
     * @note This function should not be called during the callback in @c
     * ExternalMediaAdapterInterface.
     */
    virtual void setPlayerInFocus(const std::string& playerInFocus) = 0;

    /**
     * Method used by External Media Player adapters to notify that a change has occurred to the discovered players
     *
     * @param addedPlayers The players that have been added
     * @param removedPlayers The players that have been removed
     */
    virtual void updateDiscoveredPlayers(
        const std::vector<DiscoveredPlayerInfo>& addedPlayers,
        const std::unordered_set<std::string>& removedLocalPlayerIds) = 0;

    /**
     * Adds a new @name ExternalMediaAdapterHandlerInterface to the list of handlers being managed by the External
     * Media Player Interface.
     *
     * @param adapterHandler The adapter handler to add
     */
    virtual void addAdapterHandler(
        std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaAdapterHandlerInterface> adapterHandler) = 0;

    /**
     * Removes an @name ExternalMediaAdapterHandlerInterface from the list of handlers being managed by the External
     * Media Player Interface
     *
     * @param adapterHandler The adapter handler to remove
     */
    virtual void removeAdapterHandler(
        std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaAdapterHandlerInterface> adapterHandler) = 0;

    /**
     * Adds an observer which will be notified on any observable state changes
     *
     * @param observer The observer to add
     */
    virtual void addObserver(
        const std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerObserverInterface> observer) = 0;

    /**
     * Removes an observer from the list of active watchers
     *
     *@param observer The observer to remove
     */
    virtual void removeObserver(
        const std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerObserverInterface> observer) = 0;
};

}  // namespace acsdkExternalMediaPlayerInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKEXTERNALMEDIAPLAYERINTERFACES_EXTERNALMEDIAPLAYERINTERFACE_H_
