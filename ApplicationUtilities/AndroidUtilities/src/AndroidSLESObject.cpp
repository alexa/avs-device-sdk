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

#include <AndroidUtilities/AndroidSLESObject.h>

namespace alexaClientSDK {
namespace applicationUtilities {
namespace androidUtilities {

/// The tag associated with log entries from this class.
static const std::string TAG{"AndroidSLESObject"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

bool AndroidSLESObject::getInterface(SLInterfaceID interfaceID, void* retObject) {
    auto result = (*m_object)->GetInterface(m_object, interfaceID, retObject);
    if (result != SL_RESULT_SUCCESS) {
        ACSDK_ERROR(LX("getInterfaceFailed").d("result", result));
        return false;
    }
    return true;
}

AndroidSLESObject::AndroidSLESObject(SLObjectItf slObject) : m_object{slObject} {
}

AndroidSLESObject::~AndroidSLESObject() {
    if (m_object) {
        (*m_object)->Destroy(m_object);
    }
}

std::unique_ptr<AndroidSLESObject> AndroidSLESObject::create(SLObjectItf object) {
    if (object != nullptr) {
        auto result = (*object)->Realize(object, SL_BOOLEAN_FALSE);
        if (result == SL_RESULT_SUCCESS) {
            return std::unique_ptr<AndroidSLESObject>(new AndroidSLESObject(object));
        } else {
            ACSDK_ERROR(LX("createSlOjectFailed").d("reason", "Failed to realize object.").d("result", result));
            (*object)->Destroy(object);
        }
    } else {
        ACSDK_ERROR(LX("createSlOjectFailed").d("reason", "nullObject"));
    }

    return nullptr;
}

SLObjectItf AndroidSLESObject::get() const {
    return m_object;
}

}  // namespace androidUtilities
}  // namespace applicationUtilities
}  // namespace alexaClientSDK
