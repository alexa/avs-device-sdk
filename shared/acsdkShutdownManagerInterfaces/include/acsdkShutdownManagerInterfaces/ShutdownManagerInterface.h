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

#ifndef ACSDKSHUTDOWNMANAGERINTERFACES_SHUTDOWNMANAGERINTERFACE_H_
#define ACSDKSHUTDOWNMANAGERINTERFACES_SHUTDOWNMANAGERINTERFACE_H_

namespace alexaClientSDK {
namespace acsdkShutdownManagerInterfaces {

/**
 * Interface for driving shutdown.
 *
 * When @c shutdown() is called, observers that have added themselves via ShutdownNotifierInterface
 * will have their @c shutdown() method called.
 */
class ShutdownManagerInterface {
public:
    /**
     * Destructor.
     */
    virtual ~ShutdownManagerInterface() = default;

    /**
     * Invoke shutdown() on all instances registered with the @c ShutdownNotifierInterface instance
     * associated with this instance.
     *
     * @return Whether shutdown was successful.  This can fail if new objects are registered with
     * ShutdownNotifierInterface while shutdown() is in progress.
     */
    virtual bool shutdown() = 0;
};

}  // namespace acsdkShutdownManagerInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKSHUTDOWNMANAGERINTERFACES_SHUTDOWNMANAGERINTERFACE_H_
