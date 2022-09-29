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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_TEMPLATERUNTIMEPRESENTATIONADAPTERBRIDGE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_TEMPLATERUNTIMEPRESENTATIONADAPTERBRIDGE_H_

#include <string>

#include <acsdk/Sample/TemplateRuntime/TemplateRuntimePresentationAdapter.h>
#include "IPCServerSampleApp/IPC/HandlerInterfaces/TemplateRuntimeHandlerInterface.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace gui {

/**
 * Bridge for binding TemplateRuntime IPC messages to the @c TemplateRuntimePresentationAdapter
 */
class TemplateRuntimePresentationAdapterBridge
        : public ipc::TemplateRuntimeHandlerInterface
        , public std::enable_shared_from_this<TemplateRuntimePresentationAdapterBridge> {
public:
    /**
     * Create an instance of @c TemplateRuntimePresentationAdapterBridge
     *
     * @param templateRuntimePresentationAdapter Pointer to the @c TemplateRuntimePresentationAdapter.
     * @return Shared pointer to @c TemplateRuntimePresentationAdapterBridge
     */
    static std::shared_ptr<TemplateRuntimePresentationAdapterBridge> create(
        const std::shared_ptr<sampleApplications::common::TemplateRuntimePresentationAdapter>&
            templateRuntimePresentationAdapter);

    /// @name TemplateRuntimeHandlerInterface methods
    /// @{
    void windowIdReport(const std::string& message) override;
    /// }

    /**
     * Sets the render player info window id on the adapter if not explicitly reported by the client via the
     * associated TemplateRuntime windowIdReport.
     * @param renderPlayerInfoWindowId the window id to use for player info window if not set by client.
     * @return returns true if the bridge accepts the explicitly set player info window id.
     */
    bool setRenderPlayerInfoWindowId(const std::string& renderPlayerInfoWindowId);

    /**
     * Returns the render player info window id used by the adapter bridge for presentations.
     * @return the render player info window id.
     */
    std::string getRenderPlayerInfoWindowId();

private:
    /**
     * Constructor.
     *
     * @param templateRuntimePresentationAdapter Pointer to the @c TemplateRuntimePresentationAdapter.
     */
    TemplateRuntimePresentationAdapterBridge(
        std::shared_ptr<sampleApplications::common::TemplateRuntimePresentationAdapter>
            templateRuntimePresentationAdapter);

    /// The @c TemplateRuntimePresentationAdapter
    std::shared_ptr<sampleApplications::common::TemplateRuntimePresentationAdapter>
        m_templateRuntimePresentationAdapter;

    /// Cached value of the reported render player info window id
    std::string m_renderPlayerInfoWindowId;
};

}  // namespace gui
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_TEMPLATERUNTIMEPRESENTATIONADAPTERBRIDGE_H_
