/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_BLUETOOTHEVENTBUS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_BLUETOOTHEVENTBUS_H_

#include <memory>
#include <mutex>
#include <unordered_map>
#include <list>
#include <string>
#include <vector>

#include "AVSCommon/Utils/Bluetooth/BluetoothEvents.h"
#include "AVSCommon/Utils/Bluetooth/BluetoothEventListenerInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace bluetooth {

/**
 * Event bus class for Bluetooth CA. Publishes Bluetooth events to all listeners.
 */
class BluetoothEventBus {
public:
    /**
     * Constructor
     */
    BluetoothEventBus();

    /**
     * A type representing a collection of @c EventListener objects.
     */
    using ListenerList = std::list<std::weak_ptr<BluetoothEventListenerInterface>>;

    /**
     * Send the event to @c EventBus. Method blocks until all the listeners process the event. The method is thread
     * safe.
     *
     * @param event Event to be sent to @c EventBus
     */
    void sendEvent(const BluetoothEvent& event);

    /**
     * Adds a listener to the bus.
     *
     * @param eventTypes A list of event types to subscribe listener to.
     * @param listener An @c EventListener<EventT> object to add as a listener. Listener cannot be registered multiple
     * times for the same BluetoothEventType.
     */
    void addListener(
        const std::vector<BluetoothEventType>& eventTypes,
        std::shared_ptr<BluetoothEventListenerInterface> listener);

    /**
     * Removes a listener from the @c EventBus.
     *
     * @param eventTypes A list of event types to unsubscribe listener from.
     * @param listener Listener object to unsubscribe.
     */
    void removeListener(
        const std::vector<BluetoothEventType>& eventTypes,
        std::shared_ptr<BluetoothEventListenerInterface> listener);

private:
    /// Mutex used to synchronize access to subscribed listener list
    std::mutex m_mutex;

    /**
     *  A collection of @c EventListener<EventT> objects grouped by event type id. Each listener may be subscribed
     *  to any number of event types.
     */
    std::unordered_map<BluetoothEventType, ListenerList, BluetoothEventTypeHash> m_listenerMap;
};

}  // namespace bluetooth
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_BLUETOOTH_BLUETOOTHEVENTBUS_H_
