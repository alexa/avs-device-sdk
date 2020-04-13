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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ENDPOINTS_ENDPOINTIDENTIFIER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ENDPOINTS_ENDPOINTIDENTIFIER_H_

#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace endpoints {

/**
 * Alias for the endpoint identifier.
 *
 * Note that every endpoint must have a unique identifier across all endpoints registered to the user. Identifiers
 * used for endpoints that represent this client or other parts that belong to this client should be generated during
 * endpoint creation using the EndpointBuilderInterface. Endpoints that are considered external to this client, i.e.,
 * that can be connected to different clients, must be able to provide its unique identifier and ensure that the
 * identifier stays the same.
 *
 * @note The identifier must not exceed 256 characters and it can contain letters or numbers, spaces, and the following
 * special characters: _ - = # ; : ? @ &.
 *
 */
using EndpointIdentifier = std::string;

}  // namespace endpoints
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_ENDPOINTS_ENDPOINTIDENTIFIER_H_
