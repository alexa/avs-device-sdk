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

#include "acsdkAlexaKeypadControllerInterfaces/Keystroke.h"

namespace alexaClientSDK {
namespace acsdkAlexaKeypadControllerInterfaces {

std::string keystrokeToString(Keystroke keystroke) {
    switch (keystroke) {
        case Keystroke::UP:
            return "UP";
        case Keystroke::DOWN:
            return "DOWN";
        case Keystroke::LEFT:
            return "LEFT";
        case Keystroke::RIGHT:
            return "RIGHT";
        case Keystroke::SELECT:
            return "SELECT";
        case Keystroke::PAGE_UP:
            return "PAGE_UP";
        case Keystroke::PAGE_DOWN:
            return "PAGE_DOWN";
        case Keystroke::PAGE_LEFT:
            return "PAGE_LEFT";
        case Keystroke::PAGE_RIGHT:
            return "PAGE_RIGHT";
        case Keystroke::INFO:
            return "INFO";
        case Keystroke::MORE:
            return "MORE";
        case Keystroke::BACK:
            return "BACK";
    }

    return "";
}

avsCommon::utils::Optional<Keystroke> stringToKeystroke(const std::string& keystrokeString) {
    avsCommon::utils::Optional<Keystroke> keystroke;
    if ("UP" == keystrokeString) {
        keystroke = avsCommon::utils::Optional<Keystroke>(Keystroke::UP);
    } else if ("DOWN" == keystrokeString) {
        keystroke = avsCommon::utils::Optional<Keystroke>(Keystroke::DOWN);
    } else if ("LEFT" == keystrokeString) {
        keystroke = avsCommon::utils::Optional<Keystroke>(Keystroke::LEFT);
    } else if ("RIGHT" == keystrokeString) {
        keystroke = avsCommon::utils::Optional<Keystroke>(Keystroke::RIGHT);
    } else if ("SELECT" == keystrokeString) {
        keystroke = avsCommon::utils::Optional<Keystroke>(Keystroke::SELECT);
    } else if ("PAGE_UP" == keystrokeString) {
        keystroke = avsCommon::utils::Optional<Keystroke>(Keystroke::PAGE_UP);
    } else if ("PAGE_DOWN" == keystrokeString) {
        keystroke = avsCommon::utils::Optional<Keystroke>(Keystroke::PAGE_DOWN);
    } else if ("PAGE_LEFT" == keystrokeString) {
        keystroke = avsCommon::utils::Optional<Keystroke>(Keystroke::PAGE_LEFT);
    } else if ("PAGE_RIGHT" == keystrokeString) {
        keystroke = avsCommon::utils::Optional<Keystroke>(Keystroke::PAGE_RIGHT);
    } else if ("INFO" == keystrokeString) {
        keystroke = avsCommon::utils::Optional<Keystroke>(Keystroke::INFO);
    } else if ("MORE" == keystrokeString) {
        keystroke = avsCommon::utils::Optional<Keystroke>(Keystroke::MORE);
    } else if ("BACK" == keystrokeString) {
        keystroke = avsCommon::utils::Optional<Keystroke>(Keystroke::BACK);
    }

    return keystroke;
}

}  // namespace acsdkAlexaKeypadControllerInterfaces
}  // namespace alexaClientSDK
