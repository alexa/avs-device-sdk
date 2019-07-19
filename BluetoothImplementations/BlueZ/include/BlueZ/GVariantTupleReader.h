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

#ifndef ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_GVARIANTTUPLEREADER_H_
#define ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_GVARIANTTUPLEREADER_H_

#include <functional>

#include <gio/gio.h>

#include "BlueZ/ManagedGVariant.h"
#include "BlueZ/ManagedGError.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

/**
 * Helper class to read values from a tuple represented by GLib's @c GVariant* .
 *
 * See <a href="https://developer.gnome.org/glib/stable/gvariant-format-strings.html">GVariant Format Strings</a>
 * for type strings explanation.
 */
class GVariantTupleReader {
public:
    /**
     * Destructor
     */
    ~GVariantTupleReader();

    /**
     * Constructor to initialize the reader with raw @c GVariant* value
     *
     * @param originalVariant @c GVariant* value
     */
    explicit GVariantTupleReader(GVariant* originalVariant);

    /**
     * Constructor to initialize the reader with the reference to @c ManagedGVariant
     *
     * @param originalVariant A reference to @c ManagedGVariant
     */
    explicit GVariantTupleReader(ManagedGVariant& originalVariant);

    GVariantTupleReader(const GVariantTupleReader& other);

    /**
     * Get string value by index (not the difference with object path type value)
     *
     * @param index Zero based index of the value in tuple
     * @return A pointer to a C-style string with the value. Method returns nullptr if there is no such value.
     * The returned value points to an internal @c GVariant buffer and must not be freed.  The pointer is valid while
     * @c GVariant is valid.
     */
    char* getCString(gsize index) const;

    /**
     * Get the object path value by index (not the difference with string type value)
     *
     * @param index Zero based index of the value in tuple
     * @return A pointer to a C-style string with the value. Method returns nullptr if there is no such value.
     * The returned value points to an internal @c GVariant buffer and must not be freed.
     */
    char* getObjectPath(gsize index) const;

    /**
     * Get the int32 value by index
     *
     * @param index Zero based index of the value in tuple
     * @return A @c gint32 value. The default value is 0.
     */
    gint32 getInt32(gsize index) const;

    /**
     * Get the boolean value by index
     *
     * @param index Zero based index of the value in tuple
     * @return A @c gboolean value. The default value is false.
     */
    gboolean getBoolean(gsize index) const;

    /**
     * Get the @c GVariant value by index and return it as @c ManagedGVariant.
     *
     * @param index Zero based index of the value in tuple
     * @return A @c ManagedGVariant value. The default is an empty @c ManagedGVariant.
     */
    ManagedGVariant getVariant(gsize index) const;

    /**
     * Returns a number of elements in a tuple.
     *
     * @return Number of elements in a tuple.
     */
    gsize size() const;

    /**
     * Helper function to traverse all the elements in a tuple
     *
     * @param iteratorFunction Accepts @c GVariant* and returns true if iteration should be continued and
     * false if no more iterations required.
     * @return true if traversal was stopped because one of the callback invocations returned true.
     * If none of the callback invocations returned false - method returns false.
     */
    bool forEach(std::function<bool(GVariant* value)> iteratorFunction) const;

private:
    /*
     * A @c GVariant* containing a tuple
     */
    GVariant* m_tuple;
};

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_GVARIANTTUPLEREADER_H_
