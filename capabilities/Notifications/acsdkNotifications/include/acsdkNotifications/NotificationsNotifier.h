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

#ifndef ACSDKNOTIFICATIONS_NOTIFICATIONSNOTIFIER_H_
#define ACSDKNOTIFICATIONS_NOTIFICATIONSNOTIFIER_H_

#include <memory>

#include <acsdkNotificationsInterfaces/NotificationsNotifierInterface.h>
#include <acsdkNotificationsInterfaces/NotificationsObserverInterface.h>
#include <acsdk/Notifier/Notifier.h>

namespace alexaClientSDK {
namespace acsdkNotifications {

/**
 * Relays notifications related to Notifications.
 */
class NotificationsNotifier : public notifier::Notifier<acsdkNotificationsInterfaces::NotificationsObserverInterface> {
public:
    /**
     * Factory method.
     *
     * @param notificationsCapabilityAgent The capability agent that provides this notifier.
     * @return The @c NotificationsCapabilityAgent's instance of @c NotificationsNotifierInterface.
     */
    static std::shared_ptr<acsdkNotificationsInterfaces::NotificationsNotifierInterface>
    createNotificationsNotifierInterface(
        const std::shared_ptr<NotificationsCapabilityAgent>& notificationsCapabilityAgent);
};

}  // namespace acsdkNotifications
}  // namespace alexaClientSDK

#endif  // ACSDKNOTIFICATIONS_NOTIFICATIONSNOTIFIER_H_
