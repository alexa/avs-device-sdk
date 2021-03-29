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

#ifndef ACSDKMANUFACTORY_FACTORYSEQUENCER_H_
#define ACSDKMANUFACTORY_FACTORYSEQUENCER_H_

#include <functional>

namespace alexaClientSDK {
namespace acsdkManufactory {

template <typename...>
class FactorySequencer;

/**
 * Template to provide a sequenced factory function to invoke a factory function after the PrecursorTypes have been
 * instantiated first.
 *
 * In other words, this class can be used to sequence calls to factory methods in the manufactory.
 * The @c PrecursorTypes are guaranteed to be instantiated by the manufactory before the @c ResultType, which will be
 * created using the provided factory method and the @c ParameterTypes.
 *
 * @tparam ResultType The type being constructed.
 * @tparam PrecursorTypes The types that are required to have been constructed before the ResultType is constructed.
 */
template <typename ResultType, typename... PrecursorTypes>
class FactorySequencer<ResultType, PrecursorTypes...> {
public:
    /**
     * Get the factory sequencer wrapping the ResultType's factory.
     *
     * @tparam ParameterTypes The parameters for the ResultType's factory.
     * @return A std::function that will return the ResultType and takes the ParameterTypes and PrecursorTypes as
     * arguments.
     */
    template <typename... ParameterTypes>
    static std::function<ResultType(ParameterTypes..., PrecursorTypes...)> get(
        ResultType (*factory)(ParameterTypes...));
};

template <typename ResultType, typename... PrecursorTypes>
template <typename... ParameterTypes>
inline std::function<ResultType(ParameterTypes..., PrecursorTypes...)> FactorySequencer<ResultType, PrecursorTypes...>::
    get(ResultType (*factory)(ParameterTypes...)) {
    return [factory](ParameterTypes... parameters, PrecursorTypes...) { return factory(std::move(parameters...)); };
}

}  // namespace acsdkManufactory
}  // namespace alexaClientSDK

#endif  // ACSDKMANUFACTORY_FACTORYSEQUENCER_H_
