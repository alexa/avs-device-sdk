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

#ifndef ACSDKCOMMUNICATIONINTERFACES_COMMUNICATIONPROPERTYCHANGESUBSCRIBER_H_
#define ACSDKCOMMUNICATIONINTERFACES_COMMUNICATIONPROPERTYCHANGESUBSCRIBER_H_

#include <string>

namespace alexaClientSDK {
namespace acsdkCommunicationInterfaces {

/**
 * The CommunicationPropertyChangeSubscriber is the interface that is used to subscribe to property change events.
 */
template <typename T>
class CommunicationPropertyChangeSubscriber {
public:
    /**
     * default destructor
     */
    virtual ~CommunicationPropertyChangeSubscriber() = default;

    /**
     * Function that is called when the Communication Property that is subscribed to has the value changed.
     * @param propertyName The name of the property
     * @param newValue The new value of the property
     */
    virtual void onCommunicationPropertyChange(const std::string& propertyName, T newValue) = 0;
};

}  // namespace acsdkCommunicationInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKCOMMUNICATIONINTERFACES_COMMUNICATIONPROPERTYCHANGESUBSCRIBER_H_
