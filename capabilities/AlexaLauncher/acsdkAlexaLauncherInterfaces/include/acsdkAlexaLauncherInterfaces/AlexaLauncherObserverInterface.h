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

#ifndef ACSDKALEXALAUNCHERINTERFACES_ALEXALAUNCHEROBSERVERINTERFACE_H_
#define ACSDKALEXALAUNCHERINTERFACES_ALEXALAUNCHEROBSERVERINTERFACE_H_

#include <string>

#include "acsdkAlexaLauncherInterfaces/AlexaLauncherTargetState.h"

namespace alexaClientSDK {
namespace acsdkAlexaLauncherInterfaces {

/**
 * This interface is used to observe changes to the launcher target properties that are caused by
 * the @c AlexaLauncherHandlerInterface.
 */
class AlexaLauncherObserverInterface {
public:
    /**
     * Destructor.
     */
    virtual ~AlexaLauncherObserverInterface() = default;

    /**
     * Notifies the change in the target value properties.
     *
     * @param target The launcher target value specified using @c target.
     */
    virtual void onLauncherTargetChanged(const acsdkAlexaLauncherInterfaces::TargetState& targetState) = 0;
};

}  // namespace acsdkAlexaLauncherInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKALEXALAUNCHERINTERFACES_ALEXALAUNCHEROBSERVERINTERFACE_H_
