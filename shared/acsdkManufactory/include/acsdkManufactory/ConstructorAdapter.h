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

template <typename...>
class ConstructorAdapter;

/**
 * Template to provide a factory function to invoke a constructor.
 *
 * @tparam ResultType The type being constructed.
 * @tparam ConstructedType The type being constructed.
 * @tparam Dependencies The parameters types of the constructor and the resulting factory function.
 */
template <typename ResultType, typename ConstructedType, typename... Dependencies>
class ConstructorAdapter<ResultType, ConstructedType, Dependencies...> {
public:
    /**
     * Get the factory function wrapping the constructor.
     *
     * @return The factory function wrapping the constructor.
     */
    static auto get() -> std::shared_ptr<ResultType> (*)(Dependencies...);

    /**
     * The factory function wrapping the constructor.
     *
     * @param dependencies The parameters of the factory function and constructor.
     * @return A new instance of @c Type.
     */
    static std::shared_ptr<ResultType> createInstance(Dependencies... dependencies);
};

template <typename ResultType, typename ConstructedType, typename... Dependencies>
inline auto ConstructorAdapter<ResultType, ConstructedType, Dependencies...>::get()
    -> std::shared_ptr<ResultType> (*)(Dependencies...) {
    return createInstance;
}

template <typename ResultType, typename ConstructedType, typename... Dependencies>
inline std::shared_ptr<ResultType> ConstructorAdapter<ResultType, ConstructedType, Dependencies...>::createInstance(
    Dependencies... dependencies) {
    return std::make_shared<ConstructedType>(std::move(dependencies)...);
}

/**
 * Template to provide a factory function to invoke a constructor.  This is a simplified version of
 * ConstructorAdapter for the case where the constructor takes no parameters and the desired return
 * type for the factory is the same as the concrete type.
 *
 * @tparam Type The type being constructed.
 */
template <typename Type>
class ConstructorAdapter<Type> : public ConstructorAdapter<Type, Type> {};

}  // namespace acsdkManufactory
}  // namespace alexaClientSDK

#endif  // ACSDKMANUFACTORY_CONSTRUCTORADAPTER_H_
