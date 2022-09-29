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

#ifndef ACSDK_VISUALCHARACTERISTICS_VISUALCHARACTERISTICSSERIALIZERFACTORY_H_
#define ACSDK_VISUALCHARACTERISTICS_VISUALCHARACTERISTICSSERIALIZERFACTORY_H_

#include <memory>

#include <acsdk/VisualCharacteristicsInterfaces/VisualCharacteristicsSerializerInterface.h>

namespace alexaClientSDK {
namespace visualCharacteristics {

class VisualCharacteristicsSerializerFactory {
public:
    /**
     * Creates an instance of the VisualCharacteristicsSerializer
     * @return An instance of the @c VisualCharacteristicsSerializerInterface
     */
    static std::shared_ptr<visualCharacteristicsInterfaces::VisualCharacteristicsSerializerInterface> create();
};

}  // namespace visualCharacteristics
}  // namespace alexaClientSDK

#endif  // ACSDK_VISUALCHARACTERISTICS_VISUALCHARACTERISTICSSERIALIZERFACTORY_H_
