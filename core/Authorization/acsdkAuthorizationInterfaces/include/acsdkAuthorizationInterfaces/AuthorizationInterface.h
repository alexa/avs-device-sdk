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

#ifndef ACSDKAUTHORIZATIONINTERFACES_AUTHORIZATIONINTERFACE_H_
#define ACSDKAUTHORIZATIONINTERFACES_AUTHORIZATIONINTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace acsdkAuthorizationInterfaces {

/**
 * The application facing interface that contains common shared methods between authorization mechanisms.
 */
class AuthorizationInterface {
public:
    /// Destructor.
    virtual ~AuthorizationInterface() = default;

    /**
     * Returns a unique identifier used to identify this adapter.
     *
     * @return The id.
     */
    virtual std::string getId() = 0;
};

}  // namespace acsdkAuthorizationInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKAUTHORIZATIONINTERFACES_AUTHORIZATIONINTERFACE_H_
