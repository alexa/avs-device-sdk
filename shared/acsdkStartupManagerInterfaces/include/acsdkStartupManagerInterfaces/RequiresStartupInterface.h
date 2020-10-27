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

#ifndef ACSDKSTARTUPMANAGERINTERFACES_REQUIRESSTARTUPINTERFACE_H_
#define ACSDKSTARTUPMANAGERINTERFACES_REQUIRESSTARTUPINTERFACE_H_

namespace alexaClientSDK {
namespace acsdkStartupManagerInterfaces {

/**
 * Interface for objects that must perform some kind of operation at startup.
 */
class RequiresStartupInterface {
public:
    /**
     * Destructor.
     */
    virtual ~RequiresStartupInterface() = default;

    /**
     * Perform a startup operation.
     *
     * @return Whether startup should continue.
     */
    virtual bool startup() = 0;
};

}  // namespace acsdkStartupManagerInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKSTARTUPMANAGERINTERFACES_REQUIRESSTARTUPINTERFACE_H_
