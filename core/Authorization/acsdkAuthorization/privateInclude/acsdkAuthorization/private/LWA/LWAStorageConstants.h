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

#ifndef ACSDKAUTHORIZATION_PRIVATE_LWA_LWASTORAGECONSTANTS_H_
#define ACSDKAUTHORIZATION_PRIVATE_LWA_LWASTORAGECONSTANTS_H_

#include <memory>
#include <string>

#include <acsdk/PropertiesInterfaces/PropertiesFactoryInterface.h>
#include <SQLiteStorage/SQLiteMiscStorage.h>

namespace alexaClientSDK {
namespace acsdkAuthorization {
namespace lwa {

/// The name of the refreshToken entry.
extern const std::string REFRESH_TOKEN_PROPERTY_NAME;

/// The name of the userId table.
extern const std::string USER_ID_PROPERTY_NAME;

/// The configuration URI.
extern const std::string CONFIG_URI;

/// The name of the refreshToken table.
extern const std::string REFRESH_TOKEN_TABLE_NAME;

/// The name of the refreshToken column.
extern const std::string REFRESH_TOKEN_COLUMN_NAME;

/// The name of the userId table.
extern const std::string USER_ID_TABLE_NAME;

/// The name of the userId column.
extern const std::string USER_ID_COLUMN_NAME;

}  // namespace lwa
}  // namespace acsdkAuthorization
}  // namespace alexaClientSDK

#endif  // ACSDKAUTHORIZATION_PRIVATE_LWA_LWASTORAGECONSTANTS_H_
