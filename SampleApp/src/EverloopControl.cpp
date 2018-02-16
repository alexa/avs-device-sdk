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

#include <sstream>

#include "SampleApp/EverloopControl.h"

#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include "AVSCommon/Utils/SDKVersion.h"

#include "SampleApp/ConsolePrinter.h"

namespace alexaClientSDK {
namespace sampleApp {

using namespace avsCommon::sdkInterfaces;

static const std::string VERSION = avsCommon::utils::sdkVersion::getCurrentVersion();

void EverloopControl::InitWishboneBus(){
    bus.SpiInit();
    everloop.Setup(&bus);
    SetEverloopColors(0,10,0,0);
    spiBusInit = true;
}

void EverloopControl::SetEverloopColors(int red, int green, int blue, int white) {
    for (hal::LedValue& led : image1d.leds) {
        led.red = red;
        led.green = green;
        led.blue = blue;
        led.white = white;
    }
    everloop.Write(&image1d);
}

void EverloopControl::onDialogUXStateChanged(DialogUXState state) {
    
    if(spiBusInit == false){
        InitWishboneBus();
    }

    m_executor.submit([this, state]() {
        switch (state) {
            case DialogUXState::IDLE:
                SetEverloopColors(10,0,0,0);
                return;
            case DialogUXState::LISTENING:
                SetEverloopColors(0, 10, 0, 0);
                return;
            case DialogUXState::THINKING:
                SetEverloopColors(0, 0, 10, 0);
                return;
                ;
            case DialogUXState::SPEAKING:
                SetEverloopColors(0, 0, 0, 10);
                return;
            /*
             * This is an intermediate state after a SPEAK directive is completed. In the case of a speech burst the
             * next SPEAK could kick in or if its the last SPEAK directive ALEXA moves to the IDLE state. So we do
             * nothing for this state.
             */
            case DialogUXState::FINISHED:
                SetEverloopColors(10, 0, 10, 0);
                return;
        }
    });
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
