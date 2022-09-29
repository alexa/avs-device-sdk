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

#ifndef ACSDKASSETSINTERFACES_RESULTCODE_H_
#define ACSDKASSETSINTERFACES_RESULTCODE_H_

namespace alexaClientSDK {
namespace acsdkAssets {
namespace commonInterfaces {

// Result codes values here are a match between DAVS server codes and davs client codes
enum class ResultCode {
    CONNECTION_FAILED = 47,
    CONNECTION_TIMED_OUT = -51,
    CHECKSUM_MISMATCH = -52,
    NO_SPACE_AVAILABLE = -53,
    SUCCESS = 200,
    UP_TO_DATE = 304,
    ILLEGAL_ARGUMENT = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NO_ARTIFACT_FOUND = 404,
    UNPACK_FAILURE = -997,
    UNHANDLED_MIME_TYPE = -998,
    CATASTROPHIC_FAILURE = -999
};

inline std::ostream& operator<<(std::ostream& os, ResultCode result) {
    switch (result) {
        case ResultCode::CONNECTION_FAILED:
            return os << "CONNECTION_FAILED";
        case ResultCode::CONNECTION_TIMED_OUT:
            return os << "CONNECTION_TIMED_OUT";
        case ResultCode::CHECKSUM_MISMATCH:
            return os << "CHECKSUM_MISMATCH";
        case ResultCode::NO_SPACE_AVAILABLE:
            return os << "NO_SPACE_AVAILABLE";
        case ResultCode::SUCCESS:
            return os << "SUCCESS";
        case ResultCode::UP_TO_DATE:
            return os << "UP_TO_DATE";
        case ResultCode::ILLEGAL_ARGUMENT:
            return os << "ILLEGAL_ARGUMENT";
        case ResultCode::UNAUTHORIZED:
            return os << "UNAUTHORIZED";
        case ResultCode::FORBIDDEN:
            return os << "FORBIDDEN";
        case ResultCode::NO_ARTIFACT_FOUND:
            return os << "NO_ARTIFACT_FOUND";
        case ResultCode::UNHANDLED_MIME_TYPE:
            return os << "UNHANDLED_MIME_TYPE";
        case ResultCode::CATASTROPHIC_FAILURE:
            return os << "CATASTROPHIC_FAILURE";
        case ResultCode::UNPACK_FAILURE:
            return os << "UNPACK_FAILURE";
    }
    return os;
}

}  // namespace commonInterfaces
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETSINTERFACES_RESULTCODE_H_
