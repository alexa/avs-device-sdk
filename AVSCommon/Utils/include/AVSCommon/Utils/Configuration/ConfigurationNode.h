/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_CONFIGURATION_CONFIGURATIONNODE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_CONFIGURATION_CONFIGURATIONNODE_H_

#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <rapidjson/document.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace configuration {

/**
 * Class providing access to a global read-only configuration object. This object is a tree structure comprised of
 * @c ConfigurationNode instances that contain key-value pairs (including @c ConfigurationNode values).
 *
 * Methods to fetch a named value from a @c ConfigurationNode are of the form:
 * @code
 *     bool get<Type>(const char* key, <Type>* out = nullptr, <Type> defaultValue = simpleDefault)
 * @endcode
 * This signature allows for fetching a value without the risk of an assertion or exception if the value is not
 * present. It also allows for checking if a key is present, or combining the check and fetch operations
 * in a single call.
 *
 * Sub @c ConfigurationNodes are accessed via operator[], where the index is the name of the sub @c ConfigurationNode.
 * If a key for a @c ConfigurationNode does not exist an empty @c ConfigurationNode is returned instead. This allows
 * for simple and safe traversal of the configuration hierarchy. For example:
 * @code
 *     std::string tempString;
 *     ConfigurationNode::getRoot()["someComponent"]["someSubComponent"].getString("someKey", &tempString);
 * @endcode
 *
 * The configuration is specified via JSON documents with a root object that corresponds to the root
 * @c ConfigurationNode value returned by ConfigurationNode::getRoot().  ConfiguriatonNode Sub-nodes accessed by
 * operator[<key>] correspond to JSON objects values with the of the name <key>.  So, the code example above would
 * return "someStringValue" if the configuration was initialized with the following JSON document:
 * @code
 *     {
 *         "someComponent" : {
 *             "someSubComponent" : {
 *                 "someKey" : "someStringValue",
 *                 "keyToSomeBooleanValue" : true,
 *                 "keyToSomeIntegerValue" : 123
 *             }
 *         }
 *     }
 * @endcode
 */
class ConfigurationNode {
public:
    /**
     * Initialize the global configuration.
     *
     * @note If @c initialize() has already been called since startup or the latest call to uninitialize(), this
     * function will reject the request and return @c false.
     *
     * @param jsonConfigurationStreams Vector of @c istreams containing JSON documents from which to parse
     * configuration parameters. Streams are processed in the order they appear in the vector. When a
     * value appears in more than one JSON stream the last processed stream's value overwrites the previous value
     * (and a debug log entry will be created). This allows for specifying default settings (by providing them
     * first) and specifying the configuration from multiple sources (e.g. a separate stream for each component).
     * The resulting global configuration may be accessed from @c getRoot().
     *
     * @return Whether the initialization was successful.
     */
    static bool initialize(const std::vector<std::istream*>& jsonStreams);

    /**
     * Uninitialize the global configuration.
     *
     * @note Once this method has been called, all existing ConfigurationNode instances will become invalid.
     */
    static void uninitialize();

    /**
     * Get the root @c ConfigurationNode of the global configuration.
     *
     * @return The root @c ConfigurationNode of the global configuration.
     */
    static ConfigurationNode getRoot();

    /**
     * Constructor.
     */
    ConfigurationNode();

    /**
     * Get @c bool value for @c key from this @c ConfigurationNode.
     *
     * @param key The key of the @c bool value to get.
     * @param[out] out Pointer to receive the returned value.
     * @param defaultValue Default value to use if this @c ConfigurationNode does not have a @c bool value for @c key.
     * @c false if not specified.
     * @return Whether this @c ConfigurationNode has a @c bool value for @c key.
     */
    bool getBool(const std::string& key, bool* out = nullptr, bool defaultValue = false) const;

    /**
     * Get @c int value for @c key from this @c ConfigurationNode.
     *
     * @param key The key of the @c int value to get.
     * @param[out] out Pointer to receive the returned value.
     * @param defaultValue Default value to use if this @c ConfigurationNode does not have an @c int value for @c key.
     * Zero if not specified.
     * @return Whether this @c ConfigurationNode has an @c int value for @c key.
     */
    bool getInt(const std::string& key, int* out = nullptr, int defaultValue = 0) const;

    /**
     * Get the @c string value for @c key from this @c ConfigurationNode.
     *
     * @param key The key of the @c string value to get.
     * @param[out] out Pointer to receive the returned value.
     * @param defaultValue Default value to use if this @c ConfigurationNode does not have a @c string value for @c key.
     * The empty string if not specified.
     * @return Whether this @c ConfigurationNode has a @c string value for @c key.
     */
    bool getString(const std::string& key, std::string* out = nullptr, std::string defaultValue = "") const;

    /**
     * Get a duration value derived from an integer value for @c key from this @c ConfigurationNode.
     *
     * @tparam InputType std::chrono::duration type whose unit specifies how the integer value in the
     * JSON input stream is to be interpreted.
     * @tparam OutputType std::chrono::duration type specifying the type of the @c out parameter to this method.
     * @tparam DefaultType std::chrono::duration type specifying the type of the @c defaultValue to this method.
     * @param key The key of of the integer value to fetch and convert to @c OutputType.
     * @param out Pointer to receive the returned value.
     * @param defaultValue Default value to use if this @c ConfigurationNode does not have an integer value
     * value for @c key.  Zero if not specified.
     * @return Whether this @c ConfigurationNode has an integer value for @c key.
     */
    template <typename InputType, typename OutputType, typename DefaultType>
    bool getDuration(
        const std::string& key,
        OutputType* out = static_cast<std::chrono::seconds>(0),
        DefaultType defaultValue = std::chrono::seconds(0)) const;

    /**
     * operator[] to get @c ConfigurationNode value for @c key from this @c ConfigurationNode.
     *
     * @param key The key of the @c ConfigurationNode value to get.
     * @return The @c ConfigurationNode value, or an empty node if this @c ConfigurationNode does not have
     * a @c ConfigurationNode value for @c key.
     */
    ConfigurationNode operator[](const std::string& key) const;

    /**
     * operator bool(). Indicates of the @c ConfigurationNode references a valid object.
     *
     * @return Whether the @c ConfigurationNode references a valid object.
     */
    operator bool() const;

    /**
     * Common logic for getting a value of a specific type.
     *
     * @tparam Type The type to be gotten.
     * @param key The key of the value to get.
     * @param out Pointer to receive the value. May be nullptr to just test for the presence of the value.
     * @param defaultValue A default output value if no value of the desired type for @c key is present.
     * @param isType rapidjson::Value member function to test for the desired type.
     * @param getType rapidjson::Value member function to get the desired type.
     * @return Whether a value of the specified @c Type is present for @c key.
     */
    template <typename Type>
    bool getValue(
        const std::string& key,
        Type* out,
        Type defaultValue,
        bool (rapidjson::Value::*isType)() const,
        Type (rapidjson::Value::*getType)() const) const;

private:
    /**
     * Constructor.
     *
     * @param object @c rapidjson::Value of type @c rapidjson::Type::kObject within the global configuration that this
     * @c ConfigurationNode will represent.
     */
    ConfigurationNode(const rapidjson::Value* object);

    /**
     * Adapt between the public version of @c getString() (which fetches @c std::string) and the @c getValue()
     * template method (which fetches @c const @c char * from a @c rapidjson::Value).
     *
     * @param key The key of the @c string value to get.
     * @param[out] out Pointer to receive the returned value.
     * @param defaultValue Default value to use if this @c ConfigurationNode does not have a @c string value for @c key.
     * @return Whether this @c ConfigurationNode has a @c string value for @c key.
     */
    bool getString(const std::string& key, const char** out, const char* defaultValue) const;

    /// Object value within the global configuration that this @c ConfigurationNode represents.
    const rapidjson::Value* m_object;

    /**
     * Static mutex to serialize access to static values @c m_document and @c m_root.  This enables enforcing
     * that @c initialize() is only performed once after startup or the latest call to @c uninitialize().
     */
    static std::mutex m_mutex;

    /// static Document containing the global configuration.
    static rapidjson::Document m_document;

    /// static instance of @c ConfigurationNode identifying the root object within the global configuration.
    static ConfigurationNode m_root;
};

template <typename InputType, typename OutputType, typename DefaultType>
bool ConfigurationNode::getDuration(const std::string& key, OutputType* out, DefaultType defaultValue) const {
    int temp;
    auto result = getInt(key, &temp);
    if (out) {
        *out = OutputType(result ? InputType(temp) : defaultValue);
    }
    return result;
}

template <typename Type>
bool ConfigurationNode::getValue(
    const std::string& key,
    Type* out,
    Type defaultValue,
    bool (rapidjson::Value::*isType)() const,
    Type (rapidjson::Value::*getType)() const) const {
    if (key.empty() || !m_object) {
        if (out) {
            *out = defaultValue;
        }
        return false;
    }
    auto it = m_object->FindMember(key.c_str());
    if (m_object->MemberEnd() == it || !(it->value.*isType)()) {
        if (out) {
            *out = defaultValue;
        }
        return false;
    }
    if (out) {
        *out = (it->value.*getType)();
    }
    return true;
}

}  // namespace configuration
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_CONFIGURATION_CONFIGURATIONNODE_H_
