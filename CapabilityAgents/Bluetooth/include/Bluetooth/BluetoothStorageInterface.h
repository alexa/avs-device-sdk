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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_BLUETOOTH_INCLUDE_BLUETOOTH_BLUETOOTHSTORAGEINTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_BLUETOOTH_INCLUDE_BLUETOOTH_BLUETOOTHSTORAGEINTERFACE_H_

#include <unordered_map>
#include <list>

#include <SQLiteStorage/SQLiteDatabase.h>
#include <SQLiteStorage/SQLiteStatement.h>
#include <SQLiteStorage/SQLiteUtils.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace bluetooth {

/// A storage interface used for Bluetooth that should provide UUID to MAC mappings and maintain insertion order.
class BluetoothStorageInterface {
public:
    /// Destructor.
    virtual ~BluetoothStorageInterface() = default;

    /**
     * Create the database.
     *
     * @return A bool indicate success.
     */
    virtual bool createDatabase() = 0;

    /**
     * Open the database.
     *
     * @return A bool indicate success.
     */
    virtual bool open() = 0;

    /// Close the database.
    virtual void close() = 0;

    /**
     * Clear the database and remove all data.
     *
     * @return A bool indicate success.
     */
    virtual bool clear() = 0;

    /**
     * Retrieve the MAC associated with a UUID.
     *
     * @param uuid The UUID in which the associated mac will be retrieved.
     * @param[out] mac The MAC address of the associated UUID.
     * @return A bool indicating success.
     */
    virtual bool getMac(const std::string& uuid, std::string* mac) = 0;

    /**
     * Retrieve the UUID associated with a MAC.
     *
     * @param mac The MAC address of the associated UUID.
     * @param[out] uuid The UUID in which the associated mac will be retrieved.
     * @return A bool indicating success.
     */
    virtual bool getUuid(const std::string& mac, std::string* uuid) = 0;

    /**
     * Retrieve a map of MAC to UUID.
     *
     * @param[out] macToUuid A map of MAC to UUID mappings.
     * @return A bool indicating success.
     */
    virtual bool getMacToUuid(std::unordered_map<std::string, std::string>* macToUuid) = 0;

    /**
     * Retrieve a map of UUID to MAC.
     *
     * @param[out] uuidToMac A map of UUID to MAC mappings.
     * @return A bool indicating success.
     */
    virtual bool getUuidToMac(std::unordered_map<std::string, std::string>* uuidToMac) = 0;

    /**
     * Gets a list of MAC Addresses ordered by their insertion order into
     * the database.
     *
     * @param ascending Whether list is in ascending or descending order.
     * @param[out] macs The ordered macs.
     * @return A bool indicating success.
     */
    virtual bool getOrderedMac(bool ascending, std::list<std::string>* macs) = 0;

    /**
     * Insert into the database a MAC and UUID row. If an existing entry
     * has the same MAC address, the operation should fail unless overwrite is specified.
     *
     * @param mac The MAC address.
     * @param uuid The UUID.
     * @param overwrite Whether or not to overwrite an existing entry with the same MAC address.
     * @return A bool indicating success.
     */
    virtual bool insertByMac(const std::string& mac, const std::string& uuid, bool overwrite) = 0;

    /**
     * Remove the entry by the MAC address. The operation is considered successful if the entry
     * no longer exists after this call, including the case where the entry did not exist prior.
     *
     * @param mac The MAC address.
     * @return A bool indicating success.
     */
    virtual bool remove(const std::string& mac) = 0;
};

}  // namespace bluetooth
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_BLUETOOTH_INCLUDE_BLUETOOTH_BLUETOOTHSTORAGEINTERFACE_H_
