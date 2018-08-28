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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_AUTHORIZATION_CBLAUTHDELEGATE_INCLUDE_CBLAUTHDELEGATE_SQLITECBLAUTHDELEGATESTORAGE_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_AUTHORIZATION_CBLAUTHDELEGATE_INCLUDE_CBLAUTHDELEGATE_SQLITECBLAUTHDELEGATESTORAGE_H_

#include <mutex>
#include <string>

#include <sqlite3.h>

#include <SQLiteStorage/SQLiteDatabase.h>

#include "CBLAuthDelegate/CBLAuthDelegateStorageInterface.h"

namespace alexaClientSDK {
namespace authorization {
namespace cblAuthDelegate {

/**
 * An implementation of CBLAuthDelegateStorageInterface using SQLite.
 */
class SQLiteCBLAuthDelegateStorage : public CBLAuthDelegateStorageInterface {
public:
    /**
     * Factory method for creating a storage object for CBLAuthDelegate based on an SQLite database.
     *
     * @param configurationRoot The global config object.
     * @return Pointer to the SQLiteCBLAuthDelegate object, nullptr if there's an error creating it.
     */
    static std::unique_ptr<SQLiteCBLAuthDelegateStorage> create(
        const avsCommon::utils::configuration::ConfigurationNode& configurationRoot);

    /**
     * Destructor
     */
    ~SQLiteCBLAuthDelegateStorage();

    /// @name CBLAuthDelegateStorageInterface method overrides.
    /// @{
    bool createDatabase() override;
    bool open() override;
    bool setRefreshToken(const std::string& refreshToken) override;
    bool clearRefreshToken() override;
    bool getRefreshToken(std::string* refreshToken) override;
    bool clear() override;
    /// @}

private:
    /**
     * Constructor.
     */
    SQLiteCBLAuthDelegateStorage(const std::string& databaseFilePath);

    /**
     * Close the database.
     */
    void close();

    /// Mutex with which to serialize database operations.
    std::mutex m_mutex;

    /// The underlying database class.
    alexaClientSDK::storage::sqliteStorage::SQLiteDatabase m_database;
};

}  // namespace cblAuthDelegate
}  // namespace authorization
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_AUTHORIZATION_CBLAUTHDELEGATE_INCLUDE_CBLAUTHDELEGATE_SQLITECBLAUTHDELEGATESTORAGE_H_
