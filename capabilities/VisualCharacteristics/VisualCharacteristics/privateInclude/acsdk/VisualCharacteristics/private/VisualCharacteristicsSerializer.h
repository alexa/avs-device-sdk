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

#ifndef ACSDK_VISUALCHARACTERISTICS_PRIVATE_VISUALCHARACTERISTICSSERIALIZER_H_
#define ACSDK_VISUALCHARACTERISTICS_PRIVATE_VISUALCHARACTERISTICSSERIALIZER_H_

#include <memory>
#include <string>
#include <vector>

#include <acsdk/VisualCharacteristicsInterfaces/VisualCharacteristicsSerializerInterface.h>

namespace alexaClientSDK {
namespace visualCharacteristics {

/**
 * Serializer for Visual Characteristics data types
 */
class VisualCharacteristicsSerializer
        : public visualCharacteristicsInterfaces::VisualCharacteristicsSerializerInterface {
public:
    /**
     * Create an instance of @c VisualCharacteristicsSerializer.
     *
     * @return Shared pointer to the instance of @c VisualCharacteristicsSerializer
     */
    static std::shared_ptr<visualCharacteristicsInterfaces::VisualCharacteristicsSerializerInterface> create();

private:
    /**
     * Constructor.
     */
    VisualCharacteristicsSerializer();

    /// @name VisualCharacteristicsSerializerInterface Functions
    /// @{
    bool serializeInteractionModes(
        const std::vector<visualCharacteristicsInterfaces::InteractionMode>& interactionModes,
        std::string& serializedJson) override;
    bool serializeWindowTemplate(
        const std::vector<visualCharacteristicsInterfaces::WindowTemplate>& windowTemplates,
        std::string& serializedJson) override;
    bool serializeDisplayCharacteristics(
        const visualCharacteristicsInterfaces::DisplayCharacteristics& display,
        std::string& serializedJson) override;
    /// @}
};

}  // namespace visualCharacteristics
}  // namespace alexaClientSDK

#endif  // ACSDK_VISUALCHARACTERISTICS_PRIVATE_VISUALCHARACTERISTICSSERIALIZER_H_
