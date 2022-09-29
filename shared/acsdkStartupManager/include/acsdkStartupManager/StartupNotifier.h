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

#ifndef ACSDKSTARTUPMANAGER_STARTUPNOTIFIER_H_
#define ACSDKSTARTUPMANAGER_STARTUPNOTIFIER_H_

#include <memory>

#include <acsdk/Notifier/Notifier.h>

#include "acsdkStartupManagerInterfaces/RequiresStartupInterface.h"
#include "acsdkStartupManagerInterfaces/StartupNotifierInterface.h"

namespace alexaClientSDK {
namespace acsdkStartupManager {

/**
 * Relays the notification when it's time to start up.
 */
class StartupNotifier : public notifier::Notifier<acsdkStartupManagerInterfaces::RequiresStartupInterface> {
public:
    /**
     * Factory method.
     * @return A new instance of @c StartupNotifierInterface.
     */
    static std::shared_ptr<acsdkStartupManagerInterfaces::StartupNotifierInterface> createStartupNotifierInterface();
};

}  // namespace acsdkStartupManager
}  // namespace alexaClientSDK

#endif  // ACSDKSTARTUPMANAGER_STARTUPNOTIFIER_H_
