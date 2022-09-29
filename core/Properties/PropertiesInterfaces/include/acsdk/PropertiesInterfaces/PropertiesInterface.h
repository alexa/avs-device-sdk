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

#ifndef ACSDK_PROPERTIESINTERFACES_PROPERTIESINTERFACE_H_
#define ACSDK_PROPERTIESINTERFACES_PROPERTIESINTERFACE_H_

#include <string>
#include <unordered_set>
#include <vector>

namespace alexaClientSDK {
namespace propertiesInterfaces {

//! This class provides an interface to a simple key/value container.
/**
 * This interface is obtained through @c PropertiesFactoryInterface factory, which handles disambiguation of properties
 * namespace.
 *
 * The implementation must do best effort for data consistency when handling data update errors for each property. If
 * it is possible, the value of property shall be either left intact, or deleted if the data corruption is unavoidable.
 *
 * \ingroup Lib_acsdkPropertiesInterfaces
 * @sa PropertiesFactoryInterface
 */
class PropertiesInterface {
public:
    /// \brief Bytes data type.
    /// This data type represent a continuous byte array.
    typedef std::vector<unsigned char> Bytes;

    //! Destructor.
    virtual ~PropertiesInterface() noexcept = default;

    //! Method to load string value from configuration.
    /**
     * This method loads string value from configuration. If the value in the storage is not a string, the method
     * behaviour is undefined.
     *
     * @param[in] key Configuration key.
     * @param[out] value Result container. If the method completes successfully, @a value will contain loaded value.
     *                   Otherwise contents of @a value is unmodified.
     * @return True if value has been loaded, false otherwise.
     *
     * @sa #putString
     */
    virtual bool getString(const std::string& key, std::string& value) noexcept = 0;

    //! Method to store string value into configuration.
    /**
     * This method stores string value into configuration. If there is an existing value for the the same key, the value
     * is overwritten.
     *
     * If operation fails, the implementation shall make a best effort for either keeping value unmodified, or clear it
     * to prevent data corruption. Other properties shall not be impacted in case of an error.
     *
     * @param[in] key Configuration key.
     * @param[in] value Value to store.
     * @return True if value has been stored, false otherwise. If this method returns false, the value may stay
     * unchanged, or lost.
     *
     * @sa #getString
     */
    virtual bool putString(const std::string& key, const std::string& value) noexcept = 0;

    //! Method to load binary value from configuration.
    /**
     * This method loads binary value from configuration. If the value in the storage is not binary data, the method
     * behaviour is undefined.
     *
     * @param[in] key Configuration key.
     * @param[out] value If the method completes successfully, @a value will contain loaded value.
     *                   Otherwise contents of @a value is unmodified.
     * @return True if value has been loaded, false otherwise.
     * @sa #putBytes
     */
    virtual bool getBytes(const std::string& key, Bytes& value) noexcept = 0;

    //! Method to store binary value into configuration.
    /**
     * This method stores binary value into configuration. If there is an existing value for the
     * the same key, the value is overwritten.
     *
     * If operation fails, the implementation shall make a best effort for either keeping value unmodified, or clear it
     * to prevent data corruption. Other properties shall not be impacted in case of an error.
     *
     * @param[in] key Configuration key.
     * @param[in] value Value to store.
     * @return True if value has been stored, false otherwise. If this method returns false, the value may stay
     * unchanged, or lost.
     *
     * @sa #getBytes
     */
    virtual bool putBytes(const std::string& key, const Bytes& value) noexcept = 0;

    //! Method to inspect existing properties.
    /**
     * This method provides a set of known property keys from a configuration container.
     *
     * @param[out] keys Container for property keys. If method completes successfully, \a keys will contain all property
     * names. On error, the contents of \a keys is undefined.
     *
     * @return True if operation succeeds, false otherwise.
     */
    virtual bool getKeys(std::unordered_set<std::string>& keys) noexcept = 0;

    //! Removes a property with a given name.
    /**
     * This method removes a property with a given name from a configuration container. If the property doesn't exist,
     * the method succeeds.
     *
     * @param[in] key Configuration key to remove.
     * @return True if the key has been removed or didn't exist. In case of error, false is
     *         returned and the state of configuration container is undefined.
     */
    virtual bool remove(const std::string& key) noexcept = 0;

    //! Removes all properties from a configuration container.
    /**
     * This method removes all properties from a configuration container.
     *
     * @return True if the container has been cleared. In case of error, false is returned,
     *         and the contents of container is undefined.
     */
    virtual bool clear() noexcept = 0;
};

}  // namespace propertiesInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_PROPERTIESINTERFACES_PROPERTIESINTERFACE_H_
