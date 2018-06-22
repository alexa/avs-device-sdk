/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_NOTIFICATIONSOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_NOTIFICATIONSOBSERVERINTERFACE_H_

#include <AVSCommon/AVS/IndicatorState.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * An interface to allow being notified of changes to the currently stored IndicatorState.
 */
class NotificationsObserverInterface {
public:
    /**
     * Destructor
     */
    virtual ~NotificationsObserverInterface() = default;

    /**
     * Notifies the observer that a SetIndicator directive has been processed
     *
     * @param state The value of the persistVisualIndicator field of the directive.
     */
    virtual void onSetIndicator(avsCommon::avs::IndicatorState state) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_NOTIFICATIONSOBSERVERINTERFACE_H_
