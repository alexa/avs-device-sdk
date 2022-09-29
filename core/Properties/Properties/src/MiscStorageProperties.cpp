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
#include <algorithm>

#include <acsdk/CodecUtils/Base64.h>
#include <acsdk/Properties/private/RetryExecutor.h>
#include <acsdk/Properties/private/MiscStorageProperties.h>
#include <acsdk/Properties/private/Logging.h>

namespace alexaClientSDK {
namespace properties {

using namespace ::alexaClientSDK::avsCommon::sdkInterfaces::storage;
using namespace ::alexaClientSDK::codecUtils;

/// String to identify log entries originating from this file.
/// @private
#define TAG "MiscStorageProperties"

std::shared_ptr<MiscStorageProperties> MiscStorageProperties::create(
    const std::shared_ptr<MiscStorageInterface>& storage,
    const std::string& configUri,
    const std::string& componentName,
    const std::string& tableName) {
    if (!storage) {
        ACSDK_ERROR(LX_CFG("createFailed", configUri).m("nullStorage"));
        return nullptr;
    }

    auto res =
        std::shared_ptr<MiscStorageProperties>(new MiscStorageProperties(storage, configUri, componentName, tableName));
    if (res->init()) {
        ACSDK_DEBUG0(LX_CFG("createSuccess", configUri));
    } else {
        res.reset();
        ACSDK_ERROR(LX_CFG("createFailed", configUri));
    }
    return res;
}

MiscStorageProperties::MiscStorageProperties(
    const std::shared_ptr<MiscStorageInterface>& storage,
    const std::string& configUri,
    const std::string& componentName,
    const std::string& tableName) :
        m_storage(storage),
        m_configUri(configUri),
        m_componentName(componentName),
        m_tableName(tableName) {
}

bool MiscStorageProperties::init() {
    bool tableExists = false;

    // create executor to invoke error callback and limit number of retries
    RetryExecutor executor{OperationType::Open, m_configUri};

    auto result = executor.execute(
        "tableExists",
        [this, &tableExists]() -> StatusCodeWithRetry {
            if (m_storage->tableExists(m_componentName, m_tableName, &tableExists)) {
                ACSDK_DEBUG9(LX_CFG("tableExistsSuccess", m_configUri));
                return RetryExecutor::SUCCESS;
            } else {
                ACSDK_DEBUG9(LX_CFG("tableExistsRetryableFailure", m_configUri));
                return RetryExecutor::RETRYABLE_INNER_PROPERTIES_ERROR;
            }
        },
        Action::FAIL);

    if (RetryableOperationResult::Success != result) {
        ACSDK_DEBUG0(LX_CFG("initFailure", m_configUri));
        return false;
    }

    if (!tableExists) {
        result = executor.execute(
            "tableExists",
            [this]() -> StatusCodeWithRetry {
                if (m_storage->createTable(
                        m_componentName,
                        m_tableName,
                        MiscStorageInterface::KeyType::STRING_KEY,
                        MiscStorageInterface::ValueType::STRING_VALUE)) {
                    ACSDK_DEBUG9(LX_CFG("createTableSuccess", m_configUri));
                    return RetryExecutor::SUCCESS;
                } else {
                    ACSDK_DEBUG9(LX_CFG("createTableRetryableFailure", m_configUri));
                    return RetryExecutor::RETRYABLE_INNER_PROPERTIES_ERROR;
                }
            },
            Action::FAIL);
        if (RetryableOperationResult::Success != result) {
            ACSDK_DEBUG0(LX_CFG("initFailure", m_configUri));
            return false;
        }
    }

    ACSDK_DEBUG0(LX_CFG("initSuccess", m_configUri));
    return true;
}

bool MiscStorageProperties::getString(const std::string& key, std::string& value) noexcept {
    // create executor to invoke error callback and limit number of retries
    RetryExecutor executor{OperationType::Get, m_configUri};

    auto success = executeRetryableKeyAction(
        executor,
        "getString",
        key,
        [this, &key, &value]() -> bool { return m_storage->get(m_componentName, m_tableName, key, &value); },
        true,
        false);

    if (success) {
        ACSDK_DEBUG0(LX_CFG_KEY("getStringSuccess", m_configUri, key));
        return true;
    } else {
        ACSDK_ERROR(LX_CFG_KEY("getStringFailed", m_configUri, key));
        return false;
    }
}

bool MiscStorageProperties::putString(const std::string& key, const std::string& value) noexcept {
    // create executor to invoke error callback and limit number of retries
    RetryExecutor executor{OperationType::Put, m_configUri};
    auto success = executeRetryableKeyAction(
        executor,
        "putString",
        key,
        [this, &key, &value]() -> bool { return m_storage->put(m_componentName, m_tableName, key, value); },
        true,
        false);

    if (success) {
        ACSDK_DEBUG0(LX_CFG_KEY("putStringSuccess", m_configUri, key));
        return true;
    } else {
        ACSDK_ERROR(LX_CFG_KEY("putStringFailed", m_configUri, key));
        return false;
    }
}

bool MiscStorageProperties::getBytes(const std::string& key, Bytes& value) noexcept {
    // create executor to invoke error callback and limit number of retries
    RetryExecutor executor{OperationType::Get, m_configUri};
    std::string base64Value;
    auto success = executeRetryableKeyAction(
        executor,
        "getBytes",
        key,
        [this, &key, &base64Value]() -> bool {
            return m_storage->get(m_componentName, m_tableName, key, &base64Value);
        },
        false,
        false);

    if (!success) {
        ACSDK_ERROR(LX_CFG_KEY("getBytesFailed", m_configUri, key));
        return false;
    }

    if (decodeBase64(base64Value, value)) {
        ACSDK_DEBUG0(LX_CFG_KEY("getBytesSuccess", m_configUri, key));
        return true;
    } else {
        ACSDK_ERROR(LX_CFG_KEY("getBytesDecodeFailed", m_configUri, key));
        return false;
    }
}

bool MiscStorageProperties::putBytes(const std::string& key, const Bytes& value) noexcept {
    // create executor to invoke error callback and limit number of retries
    RetryExecutor executor{OperationType::Put, m_configUri};

    std::string base64Value;
    if (!encodeBase64(value, base64Value)) {
        ACSDK_ERROR(LX_CFG_KEY("putBytesEncodingFailed", m_configUri, key));
        return false;
    }

    auto success = executeRetryableKeyAction(
        executor,
        "putBytes",
        key,
        [this, &key, &base64Value]() -> bool { return m_storage->put(m_componentName, m_tableName, key, base64Value); },
        false,
        false);

    if (success) {
        ACSDK_DEBUG0(LX_CFG_KEY("putBytesSuccess", m_configUri, key));
        return true;
    } else {
        ACSDK_ERROR(LX_CFG_KEY("putBytesFailed", m_configUri, key));
        return false;
    }
}

bool MiscStorageProperties::remove(const std::string& key) noexcept {
    RetryExecutor executor{OperationType::Put, m_configUri};
    auto success = executeRetryableKeyAction(
        executor,
        "remove",
        key,
        [this, &key]() -> bool { return m_storage->remove(m_componentName, m_tableName, key); },
        false,
        false);

    if (success) {
        ACSDK_DEBUG0(LX_CFG_KEY("removeSuccess", m_configUri, key));
        return true;
    } else {
        ACSDK_ERROR(LX_CFG_KEY("removeFailed", m_configUri, key));
        return false;
    }
}

bool MiscStorageProperties::getKeys(std::unordered_set<std::string>& valueContainer) noexcept {
    RetryExecutor executor{OperationType::Get, m_configUri};

    if (loadKeysWithRetries(executor, valueContainer)) {
        ACSDK_DEBUG0(LX_CFG("getKeysSuccess", m_configUri));
        return true;
    } else {
        ACSDK_ERROR(LX_CFG("getKeysFailed", m_configUri));
        return false;
    }
}

bool MiscStorageProperties::clear() noexcept {
    RetryExecutor executor{OperationType::Put, m_configUri};
    if (clearAllValuesWithRetries(executor)) {
        ACSDK_DEBUG0(LX_CFG("clearSuccess", m_configUri));
        return true;
    } else {
        ACSDK_ERROR(LX_CFG("clearFailed", m_configUri));
        return false;
    }
}

bool MiscStorageProperties::loadKeysWithRetries(
    RetryExecutor& executor,
    std::unordered_set<std::string>& keys) noexcept {
    std::unordered_map<std::string, std::string> tmp;
    auto result = executor.execute(
        "loadKeysWithRetries",
        [this, &tmp]() -> StatusCodeWithRetry {
            if (m_storage->load(m_componentName, m_tableName, &tmp)) {
                ACSDK_DEBUG9(LX_CFG("loadKeysSuccess", m_configUri));
                return RetryExecutor::SUCCESS;
            } else {
                ACSDK_DEBUG9(LX_CFG("loadKeysRetryableFailure", m_configUri));
                return RetryExecutor::RETRYABLE_INNER_PROPERTIES_ERROR;
            }
        },
        Action::FAIL);

    if (RetryableOperationResult::Success != result) {
        ACSDK_DEBUG0(LX_CFG("loadKeysWithRetriesFailed", m_configUri));
        return false;
    }

    keys.clear();
    std::transform(
        tmp.cbegin(),
        tmp.cend(),
        std::inserter(keys, keys.end()),
        [](const std::pair<std::string, std::string>& x) -> const std::string& {
            ACSDK_INFO(LX("getKey").d("name", x.first));
            return x.first;
        });

    ACSDK_DEBUG0(LX_CFG("loadKeysWithRetriesSuccess", m_configUri));
    return true;
}

bool MiscStorageProperties::executeRetryableKeyAction(
    RetryExecutor& executor,
    const std::string& actionName,
    const std::string& key,
    const std::function<bool()>& action,
    bool canCleanup,
    bool failOnCleanup) noexcept {
    auto result = executor.execute(
        actionName,
        [this, &actionName, &key, &action]() -> StatusCodeWithRetry {
            // suppress unused variable errors
            ACSDK_UNUSED_VARIABLE(this);
            ACSDK_UNUSED_VARIABLE(actionName);
            ACSDK_UNUSED_VARIABLE(key);
            if (action()) {
                ACSDK_DEBUG9(LX_CFG_KEY(actionName + "Success", m_configUri, key));
                return RetryExecutor::SUCCESS;
            } else {
                ACSDK_DEBUG9(LX_CFG_KEY(actionName + "RetryableFailure", m_configUri, key));
                return RetryExecutor::RETRYABLE_INNER_PROPERTIES_ERROR;
            }
        },
        Action::FAIL);

    if (RetryableOperationResult::Success == result) {
        ACSDK_DEBUG0(LX_CFG_KEY(actionName + "Success", m_configUri, key));
        return true;
    } else if (canCleanup && RetryableOperationResult::Cleanup == result) {
        if (deleteValueWithRetries(executor, key)) {
            ACSDK_DEBUG0(LX_CFG_KEY(actionName + "CleanupSuccess", m_configUri, key));
            return !failOnCleanup;
        } else {
            ACSDK_DEBUG0(LX_CFG_KEY(actionName + "CleanupFailure", m_configUri, key));
            return false;
        }
    } else {
        ACSDK_DEBUG0(LX_CFG_KEY(actionName + "Failure", m_configUri, key));
        return false;
    }
}

bool MiscStorageProperties::deleteValueWithRetries(RetryExecutor& executor, const std::string& key) noexcept {
    auto result = executor.execute(
        "deleteValue",
        [this, &key]() -> StatusCodeWithRetry {
            if (!m_storage->remove(m_componentName, m_tableName, key)) {
                ACSDK_DEBUG9(LX_CFG_KEY("deleteValueSuccess", m_configUri, key));
                return RetryExecutor::SUCCESS;
            } else {
                ACSDK_DEBUG9(LX_CFG_KEY("deleteValueRetryableFailure", m_configUri, key));
                return RetryExecutor::RETRYABLE_INNER_PROPERTIES_ERROR;
            }
        },
        Action::FAIL);

    if (RetryableOperationResult::Success != result) {
        ACSDK_DEBUG0(LX_CFG_KEY("deleteValueFailure", m_configUri, key));
        return false;
    }

    ACSDK_DEBUG0(LX_CFG_KEY("deleteValueSuccess", m_configUri, key));
    return true;
}

bool MiscStorageProperties::clearAllValuesWithRetries(RetryExecutor& executor) noexcept {
    auto result = executor.execute(
        "clear",
        [this]() -> StatusCodeWithRetry {
            if (m_storage->clearTable(m_componentName, m_tableName)) {
                ACSDK_DEBUG9(LX_CFG("clearTableSuccess", m_configUri));
                return RetryExecutor::SUCCESS;
            } else {
                ACSDK_DEBUG9(LX_CFG("clearTableRetryableFailure", m_configUri));
                return RetryExecutor::RETRYABLE_INNER_PROPERTIES_ERROR;
            }
        },
        Action::FAIL);

    if (RetryableOperationResult::Success != result) {
        ACSDK_DEBUG0(LX_CFG("clearTableFailure", m_configUri));
        return false;
    }

    ACSDK_DEBUG0(LX_CFG("clearValuesSuccess", m_configUri));
    return true;
}

}  // namespace properties
}  // namespace alexaClientSDK
