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

#include <acsdk/Properties/private/MiscStorageProperties.h>
#include <acsdk/Properties/private/MiscStoragePropertiesFactory.h>
#include <acsdk/Properties/private/Logging.h>

namespace alexaClientSDK {
namespace properties {

using namespace alexaClientSDK::propertiesInterfaces;
using namespace alexaClientSDK::avsCommon::sdkInterfaces::storage;

/// String to identify log entries originating from this file.
/// @private
#define TAG "MiscStoragePropertiesFactory"

std::shared_ptr<PropertiesFactoryInterface> MiscStoragePropertiesFactory::create(
    const std::shared_ptr<MiscStorageInterface>& storage,
    const std::shared_ptr<MiscStorageUriMapperInterface>& uriMapper) noexcept {
    auto res = std::shared_ptr<MiscStoragePropertiesFactory>(new MiscStoragePropertiesFactory(storage, uriMapper));
    if (!res->init()) {
        res.reset();
    }
    return res;
}

MiscStoragePropertiesFactory::MiscStoragePropertiesFactory(
    const std::shared_ptr<MiscStorageInterface>& storage,
    const std::shared_ptr<MiscStorageUriMapperInterface>& uriMapper) noexcept :
        m_storage{storage},
        m_uriMapper{uriMapper} {
}

bool MiscStoragePropertiesFactory::init() noexcept {
    if (!m_storage || !m_uriMapper) {
        ACSDK_ERROR(LX("initFailed").d("reason", "storageOrMapperNull"));
        return false;
    }

    if (!m_storage->isOpened()) {
        if (!m_storage->open()) {
            ACSDK_DEBUG9(LX("init").m("openFailed"));
            if (!m_storage->createDatabase()) {
                ACSDK_ERROR(LX("initFailed").d("reason", "storageOpenAndCreateFailed"));
                return false;
            }
        }
    }

    ACSDK_DEBUG5(LX("initSuccess"));

    return true;
}

void MiscStoragePropertiesFactory::dropNullReferences() noexcept {
    for (auto it = m_openProperties.begin(); it != m_openProperties.end();) {
        if (it->second.expired()) {
            it = m_openProperties.erase(it);
        } else {
            it++;
        }
    }
}

std::shared_ptr<PropertiesInterface> MiscStoragePropertiesFactory::getProperties(
    const std::string& configUri) noexcept {
    std::lock_guard<std::mutex> stateLock(m_stateMutex);
    dropNullReferences();

    std::shared_ptr<PropertiesInterface> result;
    auto it = m_openProperties.find(configUri);
    if (it != m_openProperties.end()) {
        result = it->second.lock();
    }

    if (!result) {
        std::string componentName;
        std::string tableName;
        if (m_uriMapper->extractComponentAndTableName(configUri, componentName, tableName)) {
            result = MiscStorageProperties::create(m_storage, configUri, componentName, tableName);

            if (result) {
                m_openProperties.emplace(configUri, result);
            }
        } else {
            ACSDK_ERROR(LX("failedToExtractComponentAndTableName").d("configURI", configUri));
            return nullptr;
        }
    }

    return result;
}

}  // namespace properties
}  // namespace alexaClientSDK
