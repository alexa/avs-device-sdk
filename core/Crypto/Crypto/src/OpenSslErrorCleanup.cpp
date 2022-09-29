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

#include <openssl/err.h>

#include <acsdk/Crypto/private/OpenSslErrorCleanup.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace crypto {

using ::alexaClientSDK::avsCommon::utils::logger::LogEntry;

OpenSslErrorCleanup::OpenSslErrorCleanup(const std::string& logTag) noexcept :
        avsCommon::utils::error::FinallyGuard{std::bind(OpenSslErrorCleanup::clearAndLogOpenSslErrors, logTag)} {
}

void OpenSslErrorCleanup::clearAndLogOpenSslErrors(const std::string& logTag) noexcept {
    unsigned long opensslErrorCode;
    while ((opensslErrorCode = ERR_get_error())) {
        char opensslErrorStr[256];
        ERR_error_string_n(opensslErrorCode, opensslErrorStr, sizeof(opensslErrorStr));
        ACSDK_DEBUG0(LogEntry(logTag, "opensslError").m(opensslErrorStr));
    }
}

}  // namespace crypto
}  // namespace alexaClientSDK
