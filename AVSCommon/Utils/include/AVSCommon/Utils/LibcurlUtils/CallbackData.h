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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_CALLBACKDATA_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_CALLBACKDATA_H_

#include <cstddef>
#include <vector>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace libcurlUtils {

/**
 * Class to handle CURL callback data
 */
class CallbackData {
public:
    /**
     * Constructor.
     */
    CallbackData() = default;

    /**
     * Constructor that initializes with a value.
     */
    CallbackData(const char* data);

    /**
     * This will append new data to the existing instance.
     *
     * @param data The data that needs to be appended.
     * @param dataSize The size of the data to be appended.
     * @return The size of data that was appended.
     */
    size_t appendData(const char* data, size_t dataSize);

    /**
     * This will append new data to the existing instance.
     *
     * @param data The data that needs to be appended.
     * @return The size of data that was appended.
     */
    size_t appendData(const char* data);

    /**
     * This will clear the data.
     */
    void clearData();

    /**
     * Will return the callback data in the passed in container.
     *
     * @param dataContainer The container in which the callback data will be returned.
     * @param dataSize The expected size of the data to be returned.
     * @return Size of the returned callback data.
     */
    size_t getData(char* dataContainer, size_t dataSize);

    /**
     * Returns the callback data size.
     *
     * @return Callback data size.
     */
    size_t getSize();

private:
    /// Callback data
    std::vector<char> m_memory;
};

}  // namespace libcurlUtils
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_CALLBACKDATA_H_
