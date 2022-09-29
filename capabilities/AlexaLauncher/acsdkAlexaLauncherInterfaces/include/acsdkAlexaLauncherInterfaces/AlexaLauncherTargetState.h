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

#ifndef ACSDKALEXALAUNCHERINTERFACES_ALEXALAUNCHERTARGETSTATE_H_
#define ACSDKALEXALAUNCHERINTERFACES_ALEXALAUNCHERTARGETSTATE_H_

#include <string>

namespace alexaClientSDK {
namespace acsdkAlexaLauncherInterfaces {

/**
 * Struct that represents the AlexaLauncher target properties.
 */
struct TargetState {
    /// The identifier for the item to launch. Application identifiers contain app,
    /// and shortcut identifiers contain shortcut.
    /// For a complete list of identifiers see https://developer.amazon.com/docs/video/launch-target-reference.html
    std::string identifier;

    /// The name associated with the identifier.
    std::string name;
};

}  // namespace acsdkAlexaLauncherInterfaces
}  // namespace alexaClientSDK
#endif  // ACSDKALEXALAUNCHERINTERFACES_ALEXALAUNCHERTARGETSTATE_H_
