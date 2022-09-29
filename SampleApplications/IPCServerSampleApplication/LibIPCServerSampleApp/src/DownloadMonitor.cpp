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

#include "IPCServerSampleApp/DownloadMonitor.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

DownloadMonitor::DownloadMonitor(APLClient::Telemetry::DownloadMetricsEmitterPtr metricsEmitter) :
        m_metricsEmitter{metricsEmitter} {
}

void DownloadMonitor::onDownloadStarted() {
    m_metricsEmitter->onDownloadStarted();
}

void DownloadMonitor::onDownloadComplete() {
    m_metricsEmitter->onDownloadComplete();
}

void DownloadMonitor::onDownloadFailed() {
    m_metricsEmitter->onDownloadFailed();
}

void DownloadMonitor::onCacheHit() {
    m_metricsEmitter->onCacheHit();
}

void DownloadMonitor::onBytesRead(uint64_t numberOfBytes) {
    m_metricsEmitter->onBytesRead(numberOfBytes);
}

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
