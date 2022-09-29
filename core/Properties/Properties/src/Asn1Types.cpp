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

#include <acsdk/Properties/private/Asn1Types.h>

namespace alexaClientSDK {
namespace properties {

ASN1_SEQUENCE(ACSDK_ENC_INFO) = {
    ASN1_EXP_OPT(ACSDK_ENC_INFO, version, ASN1_INTEGER, 0),
    ASN1_SIMPLE(ACSDK_ENC_INFO, mainKeyAlias, ASN1_UTF8STRING),
    ASN1_SIMPLE(ACSDK_ENC_INFO, mainKeyChecksum, ASN1_OCTET_STRING),
    ASN1_EXP_OPT(ACSDK_ENC_INFO, dataKeyAlgorithm, ASN1_INTEGER, 1),
    ASN1_SIMPLE(ACSDK_ENC_INFO, dataKeyIV, ASN1_OCTET_STRING),
    ASN1_SIMPLE(ACSDK_ENC_INFO, dataKeyCiphertext, ASN1_OCTET_STRING),
    ASN1_SIMPLE(ACSDK_ENC_INFO, dataKeyTag, ASN1_OCTET_STRING),
    ASN1_EXP_OPT(ACSDK_ENC_INFO, dataAlgorithm, ASN1_INTEGER, 2),
} ASN1_SEQUENCE_END(ACSDK_ENC_INFO);
IMPLEMENT_ASN1_FUNCTIONS(ACSDK_ENC_INFO);

ASN1_SEQUENCE(ACSDK_ENC_PROP) = {
    ASN1_SIMPLE(ACSDK_ENC_PROP, encryptionInfo, ACSDK_ENC_INFO),
    ASN1_EXP_OPT(ACSDK_ENC_PROP, digestAlgorithm, ASN1_INTEGER, 0),
    ASN1_SIMPLE(ACSDK_ENC_PROP, digest, ASN1_OCTET_STRING),
} ASN1_SEQUENCE_END(ACSDK_ENC_PROP);
IMPLEMENT_ASN1_FUNCTIONS(ACSDK_ENC_PROP);

ASN1_SEQUENCE(ACSDK_DATA_INFO) = {
    ASN1_EXP_OPT(ACSDK_DATA_INFO, version, ASN1_INTEGER, 0),
    ASN1_SIMPLE(ACSDK_DATA_INFO, dataIV, ASN1_OCTET_STRING),
    ASN1_SIMPLE(ACSDK_DATA_INFO, dataCiphertext, ASN1_OCTET_STRING),
    ASN1_SIMPLE(ACSDK_DATA_INFO, dataTag, ASN1_OCTET_STRING),
} ASN1_SEQUENCE_END(ACSDK_DATA_INFO);
IMPLEMENT_ASN1_FUNCTIONS(ACSDK_DATA_INFO);

ASN1_SEQUENCE(ACSDK_DATA_PROP) = {
    ASN1_SIMPLE(ACSDK_DATA_PROP, dataInfo, ACSDK_DATA_INFO),
    ASN1_EXP_OPT(ACSDK_DATA_PROP, digestAlgorithm, ASN1_INTEGER, 0),
    ASN1_SIMPLE(ACSDK_DATA_PROP, digest, ASN1_OCTET_STRING),
} ASN1_SEQUENCE_END(ACSDK_DATA_PROP);
IMPLEMENT_ASN1_FUNCTIONS(ACSDK_DATA_PROP);

}  // namespace properties
}  // namespace alexaClientSDK
