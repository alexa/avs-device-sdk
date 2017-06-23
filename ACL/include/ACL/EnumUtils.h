/*
 * EnumUtils.h
 *
 * Copyright 2016-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_ENUM_UTILS_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_ENUM_UTILS_H_


#include <ostream>
#include <string>
#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>

namespace alexaClientSDK {
namespace acl {

/**
 * Utility function to convert a modern enum class to a string.
 *
 * @param status The enum value
 * @return The string representation of the incoming value.
 */
std::string connectionStatusToString(avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status status);

/**
 * Utility function to convert a modern enum class to a string.
 *
 * @param status The enum value
 * @return The string representation of the incoming value.
 */
std::string connectionChangedReasonToString(
        avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason reason);

} // namespace acl
} // namespace alexaClientSDK

/**
 * Write a @c Status value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param status The Status value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator << (
            std::ostream& stream, 
            alexaClientSDK::avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status status) {
    return stream << alexaClientSDK::acl::connectionStatusToString(status);
}

/**
 * Write a @c ChangedReason value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param reason The ChangedReason value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator << (
            std::ostream& stream, 
            alexaClientSDK::avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason reason) {
    return stream << alexaClientSDK::acl::connectionChangedReasonToString(reason);
}

#endif // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_ENUM_UTILS_H_
