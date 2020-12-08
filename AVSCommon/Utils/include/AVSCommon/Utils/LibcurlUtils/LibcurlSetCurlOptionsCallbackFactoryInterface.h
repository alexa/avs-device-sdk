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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_LIBCURLSETCURLOPTIONSCALLBACKFACTORYINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_LIBCURLSETCURLOPTIONSCALLBACKFACTORYINTERFACE_H_

#include <memory>

#include <AVSCommon/Utils/LibcurlUtils/LibcurlSetCurlOptionsCallbackInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace libcurlUtils {

/**
 * A class that creates a new instance of @c LibcurlSetCurlOptionsCallbackInterface.
 */
class LibcurlSetCurlOptionsCallbackFactoryInterface {
public:
    /**
     * Destructor.
     */
    virtual ~LibcurlSetCurlOptionsCallbackFactoryInterface() = default;

    /**
     * Create an instance of @c LibcurlSetCurlOptionsCallbackInterface.
     *
     * @return An instance of @c LibcurlSetCurlOptionsCallbackInterface.
     */
    virtual std::shared_ptr<LibcurlSetCurlOptionsCallbackInterface> createSetCurlOptionsCallback() = 0;
};

}  // namespace libcurlUtils
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_LIBCURLSETCURLOPTIONSCALLBACKFACTORYINTERFACE_H_
