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

#ifndef ACSDKSTARTUPMANAGER_STARTUPMANAGER_H_
#define ACSDKSTARTUPMANAGER_STARTUPMANAGER_H_

#include <memory>

#include <acsdkStartupManagerInterfaces/StartupManagerInterface.h>
#include <acsdkStartupManagerInterfaces/StartupNotifierInterface.h>

namespace alexaClientSDK {
namespace acsdkStartupManager {

/**
 * Implementation of StartupManagerInterface.
 */
class StartupManager : public acsdkStartupManagerInterfaces::StartupManagerInterface {
public:
    /**
     * Create a new instance of StartupManagerInterface.
     *
     * @param notifier The notifier to use to invoke RequiresStartupInterface::startup().
     */
    static std::shared_ptr<acsdkStartupManagerInterfaces::StartupManagerInterface> createStartupManagerInterface(
        const std::shared_ptr<acsdkStartupManagerInterfaces::StartupNotifierInterface>& notifier);

    /// @name StartupManagerInterface methods.
    /// @{
    bool startup() override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param notifier The notifier to use to invoke RequiresStartupInterface::startup().
     */
    StartupManager(const std::shared_ptr<acsdkStartupManagerInterfaces::StartupNotifierInterface>& notifier);

    /// The notifier to use to invoke RequiresStartupInterface::doStartup().
    std::shared_ptr<acsdkStartupManagerInterfaces::StartupNotifierInterface> m_notifier;
};

}  // namespace acsdkStartupManager
}  // namespace alexaClientSDK

#endif  // ACSDKSTARTUPMANAGER_STARTUPMANAGER_H_
