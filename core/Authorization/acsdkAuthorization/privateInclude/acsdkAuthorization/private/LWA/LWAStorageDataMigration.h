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

#ifndef ACSDKAUTHORIZATION_PRIVATE_LWA_LWASTORAGEDATAMIGRATION_H_
#define ACSDKAUTHORIZATION_PRIVATE_LWA_LWASTORAGEDATAMIGRATION_H_

#include <memory>
#include <string>

#include <acsdk/PropertiesInterfaces/PropertiesFactoryInterface.h>
#include <SQLiteStorage/SQLiteMiscStorage.h>

namespace alexaClientSDK {
namespace acsdkAuthorization {
namespace lwa {

/**
 * @brief Class to migrate storage database format to support @ref SQLiteMiscStorage.
 *
 * This class provides method to migrate data from existing instances of LWAAuthorizationStorage from a format before
 * 1.26 release into a new one.
 *
 * Migration allows end-users to continue using Alexa device without need to reauthorize it.
 */
class LWAStorageDataMigration {
public:
    LWAStorageDataMigration(
        const std::shared_ptr<storage::sqliteStorage::SQLiteMiscStorage>& storage,
        const std::shared_ptr<propertiesInterfaces::PropertiesFactoryInterface>& propertiesFactory) noexcept;

    /**
     * @brief Upgrades storage structure if required.
     *
     * This method checks if \a storage contains tables with data from previous SDK releases, and if so, the data
     * is loaded and stored into \a properties, and tables are dropped.
     */
    void upgradeStorage() noexcept;

private:
    /**
     * @brief Migrate single property from old table into properties.
     *
     * This method checks if the old table \a tableName exists, and has a value in \a columnName column. If there is a
     * value, it is stored into \a properties with a \a propertyName name.
     *
     * Table \a tableName is deleted before operation completes, and if there is an error, property value may be lost.
     *
     * @param[in] storage Storage to check for data to migrate.
     * @param[in] tableName Table name with data to migrate.
     * @param[in] columnName Column name with property value to migrate.
     * @param[in] properties Destination properties interface.
     * @param[in] propertyName New property name to keep the value.
     *
     * @return True on success, False on error.
     */
    bool migrateSinglePropertyTable(
        const std::string& tableName,
        const std::string& columnName,
        const std::shared_ptr<alexaClientSDK::propertiesInterfaces::PropertiesInterface>& properties,
        const std::string& propertyName) noexcept;

private:
    const std::shared_ptr<storage::sqliteStorage::SQLiteMiscStorage> m_storage;
    const std::shared_ptr<propertiesInterfaces::PropertiesFactoryInterface> m_propertiesFactory;
};

}  // namespace lwa
}  // namespace acsdkAuthorization
}  // namespace alexaClientSDK

#endif  // ACSDKAUTHORIZATION_PRIVATE_LWA_LWASTORAGEDATAMIGRATION_H_
