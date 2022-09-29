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

#include "SynchronizeStateSender/PostConnectSynchronizeStateSender.h"
#include "SynchronizeStateSender/SynchronizeStateSenderFactory.h"

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace synchronizeStateSender {

using namespace avsCommon::sdkInterfaces;

/// String to identify log entries originating from this file.
#define TAG "SynchronizeStateSenderFactory"

/**
 * Create a LogEntry using the file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationProviderInterface> SynchronizeStateSenderFactory::
    createPostConnectOperationProviderInterface(
        const std::shared_ptr<
            acsdkPostConnectOperationProviderRegistrarInterfaces::PostConnectOperationProviderRegistrarInterface>&
            providerRegistrar,
        const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
        const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder) {
    ACSDK_DEBUG5(LX(__func__));
    if (!providerRegistrar) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullProviderRegistrar"));
        return nullptr;
    }
    if (!contextManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullContextManager"));
        return nullptr;
    }
    std::shared_ptr<SynchronizeStateSenderFactory> provider(
        new SynchronizeStateSenderFactory(contextManager, metricRecorder));
    if (!providerRegistrar->registerProvider(provider)) {
        return nullptr;
    }
    return provider;
}

std::shared_ptr<SynchronizeStateSenderFactory> SynchronizeStateSenderFactory::create(
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) {
    ACSDK_DEBUG5(LX(__func__));
    if (!contextManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullContextManager"));
    } else {
        return std::shared_ptr<SynchronizeStateSenderFactory>(
            new SynchronizeStateSenderFactory(contextManager, metricRecorder));
    }
    return nullptr;
}

SynchronizeStateSenderFactory::SynchronizeStateSenderFactory(
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) :
        m_contextManager{contextManager},
        m_metricRecorder{metricRecorder} {
}

std::shared_ptr<PostConnectOperationInterface> SynchronizeStateSenderFactory::createPostConnectOperation() {
    ACSDK_DEBUG5(LX(__func__));
    return PostConnectSynchronizeStateSender::create(m_contextManager, m_metricRecorder);
}

}  // namespace synchronizeStateSender
}  // namespace alexaClientSDK
