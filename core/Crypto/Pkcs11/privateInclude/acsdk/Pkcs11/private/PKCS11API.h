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

#ifndef ACSDK_PKCS11_PRIVATE_PKCS11API_H_
#define ACSDK_PKCS11_PRIVATE_PKCS11API_H_

/// @addtogroup Lib_acsdkPkcs11
/// @{

// Define platform-specific macros for PKCS#11 API (UNIX)
#define CK_PTR *
#define CK_DECLARE_FUNCTION(returnType, name) returnType name
#define CK_DECLARE_FUNCTION_POINTER(returnType, name) returnType(*name)
#define CK_CALLBACK_FUNCTION(returnType, name) returnType(*name)
#define NULL_PTR nullptr

/// @}

#ifdef _WIN32
#pragma pack(push, cryptoki, 1)
#endif

#include <pkcs11.h>

#ifdef _WIN32
#pragma pack(pop, cryptoki)
#endif

/// @brief Constant for object class initialization.
static constexpr CK_OBJECT_CLASS UNDEFINED_OBJECT_CLASS = static_cast<CK_OBJECT_CLASS>(-1);

/// @brief Constant for key type initialization.
static constexpr CK_KEY_TYPE UNDEFINED_KEY_TYPE = static_cast<CK_KEY_TYPE>(-1);

#endif  // ACSDK_PKCS11_PRIVATE_PKCS11API_H_
