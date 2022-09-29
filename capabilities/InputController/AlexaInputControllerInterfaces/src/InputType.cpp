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

#include <acsdk/AlexaInputControllerInterfaces/InputType.h>

namespace alexaClientSDK {
namespace alexaInputControllerInterfaces {

std::string inputToString(Input input) {
    switch (input) {
        case Input::AUX_1:
            return "AUX 1";
        case Input::AUX_2:
            return "AUX 2";
        case Input::AUX_3:
            return "AUX 3";
        case Input::AUX_4:
            return "AUX 4";
        case Input::AUX_5:
            return "AUX 5";
        case Input::AUX_6:
            return "AUX 6";
        case Input::AUX_7:
            return "AUX 7";
        case Input::BLURAY:
            return "BLURAY";
        case Input::CABLE:
            return "CABLE";
        case Input::CD:
            return "CD";
        case Input::COAX_1:
            return "COAX 1";
        case Input::COAX_2:
            return "COAX 2";
        case Input::COMPOSITE_1:
            return "COMPOSITE 1";
        case Input::DVD:
            return "DVD";
        case Input::GAME:
            return "GAME";
        case Input::HD_RADIO:
            return "HD RADIO";
        case Input::HDMI_1:
            return "HDMI 1";
        case Input::HDMI_2:
            return "HDMI 2";
        case Input::HDMI_3:
            return "HDMI 3";
        case Input::HDMI_4:
            return "HDMI 4";
        case Input::HDMI_5:
            return "HDMI 5";
        case Input::HDMI_6:
            return "HDMI 6";
        case Input::HDMI_7:
            return "HDMI 7";
        case Input::HDMI_8:
            return "HDMI 8";
        case Input::HDMI_9:
            return "HDMI 9";
        case Input::HDMI_10:
            return "HDMI 10";
        case Input::HDMI_ARC:
            return "HDMI ARC";
        case Input::INPUT_1:
            return "INPUT 1";
        case Input::INPUT_2:
            return "INPUT 2";
        case Input::INPUT_3:
            return "INPUT 3";
        case Input::INPUT_4:
            return "INPUT 4";
        case Input::INPUT_5:
            return "INPUT 5";
        case Input::INPUT_6:
            return "INPUT 6";
        case Input::INPUT_7:
            return "INPUT 7";
        case Input::INPUT_8:
            return "INPUT 8";
        case Input::INPUT_9:
            return "INPUT 9";
        case Input::INPUT_10:
            return "INPUT 10";
        case Input::IPOD:
            return "IPOD";
        case Input::LINE_1:
            return "LINE 1";
        case Input::LINE_2:
            return "LINE 2";
        case Input::LINE_3:
            return "LINE 3";
        case Input::LINE_4:
            return "LINE 4";
        case Input::LINE_5:
            return "LINE 5";
        case Input::LINE_6:
            return "LINE 6";
        case Input::LINE_7:
            return "LINE 7";
        case Input::MEDIA_PLAYER:
            return "MEDIA PLAYER";
        case Input::OPTICAL_1:
            return "OPTICAL 1";
        case Input::OPTICAL_2:
            return "OPTICAL 2";
        case Input::PHONO:
            return "PHONO";
        case Input::PLAYSTATION:
            return "PLAYSTATION";
        case Input::PLAYSTATION_3:
            return "PLAYSTATION 3";
        case Input::PLAYSTATION_4:
            return "PLAYSTATION 4";
        case Input::SATELLITE:
            return "SATELLITE";
        case Input::SMARTCAST:
            return "SMARTCAST";
        case Input::TUNER:
            return "TUNER";
        case Input::TV:
            return "TV";
        case Input::USB_DAC:
            return "USB DAC";
        case Input::VIDEO_1:
            return "VIDEO 1";
        case Input::VIDEO_2:
            return "VIDEO 2";
        case Input::VIDEO_3:
            return "VIDEO 3";
        case Input::XBOX:
            return "XBOX";
    }
    return "";
}

avsCommon::utils::Optional<Input> stringToInput(const std::string& inputString) {
    avsCommon::utils::Optional<Input> input;
    if ("AUX 1" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::AUX_1);
    } else if ("AUX 2" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::AUX_2);
    } else if ("AUX 3" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::AUX_3);
    } else if ("AUX 4" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::AUX_4);
    } else if ("AUX 5" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::AUX_5);
    } else if ("AUX 6" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::AUX_6);
    } else if ("AUX 7" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::AUX_7);
    } else if ("BLURAY" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::BLURAY);
    } else if ("CABLE" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::CABLE);
    } else if ("CD" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::CD);
    } else if ("COAX 1" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::COAX_1);
    } else if ("COAX 2" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::COAX_2);
    } else if ("COMPOSITE 1" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::COMPOSITE_1);
    } else if ("DVD" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::DVD);
    } else if ("GAME" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::GAME);
    } else if ("HD RADIO" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::HD_RADIO);
    } else if ("HDMI 1" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::HDMI_1);
    } else if ("HDMI 2" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::HDMI_2);
    } else if ("HDMI 3" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::HDMI_3);
    } else if ("HDMI 4" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::HDMI_4);
    } else if ("HDMI 5" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::HDMI_5);
    } else if ("HDMI 6" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::HDMI_6);
    } else if ("HDMI 7" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::HDMI_7);
    } else if ("HDMI 8" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::HDMI_8);
    } else if ("HDMI 9" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::HDMI_9);
    } else if ("HDMI 10" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::HDMI_10);
    } else if ("HDMI ARC" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::HDMI_ARC);
    } else if ("INPUT 1" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::INPUT_1);
    } else if ("INPUT 2" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::INPUT_2);
    } else if ("INPUT 3" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::INPUT_3);
    } else if ("INPUT 4" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::INPUT_4);
    } else if ("INPUT 5" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::INPUT_5);
    } else if ("INPUT 6" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::INPUT_6);
    } else if ("INPUT 7" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::INPUT_7);
    } else if ("INPUT 8" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::INPUT_8);
    } else if ("INPUT 9" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::INPUT_9);
    } else if ("INPUT 10" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::INPUT_10);
    } else if ("IPOD" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::IPOD);
    } else if ("LINE 1" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::LINE_1);
    } else if ("LINE 2" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::LINE_2);
    } else if ("LINE 3" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::LINE_3);
    } else if ("LINE 4" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::LINE_4);
    } else if ("LINE 5" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::LINE_5);
    } else if ("LINE 6" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::LINE_6);
    } else if ("LINE 7" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::LINE_7);
    } else if ("MEDIA PLAYER" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::MEDIA_PLAYER);
    } else if ("OPTICAL 1" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::OPTICAL_1);
    } else if ("OPTICAL 2" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::OPTICAL_2);
    } else if ("PHONO" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::PHONO);
    } else if ("PLAYSTATION" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::PLAYSTATION);
    } else if ("PLAYSTATION 3" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::PLAYSTATION_3);
    } else if ("PLAYSTATION 4" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::PLAYSTATION_4);
    } else if ("SATELLITE" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::SATELLITE);
    } else if ("SMARTCAST" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::SMARTCAST);
    } else if ("TUNER" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::TUNER);
    } else if ("TV" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::TV);
    } else if ("USB DAC" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::USB_DAC);
    } else if ("VIDEO 1" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::VIDEO_1);
    } else if ("VIDEO 2" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::VIDEO_2);
    } else if ("VIDEO 3" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::VIDEO_3);
    } else if ("XBOX" == inputString) {
        return avsCommon::utils::Optional<Input>(Input::XBOX);
    }
    return input;
}

}  // namespace alexaInputControllerInterfaces
}  // namespace alexaClientSDK
