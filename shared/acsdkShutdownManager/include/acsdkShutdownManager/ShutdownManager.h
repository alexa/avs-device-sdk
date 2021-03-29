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

#ifndef ACSDKSHUTDOWNMANAGER_SHUTDOWNMANAGER_H_
#define ACSDKSHUTDOWNMANAGER_SHUTDOWNMANAGER_H_

#include <memory>

#include <acsdkShutdownManagerInterfaces/ShutdownManagerInterface.h>
#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>

namespace alexaClientSDK {
namespace acsdkShutdownManager {

/**
 * Implementation of ShutdownManagerInterface.
 *
 * When @c shutdown() is called, observers that have added themselves via ShutdownNotifierInterface
 * will have their @c shutdown() method called.
 */
class ShutdownManager : public acsdkShutdownManagerInterfaces::ShutdownManagerInterface {
public:
    /**
     * Create a new instance of ShutdownManagerInterface.
     *
     * @param notifier The notifier to use to invoke RequiresShutdown::shutdown().
     */
    static std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface> createShutdownManagerInterface(
        const std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>& notifier);

    /// @name ShutdownManagerInterface methods.
    /// @{
    bool shutdown() override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param notifier The notifier to use to invoke RequiresShutdown::shutdown().
     */
    ShutdownManager(const std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>& notifier);

    /// The notifier to use to invoke RequiresShutdown::shutdown().
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface> m_notifier;
};

}  // namespace acsdkShutdownManager
}  // namespace alexaClientSDK

#endif  // ACSDKSHUTDOWNMANAGER_SHUTDOWNMANAGER_H_
