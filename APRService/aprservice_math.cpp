#include "aprservice.hpp"

#include <AL/Serialization/APRS/Position.hpp>

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
