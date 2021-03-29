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

#ifndef ALEXA_CLIENT_SDK_CERTIFIEDSENDER_INCLUDE_CERTIFIEDSENDER_SQLITEMESSAGESTORAGE_H_
#define ALEXA_CLIENT_SDK_CERTIFIEDSENDER_INCLUDE_CERTIFIEDSENDER_SQLITEMESSAGESTORAGE_H_

#include "CertifiedSender/MessageStorageInterface.h"

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <SQLiteStorage/SQLiteDatabase.h>

namespace alexaClientSDK {
namespace certifiedSender {

/**
 * An implementation that allows us to store messages using SQLite.
 *
 * This class is not thread-safe.
 */
class SQLiteMessageStorage : public MessageStorageInterface {
public:
    /**
     * Factory method for creating a storage object for Messages based on an SQLite database.
     *
     * @param configurationRoot The global config object.
     * @return Pointer to the SQLiteMessagetStorge object, nullptr if there's an error creating it.
     */
    static std::shared_ptr<MessageStorageInterface> createMessageStorageInterface(
        const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationRoot);

    /**
     * Factory method for creating a storage object for Messages based on an SQLite database.
     *
     * @param configurationRoot The global config object.
     * @return Pointer to the SQLiteMessagetStorge object, nullptr if there's an error creating it.
     */
    static std::unique_ptr<SQLiteMessageStorage> create(
        const avsCommon::utils::configuration::ConfigurationNode& configurationRoot);

    /**
     * Constructor.
     *
     * @param dbFilePath The location of the SQLite database file.
     */
    SQLiteMessageStorage(const std::string& databaseFilePath);

    ~SQLiteMessageStorage();

    bool createDatabase() override;

    bool open() override;

    void close() override;

    bool store(const std::string& message, int* id) override;

    bool store(const std::string& message, const std::string& uriPathExtension, int* id) override;

    bool load(std::queue<StoredMessage>* messageContainer) override;

    bool erase(int messageId) override;

    bool clearDatabase() override;

private:
    /// The underlying database class.
    alexaClientSDK::storage::sqliteStorage::SQLiteDatabase m_database;

    /**
     * Utility that checks if database is legacy.
     *
     * @return
     * 1 if the database is legacy
     * 0 if the database is not legacy
     * -1 if there is an error while checking the database is legacy
     */
    int isDatabaseLegacy();

    /**
     * Utility that drops the current database.
     * WARNING: Action cannot be undone.
     *
     * @return true if success, otherwise false.
     */
    bool dropTable();

    /**
     * Utility that deletes all message that older than 5 mins
     * WARNING: Action cannot be undone
     *
     * @return true if success, otherwise false.
     */
    bool eraseMessageOverAgeLimit();

    /**
     * Utility that keeps the size of the messages at most 25
     * WARNING: Action cannot be undone
     *
     * @return true if success, otherwise false.
     */
    bool eraseMessageOverSizeLimit();
};

}  // namespace certifiedSender
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CERTIFIEDSENDER_INCLUDE_CERTIFIEDSENDER_SQLITEMESSAGESTORAGE_H_
