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

#ifndef ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_STORAGE_CAPABILITIESDELEGATESTORAGEINTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_STORAGE_CAPABILITIESDELEGATESTORAGEINTERFACE_H_

#include <string>
#include <unordered_map>

namespace alexaClientSDK {
namespace capabilitiesDelegate {
namespace storage {

/**
 * An Interface class which defines APIs for interacting with a database for storing, loading and modifying
 * capabilities information.
 *
 * @note: The implementation of this interface should be thread safe.
 */
class CapabilitiesDelegateStorageInterface {
public:
    /**
     * Destructor.
     */
    virtual ~CapabilitiesDelegateStorageInterface() = default;

    /**
     * Creates a new database.
     *
     * @return True if successful, else false.
     */
    virtual bool createDatabase() = 0;

    /**
     * Opens the database.
     *
     * @return True if successful, else false.
     */
    virtual bool open() = 0;

    /**
     * Closes the database.
     */
    virtual void close() = 0;

    /**
     * Stores the endpointConfig with the endpointId in the database.
     *
     * @param endpointId The endpoint ID string.
     * @param endpointConfig The full endpoint config sent in the discovery.
     * @return True if successful, else false.
     */
    virtual bool store(const std::string& endpointId, const std::string& endpointConfig) = 0;

    /**
     * Stores the endpointConfigMap.
     *
     * @param endpointIdToConfigMap The endpointId to configuration map.
     * @return True if successful, else false.
     */
    virtual bool store(const std::unordered_map<std::string, std::string>& endpointIdToConfigMap) = 0;

    /**
     * Loads the endpointConfigMap.
     *
     * @param endpointConfigMap The endpoint config map pointer.
     * @return True if successful, else false.
     */
    virtual bool load(std::unordered_map<std::string, std::string>* endpointConfigMap) = 0;

    /**
     * Loads the endpointConfig with the given endpoint Id.
     *
     * @param endpointId The endpointId key used to load data from the storage.
     * @param endpointConfig The pointer to the endpointConfig that will be updated, if the key is present in storage.
     * @return True if successful, else false.
     */
    virtual bool load(const std::string& endpointId, std::string* endpointConfig) = 0;

    /**
     * Erases a single endpointConfig from the database.
     *
     * @param endpointId The endpointId key to erase from storage.
     * @return True if successful, else false.
     */
    virtual bool erase(const std::string& endpointId) = 0;

    /**
     * Erases a vector of endpoint Ids from the data base.
     * @param endpointIdToConfigMap the endpointId to Configuration map.
     * @return  True if successful, else false.
     */
    virtual bool erase(const std::unordered_map<std::string, std::string>& endpointIdToConfigMap) = 0;

    /**
     * Erases the entire storage.
     *
     * @return True if clear operation was successful, false if there was a failure to erase the database.
     */
    virtual bool clearDatabase() = 0;
};

}  // namespace storage
}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_STORAGE_CAPABILITIESDELEGATESTORAGEINTERFACE_H_
