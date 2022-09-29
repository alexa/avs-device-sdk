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

#ifndef ACSDK_PROPERTIES_PRIVATE_ASN1TYPES_H_
#define ACSDK_PROPERTIES_PRIVATE_ASN1TYPES_H_

#include <openssl/asn1t.h>

namespace alexaClientSDK {
namespace properties {

/// @addtogroup Lib_acsdkProperties
/// @{

/*-
 * EncryptionDataVersion ::= INTEGER { v1(1) }
 */
constexpr int64_t ACSDK_DATA_KEY_VER_V1 = 1;
/*-
 * DataVersion ::= INTEGER { v1(1) }
 */
constexpr int64_t ACSDK_DATA_VER_V1 = 1;

/*-
 * CipherAlgorithm ::= INTEGER { aes_256_gcm(1) }
 */
constexpr int64_t ACSDK_CIP_ALG_AES_256_GCM = 1;

/*-
 * DigestAlgorithm ::= INTEGER { sha_256(1) }
 */
constexpr int64_t ACSDK_DIG_ALG_SHA_256 = 1;

/*-
 * EncryptionData ::= SEQUENCE {
 *   version           [0] EncryptionDataVersion DEFAULT v1,
 *   mainKeyAlias          OCTET STRING,
 *   mainKeyChecksum       OCTET STRING,
 *   dataKeyAlgorithm  [1] CipherAlgorithm DEFAULT aes_256_gcm,
 *   dataKeyIV             OCTET STRING,
 *   dataKeyCiphertext     OCTET STRING,
 *   dataAlgorithm     [2] CipherAlgorithm DEFAULT aes_256_gcm
 *   dataKeyTag        [3] OCTET STRING,
 * }
 */
/// Data structure to produce and parse DER for encryption key property data.
typedef struct EncryptionInfo {
    /// @brief Default value for optional version.
    constexpr static int64_t DEF_VER = ACSDK_DATA_KEY_VER_V1;
    /// @brief Default value for optional data key encryption algorithm.
    constexpr static int64_t DEF_DATA_KEY_ALG = ACSDK_CIP_ALG_AES_256_GCM;
    /// @brief Default value for optional data encryption algorithm.
    constexpr static int64_t DEF_DATA_ALG = ACSDK_CIP_ALG_AES_256_GCM;

    ASN1_INTEGER* version;  // Optional
    ASN1_UTF8STRING* mainKeyAlias;
    ASN1_OCTET_STRING* mainKeyChecksum;
    ASN1_INTEGER* dataKeyAlgorithm;  // Optional
    ASN1_OCTET_STRING* dataKeyIV;
    ASN1_OCTET_STRING* dataKeyCiphertext;
    ASN1_OCTET_STRING* dataKeyTag;
    ASN1_INTEGER* dataAlgorithm;  // Optional
} ACSDK_ENC_INFO;
DECLARE_ASN1_FUNCTIONS(ACSDK_ENC_INFO);

/*-
 * EncryptionProperty ::= SEQUENCE {
 *   encryptionData         EncryptionData,
 *   digestAlgorithm   [0]  DigestAlgorithm DEFAULT sha_256,
 *   digest                 OCTET STRING
 * }
 */
/// Data structure to produce and parse DER for encryption key property data.
typedef struct EncryptionProperty {
    constexpr static int DEF_DIG_ALG = ACSDK_DIG_ALG_SHA_256;

    ACSDK_ENC_INFO* encryptionInfo;
    ASN1_INTEGER* digestAlgorithm;  // Optional
    ASN1_OCTET_STRING* digest;
} ACSDK_ENC_PROP;
DECLARE_ASN1_FUNCTIONS(ACSDK_ENC_PROP);

/*-
 * DataInfo ::= SEQUENCE {
 *   version        [0] DataVersion DEFAULT v1,
 *   dataIV             OCTET STRING,
 *   dataCiphertext     OCTET STRING,
 *   dataTag        [1] OCTET STRING
 * }
 */
/// Data structure to produce and parse DER for encrypted property data.
typedef struct DataInfo {
    /// Default value for optional version.
    constexpr static int64_t DEF_VER = ACSDK_DATA_VER_V1;

    ASN1_INTEGER* version;  // Optional
    ASN1_OCTET_STRING* dataIV;
    ASN1_OCTET_STRING* dataCiphertext;
    ASN1_OCTET_STRING* dataTag;
} ACSDK_DATA_INFO;
DECLARE_ASN1_FUNCTIONS(ACSDK_DATA_INFO);

/*-
 * DataProperty ::= SEQUENCE {
 *   dataInfo               DataInfo,
 *   digestAlgorithm   [0]  DigestAlgorithm DEFAULT sha_256,
 *   digest                 OCTET STRING
 * }
 */
/// Data structure to produce and parse DER for encrypted property data.
typedef struct DataProperty {
    constexpr static int DEF_DIG_ALG = ACSDK_DIG_ALG_SHA_256;
    ACSDK_DATA_INFO* dataInfo;
    ASN1_INTEGER* digestAlgorithm;  // Optional
    ASN1_OCTET_STRING* digest;
} ACSDK_DATA_PROP;
DECLARE_ASN1_FUNCTIONS(ACSDK_DATA_PROP);

/// @}

}  // namespace properties
}  // namespace alexaClientSDK

#endif  // ACSDK_PROPERTIES_PRIVATE_ASN1TYPES_H_
