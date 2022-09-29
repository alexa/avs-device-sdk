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

#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>

#include "ACL/Transport/HTTP2TransportFactory.h"
#include "ACL/Transport/HTTP2Transport.h"

namespace alexaClientSDK {
namespace acl {

using namespace alexaClientSDK::avsCommon::utils;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::avs::attachment;

/// String to identify log entries originating from this file.
#define TAG "HTTP2TransportFactory"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<TransportFactoryInterface> HTTP2TransportFactory::createTransportFactoryInterface(
    const std::shared_ptr<avsCommon::utils::http2::HTTP2ConnectionFactoryInterface>& connectionFactory,
    const std::shared_ptr<PostConnectFactoryInterface>& postConnectFactory,
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder,
    const std::shared_ptr<avsCommon::sdkInterfaces::EventTracerInterface>& eventTracer) {
    if (!connectionFactory) {
        ACSDK_ERROR(LX("createTransportFactoryInterfaceFailed").d("reason", "nullConnectionFactory"));
        return nullptr;
    }
    if (!postConnectFactory) {
        ACSDK_ERROR(LX("createTransportFactoryInterfaceFailed").d("reason", "nullPostConnectFactory"));
        return nullptr;
    }
    return std::make_shared<HTTP2TransportFactory>(connectionFactory, postConnectFactory, metricRecorder, eventTracer);
}

std::shared_ptr<TransportInterface> HTTP2TransportFactory::createTransport(
    std::shared_ptr<AuthDelegateInterface> authDelegate,
    std::shared_ptr<AttachmentManagerInterface> attachmentManager,
    const std::string& avsGateway,
    std::shared_ptr<MessageConsumerInterface> messageConsumerInterface,
    std::shared_ptr<TransportObserverInterface> transportObserverInterface,
    std::shared_ptr<SynchronizedMessageRequestQueue> sharedMessageRequestQueue) {
    auto connection = m_connectionFactory->createHTTP2Connection();
    if (!connection) {
        return nullptr;
    }

    return HTTP2Transport::create(
        authDelegate,
        avsGateway,
        connection,
        messageConsumerInterface,
        attachmentManager,
        transportObserverInterface,
        m_postConnectFactory,
        sharedMessageRequestQueue,
        HTTP2Transport::Configuration(),
        m_metricRecorder,
        m_eventTracer);
}

HTTP2TransportFactory::HTTP2TransportFactory(
    std::shared_ptr<avsCommon::utils::http2::HTTP2ConnectionFactoryInterface> connectionFactory,
    std::shared_ptr<PostConnectFactoryInterface> postConnectFactory,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
    std::shared_ptr<avsCommon::sdkInterfaces::EventTracerInterface> eventTracer) :
        m_connectionFactory{std::move(connectionFactory)},
        m_postConnectFactory{std::move(postConnectFactory)},
        m_metricRecorder{std::move(metricRecorder)},
        m_eventTracer{eventTracer} {
}

}  // namespace acl
}  // namespace alexaClientSDK
