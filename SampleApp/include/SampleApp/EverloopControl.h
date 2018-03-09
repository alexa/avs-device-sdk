/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_EVERLOOPCONTROL_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_EVERLOOPCONTROL_H_

#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>

#include "../../../ThirdParty/matrixio-libs/everloop_image.h"
#include "../../../ThirdParty/matrixio-libs/everloop.h"

namespace hal = matrix_hal;

namespace alexaClientSDK {
namespace sampleApp {

/**
 * This class manages the states that the user will see when interacting with the Sample Application. For now, it simply
 * prints states to the screen.
 */
class EverloopControl
        : public avsCommon::sdkInterfaces::DialogUXStateObserverInterface{
public:
    void onDialogUXStateChanged(DialogUXState state) override;

private:
    /// The current dialog UX state of the SDK
    DialogUXState m_dialogState;

    /// An internal executor that performs execution of callable objects passed to it sequentially but asynchronously.
    avsCommon::utils::threading::Executor m_executor;

    void SetEverloopColors(int red, int green, int blue, int white);

    hal::Everloop everloop;
    hal::EverloopImage image1d;
};

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_EVERLOOPCONTROL_H_
