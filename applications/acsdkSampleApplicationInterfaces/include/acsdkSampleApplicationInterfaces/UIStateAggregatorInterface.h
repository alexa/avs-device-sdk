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

#ifndef ACSDKSAMPLEAPPLICATIONINTERFACES_UISTATEAGGREGATORINTERFACE_H_
#define ACSDKSAMPLEAPPLICATIONINTERFACES_UISTATEAGGREGATORINTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace acsdkSampleApplicationInterfaces {

/*
 * Contract to notify user interface about relevant application state changes.
 */
class UIStateAggregatorInterface {
public:
    /**
     * Destructor.
     */
    virtual ~UIStateAggregatorInterface() = default;

    /**
     * Notify User interface about the interaction state
     *
     * @param state UI state mainly to display the connection and Alexa states to user.
     */
    virtual void notifyAlexaState(const std::string& state) = 0;
};

}  // namespace acsdkSampleApplicationInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKSAMPLEAPPLICATIONINTERFACES_UISTATEAGGREGATORINTERFACE_H_
