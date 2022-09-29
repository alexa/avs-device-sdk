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

#ifndef ACSDK_ALEXAINPUTCONTROLLERINTERFACES_INPUTCONTROLLEROBSERVERINTERFACE_H_
#define ACSDK_ALEXAINPUTCONTROLLERINTERFACES_INPUTCONTROLLEROBSERVERINTERFACE_H_

#include <string>

#include "InputType.h"

namespace alexaClientSDK {
namespace alexaInputControllerInterfaces {

/**
 * This interface is used to observe changes to the Input controller property of an endpoint
 * that are caused by the @c InputControllerInterface.
 */
class InputControllerObserverInterface {
public:
    /**
     * Destructor.
     */
    virtual ~InputControllerObserverInterface() = default;

    /**
     * Notifies the change in the input state property of the endpoint.
     *
     * @param input The input state specified
     */
    virtual void onInputChanged(Input input) = 0;
};

}  // namespace alexaInputControllerInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_ALEXAINPUTCONTROLLERINTERFACES_INPUTCONTROLLEROBSERVERINTERFACE_H_
