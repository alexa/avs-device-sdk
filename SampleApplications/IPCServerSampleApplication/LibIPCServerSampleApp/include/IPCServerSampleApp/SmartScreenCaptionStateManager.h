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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_SMARTSCREENCAPTIONSTATEMANAGER_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_SMARTSCREENCAPTIONSTATEMANAGER_H_

#include <memory>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

/**
 * This class manages if captions are turned on or off. It stores the status persistently
 * in disk
 */
class SmartScreenCaptionStateManager {
public:
    /**
     * Constructor
     * @param miscStorage the storage where Captions settings are stored
     */
    explicit SmartScreenCaptionStateManager(
        std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> miscStorage);
    /**
     * Retrieves the current captions status
     * @return whether or not Captions are enabled
     */
    bool areCaptionsEnabled();

    /**
     * Set the state of captions in the DB.
     * @param enabled true if captions should be enabled
     */
    void setCaptionsState(bool enabled);

private:
    /// Pointer to the storage interface
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> m_miscStorage;
};

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_SMARTSCREENCAPTIONSTATEMANAGER_H_
