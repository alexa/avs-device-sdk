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

#ifndef ACSDKSAMPLEAPPLICATIONINTERFACES_UIMANAGERINTERFACE_H_
#define ACSDKSAMPLEAPPLICATIONINTERFACES_UIMANAGERINTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace acsdkSampleApplicationInterfaces {

class UIManagerInterface {
public:
    /**
     * Destructor.
     */
    virtual ~UIManagerInterface() = default;

    /**
     * Show a message to the user.
     *
     * @param message The message to show the user.
     */
    virtual void printMessage(const std::string& message) = 0;
};

}  // namespace acsdkSampleApplicationInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKSAMPLEAPPLICATIONINTERFACES_UIMANAGERINTERFACE_H_
