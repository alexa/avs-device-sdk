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

/**
 * @file
 * This file contains wrappers macros to help with compatibility with MSVC, future operating system helper macros should
 * be placed here.
 */

#include <AVSCommon/Utils/SDKConfig.h>

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#if defined(_MSC_VER) && defined(ACSDK_CONFIG_SHARED_LIBS)
#if defined(IN_AVSCOMMON)
#define avscommon_EXPORT __declspec(dllexport)
#else
#define avscommon_EXPORT __declspec(dllimport)
#endif
#else
#define avscommon_EXPORT
#endif

#if defined(_MSC_VER)
#ifndef ACSDK_USE_RTTI
#define ACSDK_USE_RTTI ON
#endif
#endif

/**
 * @macro ACSDK_ALWAYS_INLINE
 *
 * Compiler-specific macro to disable cost-benefit analysis and always inline a method.
 */

/**
 * @macro ACSDK_NO_INLINE
 *
 * Compiler-specific macro to disable cost-benefit analysis and never inline a method.
 */

/**
 * @macro ACSDK_HIDDEN
 *
 * Compiler-specific macro to make symbol not visible outside of binary object.
 */

/**
 * @macro ACSDK_INTERNAL_LINKAGE
 *
 * Compiler-specific macro to make symbol not visible outside of the scope it is instantiated.
 */

/**
 * @macro ACSDK_HIDE_FROM_ABI
 * @brief Compiler-specific macro to exclude symbol from binary exports.
 *
 * Macro excludes symbols from binary export table. This helps to reduce binary size when building shared libraries.
 */

#if defined(_MSC_VER)
#define ACSDK_ALWAYS_INLINE __forceinline
#define ACSDK_NO_INLINE __declspec(noinline)
#define ACSDK_HIDDEN
#define ACSDK_INTERNAL_LINKAGE ACSDK_ALWAYS_INLINE
#elif defined(__GNUC__)

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

#define ACSDK_ALWAYS_INLINE inline __attribute__((__always_inline__))
#define ACSDK_NO_INLINE __attribute__((__noinline__))
#define ACSDK_HIDDEN __attribute__((__visibility__("hidden")))

#if defined(__clang__)
#if __has_attribute(internal_linkage)
#define ACSDK_INTERNAL_LINKAGE __attribute__((internal_linkage))
#else
#define ACSDK_INTERNAL_LINKAGE __attribute__((__visibility__("hidden")))
#endif
#else
#define ACSDK_INTERNAL_LINKAGE __attribute__((__visibility__("internal")))
#endif
#else
#define ACSDK_ALWAYS_INLINE inline
#define ACSDK_NO_INLINE
#define ACSDK_HIDDEN
#define ACSDK_INTERNAL_LINKAGE ACSDK_ALWAYS_INLINE
#endif
#define ACSDK_HIDE_FROM_ABI ACSDK_INTERNAL_LINKAGE

/**
 * @brief Compiler-specific macro for inline-only methods.
 *
 * Macro excludes symbols from binary export table. This helps to reduce binary size when building shared libraries.
 */
#define ACSDK_INLINE_VISIBILITY ACSDK_HIDE_FROM_ABI

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_PLATFORMDEFINITIONS_H_
