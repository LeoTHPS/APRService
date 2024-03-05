#include "aprservice.hpp"

#include <AL/Serialization/APRS/Position.hpp>

AL::Float aprservice_math_convert_speed_from_feet(AL::Float value, AL::uint8 measurement_type)
{
	switch (measurement_type)
	{
		case APRSERVICE_MEASUREMENT_TYPE_FEET:       return value;
		case APRSERVICE_MEASUREMENT_TYPE_MILES:      return value * 5280;
		case APRSERVICE_MEASUREMENT_TYPE_METERS:     return value / 3.281f;
		case APRSERVICE_MEASUREMENT_TYPE_KILOMETERS: return value / 3281;
	}

	return 0;
}
AL::Float aprservice_math_convert_speed_from_miles(AL::Float value, AL::uint8 measurement_type)
{
	switch (measurement_type)
	{
		case APRSERVICE_MEASUREMENT_TYPE_FEET:       return value * 5280;
		case APRSERVICE_MEASUREMENT_TYPE_MILES:      return value;
		case APRSERVICE_MEASUREMENT_TYPE_METERS:     return value * 1609;
		case APRSERVICE_MEASUREMENT_TYPE_KILOMETERS: return value * 1.609f;
	}

	return 0;
}
AL::Float aprservice_math_convert_speed_from_meters(AL::Float value, AL::uint8 measurement_type)
{
	switch (measurement_type)
	{
		case APRSERVICE_MEASUREMENT_TYPE_FEET:       return value * 3.281f;
		case APRSERVICE_MEASUREMENT_TYPE_MILES:      return value / 1609;
		case APRSERVICE_MEASUREMENT_TYPE_METERS:     return value;
		case APRSERVICE_MEASUREMENT_TYPE_KILOMETERS: return value / 1000;
	}

	return 0;
}
AL::Float aprservice_math_convert_speed_from_kilometers(AL::Float value, AL::uint8 measurement_type)
{
	switch (measurement_type)
	{
		case APRSERVICE_MEASUREMENT_TYPE_FEET:       return value * 3281;
		case APRSERVICE_MEASUREMENT_TYPE_MILES:      return value / 1.609f;
		case APRSERVICE_MEASUREMENT_TYPE_METERS:     return value * 1000;
		case APRSERVICE_MEASUREMENT_TYPE_KILOMETERS: return value;
	}

	return 0;
}
AL::Float aprservice_math_convert_speed(AL::Float value, AL::uint8 measurement_type_input, AL::uint8 measurement_type_output)
{
	switch (measurement_type_input)
	{
		case APRSERVICE_MEASUREMENT_TYPE_FEET:       return aprservice_math_convert_speed_from_feet(value, measurement_type_output);
		case APRSERVICE_MEASUREMENT_TYPE_MILES:      return aprservice_math_convert_speed_from_miles(value, measurement_type_output);
		case APRSERVICE_MEASUREMENT_TYPE_METERS:     return aprservice_math_convert_speed_from_meters(value, measurement_type_output);
		case APRSERVICE_MEASUREMENT_TYPE_KILOMETERS: return aprservice_math_convert_speed_from_kilometers(value, measurement_type_output);
	}

	return 0;
}

AL::Float aprservice_math_get_distance_between_points(AL::Float latitude1, AL::Float longitude1, AL::Float latitude2, AL::Float longitude2, AL::uint8 measurement_type)
{
	auto distance = AL::Serialization::APRS::Position::CalculateDistance(latitude1, longitude1, latitude2, longitude2);

	switch (measurement_type)
	{
		case APRSERVICE_MEASUREMENT_TYPE_FEET:       return distance;
		case APRSERVICE_MEASUREMENT_TYPE_MILES:      return distance / 5280.0f;
		case APRSERVICE_MEASUREMENT_TYPE_METERS:     return distance / 3.281f;
		case APRSERVICE_MEASUREMENT_TYPE_KILOMETERS: return distance / 3281.0f;
	}

	return 0;
}
