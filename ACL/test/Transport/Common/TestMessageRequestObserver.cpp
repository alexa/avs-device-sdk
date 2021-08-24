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

#include "TestMessageRequestObserver.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace observer {
namespace test {

void TestMessageRequestObserver::onSendCompleted(MessageRequestObserverInterface::Status status) {
    m_status.setValue(status);
}

void TestMessageRequestObserver::onExceptionReceived(const std::string& exceptionMessage) {
    m_exception.setValue(exceptionMessage);
}

}  // namespace test
}  // namespace observer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK