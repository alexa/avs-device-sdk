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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ALEXAUNITOFMEASURE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ALEXAUNITOFMEASURE_H_

#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace resources {

/// This represents the Alexa unit of measure.
using AlexaUnitOfMeasure = std::string;

/**
 * String constants for the unit of measure.
 * @see https://developer.amazon.com/docs/alexa/device-apis/alexa-property-schemas.html#units-of-measure
 */

/// The Alexa unit of measure as angle degrees.
static const AlexaUnitOfMeasure ALEXA_UNIT_ANGLE_DEGREES = "Alexa.Unit.Angle.Degrees";

/// The Alexa unit of measure as angle radians.
static const AlexaUnitOfMeasure ALEXA_UNIT_ANGLE_RADIANS = "Alexa.Unit.Angle.Radians";

/// The Alexa unit of measure as feet.
static const AlexaUnitOfMeasure ALEXA_UNIT_DISTANCE_FEET = "Alexa.Unit.Distance.Feet";

/// The Alexa unit of measure as inches.
static const AlexaUnitOfMeasure ALEXA_UNIT_DISTANCE_INCHES = "Alexa.Unit.Distance.Inches";

/// The Alexa unit of measure as kilometeres.
static const AlexaUnitOfMeasure ALEXA_UNIT_DISTANCE_KILOMETERS = "Alexa.Unit.Distance.Kilometers";

/// The Alexa unit of measure as meters.
static const AlexaUnitOfMeasure ALEXA_UNIT_DISTANCE_METERS = "Alexa.Unit.Distance.Meters";

/// The Alexa unit of measure as miles.
static const AlexaUnitOfMeasure ALEXA_UNIT_DISTANCE_MILES = "Alexa.Unit.Distance.Miles";

/// The Alexa unit of measure as yards.
static const AlexaUnitOfMeasure ALEXA_UNIT_DISTANCE_YARDS = "Alexa.Unit.Distance.Yards";

/// The Alexa unit of measure as grams.
static const AlexaUnitOfMeasure ALEXA_UNIT_MASS_GRAMS = "Alexa.Unit.Mass.Grams";

/// The Alexa unit of measure as kilograms.
static const AlexaUnitOfMeasure ALEXA_UNIT_MASS_KILOGRAMS = "Alexa.Unit.Mass.Kilograms";

/// The Alexa unit of measure as percentage.
static const AlexaUnitOfMeasure ALEXA_UNIT_PERCENT = "Alexa.Unit.Percent";

/// The Alexa unit of measure as temperature in celsius.
static const AlexaUnitOfMeasure ALEXA_UNIT_TEMPERATURE_CELSIUS = "Alexa.Unit.Temperature.Celsius";

/// The Alexa unit of measure as temperature in fahrenheit.
static const AlexaUnitOfMeasure ALEXA_UNIT_TEMPERATURE_FAHRENHEIT = "Alexa.Unit.Temperature.Fahrenheit";

/// The Alexa unit of measure as temperature in kelvin.
static const AlexaUnitOfMeasure ALEXA_UNIT_TEMPERATURE_KELVIN = "Alexa.Unit.Temperature.Kelvin";

/// The Alexa unit of measure as cubic feet.
static const AlexaUnitOfMeasure ALEXA_UNIT_VOLUME_CUBICFEET = "Alexa.Unit.Volume.CubicFeet";

/// The Alexa unit of measure as cubic meters.
static const AlexaUnitOfMeasure ALEXA_UNIT_VOLUME_CUBICMETERS = "Alexa.Unit.Volume.CubicMeters";

/// The Alexa unit of measure as gallons.
static const AlexaUnitOfMeasure ALEXA_UNIT_VOLUME_GALLONS = "Alexa.Unit.Volume.Gallons";

/// The Alexa unit of measure as liters.
static const AlexaUnitOfMeasure ALEXA_UNIT_VOLUME_LITERS = "Alexa.Unit.Volume.Liters";

/// The Alexa unit of measure as pints.
static const AlexaUnitOfMeasure ALEXA_UNIT_VOLUME_PINTS = "Alexa.Unit.Volume.Pints";

/// The Alexa unit of measure as quarts.
static const AlexaUnitOfMeasure ALEXA_UNIT_VOLUME_QUARTS = "Alexa.Unit.Volume.Quarts";

/// The Alexa unit of measure as ounces.
static const AlexaUnitOfMeasure ALEXA_UNIT_WEIGHT_OUNCES = "Alexa.Unit.Weight.Ounces";

/// The Alexa unit of measure as pounds.
static const AlexaUnitOfMeasure ALEXA_UNIT_WEIGHT_POUNDS = "Alexa.Unit.Weight.Pounds";

}  // namespace resources
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ALEXAUNITOFMEASURE_H_
