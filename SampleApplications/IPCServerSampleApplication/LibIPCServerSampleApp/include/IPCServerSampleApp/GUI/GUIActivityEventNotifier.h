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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_GUIACTIVITYEVENTNOTIFIER_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_GUIACTIVITYEVENTNOTIFIER_H_

#include <memory>

#include <AVSCommon/SDKInterfaces/GUIActivityEvent.h>
#include <AVSCommon/SDKInterfaces/GUIActivityEventObserverInterface.h>
#include "IPCServerSampleApp/GUI/GUIActivityEventNotifierInterface.h"
#include <acsdk/Notifier/Notifier.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace gui {

/**
 * Manages all observers of @c GUIActivityEvent
 */
class GUIActivityEventNotifier
        : public gui::GUIActivityEventNotifierInterface
        , public notifier::Notifier<avsCommon::sdkInterfaces::GUIActivityEventObserverInterface> {
public:
    /**
     * Create an instance of @c GUIActivityEventNotifier
     * @return shared pointer to @c GUIActivityEventNotifier
     */
    static std::shared_ptr<GUIActivityEventNotifier> create();

    /// @c GUIActivityEventNotifierInterface Functions
    /// @{
    void notifyObserversOfGUIActivityEvent(
        const std::string& source,
        const avsCommon::sdkInterfaces::GUIActivityEvent& activityEvent) override;
    /// @}

private:
    /**
     * Constructor
     */
    GUIActivityEventNotifier();
};

}  // namespace gui
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_GUIACTIVITYEVENTNOTIFIER_H_
