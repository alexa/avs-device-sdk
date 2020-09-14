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

#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGCONNECTIONOBSERVER_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGCONNECTIONOBSERVER_H_

#include <functional>
#include <memory>

#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>

namespace alexaClientSDK {
namespace settings {

/**
 * An implementation of @c ConnectionStatusObserverInterface used to proxy connection status notifications used by the
 * settings protocol.
 */
class SettingConnectionObserver
        : public avsCommon::sdkInterfaces::ConnectionStatusObserverInterface
        , public std::enable_shared_from_this<SettingConnectionObserver> {
public:
    /**
     * Callback function type used for notifying connections status changes.
     *
     * @param isConnected If true, indicates that the device is connected to AVS, otherwise false.
     */
    using ConnectionStatusCallback = std::function<void(bool isConnected)>;

    /**
     * Creates a @c SettingConnectionObserver shared object instance.
     *
     * @param notifyCallback The function callback to be called whenever the connection status changes.
     * @return A pointer to the new @c SettingConnectionObserver object if it succeeds; @c nullptr otherwise.
     */
    static std::shared_ptr<SettingConnectionObserver> create(ConnectionStatusCallback notifyCallback);

    /**
     * Destructor.
     */
    ~SettingConnectionObserver() = default;

    /// @name ConnectionStatusObserverInterface Functions
    /// @{
    void onConnectionStatusChanged(
        avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status status,
        avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason reason) override;
    /// @}

private:
    /**
     * Constructor.
     * @param notifyCallback The function callback to be called whenever the connection status changes.
     */
    SettingConnectionObserver(ConnectionStatusCallback notifyCallback);

    /// The function callback to be called whenever the connection status changes.
    ConnectionStatusCallback m_connectionStatusCallback;
};

}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGCONNECTIONOBSERVER_H_
