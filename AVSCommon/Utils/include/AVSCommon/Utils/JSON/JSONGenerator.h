/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_JSON_JSONGENERATOR_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_JSON_JSONGENERATOR_H_

#include <cstdint>
#include <string>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace json {

/**
 * Utility class that can be used to build a json string.
 *
 * E.g.: To build the following json: {"param1":"value","param2":{"param2.1":100}}. Use:
 *
 * JsonGenerator generator;
 * generator.addMember("param1", "value");
 * generator.startObject("param2");
 * generator.addMember("param2.1", 100);
 * generator.toString();
 *
 * For debugging purpose, you may be able to get the partial string by calling:
 *
 * generator.toString(false)
 *
 * @note This class is NOT thread safe.
 * @note This class doesn't support arrays yet.
 */
class JsonGenerator {
public:
    /**
     * Constructor.
     */
    JsonGenerator();

    /**
     * Default destructor.
     */
    ~JsonGenerator() = default;

    /**
     * Checks whether the generator has been finalized (i.e., no changes can be made to the current json).
     *
     * @return @c true if it has been finalized, @c false otherwise
     */
    bool isFinalized();

    /**
     * Starts a new json object with the given key.
     *
     * @param key The new object name.
     * @return @c true if it succeeds to create a new object and @c false if it fails.
     */
    bool startObject(const std::string& key);

    /**
     * Close the last object that has been opened.
     *
     * @return @c true if the last object was closed @c false if it fails.
     */
    bool finishObject();

    ///@{
    /**
     * Add a new member with the key and value.
     *
     * @param key The name of the member.
     * @param value The value of the member.
     * @return @c true if it succeeded to add the new member and @c false otherwise.
     */
    bool addMember(const std::string& key, const char* value);
    bool addMember(const std::string& key, const std::string& value);
    bool addMember(const std::string& key, int64_t value);
    bool addMember(const std::string& key, uint64_t value);
    bool addMember(const std::string& key, int value);
    bool addMember(const std::string& key, unsigned int value);
    bool addMember(const std::string& key, bool value);
    ///@}

    /**
     * Adds a raw json as a value to the given key.
     *
     * @param key The object key to the raw json provided.
     * @param json A string representation of a @b valid json.
     * @param validate Enable json validation for the raw json parameter.
     * @return @c true if it succeeded to add the raw json and @c false otherwise.
     */
    bool addRawJsonMember(const std::string& key, const std::string& json, bool validate = true);

    /**
     * Return the string representation of the object.
     *
     * @param finalize If set to @c true the object will be finalized and the string returned will be a complete json
     * document. If @c false, the returned string will represent the current state of the json generation which could
     * be partial.
     * @note Once the object has been finalized, no changes can be made to the generator.
     * @return The string representation of the json object.
     */
    std::string toString(bool finalize = true);

private:
    /// Checks if the writer is still open and ready to be used.
    inline bool checkWriter();

    /**
     * Method used to finalize the json. This will close all the open objects including the root object.
     *
     * @return @c true if it succeeded to finalize the generator.
     */
    bool finalize();

    /// The string buffer used to store the json string.
    rapidjson::StringBuffer m_buffer;

    /// The json writer.
    rapidjson::Writer<rapidjson::StringBuffer> m_writer;
};

}  // namespace json
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_JSON_JSONGENERATOR_H_
