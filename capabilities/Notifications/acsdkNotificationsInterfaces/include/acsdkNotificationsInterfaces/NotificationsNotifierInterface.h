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

#ifndef ACSDKNOTIFICATIONSINTERFACES_NOTIFICATIONSNOTIFIERINTERFACE_H_
#define ACSDKNOTIFICATIONSINTERFACES_NOTIFICATIONSNOTIFIERINTERFACE_H_

#include <memory>

#include <acsdk/NotifierInterfaces/NotifierInterface.h>

#include "acsdkNotificationsInterfaces/NotificationsObserverInterface.h"

namespace alexaClientSDK {
namespace acsdkNotificationsInterfaces {

/**
 * Interface for registering to observe Bluetooth notifications.
 */
using NotificationsNotifierInterface = notifierInterfaces::NotifierInterface<NotificationsObserverInterface>;

}  // namespace acsdkNotificationsInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKNOTIFICATIONSINTERFACES_NOTIFICATIONSNOTIFIERINTERFACE_H_
