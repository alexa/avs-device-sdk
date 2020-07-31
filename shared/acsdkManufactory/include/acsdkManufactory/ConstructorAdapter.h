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

#ifndef ACSDKMANUFACTORY_CONSTRUCTORADAPTER_H_
#define ACSDKMANUFACTORY_CONSTRUCTORADAPTER_H_

#include <memory>

namespace alexaClientSDK {
namespace acsdkManufactory {

/**
 * Template to provide a factory function to invoke a constructor.
 *
 * @tparam Type The type being constructed.
 * @tparam Dependencies The parameters types of the constructor and the resulting factory function.
 */
template <typename Type, typename... Dependencies>
class ConstructorAdapter {
public:
    /**
     * Get the factory function wrapping the constructor.
     *
     * @return The factory function wrapping the constructor.
     */
    static auto get() -> std::shared_ptr<Type> (*)(Dependencies...);

    /**
     * The factory function wrapping the constructor.
     *
     * @param dependencies The parameters of the factory function and constructor.
     * @return A new instance of @c Type.
     */
    static std::shared_ptr<Type> createInstance(Dependencies... dependencies);
};

template <typename Type, typename... Dependencies>
inline auto ConstructorAdapter<Type, Dependencies...>::get() -> std::shared_ptr<Type> (*)(Dependencies...) {
    return createInstance;
}

template <typename Type, typename... Dependencies>
inline std::shared_ptr<Type> ConstructorAdapter<Type, Dependencies...>::createInstance(Dependencies... dependencies) {
    return std::make_shared<Type>(std::move(dependencies)...);
}

}  // namespace acsdkManufactory
}  // namespace alexaClientSDK

#endif  // ACSDKMANUFACTORY_CONSTRUCTORADAPTER_H_
