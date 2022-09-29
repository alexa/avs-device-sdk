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

#ifndef CBLAUTHDELEGATE_SQLITECBLAUTHDELEGATESTORAGE_H_
#define CBLAUTHDELEGATE_SQLITECBLAUTHDELEGATESTORAGE_H_

#include <memory>
#include <string>

#include <acsdkAuthorizationInterfaces/LWA/LWAAuthorizationStorageInterface.h>
#include <acsdk/CryptoInterfaces/CryptoFactoryInterface.h>
#include <acsdk/CryptoInterfaces/KeyStoreInterface.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>

#include "CBLAuthDelegate/CBLAuthDelegateStorageInterface.h"

namespace alexaClientSDK {
namespace authorization {
namespace cblAuthDelegate {

/**
 * An implementation of CBLAuthDelegateStorageInterface using SQLite.
 *
 * IMPORTANT NOTE: Your token storage MUST be encrypted.
 * Note that in this default SDK implementation, we do not provide encryption.
 */
class SQLiteCBLAuthDelegateStorage : public CBLAuthDelegateStorageInterface {
public:
    /**
     * Factory method for creating a storage object for CBLAuthDelegate based on an SQLite database.
     *
     * @param configurationRoot The global config object.
     * @param cryptoFactory Crypto factory interface.
     * @param keyStore Key store interface.
     *
     * @return Pointer to the SQLiteCBLAuthDelegate object, nullptr if there's an error creating it.
     */
    static std::shared_ptr<CBLAuthDelegateStorageInterface> createCBLAuthDelegateStorageInterface(
        const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationRoot,
        const std::shared_ptr<cryptoInterfaces::CryptoFactoryInterface>& cryptoFactory,
        const std::shared_ptr<cryptoInterfaces::KeyStoreInterface>& keyStore);

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
    SQLiteCBLAuthDelegateStorage(
        const std::shared_ptr<acsdkAuthorizationInterfaces::lwa::LWAAuthorizationStorageInterface>& lwaStorage);

    /// LWAAuthorizationStorageInterface instance that contains all of the database logic.
    std::shared_ptr<acsdkAuthorizationInterfaces::lwa::LWAAuthorizationStorageInterface> m_lwaStorage;
};

}  // namespace cblAuthDelegate
}  // namespace authorization
}  // namespace alexaClientSDK

#endif  // CBLAUTHDELEGATE_SQLITECBLAUTHDELEGATESTORAGE_H_
