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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_PLATFORMDEFINITIONS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_PLATFORMDEFINITIONS_H_

/*
This file contains wrappers macros to help with compatibility with MSVC,
future operating system helper macros should be placed here.
*/

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#if defined(_MSC_VER)
#if defined(IN_AVSCOMMON)
#define avscommon_EXPORT __declspec(dllexport)
#else
#define avscommon_EXPORT __declspec(dllimport)
#endif
#else
#define avscommon_EXPORT
#endif

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_PLATFORMDEFINITIONS_H_
