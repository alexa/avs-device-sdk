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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_LIBCURLSETCURLOPTIONSCALLBACKINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_LIBCURLSETCURLOPTIONSCALLBACKINTERFACE_H_

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace libcurlUtils {
class CurlEasyHandleWrapperOptionsSettingAdapter;

/**
 * This class allows to pass a user defined callback function for setting curl options when a new connection is created.
 * The callback will be implemented in the applciation layer.
 *
 *
 * @see https://curl.haxx.se/libcurl/c/curl_easy_setopt.html
 */
class LibcurlSetCurlOptionsCallbackInterface {
public:
    /**
     * Destructor.
     */
    virtual ~LibcurlSetCurlOptionsCallbackInterface() = default;

    /**
     *
     * @param CurlEasyHandleWrapperOptionsSettingAdapter that allows setting curl options on a curl handle.
     *
     * @return true if the callback processing is successful, else false.
     */
    virtual bool processCallback(CurlEasyHandleWrapperOptionsSettingAdapter& optionsSetter) = 0;
};

}  // namespace libcurlUtils
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_LIBCURLSETCURLOPTIONSCALLBACKINTERFACE_H_
