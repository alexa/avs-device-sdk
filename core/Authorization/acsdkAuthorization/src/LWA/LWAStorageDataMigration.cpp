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

#include <acsdkAuthorization/private/Logging.h>
#include <acsdkAuthorization/private/LWA/LWAStorageConstants.h>
#include <acsdkAuthorization/private/LWA/LWAStorageDataMigration.h>
#include <AVSCommon/Utils/Error/FinallyGuard.h>

namespace alexaClientSDK {
namespace acsdkAuthorization {
namespace lwa {

using namespace ::alexaClientSDK::propertiesInterfaces;
using namespace ::alexaClientSDK::storage;
using namespace ::alexaClientSDK::avsCommon::utils::logger;
using namespace ::alexaClientSDK::avsCommon::utils::error;

/// String to identify log entries originating from this file.
#define TAG "LWAStorageDataMigration"

LWAStorageDataMigration::LWAStorageDataMigration(
    const std::shared_ptr<storage::sqliteStorage::SQLiteMiscStorage>& storage,
    const std::shared_ptr<propertiesInterfaces::PropertiesFactoryInterface>& propertiesFactory) noexcept :
        m_storage{storage},
        m_propertiesFactory{propertiesFactory} {
}

void LWAStorageDataMigration::upgradeStorage() noexcept {
    if (!m_storage) {
        ACSDK_ERROR(LX("upgradeStorageFailed").d("reason", "storageNull"));
        return;
    }
    if (!m_propertiesFactory) {
        ACSDK_ERROR(LX("upgradeStorageFailed").d("reason", "propertiesFactoryNull"));
        return;
    }

    auto& db = m_storage->getDatabase();
    if (db.tableExists(REFRESH_TOKEN_TABLE_NAME) || db.tableExists(USER_ID_TABLE_NAME)) {
        auto properties = m_propertiesFactory->getProperties(CONFIG_URI);
        if (!properties) {
            ACSDK_ERROR(LX("upgradeStorageFailed").d("reason", "getPropertiesError"));
            return;
        }

        if (!migrateSinglePropertyTable(USER_ID_TABLE_NAME, USER_ID_COLUMN_NAME, properties, USER_ID_PROPERTY_NAME)) {
            ACSDK_WARN(LX("migrateLegacyTablesError").m("errorWhileMigratingUserId"));
        }

        if (!migrateSinglePropertyTable(
                REFRESH_TOKEN_TABLE_NAME, REFRESH_TOKEN_COLUMN_NAME, properties, REFRESH_TOKEN_PROPERTY_NAME)) {
            ACSDK_WARN(LX("migrateLegacyTablesError").m("errorWhileMigratingRefreshToken"));
        }
    }
}

bool LWAStorageDataMigration::migrateSinglePropertyTable(
    const std::string& tableName,
    const std::string& columnName,
    const std::shared_ptr<alexaClientSDK::propertiesInterfaces::PropertiesInterface>& properties,
    const std::string& propertyName) noexcept {
    if (!m_storage) {
        ACSDK_ERROR(LX("migrateSinglePropertyTableFailed").d("reason", "storageNull"));
        return false;
    }

    if (!properties) {
        ACSDK_ERROR(LX("migrateSinglePropertyTableFailed").d("reason", "propertiesNull"));
        return false;
    }

    auto& db = m_storage->getDatabase();
    if (db.tableExists(tableName)) {
        FinallyGuard clearAndDropTable{[&]() -> void {
            if (!db.clearTable(tableName)) {
                ACSDK_WARN(LX("migrateSinglePropertyTable").m("tableClearFailed").sensitive("tableName", tableName));
            }

            if (!db.dropTable(tableName)) {
                ACSDK_WARN(LX("migrateSinglePropertyTable").m("tableDropFailed").sensitive("tableName", tableName));
            }

            ACSDK_DEBUG0(LX("migrateSinglePropertyTable").m("tableRemoved").sensitive("tableName", tableName));
        }};

        std::string sqlString = "SELECT * FROM " + tableName + ";";
        auto statement = db.createStatement(sqlString);
        if (!statement) {
            ACSDK_ERROR(LX("migrateSinglePropertyTableFailed")
                            .d("reason", "createStatementFailed")
                            .sensitive("tableName", tableName));
            return false;
        }

        if (!statement->step()) {
            ACSDK_ERROR(
                LX("migrateSinglePropertyTableFailed").d("reason", "stepFailed").sensitive("tableName", tableName));
            return false;
        }

        if (statement->getStepResult() != SQLITE_ROW) {
            ACSDK_DEBUG5(LX("migrateSinglePropertyTableSuccess")
                             .d("reason", "noDataToMigrate")
                             .sensitive("tableName", tableName));
            return true;
        }

        std::string resultColumnName = statement->getColumnName(0);
        if (columnName != resultColumnName) {
            ACSDK_ERROR(LX("migrateSinglePropertyTableFailed")
                            .d("reason", "unexpectedColumnName")
                            .sensitive("tableName", tableName)
                            .sensitive("columnName", resultColumnName)
                            .sensitive("expectedName", columnName));
            return false;
        }

        auto text = statement->getColumnText(0);

        if (!properties->putString(propertyName, text)) {
            ACSDK_ERROR(
                LX("migrateSinglePropertyTableFailed").d("reason", "putStringError").sensitive("tableName", tableName));
            return false;
        }
    }

    ACSDK_DEBUG5(LX("migrateSinglePropertyTableSuccess").sensitive("tableName", tableName));
    return true;
}

}  // namespace lwa
}  // namespace acsdkAuthorization
}  // namespace alexaClientSDK
