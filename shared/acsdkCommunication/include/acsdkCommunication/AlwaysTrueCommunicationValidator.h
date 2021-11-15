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

#ifndef ACSDKCOMMUNICATION_ALWAYSTRUECOMMUNICATIONVALIDATOR_H_
#define ACSDKCOMMUNICATION_ALWAYSTRUECOMMUNICATIONVALIDATOR_H_

#include <acsdkCommunicationInterfaces/CommunicationPropertyValidatorInterface.h>

namespace alexaClientSDK {
namespace acsdkCommunication {

/**
 * This is an implementation of the CommunicationPropertyValidatorInterface, that will always result to true.
 * This class is meant to offer implementors a quick way to create Writable properties without creating a new
 * CommunicationPropertyWriter if they don't want to validate the value being written.
 */
template <typename T>
class AlwaysTrueCommunicationValidator
        : public acsdkCommunicationInterfaces::CommunicationPropertyValidatorInterface<T> {
public:
    /**
     * Default Constructor
     */
    AlwaysTrueCommunicationValidator() = default;

    /**
     * Default Destructor
     */
    ~AlwaysTrueCommunicationValidator() override = default;

    /// @name CommunicationPropertyValidatorInterface methods
    /// @{
    bool validateWriteRequest(const std::string& propertyName, T newValue) override {
        return true;
    }
    /// @}
};
}  // namespace acsdkCommunication
}  // namespace alexaClientSDK

#endif  // ACSDKCOMMUNICATION_ALWAYSTRUECOMMUNICATIONVALIDATOR_H_
