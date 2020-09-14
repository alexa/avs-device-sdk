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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_RENDERPLAYERINFOCARDSOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_RENDERPLAYERINFOCARDSOBSERVERINTERFACE_H_

#include <chrono>
#include <memory>
#include <string>

#include <AVSCommon/AVS/PlayerActivity.h>
#include <AVSCommon/SDKInterfaces/MediaPropertiesInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This class allows any @c RenderPlayerInfoCardsObserverInterface observers to be notified of changes in the @c Context
 * of RenderPlayerInfoCards.
 */
class RenderPlayerInfoCardsObserverInterface {
public:
    /**
     * Destructor
     */
    virtual ~RenderPlayerInfoCardsObserverInterface() = default;

    /// The context of the RenderPlayerInfoCards from the RenderPlayerInfoCards provider.
    struct Context {
        /// The @c AudioItem ID the RenderPlayerInfoCards provider is handling.
        std::string audioItemId;

        /// The offset in millisecond from the start of the @c AudioItem.
        std::chrono::milliseconds offset;

        /**
         * The @c MediaPropertiesInterface so that the observer can retrieve the live offset.
         *
         * @note Observer shall not call methods of the interface during the onRenderPlayerCardsInfoChanged callback.
         */
        std::shared_ptr<MediaPropertiesInterface> mediaProperties;
    };

    /**
     * Used to notify the observer when there is a change in @c PlayerActivity or @c Context.
     *
     * @param state The @c PlayerActivity of the Player.
     * @param context The @c Context of the Player.
     */
    virtual void onRenderPlayerCardsInfoChanged(avsCommon::avs::PlayerActivity state, const Context& context) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_RENDERPLAYERINFOCARDSOBSERVERINTERFACE_H_
