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

#ifndef ACSDKBLUETOOTH_BLUETOOTHMEDIAINPUTTRANSFORMER_H_
#define ACSDKBLUETOOTH_BLUETOOTHMEDIAINPUTTRANSFORMER_H_

#include <memory>
#include <mutex>

#include <AVSCommon/SDKInterfaces/PlaybackRouterInterface.h>
#include <AVSCommon/Utils/Bluetooth/BluetoothEventBus.h>
#include <AVSCommon/Utils/Bluetooth/BluetoothEvents.h>

namespace alexaClientSDK {
namespace acsdkBluetooth {

/**
 * A class which will convert Media commands to the related @c PlaybackRouterInterface commands.
 */
class BluetoothMediaInputTransformer
        : public std::enable_shared_from_this<BluetoothMediaInputTransformer>
        , public avsCommon::utils::bluetooth::BluetoothEventListenerInterface {
public:
    /**
     * Creates an instance of the BluetoothMediaInputTransformer.
     *
     * @param eventBus The @c BluetoothEventBus in which @c MediaCommandReceivedEvent events will appear.
     * @param playbackRouter The @c PlaybackRouterInterface in which Media commands will be transformed to.
     *
     * @return An instance if successful else a nullptr.
     */
    static std::shared_ptr<BluetoothMediaInputTransformer> create(
        std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> eventBus,
        std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> playbackRouter);

protected:
    /// @name BluetoothEventBusListenerInterface Functions
    /// @{
    void onEventFired(const avsCommon::utils::bluetooth::BluetoothEvent& event) override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param eventBus The @c BluetoothEventBus in which @c MediaCommandReceivedEvent events will appear.
     * @param playbackRouter The @c PlaybackRouterInterface in which Media commands will be transformed to.
     */
    BluetoothMediaInputTransformer(
        std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> eventBus,
        std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> playbackRouter);

    /**
     * Initializes the component.
     *
     * @return Indicates the success of the operation.
     */
    bool init();

    /// The eventbus on which to listen for @c MediaCommandReceivedEvents.
    std::shared_ptr<avsCommon::utils::bluetooth::BluetoothEventBus> m_eventBus;

    /// Componenet responsible for executing the playback commands.
    std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface> m_playbackRouter;
};

}  // namespace acsdkBluetooth
}  // namespace alexaClientSDK

#endif  // ACSDKBLUETOOTH_BLUETOOTHMEDIAINPUTTRANSFORMER_H_
