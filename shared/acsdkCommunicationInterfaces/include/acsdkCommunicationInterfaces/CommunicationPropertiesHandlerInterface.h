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

#ifndef ACSDKCOMMUNICATIONINTERFACES_COMMUNICATIONPROPERTIESHANDLERINTERFACE_H_
#define ACSDKCOMMUNICATIONINTERFACES_COMMUNICATIONPROPERTIESHANDLERINTERFACE_H_

#include <functional>

#include "CommunicationProperty.h"
#include "CommunicationPropertyChangeSubscriber.h"
#include "CommunicationPropertyValidatorInterface.h"

namespace alexaClientSDK {
namespace acsdkCommunicationInterfaces {

/**
 * The CommunicationPropertiesHandlerInterface is used to register, deregister, write property, read property, subscribe
 * to property change events, and unsubscribe to change events. The implementation will allow multiple different
 * components to have access to properties without have explict ownership. The CommunicationPropertiesHandlerInterface
 * isn't responsible for property ownership.
 */
template <typename T>
class CommunicationPropertiesHandlerInterface {
public:
    /**
     * Virtual destructor to assure proper cleanup of derived types.
     */
    virtual ~CommunicationPropertiesHandlerInterface() = default;

    /**
     * Register a new Property
     * @param name The name of the new property
     * @param initValue The initial value that the property will have.
     * @param writeValidator The class that we will use to validate the write operation. If a nullptr
     *                        the property will be read only.
     * @return std::shared_ptr<Property<T>> The component that registers the property will be the owner and be in charge
     * of keeping the pointer alive.
     */
    virtual std::shared_ptr<CommunicationProperty<T>> registerProperty(
        const std::string& propertyName,
        T initValue,
        const std::shared_ptr<CommunicationPropertyValidatorInterface<T>>& writeValidator = nullptr) = 0;
    /**
     * deregister the property, deregistration of the property only occurs when the property can be found and the passed
     * in property matches the registered property.
     * @param name Name of the property to deregister
     * @param property The property that we are deregistering. This is used to prove ownership of the property.
     * the parameter can be a nullptr.
     */
    virtual void deregisterProperty(
        const std::string& propertyName,
        const std::shared_ptr<CommunicationProperty<T>>& property) = 0;
    /**
     * Write a new value to the property, this will be validated by the writeValidator.
     * @param name The property that we are writing the newValue to.
     * @param newValue The new value for the property.
     * @return true if write succeeds, false otherwise.
     */
    virtual bool writeProperty(const std::string& propertyName, T newValue) = 0;
    /**
     * Read the value from a property.
     * @param name Name of the property we are trying to read.
     * @param[out] value The reference where we will populate the read value.
     * @return true if successful, false otherwise.
     */
    virtual bool readProperty(const std::string& propertyName, T& value) = 0;
    /**
     * Subscribe to change events for a specific property. No value will be passed back. The user should read the
     * value of the property after subscribing.
     * @param propertyName Name of property we want to subscribe to.
     * @param subscriber The subscriber that will define the action for on change event.
     * @return true if successfully subscribed, false otherwise.
     */
    virtual bool subscribeToPropertyChangeEvent(
        const std::string& propertyName,
        const std::weak_ptr<CommunicationPropertyChangeSubscriber<T>>& subscriber) = 0;
    /**
     * Unsubscribe to change events for a specific property.
     * @param propertyName Name of property we want to unsubscribe from.
     * @param subscriber The subscriber that will be unsubscribed from the change events.
     * @return true if successfully unsubscribed, false otherwise.
     */
    virtual bool unsubscribeToPropertyChangeEvent(
        const std::string& propertyName,
        const std::shared_ptr<CommunicationPropertyChangeSubscriber<T>>& subscriber) = 0;
};

}  // namespace acsdkCommunicationInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKCOMMUNICATIONINTERFACES_COMMUNICATIONPROPERTIESHANDLERINTERFACE_H_
