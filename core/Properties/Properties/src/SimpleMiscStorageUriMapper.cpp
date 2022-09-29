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

#include <acsdk/Properties/MiscStorageAdapter.h>
#include <acsdk/Properties/private/Logging.h>

namespace alexaClientSDK {
namespace properties {

#define TAG "SimpleMiscStorageUriMapper"

std::shared_ptr<SimpleMiscStorageUriMapper> SimpleMiscStorageUriMapper::create(char sep) noexcept {
    return std::shared_ptr<SimpleMiscStorageUriMapper>(new SimpleMiscStorageUriMapper(sep));
}

SimpleMiscStorageUriMapper::SimpleMiscStorageUriMapper(char separator) noexcept : m_separator(separator) {
}

bool SimpleMiscStorageUriMapper::extractComponentAndTableName(
    const std::string& configUri,
    std::string& componentName,
    std::string& tableName) noexcept {
    size_t index = configUri.find(m_separator);

    if (std::string::npos != index) {
        componentName = configUri.substr(0, index);
        tableName = configUri.substr(index + 1);
        ACSDK_DEBUG0(LX_CFG("extractComponentAndTableNameSuccess", configUri)
                         .d("componentName", componentName)
                         .d("tableName", tableName));
        return true;
    } else {
        ACSDK_ERROR(LX_CFG("extractComponentAndTableNameError", configUri));
        return false;
    }
}

}  // namespace properties
}  // namespace alexaClientSDK
