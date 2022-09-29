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

#include <memory>
#include <string>

#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "IPCServerSampleApp/GUI/TemplateRuntimePresentationAdapterBridge.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace gui {

using namespace avsCommon::utils::json;

#define TAG "TemplateRuntimePresentationAdapterBridge"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<TemplateRuntimePresentationAdapterBridge> TemplateRuntimePresentationAdapterBridge::create(
    const std::shared_ptr<sampleApplications::common::TemplateRuntimePresentationAdapter>&
        templateRuntimePresentationAdapter) {
    if (!templateRuntimePresentationAdapter) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullTemplateRuntimePresentationAdapter"));
        return nullptr;
    }

    auto templateRuntimePresentationAdapterBridge = std::shared_ptr<TemplateRuntimePresentationAdapterBridge>(
        new TemplateRuntimePresentationAdapterBridge(templateRuntimePresentationAdapter));

    return templateRuntimePresentationAdapterBridge;
}

TemplateRuntimePresentationAdapterBridge::TemplateRuntimePresentationAdapterBridge(
    std::shared_ptr<sampleApplications::common::TemplateRuntimePresentationAdapter>
        templateRuntimePresentationAdapter) :
        m_templateRuntimePresentationAdapter{std::move(templateRuntimePresentationAdapter)} {
}

bool TemplateRuntimePresentationAdapterBridge::setRenderPlayerInfoWindowId(
    const std::string& renderPlayerInfoWindowId) {
    if (m_renderPlayerInfoWindowId.empty()) {
        m_templateRuntimePresentationAdapter->setRenderPlayerInfoWindowId(renderPlayerInfoWindowId);
        return true;
    } else {
        ACSDK_WARN(LX(__func__).d("reason", "player info window id already set by TemplateRuntime windowIdReport."));
        return false;
    }
}

std::string TemplateRuntimePresentationAdapterBridge::getRenderPlayerInfoWindowId() {
    return m_renderPlayerInfoWindowId;
}

void TemplateRuntimePresentationAdapterBridge::windowIdReport(const std::string& message) {
    rapidjson::Document payload;
    if (!jsonUtils::parseJSON(message, &payload)) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalid message string"));
        return;
    }

    std::string renderTemplateWindowId;
    if (!jsonUtils::retrieveValue(message, ipc::RENDER_TEMPLATE_WINDOW_ID_TAG, &renderTemplateWindowId)) {
        ACSDK_WARN(LX(__func__).d("reason", "render template window ID not found"));
    } else {
        m_templateRuntimePresentationAdapter->setRenderTemplateWindowId(renderTemplateWindowId);
    }

    std::string renderPlayerInfoWindowId;
    if (!jsonUtils::retrieveValue(message, ipc::RENDER_PLAYER_INFO_WINDOW_ID_TAG, &renderPlayerInfoWindowId)) {
        ACSDK_WARN(LX(__func__).d("reason", "render player info window ID not found"));
    } else {
        m_renderPlayerInfoWindowId = renderPlayerInfoWindowId;
        m_templateRuntimePresentationAdapter->setRenderPlayerInfoWindowId(m_renderPlayerInfoWindowId);
    }
}

}  // namespace gui
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
