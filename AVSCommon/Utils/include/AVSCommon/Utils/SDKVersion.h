/*
 * SDKVersion.h
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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


#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_SDKVERSION_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_SDKVERSION_H_

#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

/// These functions are responsible for providing access to the current SDK version.
/// NOTE: To make changes to this file you *MUST* do so via SDKVersion.h.in.
namespace sdkVersion{

inline static std::string getCurrentVersion(){
	return "0.0.0";
}

inline static int getMajorVersion(){
	return 0;
}

inline static int getMinorVersion(){
	return 0;
}

inline static int getPatchVersion(){
	return 0;
}



}  // namespace sdkVersion
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_SDKVERSION_H_
