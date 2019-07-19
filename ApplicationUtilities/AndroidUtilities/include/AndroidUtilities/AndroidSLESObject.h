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
#ifndef ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_ANDROIDUTILITIES_INCLUDE_ANDROIDUTILITIES_ANDROIDSLESOBJECT_H_
#define ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_ANDROIDUTILITIES_INCLUDE_ANDROIDUTILITIES_ANDROIDSLESOBJECT_H_

#include <functional>
#include <memory>

#include <SLES/OpenSLES.h>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Logger/LoggerUtils.h>
#include <AVSCommon/Utils/Memory/Memory.h>

namespace alexaClientSDK {
namespace applicationUtilities {
namespace androidUtilities {

/**
 * This class will represent an OpenSL ES object.
 *
 * This class will abstract the object states as well as interface relationship from Open SL to be more C++ friendly.
 * For more details see https://www.khronos.org/registry/OpenSL-ES/specs/OpenSL_ES_Specification_1.0.1.pdf
 */
class AndroidSLESObject {
public:
    /**
     * This method will create an @c AndroidSLESObject and perform a synchronous realization.
     *
     * At the end of this method, the internal @c SLObjectItf will be ready to be used.
     *
     * @param slObject The C object that the new instance will wrap and own.
     * @return A unique_ptr with a valid @c AndroidSLESObject if creation was successful, @c nullptr otherwise.
     */
    static std::unique_ptr<AndroidSLESObject> create(SLObjectItf slObject);

    /**
     * AndroidSLESObject destructor. It will be responsible for destroying the underlying @c SLObjectItf.
     */
    ~AndroidSLESObject();

    /**
     * Get the object C interface.
     *
     * @param interfaceID The object C interface ID.
     * @param[out] retObject The returned object.
     * @return true if it succeeds and false if it fails.
     * @warning The object interface is actually a pointer to the OpenSLObject internal object. Make sure that
     * its lifecycle is not longer than the parent OpenSLObject.
     */
    bool getInterface(SLInterfaceID interfaceID, void* retObject);

    /**
     * Get a raw pointer to the internal C object.
     *
     * @return A pointer to the internal C object.
     * @warning Avoid storing the SLObjectItf since the parent OpenSLObject owns the underlying object.
     */
    SLObjectItf get() const;

private:
    /**
     * AndroidSLESObject constructor.
     *
     * @param slObject
     */
    AndroidSLESObject(SLObjectItf slObject);

    /// The OpenSL object that is wrapped.
    SLObjectItf m_object;
};

}  // namespace androidUtilities
}  // namespace applicationUtilities
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_ANDROIDUTILITIES_INCLUDE_ANDROIDUTILITIES_ANDROIDSLESOBJECT_H_
