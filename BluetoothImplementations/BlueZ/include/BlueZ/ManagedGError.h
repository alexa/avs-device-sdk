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

#ifndef ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_MANAGEDGERROR_H_
#define ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_MANAGEDGERROR_H_

#include <string>

#include <gio/gio.h>

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

/**
 * Wrapper for GLib's @c GError returned by the most of DBus methods. This class is not thread safe.
 */
class ManagedGError {
public:
    /**
     * A constructor initializing object with existing @c GError* value.
     *
     * @param error error to be attached.
     */
    explicit ManagedGError(GError* error);

    /**
     * A constructor initializing object with an emtpy (no error) value
     */
    ManagedGError();

    /**
     * Check if object contains any error.
     *
     * @return true if there is an error
     */
    bool hasError();

    /**
     * Get a pointer to @c GError* variable
     *
     * @return A pointer to the internal variable holding the error
     */
    GError** toOutputParameter();

    /**
     * A destructor
     */
    ~ManagedGError();

    /**
     * Get the message associated with the error. Returns nullptr if there is no error. The pointer is valid as long
     * as the @c ManagedGError object exists.
     *
     * @return A message associated with the error.
     */
    const char* getMessage();

private:
    /**
     * A @c GError* value attached to the object or nullptr if there is no value
     */
    GError* m_error;
};

inline ManagedGError::ManagedGError(GError* error) : m_error{error} {
}

inline ManagedGError::ManagedGError() {
    m_error = nullptr;
}

inline bool ManagedGError::hasError() {
    return m_error != nullptr;
}

inline GError** ManagedGError::toOutputParameter() {
    return &m_error;
}

inline ManagedGError::~ManagedGError() {
    if (m_error != nullptr) {
        g_error_free(m_error);
    }
}

inline const char* ManagedGError::getMessage() {
    return m_error == nullptr ? nullptr : m_error->message;
}

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_MANAGEDGERROR_H_
