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

#ifndef ACSDKASSETSINTERFACES_STATE_H_
#define ACSDKASSETSINTERFACES_STATE_H_

namespace alexaClientSDK {
namespace acsdkAssets {
namespace commonInterfaces {
/*
 * The state of the DAVS artifacts on the system.
 *
 * @startuml
 * (INIT) --> (LOADED) : From Storage
 * (INIT) --> (REQUESTING) : From DAVS
 * (REQUESTING) --> (LOADED) : Already found \n on device
 * (REQUESTING) --> (DOWNLOADING)
 * (REQUESTING) --> (INVALID) : Failed
 * (DOWNLOADING) --> (LOADED)
 * (DOWNLOADING) --> (INVALID) : Failed
 * @enduml
 */
enum class State { INIT, REQUESTING, DOWNLOADING, INVALID, LOADED };

inline std::string toString(State state) {
    switch (state) {
        case State::INIT:
            return "INIT";
        case State::REQUESTING:
            return "REQUESTING";
        case State::DOWNLOADING:
            return "DOWNLOADING";
        case State::INVALID:
            return "INVALID";
        case State::LOADED:
            return "LOADED";
    }
    return "";
}

inline std::ostream& operator<<(std::ostream& os, State value) {
    return os << toString(value);
}

}  // namespace commonInterfaces
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETSINTERFACES_STATE_H_
