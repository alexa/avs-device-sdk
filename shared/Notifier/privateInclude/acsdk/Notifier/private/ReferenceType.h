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

#ifndef ACSDK_NOTIFIER_PRIVATE_REFERENCETYPE_H_
#define ACSDK_NOTIFIER_PRIVATE_REFERENCETYPE_H_

namespace alexaClientSDK {
namespace notifier {

/**
 * @brief Type of managed reference.
 *
 * @ingroup Lib_acsdkNotifier
 * @private
 */
enum class ReferenceType {
    /// Wrapper is empty.
    None = 1,
    /// Wrapper is using std::shared_ptr
    StrongRef,
    /// Wrapper is using std::weak_ptr
    WeakRef,
};

}  // namespace notifier
}  // namespace alexaClientSDK

#endif  // ACSDK_NOTIFIER_PRIVATE_REFERENCETYPE_H_
