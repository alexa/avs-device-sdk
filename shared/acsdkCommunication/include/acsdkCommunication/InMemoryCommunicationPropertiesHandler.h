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

#ifndef ACSDKCOMMUNICATION_INMEMORYCOMMUNICATIONPROPERTIESHANDLER_H_
#define ACSDKCOMMUNICATION_INMEMORYCOMMUNICATIONPROPERTIESHANDLER_H_

#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <acsdkCommunicationInterfaces/CommunicationProperty.h>
#include <acsdkCommunicationInterfaces/CommunicationPropertyChangeSubscriber.h>
#include <acsdkCommunicationInterfaces/CommunicationPropertyValidatorInterface.h>
#include <acsdkCommunicationInterfaces/CommunicationPropertiesHandlerInterface.h>
#include <acsdk/Notifier/Notifier.h>

namespace alexaClientSDK {
namespace acsdkCommunication {

/**
 * Struct to keep properties and writeValidators linked to each other.
 */
template <typename T>
struct PropertyInfo {
    std::weak_ptr<acsdkCommunicationInterfaces::CommunicationProperty<T>> property;
    std::weak_ptr<acsdkCommunicationInterfaces::CommunicationPropertyValidatorInterface<T>> writeValidator;
};
/**
 * This is the in memory implementation of the CommunicationPropertiesHandlerInterface.
 * This is a thread safe class that provides users the ability to register properties.
 */
template <typename T>
class InMemoryCommunicationPropertiesHandler
        : public virtual acsdkCommunicationInterfaces::CommunicationPropertiesHandlerInterface<T> {
public:
    /**
     * Default destructor
     */
    ~InMemoryCommunicationPropertiesHandler() override = default;

    /**
     * Internal class that is used to notify subscribers when a property has been changed.
     */
    class WeakSubscriptionProxy
            : public acsdkCommunicationInterfaces::CommunicationPropertyChangeSubscriber<T>
            , public notifier::Notifier<acsdkCommunicationInterfaces::CommunicationPropertyChangeSubscriber<T>> {
    public:
        /// @name CommunictionPropertyChangeSubscriber methods
        /// @{
        void onCommunicationPropertyChange(const std::string& propertyName, T newValue) override {
            this->notifyObservers(
                [=](const std::shared_ptr<acsdkCommunicationInterfaces::CommunicationPropertyChangeSubscriber<T>>&
                        observer) { observer->onCommunicationPropertyChange(propertyName, newValue); });
        }
        /// }@
    };
    /// @name CommunicationPropertiesHandlerInterface methods
    /// @{
    std::shared_ptr<acsdkCommunicationInterfaces::CommunicationProperty<T>> registerProperty(
        const std::string& propertyName,
        T initValue,
        const std::shared_ptr<acsdkCommunicationInterfaces::CommunicationPropertyValidatorInterface<T>>&
            writeValidator = nullptr) override;
    void deregisterProperty(
        const std::string& propertyName,
        const std::shared_ptr<acsdkCommunicationInterfaces::CommunicationProperty<T>>& property) override;
    bool writeProperty(const std::string& propertyName, T newValue) override;

    bool readProperty(const std::string& propertyName, T& value) override;

    bool subscribeToPropertyChangeEvent(
        const std::string& propertyName,
        const std::weak_ptr<acsdkCommunicationInterfaces::CommunicationPropertyChangeSubscriber<T>>& subscriber)
        override;

    bool unsubscribeToPropertyChangeEvent(
        const std::string& propertyName,
        const std::shared_ptr<acsdkCommunicationInterfaces::CommunicationPropertyChangeSubscriber<T>>& subscriber)
        override;
    /// }@

private:
    // map of names to property info.
    std::unordered_map<std::string, PropertyInfo<T>> m_properties;
    // list of property names to subscribers
    std::unordered_map<std::string, std::shared_ptr<WeakSubscriptionProxy>> m_subscribers;
    // mutex to protect the properties.
    std::mutex m_propertiesMutex;
};
template <typename T>
std::shared_ptr<acsdkCommunicationInterfaces::CommunicationProperty<T>> InMemoryCommunicationPropertiesHandler<T>::
    registerProperty(
        const std::string& propertyName,
        T initValue,
        const std::shared_ptr<acsdkCommunicationInterfaces::CommunicationPropertyValidatorInterface<T>>&
            writeValidator) {
    std::unique_lock<std::mutex> lock(m_propertiesMutex);
    auto it = m_properties.find(propertyName);
    if (it != m_properties.end()) {
        if (!it->second.property.expired()) {
            ACSDK_ERROR(alexaClientSDK::avsCommon::utils::logger::LogEntry(
                            "InMemoryCommunicationPropertiesHandler", "registerProperty")
                            .m("Property is already Registered")
                            .d("property", propertyName));
            return nullptr;
        }
        // Erase if the property isn't available anymore and reregister
        m_properties.erase(it);
    }
    auto property = acsdkCommunicationInterfaces::CommunicationProperty<T>::create(
        propertyName, initValue, writeValidator != nullptr);
    std::weak_ptr<acsdkCommunicationInterfaces::CommunicationProperty<T>> weakProperty =
        std::weak_ptr<acsdkCommunicationInterfaces::CommunicationProperty<T>>(property);
    std::weak_ptr<acsdkCommunicationInterfaces::CommunicationPropertyValidatorInterface<T>> weakValidator =
        std::weak_ptr<acsdkCommunicationInterfaces::CommunicationPropertyValidatorInterface<T>>(writeValidator);
    PropertyInfo<T> currentInfo{weakProperty, weakValidator};
    ACSDK_DEBUG(
        alexaClientSDK::avsCommon::utils::logger::LogEntry("InMemoryCommunicationPropertiesHandler", "registerProperty")
            .m("Property is Registered")
            .d("property", propertyName));

    m_properties.insert({propertyName, currentInfo});

    auto& subscriberProxy = m_subscribers[propertyName];
    if (!subscriberProxy) {
        subscriberProxy = std::make_shared<WeakSubscriptionProxy>();
    }
    property->addSubscriber(subscriberProxy);

    return property;
}

template <typename T>
void InMemoryCommunicationPropertiesHandler<T>::deregisterProperty(
    const std::string& propertyName,
    const std::shared_ptr<acsdkCommunicationInterfaces::CommunicationProperty<T>>& property) {
    std::unique_lock<std::mutex> lock(m_propertiesMutex);
    auto it = m_properties.find(propertyName);
    if (it == m_properties.end()) {
        ACSDK_ERROR(alexaClientSDK::avsCommon::utils::logger::LogEntry(
                        "InMemoryCommunicationPropertiesHandler", "deregisterProperty")
                        .m("Property is not Registered")
                        .d("property", propertyName));
        return;
    }
    if (it->second.property.lock() != property) {
        ACSDK_ERROR(alexaClientSDK::avsCommon::utils::logger::LogEntry(
                        "InMemoryCommunicationPropertiesHandler", "deregisterProperty")
                        .m("Property is registered but can not be matched")
                        .d("property", propertyName));
        return;
    }
    m_properties.erase(it);
}

template <typename T>
bool InMemoryCommunicationPropertiesHandler<T>::writeProperty(const std::string& propertyName, T newValue) {
    std::unique_lock<std::mutex> lock(m_propertiesMutex);
    auto it = m_properties.find(propertyName);
    if (it == m_properties.end()) {
        ACSDK_ERROR(alexaClientSDK::avsCommon::utils::logger::LogEntry(
                        "InMemoryCommunicationPropertiesHandler", "writeProperty")
                        .m("Property is not Registered")
                        .d("property", propertyName));
        return false;
    }
    auto property = it->second.property.lock();
    if (!property) {
        ACSDK_ERROR(alexaClientSDK::avsCommon::utils::logger::LogEntry(
                        "InMemoryCommunicationPropertiesHandler", "writeProperty")
                        .m("Property has expired")
                        .d("property", propertyName));
        m_properties.erase(it);
        return false;
    }
    if (!property->isWriteable()) {
        ACSDK_ERROR(alexaClientSDK::avsCommon::utils::logger::LogEntry(
                        "InMemoryCommunicationPropertiesHandler", "writeProperty")
                        .m("Property is not writeable")
                        .d("property", propertyName));
        return false;
    }
    auto validator = it->second.writeValidator.lock();
    if (!validator) {
        ACSDK_ERROR(alexaClientSDK::avsCommon::utils::logger::LogEntry(
                        "InMemoryCommunicationPropertiesHandler", "writeProperty")
                        .m("Can't validate property")
                        .d("property", propertyName));
        return false;
    }
    lock.unlock();
    if (validator->validateWriteRequest(propertyName, newValue)) {
        property->setValue(newValue);
        return true;
    }
    return false;
}
template <typename T>
bool InMemoryCommunicationPropertiesHandler<T>::readProperty(const std::string& propertyName, T& value) {
    std::unique_lock<std::mutex> lock(m_propertiesMutex);
    auto it = m_properties.find(propertyName);
    if (it == m_properties.end()) {
        ACSDK_ERROR(
            alexaClientSDK::avsCommon::utils::logger::LogEntry("InMemoryCommunicationPropertiesHandler", "readProperty")
                .m("Property is not Registered")
                .d("property", propertyName));
        return false;
    }
    auto property = it->second.property.lock();
    if (!property) {
        ACSDK_ERROR(
            alexaClientSDK::avsCommon::utils::logger::LogEntry("InMemoryCommunicationPropertiesHandler", "readProperty")
                .m("Property has expired")
                .d("property", propertyName));
        m_properties.erase(it);
        return false;
    }
    value = property->getValue();
    return true;
}

template <typename T>
bool InMemoryCommunicationPropertiesHandler<T>::subscribeToPropertyChangeEvent(
    const std::string& propertyName,
    const std::weak_ptr<acsdkCommunicationInterfaces::CommunicationPropertyChangeSubscriber<T>>& subscriber) {
    std::unique_lock<std::mutex> lock(m_propertiesMutex);
    auto& subscriberProxy = m_subscribers[propertyName];
    if (!subscriberProxy) {
        subscriberProxy = std::make_shared<WeakSubscriptionProxy>();
    }
    subscriberProxy->addWeakPtrObserver(subscriber);
    return !subscriber.expired();
}

template <typename T>
bool InMemoryCommunicationPropertiesHandler<T>::unsubscribeToPropertyChangeEvent(
    const std::string& propertyName,
    const std::shared_ptr<acsdkCommunicationInterfaces::CommunicationPropertyChangeSubscriber<T>>& subscriber) {
    std::unique_lock<std::mutex> lock(m_propertiesMutex);
    auto& subscriberProxy = m_subscribers[propertyName];
    if (!subscriberProxy) {
        subscriberProxy = std::make_shared<WeakSubscriptionProxy>();
    }
    subscriberProxy->removeWeakPtrObserver(subscriber);
    return true;
}
}  // namespace acsdkCommunication
}  // namespace alexaClientSDK

#endif  // ACSDKCOMMUNICATION_INMEMORYCOMMUNICATIONPROPERTIESHANDLER_H_
