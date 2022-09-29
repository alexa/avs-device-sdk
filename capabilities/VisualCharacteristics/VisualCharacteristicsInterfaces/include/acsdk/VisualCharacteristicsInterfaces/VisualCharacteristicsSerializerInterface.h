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
#ifndef ACSDK_VISUALCHARACTERISTICSINTERFACES_VISUALCHARACTERISTICSSERIALIZERINTERFACE_H_
#define ACSDK_VISUALCHARACTERISTICSINTERFACES_VISUALCHARACTERISTICSSERIALIZERINTERFACE_H_

#include <string>
#include <vector>

#include <acsdk/VisualCharacteristicsInterfaces/VisualCharacteristicsInterface.h>

namespace alexaClientSDK {
namespace visualCharacteristicsInterfaces {

/**
 * Interface contract for @c VisualCharacteristicsSerializer
 */
class VisualCharacteristicsSerializerInterface {
public:
    /**
     * Destructor
     */
    virtual ~VisualCharacteristicsSerializerInterface() = default;

    /**
     * Serialize interaction modes into reportable json format.
     * @param interactionModes  Collection of @c InteractionMode to be serialized
     * @param serializedJson [ out ] Serialized json payload
     *
     * @return True if serialization successful, false otherwise.
     */
    virtual bool serializeInteractionModes(
        const std::vector<visualCharacteristicsInterfaces::InteractionMode>& interactionModes,
        std::string& serializedJson) = 0;

    /**
     * Serialize window template into reportable json format.
     * @param windowTemplates  Collection of @c WindowTemplate to be serialized
     * @param serializedJson [ out ] Serialized json payload
     *
     * @return True if serialization successful, false otherwise.
     */
    virtual bool serializeWindowTemplate(
        const std::vector<visualCharacteristicsInterfaces::WindowTemplate>& windowTemplates,
        std::string& serializedJson) = 0;

    /**
     * Serialize display characteristics into reportable json format.
     * @param display Instance of @c DisplayCharacteristics to be serialized
     * @param serializedJson [ out ] Serialized json payload
     *
     * @return True if serialization successful, false otherwise.
     */
    virtual bool serializeDisplayCharacteristics(
        const visualCharacteristicsInterfaces::DisplayCharacteristics& display,
        std::string& serializedJson) = 0;
};
}  // namespace visualCharacteristicsInterfaces
}  // namespace alexaClientSDK
#endif  // ACSDK_VISUALCHARACTERISTICSINTERFACES_VISUALCHARACTERISTICSSERIALIZERINTERFACE_H_
