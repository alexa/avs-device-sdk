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

#ifndef ACSDKAUTHORIZATION_LWA_SQLITELWAAUTHORIZATIONSTORAGE_H_
#define ACSDKAUTHORIZATION_LWA_SQLITELWAAUTHORIZATIONSTORAGE_H_

#include <memory>
#include <mutex>
#include <string>

#include <sqlite3.h>

#include <SQLiteStorage/SQLiteDatabase.h>
#include <acsdkAuthorizationInterfaces/LWA/LWAAuthorizationStorageInterface.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>

namespace alexaClientSDK {
namespace acsdkAuthorization {
namespace lwa {

/// A SQLite based version of @c LWAAuthorizationStorageInterface.
class SQLiteLWAAuthorizationStorage : public acsdkAuthorizationInterfaces::lwa::LWAAuthorizationStorageInterface {
public:
    /**
     * Factory method for creating a storage object for SQLiteLWAAuthorizationStorage based on an SQLite database.
     *
     * @param configurationRoot The global config object.
     * @param storageRootKey The key to use to find the parent node.
     * @return Pointer to the SQLiteLWAAuthorizationStorage object, nullptr if there's an error creating it.
     */
    static std::shared_ptr<acsdkAuthorizationInterfaces::lwa::LWAAuthorizationStorageInterface>
    createLWAAuthorizationStorageInterface(
        const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationRoot,
        const std::string& storageRootKey = "");

    /**
     * Destructor.
     */
    ~SQLiteLWAAuthorizationStorage();

    /// @name LWAAuthorizationStorageInterface method overrides.
    /// @{
    bool createDatabase() override;
    bool open() override;
    bool openOrCreate() override;
    bool setRefreshToken(const std::string& refreshToken) override;
    bool clearRefreshToken() override;
    bool getRefreshToken(std::string* refreshToken) override;
    bool setUserId(const std::string& userId) override;
    bool getUserId(std::string* userId) override;
    bool clear() override;
    /// @}

private:
    /**
     * Clears the specified table. The std::m_mutex must be held.
     *
     * @param tableName The table.
     * @return Bool indicating success.
     */
    bool clearTableLocked(const std::string& tableName);

    /**
     * Constructor.
     */
    SQLiteLWAAuthorizationStorage(const std::string& databaseFilePath);

    /**
     * Close the database.
     */
    void close();

    /// Mutex with which to serialize database operations.
    std::mutex m_mutex;

    /// The underlying database class.
    alexaClientSDK::storage::sqliteStorage::SQLiteDatabase m_database;
};

}  // namespace lwa
}  // namespace acsdkAuthorization
}  // namespace alexaClientSDK

#endif  // ACSDKAUTHORIZATION_LWA_SQLITELWAAUTHORIZATIONSTORAGE_H_
