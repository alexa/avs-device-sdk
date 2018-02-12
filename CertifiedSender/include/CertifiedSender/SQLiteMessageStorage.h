/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <sqlite3.h>

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
     * Constructor.
     */
    SQLiteMessageStorage();

    ~SQLiteMessageStorage();

    bool createDatabase(const std::string& filePath) override;

    bool open(const std::string& filePath) override;

    bool isOpen() override;

    void close() override;

    bool store(const std::string& message, int* id) override;

    bool load(std::queue<StoredMessage>* messageContainer) override;

    bool erase(int messageId) override;

    bool clearDatabase() override;

protected:
    /**
     * A non-virtual function that may be called to clean up resources managed by this class.
     */
    void doClose();

private:
    /// The sqlite database handle.
    sqlite3* m_dbHandle;
};

}  // namespace certifiedSender
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CERTIFIEDSENDER_INCLUDE_CERTIFIEDSENDER_SQLITEMESSAGESTORAGE_H_
