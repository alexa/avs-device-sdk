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

#ifndef ACSDKAUTHORIZATION_AUTHORIZATIONMANAGERSTORAGE_H_
#define ACSDKAUTHORIZATION_AUTHORIZATIONMANAGERSTORAGE_H_

#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>

#include <memory>
#include <mutex>
#include <string>

namespace alexaClientSDK {
namespace acsdkAuthorization {

/**
 * An encapsulation of logic used to read and write the adapterId and userId, which
 * are used to identify the instance of the user that is currently logged in. This
 * reuses @c MiscStorageInterface.
 */
class AuthorizationManagerStorage {
public:
    /**
     * Create a storage interface.
     *
     * @param storage The underlying storage interface.
     */
    static std::shared_ptr<AuthorizationManagerStorage> create(
        const std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface>& storage);

    /**
     *  Stores the information into the database. This will overwrite any existing entries. are existing entries.
     *  Upon failure, the database may not be in
     *  a consistent state. Clearing is recommended.
     *
     *  @param adapterId The adapterId. This is required.
     *  @param userId The userId. This can be empty.
     *  @return Whether the data was successfully stored.
     */
    bool store(const std::string& adapterId, const std::string& userId);

    /**
     *  Loads information from the database.
     *
     *  @param[out] adapterId The adapterId.
     *  @param[out] userId The userId.
     *  @return Whether the data was successfully loaded.
     */
    bool load(std::string& adapterId, std::string& userId);

    /**
     * Clears the table. This will not delete the database.
     */
    void clear();

private:
    /**
     * Constructor.
     *
     * @param storage The underlying storage interface.
     */
    AuthorizationManagerStorage(
        const std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface>& storage);

    /**
     * Create and open the database and create the table.
     *
     * @return Returns true if at the end of the function the database and table
     * are ready to be written to.
     */
    bool initializeDatabase();

    /**
     * Opens the database and prepares it for reading.
     *
     * @param Whether the database successfully opened.
     */
    bool openLocked();

    /// Mutex
    std::mutex m_mutex;

    /// The @c MiscStorageInterface used to handle creation.
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> m_storage;
};

}  // namespace acsdkAuthorization
}  // namespace alexaClientSDK

#endif  // ACSDKAUTHORIZATION_AUTHORIZATIONMANAGERSTORAGE_H_
