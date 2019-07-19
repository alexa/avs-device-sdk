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

#ifndef ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_PAIRINGAGENT_H_
#define ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_PAIRINGAGENT_H_

#include <memory>

#include <gio/gio.h>

#include "BlueZ/DBusObject.h"
#include "BlueZ/DBusProxy.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

/**
 * A DBus object to handle pairing requests from BlueZ.
 * This implementation by default automatically authorizes all pairing requests.
 */
class PairingAgent : public DBusObject<PairingAgent> {
public:
    /// Destructor.
    virtual ~PairingAgent();

    /**
     * Creates an instance of the PairingAgent.
     *
     * @param connection A DBusConnection object.
     * @return An instance of the PairingAgent if successful, else a nullptr.
     */
    static std::unique_ptr<PairingAgent> create(std::shared_ptr<DBusConnection> connection);

private:
    /// Constructor.
    PairingAgent(std::shared_ptr<DBusConnection> connection);

    /**
     * Performs initialization.
     *
     * @return Whether the operation was succesful or not.
     */
    bool init();

    /**
     * Register this PairingAgent with the AgentManager.
     *
     * @return Whether the operation was successful.
     */
    bool registerAgent();

    /**
     * Unregister this PairingAgent with the AgentManager.
     *
     * @return Whether the operation was successful.
     */
    bool unregisterAgent();

    /**
     * Sets this PairingAgent as the default one to handle pairing requests.
     *
     * @param agentManager A DBusProxy to an AgentManager interface.
     * @return Whether the operation was successful.
     */
    bool requestDefaultAgent();

    /**
     * These are part of the org.bluez.PairingAgent1 interface. They will be called
     * by BlueZ when a pairing request arrives. For use, please see
     * https://git.kernel.org/pub/scm/bluetooth/bluez.git/tree/doc/agent-api.txt
     */
    /// @name org.bluez.PairingAgent1 interfaces.
    /// @{
    void release(GVariant* arguments, GDBusMethodInvocation* invocation);
    void requestPinCode(GVariant* arguments, GDBusMethodInvocation* invocation);
    void displayPinCode(GVariant* arguments, GDBusMethodInvocation* invocation);
    void requestPasskey(GVariant* arguments, GDBusMethodInvocation* invocation);
    void displayPasskey(GVariant* arguments, GDBusMethodInvocation* invocation);
    void requestConfirmation(GVariant* arguments, GDBusMethodInvocation* invocation);
    void requestAuthorization(GVariant* arguments, GDBusMethodInvocation* invocation);
    void authorizeService(GVariant* arguments, GDBusMethodInvocation* invocation);
    void cancel(GVariant* arguments, GDBusMethodInvocation* invocation);
    /// @}

    /// A Proxy for the AgentManager.
    std::shared_ptr<DBusProxy> m_agentManager;
};

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_PAIRINGAGENT_H_
