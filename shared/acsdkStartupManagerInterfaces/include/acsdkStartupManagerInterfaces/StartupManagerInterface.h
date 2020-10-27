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

#ifndef ACSDKSTARTUPMANAGERINTERFACES_STARTUPMANAGERINTERFACE_H_
#define ACSDKSTARTUPMANAGERINTERFACES_STARTUPMANAGERINTERFACE_H_

namespace alexaClientSDK {
namespace acsdkStartupManagerInterfaces {

/**
 * Interface for driving startup.
 *
 * When @c startup() is called, observers that have added themselves via StartupNotifierInterface
 * will have their @c startup() method called.  If any of then return false from that call,
 * startup will abort.
 */
class StartupManagerInterface {
public:
    /**
     * Destructor.
     */
    virtual ~StartupManagerInterface() = default;

    /**
     * Trigger the startup sequence.
     *
     * @return Whether the startup sequence ran to completion.
     */
    virtual bool startup() = 0;
};

}  // namespace acsdkStartupManagerInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKSTARTUPMANAGERINTERFACES_STARTUPMANAGERINTERFACE_H_
