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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLVIEWHOSTINTERFACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLVIEWHOSTINTERFACE_H_

#include <chrono>
#include <memory>
#include <rapidjson/document.h>
#include <string>

#include <acsdk/APLCapabilityCommonInterfaces/PresentationSession.h>
#include <acsdk/APLCapabilityCommonInterfaces/VisualStateProviderInterface.h>
#include <AVSCommon/SDKInterfaces/MediaPropertiesInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include "APLViewhostObserverInterface.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

/**
 * This interface is intended to define a well defined surface for APLCore / APLViewhost integrations.
 */
class APLViewhostInterface : public aplCapabilityCommonInterfaces::VisualStateProviderInterface {
public:
    /**
     * Destructor
     */
    virtual ~APLViewhostInterface() = default;

    /**
     * Adds a viewhost observer
     * @param observer the observer to register
     */
    virtual void addObserver(const APLViewhostObserverInterfacePtr& observer) = 0;

    /**
     * Removes a viewhost observer.
     * @param observer the observer to unregister
     */
    virtual void removeObserver(const APLViewhostObserverInterfacePtr& observer) = 0;

    /**
     * Render an APL document with extra associated data payload
     * @param presentationSession The presentation session associated with this renderDocument request
     * @param document JSON string containing APL document
     * @param data JSON string containing data payload associated with APL document
     * @param token Presentation token uniquely identifying the document
     * @param supportedViewports Viewports supported by this document
     * @param windowId Target windowId to display the document within
     */
    virtual void renderDocument(
        const aplCapabilityCommonInterfaces::PresentationSession& presentationSession,
        const std::string& token,
        const std::string& document,
        const std::string& datasources,
        const std::string& supportedViewports,
        const std::string& windowId) = 0;

    /**
     * Clear APL document
     * @deprecated provide an explicit token instead
     */
    virtual void clearDocument() {
        clearDocument("");
    }

    /**
     * Clear APL document
     * @param token token of document to clear
     */
    virtual void clearDocument(const std::string& token) {
        // no-op by default for backwards compatibility
    }

    /**
     * Execute commands referenced in APL document
     * @param document JSON string containing APL command to execute
     * @param token Presentation token to identify document to execute commands on
     */
    virtual void executeCommands(const std::string& commands, const std::string& token) = 0;

    /**
     * Update the data source payload for a given APL document
     * @param sourceType APL spec source type for data update
     * @param data JSON string containing new data payload associated with APL document
     * @param token Presentation token to identify document to update
     */
    virtual void dataSourceUpdate(const std::string& sourceType, const std::string& data, const std::string& token) = 0;

    /**
     * Interrupt active command sequence
     * @deprecated provide an explicit token instead
     */
    virtual void interruptCommandSequence() {
        interruptCommandSequence("");
    }

    /**
     * Interrupt active command sequence
     * @param token token of document to interrupt
     */
    virtual void interruptCommandSequence(const std::string& token) {
        // no-op by default for backwards compatibility
    }

    /**
     * Used for metrics purposes, notifies the APLRuntime of the time when the directive was received
     * @param token token of the document
     * @param receiveTime The receive time
     */
    virtual void onRenderDirectiveReceived(
        const std::string& token,
        const std::chrono::steady_clock::time_point& receiveTime){
        // no-op by default for compatibility
    };

    /**
     * Retrieves the maximum APL version supported by this runtime
     * @returns the max supported APL version
     */
    virtual std::string getMaxAPLVersion() const = 0;

    /**
     * Sets the metrics recorder to be used by the runtime to record and emit metric events.
     * @param metricRecorder Shared pointer to the @c MetricRecorderInterface
     */
    virtual void setMetricRecorder(
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) = 0;

    /**
     * Handle back navigation
     * @param windowId The window ID on which to perform the back action
     * @return true if the viewhost can handle back, false otherwise
     */
    virtual bool handleBack(const std::string& windowId) = 0;
};

using APLViewhostInterfacePtr = std::shared_ptr<APLViewhostInterface>;
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLVIEWHOSTINTERFACE_H_
