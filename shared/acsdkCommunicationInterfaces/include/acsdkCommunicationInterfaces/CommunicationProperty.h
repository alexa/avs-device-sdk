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
#ifndef ACSDKCOMMUNICATIONINTERFACES_COMMUNICATIONPROPERTY_H_
#define ACSDKCOMMUNICATIONINTERFACES_COMMUNICATIONPROPERTY_H_

#include <functional>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <acsdkCommunicationInterfaces/CommunicationPropertyChangeSubscriber.h>
#include <acsdk/Notifier/Notifier.h>

namespace alexaClientSDK {
namespace acsdkCommunicationInterfaces {

/**
 * The CommunicationProperty is the class that will be returned when we register a new property with the
 * CommunicationPropertiesHandlerInterface. This will hold a value of type T. This class allows the owner direct read
 * and write access.
 */
template <typename T>
class CommunicationProperty {
public:
    /**
     * setValue allows setting of the value without going through the CommunicationPropertiesHandlerInterface
     * @param newValue The newValue which we are setting the value to.
     * @return true if value was set, false otherwise.
     */
    bool setValue(T newValue) {
        std::unique_lock<std::mutex> lock(m_propertyMutex);
        m_value = newValue;
        auto weakSubscriptionProxy = m_weakSubscriptionProxy;
        auto propertyName = m_name;
        /// Capturing a snapshot of this communication property and notifying the subscribers by submitting this
        /// to the executor.
        m_staticExecutor.execute([weakSubscriptionProxy, newValue, propertyName]() {
            weakSubscriptionProxy->notifyObservers(
                [=](const std::shared_ptr<CommunicationPropertyChangeSubscriber<T>>& obs) {
                    obs->onCommunicationPropertyChange(propertyName, newValue);
                });
        });
        return true;
    }

    /**
     * Owner of the property can get the value of the property without going through the
     * CommunicationPropertiesHandlerInterface
     * @return the value of the property
     */
    T getValue() {
        std::unique_lock<std::mutex> lock(m_propertyMutex);
        return m_value;
    }

    /**
     * Create a new Property
     * @param name Name of the new property.
     * @param initValue The initial value of the property
     * @param writeable If the property is writeable or not.
     * @return shared_ptr to a new Property.
     */
    static std::shared_ptr<CommunicationProperty<T>> create(const std::string& name, T initValue, bool writeable) {
        return std::shared_ptr<CommunicationProperty<T>>(new CommunicationProperty(name, initValue, writeable));
    }

    /**
     * A function used to determine if the property is writeable or not.
     * @return true if writeable, false otherwise
     */
    bool isWriteable() {
        return m_writeable;
    }

    /**
     * Add a subscriber to property change events.
     * @param subscriber The new subscriber that wants to listen to property change events.
     * @return true if subscriber is valid and was added, false otherwise.
     */
    bool addSubscriber(const std::weak_ptr<CommunicationPropertyChangeSubscriber<T>>& subscriber) {
        m_weakSubscriptionProxy->addWeakPtrObserver(subscriber);
        return !subscriber.expired();
    }

    /**
     * Remove a subscriber from property change events.
     * @param subscriber The subscriber that doesn't want to listen to property change events.
     */
    void removeSubscriber(const std::shared_ptr<CommunicationPropertyChangeSubscriber<T>>& subscriber) {
        m_weakSubscriptionProxy->removeWeakPtrObserver(subscriber);
    }

private:
    /**
     * Notify subscribers of a change to a the property value. This is called from an executor.
     */
    bool notifyOnCommunicationPropertyChange(const std::string& propertyName, T newValue) {
        m_weakSubscriptionProxy->notifyObservers(
            [=](const std::shared_ptr<CommunicationPropertyChangeSubscriber<T>>& obs) {
                obs->onCommunicationPropertyChange(propertyName, newValue);
            });
        return true;
    }

    /**
     * Private constructor
     * @param name Name of the property
     * @param initValue Initial value of the property
     * @param writeable If the property is writeable or not.
     */
    CommunicationProperty<T>(std::string name, T initValue, bool writeable) :
            m_weakSubscriptionProxy{std::make_shared<notifier::Notifier<CommunicationPropertyChangeSubscriber<T>>>()},
            m_name{std::move(name)},
            m_value{std::move(initValue)},
            m_writeable{writeable} {
    }

private:
    /// The communication Property to notify on setValue events.
    const std::shared_ptr<notifier::Notifier<CommunicationPropertyChangeSubscriber<T>>> m_weakSubscriptionProxy;

    /// The name of property
    const std::string m_name;

    /// The value of the property
    T m_value;

    /// Is the property writeable
    const bool m_writeable;

    /// Mutex to protect the property value
    std::mutex m_propertyMutex;

    /// Static executor for all Communication Properties.
    static avsCommon::utils::threading::Executor m_staticExecutor;
};

template <typename T>
avsCommon::utils::threading::Executor CommunicationProperty<T>::m_staticExecutor;

}  // namespace acsdkCommunicationInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKCOMMUNICATIONINTERFACES_COMMUNICATIONPROPERTY_H_
