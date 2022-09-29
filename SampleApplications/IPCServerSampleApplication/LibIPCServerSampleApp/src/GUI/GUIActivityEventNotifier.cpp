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

#include <AVSCommon/SDKInterfaces/GUIActivityEventObserverInterface.h>

#include "IPCServerSampleApp/GUI/GUIActivityEventNotifier.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace gui {

std::shared_ptr<gui::GUIActivityEventNotifier> GUIActivityEventNotifier::create() {
    auto activityEventNotifier = std::shared_ptr<GUIActivityEventNotifier>(new GUIActivityEventNotifier());
    return activityEventNotifier;
}

GUIActivityEventNotifier::GUIActivityEventNotifier() {
}

void GUIActivityEventNotifier::notifyObserversOfGUIActivityEvent(
    const std::string& source,
    const avsCommon::sdkInterfaces::GUIActivityEvent& activityEvent) {
    notifyObservers(
        [source, activityEvent](std::shared_ptr<avsCommon::sdkInterfaces::GUIActivityEventObserverInterface> observer) {
            observer->onGUIActivityEventReceived(source, activityEvent);
        });
}

}  // namespace gui
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
