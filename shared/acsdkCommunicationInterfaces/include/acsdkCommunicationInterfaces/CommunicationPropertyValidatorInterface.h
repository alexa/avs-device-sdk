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

#ifndef ACSDKCOMMUNICATIONINTERFACES_COMMUNICATIONPROPERTYVALIDATORINTERFACE_H_
#define ACSDKCOMMUNICATIONINTERFACES_COMMUNICATIONPROPERTYVALIDATORINTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace acsdkCommunicationInterfaces {

/**
 * The CommunicationPropertyValidatorInterface is used to validate the new value that is being written to a property.
 * This allows the component that registered the property to have some control over what is written into their property
 * by external components.
 */
template <typename T>
class CommunicationPropertyValidatorInterface {
public:
    /**
     * default destructor
     */
    virtual ~CommunicationPropertyValidatorInterface() = default;

    /**
     * Called when we want to write to a property. Used to validate before we write the newValue
     * @param propertyName The name of the property
     * @param newValue The new value of the property
     * @return true if alright to write value, false otherwise.
     */
    virtual bool validateWriteRequest(const std::string& propertyName, T newValue) = 0;
};

}  // namespace acsdkCommunicationInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKCOMMUNICATIONINTERFACES_COMMUNICATIONPROPERTYVALIDATORINTERFACE_H_