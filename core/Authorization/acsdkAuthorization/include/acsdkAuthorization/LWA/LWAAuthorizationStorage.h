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

#ifndef ACSDKAUTHORIZATION_LWA_LWAAUTHORIZATIONSTORAGE_H_
#define ACSDKAUTHORIZATION_LWA_LWAAUTHORIZATIONSTORAGE_H_

#include <memory>
#include <string>

#include <acsdkAuthorizationInterfaces/LWA/LWAAuthorizationStorageInterface.h>
#include <acsdk/CryptoInterfaces/CryptoFactoryInterface.h>
#include <acsdk/CryptoInterfaces/KeyStoreInterface.h>
#include <acsdk/PropertiesInterfaces/PropertiesFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <SQLiteStorage/SQLiteMiscStorage.h>

namespace alexaClientSDK {
namespace acsdkAuthorization {
namespace lwa {

/// A SQLite based version of @c LWAAuthorizationStorageInterface.

/**
 * @brief Storage implementation based on Properties API.
 *
 * This implementation class adapts properties interface to domain-specific authorization storage interface. Depending
 * on
 *
 * @sa Lib_acsdkPropertiesInterfaces
 */
class LWAAuthorizationStorage : public acsdkAuthorizationInterfaces::lwa::LWAAuthorizationStorageInterface {
public:
    /**
     * @brief Create storage interface.
     *
     * Factory method for creating a storage interface using properties API. For certification it is important
     * to use encrypted properties factory.
     *
     * @param[in] propertiesFactory Properties factory interface.
     *
     * @return Pointer to the LWAAuthorizationStorage object, nullptr if there's an error creating it.
     */
    static std::shared_ptr<LWAAuthorizationStorageInterface> createStorage(
        const std::shared_ptr<propertiesInterfaces::PropertiesFactoryInterface>& propertiesFactory);

    /**
     * @brief Create storage interface backed by SQLite.
     *
     * Factory method for creating a storage object for creating a storage interface based on an SQLite database. If
     * platform configuration has both cryptography and hardware security module support, all the stored values will
     * be encrypted. If there is no cryptography module and/or HSM support, all values will be stored in unencrypted
     * form.
     *
     * @param[in] configurationRoot The global config object.
     * @param[in] storageRootKey The key to use to find the parent node.
     * @param[in] cryptoFactory Crypto factory interface. This interface is required if HSM integration is enabled.
     * @param[in] keyStore Key store interface. This interface is required if HSM integration is enabled.
     *
     * @return Pointer to the LWAAuthorizationStorage object, nullptr if there's an error creating it.
     *
     * @sa CryptoIMPL
     * @sa CryptoPKCS11
     */
    static std::shared_ptr<acsdkAuthorizationInterfaces::lwa::LWAAuthorizationStorageInterface>
    createLWAAuthorizationStorageInterface(
        const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationRoot,
        const std::string& storageRootKey,
        const std::shared_ptr<cryptoInterfaces::CryptoFactoryInterface>& cryptoFactory,
        const std::shared_ptr<cryptoInterfaces::KeyStoreInterface>& keyStore);

    /**
     * Destructor.
     */
    ~LWAAuthorizationStorage();

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
     * @brief Create SQLite interface.
     *
     * Method initialized SQLite database object and returns reference to it. The database path is taken from
     * configuration.
     *
     * @param[in] configurationRoot The global config object.
     * @param[in] storageRootKey The key to use to find the parent node.
     *
     * @return Reference to database object or nullptr on error.
     */
    static std::shared_ptr<storage::sqliteStorage::SQLiteMiscStorage> createSQLiteStorage(
        const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationRoot,
        const std::string& storageRootKey);

    /**
     * @brief Create database file.
     *
     * This method creates an empty database file and sets file permissions to owner-only read write if and only if
     * file doesn't exist.
     *
     * If file already exists, this method does nothing.
     *
     * @param[in] filepath Database file path.
     *
     * @return True if file already exists, or a new empty file created and permissions were set.
     */
    static bool createStorageFileAndSetPermissions(const std::string& filepath) noexcept;

    /**
     * Constructor.
     */
    LWAAuthorizationStorage(
        const std::shared_ptr<alexaClientSDK::propertiesInterfaces::PropertiesFactoryInterface>& propertiesFactory);

    /// The underlying properties factory class;
    std::shared_ptr<alexaClientSDK::propertiesInterfaces::PropertiesFactoryInterface> m_propertiesFactory;

    /// The underlying properties class;
    std::shared_ptr<alexaClientSDK::propertiesInterfaces::PropertiesInterface> m_properties;

    /// Friend class for member access.
    friend class LWAAuthorizationStorageTestHelper;
};

}  // namespace lwa
}  // namespace acsdkAuthorization
}  // namespace alexaClientSDK

#endif  // ACSDKAUTHORIZATION_LWA_LWAAUTHORIZATIONSTORAGE_H_
