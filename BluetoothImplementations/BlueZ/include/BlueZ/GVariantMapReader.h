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

#ifndef ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_GVARIANTMAPREADER_H_
#define ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_GVARIANTMAPREADER_H_

#include <functional>

#include <gio/gio.h>

#include "BlueZ/ManagedGVariant.h"
#include "BlueZ/ManagedGError.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

/**
 * Helper class to read values from a map represented by GLib's @c GVariant* .
 *
 * See <a href="https://developer.gnome.org/glib/stable/gvariant-format-strings.html">GVariant Format Strings</a>
 * for type strings explanation.
 */
class GVariantMapReader {
public:
    /**
     * Destructor
     */
    ~GVariantMapReader();

    /**
     * Constructor that initalizes the reader with a raw @c GVariant* value.
     *
     * @param originalVariant A @c GVariant* to wrap
     * @param useObjectPathAsKey Set to true if map key type is object path instead of string
     */
    explicit GVariantMapReader(GVariant* originalVariant, bool useObjectPathAsKey = false);

    /**
     * Constructor that initalizes the reader with a reference to @c ManagedGVariant.
     *
     * @param originalVariant A @c ManagedGVariant to wrap
     * @param useObjectPathAsKey Set to true if map key type is object path instead of string
     */
    explicit GVariantMapReader(ManagedGVariant& originalVariant, bool useObjectPathAsKey = false);

    /**
     * Copy constructor
     * @param other
     */
    GVariantMapReader(const GVariantMapReader& other);

    /**
     * Get string value by the key
     *
     * @param name A key to lookup map with
     * @param[out] value A pointer to char* variable that will receive the result of the operation.
     * Variable receives the pointer to an internal GVariant buffer and must not be freed. The pointer is valid while
     * @c GVariant is valid.
     * @return true if value exists, false otherwise. If value was not found the contents of the @c value is not
     * changed.
     */
    bool getCString(const char* name, char** value) const;

    /**
     * Get int32 value by the key
     *
     * @param name A key to lookup map with
     * @param[out] value A pointer to gint32 variable that will receive the result of the operation
     * @return true if value exists, false otherwise. If value was not found the contents of the @c value is not
     * changed.
     */
    bool getInt32(const char* name, gint32* value) const;

    /**
     * Get boolean value by the key
     *
     * @param name A key to lookup map with
     * @param[out] value A pointer to gboolean variable that will receive the result of the operation
     * @return true if value exists, false otherwise. If value was not found the contents of the @c value is not
     * changed.
     */
    bool getBoolean(const char* name, gboolean* value) const;

    /**
     * Get a variant value by the key
     *
     * @param name A key to lookup map with
     * @return A ManagedGVariant object containing a variant in case of success and an empty @c ManagedGVariant if
     * value has not been found.
     */
    ManagedGVariant getVariant(const char* name) const;

    /**
     * Helper method to traverse
     *
     * @param iteratorFunction A boolean(char *key, GVariant *value) callback that receives the items in a map.
     * Callback may return true if it wants the traversal to stop. Returning false will continue the traversal.
     * @return true if traversal was stopped because one of the callback invocations returned true.
     * If none of the callback invocations returned false - method returns false.
     */
    bool forEach(std::function<bool(char* key, GVariant* value)> iteratorFunction) const;

    /**
     * Get the raw @c GVariant* value attached to the reader
     *
     * @return Raw @c GVariant* value attached to the reader.
     */
    GVariant* get() const;

private:
    // A @c GVariant* attached to the reader
    GVariant* m_map;

    // Contains true if the map key type is object path instead of string
    bool m_useObjectPathKeys;
};

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_GVARIANTMAPREADER_H_
