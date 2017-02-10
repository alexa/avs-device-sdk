/**
 * Executor.cpp
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "AVSUtils/Threading/Executor.h"

namespace alexaClientSDK {
namespace avsUtils {
namespace threading {

Executor::Executor() : m_taskQueue{std::make_shared<TaskQueue>()}, m_taskThread{m_taskQueue} {
    m_taskThread.start();
}

Executor::~Executor() {
    m_taskQueue->shutdown();
}

} // namespace threading
} // namespace acl
} // namespace alexaClientSDK
