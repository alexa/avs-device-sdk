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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_GUIACTIVITYEVENTNOTIFIERINTERFACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_GUIACTIVITYEVENTNOTIFIERINTERFACE_H_

#include <AVSCommon/SDKInterfaces/GUIActivityEvent.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace gui {

/**
 * Notifier interface for @c GUIActivityEvent
 */
class GUIActivityEventNotifierInterface {
public:
    /**
     * Notify observers of activity event
     * @param source The source of the activity event.
     * @param activityEvent The activity event.
     */
    virtual void notifyObserversOfGUIActivityEvent(
        const std::string& source,
        const avsCommon::sdkInterfaces::GUIActivityEvent& activityEvent) = 0;
};

}  // namespace gui
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_GUIACTIVITYEVENTNOTIFIERINTERFACE_H_
