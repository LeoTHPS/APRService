#include "APRS.hpp"

#include <array>
#include <cmath>
#include <ctime>
#include <regex>
#include <string>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <type_traits>

// make sure this is never changed
// logic depends on DHM being a subset of MDHM
static_assert(APRS_TIME_MDHM & APRS_TIME_DHM);

constexpr double   APRS_DEG2RAD                      = 3.14159265358979323846 / 180;

constexpr uint8_t  APRS_DATA_EXTENSION_POWER[]       = { 0,  1,  4,  9,   16,  25,  36,  49,   64,   81 };
constexpr uint16_t APRS_DATA_EXTENSION_HEIGHT[]      = { 10, 20, 40, 80,  160, 320, 640, 1280, 2560, 5120 };
constexpr uint16_t APRS_DATA_EXTENSION_DIRECTIVITY[] = { 0,  45, 90, 135, 180, 225, 270, 315,  360 };

struct aprs_path
{
	uint8_t                    size;
	std::array<std::string, 8> chunks;
	std::array<const char*, 9> chunks_c;

	std::string                string;

	size_t                     reference_count;
};

struct aprs_time
	: public tm
{
	int type;
};

struct aprs_packet_data_extensions
{
	uint16_t speed;
	uint16_t course;
	int32_t  altitude;

	struct
	{
		uint8_t  strength;
		uint16_t height;
		uint8_t  gain;
		uint16_t directivity;
	} dfs;

	struct
	{
		uint8_t  power;
		uint16_t height;
		uint8_t  gain;
		uint16_t directivity;
	} phg;

	struct
	{
		uint16_t miles;
	} rng;
};

struct aprs_packet_gps
{
	std::string nmea;
	std::string comment;
};
struct aprs_packet_item
{
	bool        is_alive;
	bool        is_compressed;

	aprs_time   time;

	std::string name;
	std::string comment;

	float       latitude;
	float       longitude;

	char        symbol_table;
	char        symbol_table_key;
};
typedef aprs_packet_item aprs_packet_object;
struct aprs_packet_status
{
	bool        is_time_set;

	aprs_time   time;
	std::string message;
};
struct aprs_packet_message
{
	std::string        id;
	APRS_MESSAGE_TYPES type;
	std::string        content;
	std::string        destination;
};
struct aprs_packet_weather
{
	aprs_time   time;

	uint16_t    wind_speed;
	uint16_t    wind_speed_gust;
	uint16_t    wind_direction;

	uint16_t    rainfall_last_hour;
	uint16_t    rainfall_last_24_hours;
	uint16_t    rainfall_since_midnight;

	uint8_t     humidity;
	int16_t     temperature;
	uint32_t    barometric_pressure;

	std::string type;
	char        software;
};
struct aprs_packet_position
{
	int         flags;

	aprs_time   time;

	float       latitude;
	float       longitude;

	std::string comment;

	char        symbol_table;
	char        symbol_table_key;
};
struct aprs_packet_telemetry
{
	APRS_TELEMETRY_TYPES                      type;

	std::array<aprs_telemetry_eqn, 15>        eqns;
	std::array<const aprs_telemetry_eqn*, 16> eqns_c;
	size_t                                    eqns_count;

	std::array<std::string, 10>               units;
	std::array<const char*, 11>               units_c;
	size_t                                    units_count;

	std::array<std::string, 10>               params;
	std::array<const char*, 11>               params_c;
	size_t                                    params_count;

	std::array<uint8_t, 5>                    analog_u8;
	std::array<const uint8_t*, 6>             analog_u8_c;
	std::array<float, 5>                      analog_float;
	std::array<const float*, 6>               analog_float_c;
	uint8_t                                   digital;
	uint16_t                                  sequence;
	std::string                               comment;
};
struct aprs_packet_user_defined
{
	char        id;
	char        type;
	std::string data;
};
struct aprs_packet_third_party
{
	std::string content;
};
struct aprs_packet
{
	APRS_PACKET_TYPES           type;
	aprs_path*                  path;
	std::string                 igate;
	std::string                 tocall;
	std::string                 sender;
	std::string                 content;
	std::string                 qconstruct;
	aprs_packet_data_extensions extensions;

	std::string                 string;

	size_t                      reference_count;

	union
	{
		aprs_packet_gps*          gps;
		aprs_packet_item*         item;
		aprs_packet_object*       object;
		aprs_packet_status*       status;
		aprs_packet_message*      message;
		aprs_packet_weather*      weather;
		aprs_packet_position*     position;
		aprs_packet_telemetry*    telemetry;
		aprs_packet_user_defined* user_defined;
		aprs_packet_third_party*  third_party;
	};
};

struct aprs_strlen_result
{
	bool   valid;
	size_t length;

	constexpr operator bool () const
	{
		return valid;
	}

	constexpr operator size_t () const
	{
		return length;
	}

	constexpr bool operator > (int value) const
	{
		return valid && (length > value);
	}
	constexpr bool operator < (int value) const
	{
		return valid && (length < value);
	}

	constexpr bool operator >= (int value) const
	{
		return valid && (length >= value);
	}
	constexpr bool operator <= (int value) const
	{
		return valid && (length <= value);
	}

	constexpr bool operator == (int value) const
	{
		return valid && (length == value);
	}
	constexpr bool operator != (int value) const
	{
		return valid && (length != value);
	}

	constexpr bool operator > (size_t value) const
	{
		return valid && (length > value);
	}
	constexpr bool operator < (size_t value) const
	{
		return valid && (length < value);
	}

	constexpr bool operator >= (size_t value) const
	{
		return valid && (length >= value);
	}
	constexpr bool operator <= (size_t value) const
	{
		return valid && (length <= value);
	}

	constexpr bool operator == (size_t value) const
	{
		return valid && (length == value);
	}
	constexpr bool operator != (size_t value) const
	{
		return valid && (length != value);
	}

	constexpr bool operator > (const aprs_strlen_result& value) const
	{
		return valid && value && (length > value.length);
	}
	constexpr bool operator < (const aprs_strlen_result& value) const
	{
		return valid && value && (length < value.length);
	}

	constexpr bool operator >= (const aprs_strlen_result& value) const
	{
		return valid && value && (length >= value.length);
	}
	constexpr bool operator <= (const aprs_strlen_result& value) const
	{
		return valid && value && (length <= value.length);
	}

	constexpr bool operator == (const aprs_strlen_result& value) const
	{
		return valid && value && (length == value.length);
	}
	constexpr bool operator != (const aprs_strlen_result& value) const
	{
		return valid && value && (length != value.length);
	}
};

typedef bool(*aprs_packet_decode_handler)(aprs_packet* packet);
typedef void(*aprs_packet_encode_handler)(aprs_packet* packet, std::stringstream& ss);

struct aprs_compressed_location
{
	uint16_t speed;
	uint16_t course;
	int32_t  altitude;

	float    latitude;
	float    longitude;

	char     symbol_table;
	char     symbol_table_key;
};

struct aprs_packet_encoder_context
{
	APRS_PACKET_TYPES          type;
	aprs_packet_encode_handler function;
};

struct aprs_packet_decoder_context
{
	char                       ident;
	aprs_packet_decode_handler function;
};

auto               aprs_string_length(const char* string, bool stop_at_whitespace = false)
{
	aprs_strlen_result result = { .valid = false, .length = 0 };

	if (string)
	{
		result.valid = true;

		for (auto c = string; *c; ++c, ++result.length)
			if (stop_at_whitespace && isspace(*c))
				break;
	}

	return result;
}
bool               aprs_string_contains(const char* string, size_t length, char value)
{
	for (size_t i = 0; *string && (i < length); ++i, ++string)
		if (*string == value)
			return true;

	return false;
}

template<typename T>
inline constexpr T aprs_from_float(float value, float& fraction)
{
	fraction = modff(value, &value);

	return value;
}

bool               aprs_regex_match(std::cmatch& match, const std::regex& regex, const char* string)
{
	if (!string)
		return false;

	try
	{
		if (!std::regex_match(string, match, regex))
			return false;
	}
	catch (const std::regex_error& exception)
	{
		std::cerr << exception.what() << std::endl;

		return false;
	}

	return true;
}

bool               aprs_validate_time(const aprs_time* value)
{
	if (!value)
		return false;

	if (value->type & APRS_TIME_DHM)
	{
		if ((value->tm_mday < 0) || (value->tm_mday > 31))
			return false;

		if ((value->tm_hour < 0) || (value->tm_hour >= 24))
			return false;

		if ((value->tm_min < 0) || (value->tm_min >= 60))
			return false;
	}

	if (value->type & APRS_TIME_HMS)
	{
		if ((value->tm_hour < 0) || (value->tm_hour >= 24))
			return false;

		if ((value->tm_min < 0) || (value->tm_min >= 60))
			return false;

		if ((value->tm_sec < 0) || (value->tm_sec > 60))
			return false;
	}

	if (value->type & APRS_TIME_MDHM)
		if ((value->tm_mon < 0) || (value->tm_mon > 12))
			return false;

	return true;
}
bool               aprs_validate_symbol(char table, char key)
{
	if ((table == '/') || (table == '\\'))
		if ((key >= '!') && (key <= '}'))
			return true;

	return false;
}
bool               aprs_validate_string(const char* value, size_t length, bool(*is_char_valid)(size_t index, char value))
{
	size_t i = 0;

	for (; *value && (i < length); ++i, ++value)
		if (!is_char_valid(i, *value))
			return false;

	return i == length;
}
auto               aprs_validate_name(const char* value)
{
	aprs_strlen_result result = { .valid = false, .length = 0 };

	if (auto length = aprs_string_length(value, true); length && (length <= 9))
	{
		result.valid  = true;
		result.length = length;
	}

	return result;
}
auto               aprs_validate_path(const char* value)
{
	aprs_strlen_result result = { .valid = false, .length = 0 };

	if (auto length = aprs_string_length(value, true); length && ((length <= 9) || ((length == 10) && (value[9] == '*'))))
	{
		result.valid  = true;
		result.length = length;
	}

	return result;
}
auto               aprs_validate_station(const char* value)
{
	// TODO: this could be better..

	aprs_strlen_result result     = { .valid = true, .length = 0 };
	bool               ssid_begin = false;

	for (; *value; ++value, ++result.length)
		if ((*value == '-') && !ssid_begin)
			ssid_begin = true;
		else if ((*value < '0') || (*value > '9'))
			if ((*value < 'A') || (*value > 'Z'))
			{
				result.valid = false;

				break;
			}

	if (!result.length || (result.length > 9))
		result.valid = false;

	return result;
}
bool               aprs_validate_status(const char* value, size_t max_length)
{
	size_t i = 0;

	for (; *value && (i < max_length); ++i, ++value)
		if (!isprint(*value) || (*value == '|') || (*value == '~'))
			return false;

	return i <= max_length;
}
auto               aprs_validate_comment(const char* value, size_t max_length)
{
	aprs_strlen_result result = { .valid = false, .length = 0 };

	if (auto length = aprs_string_length(value); length && (length <= max_length))
	{
		result.valid  = true;
		result.length = length;
	}

	return result;
}
auto               aprs_validate_user_defined_data(const char* value)
{
	aprs_strlen_result result = { .valid = true, .length = 0 };

	for (; *value; ++value, ++result.length)
		if (!isprint(*value))
		{
			result.valid = false;

			break;
		}

	return result;
}

template<typename T>
T                  aprs_decode_int(const char* string, size_t max_length)
{
	size_t i     = 0;
	T      value = 0;

	if (*string == '-')
	{
		++i;
		++string;

		value = 1 << ((sizeof(T) * 8) - 1);
	}

	for (; *string && (i < max_length); ++i, ++string)
		value = 10 * value + (*string - '0');

	return value;
}
template<typename T>
T                  aprs_decode_int_ex(const char* string, size_t max_length, char(*get_char)(size_t index, char value))
{
	size_t i     = 0;
	T      value = 0;

	if (*string == '-')
	{
		++i;
		++string;

		value = 1 << ((sizeof(T) * 8) - 1);
	}

	for (; *string && (i < max_length); ++i, ++string)
		value = 10 * value + (get_char(i, *string) - '0');

	return value;
}
inline float       aprs_decode_float(const char* string)
{
	if (!string)
		return 0;

	return strtof(string, nullptr);
}
bool               aprs_decode_time(aprs_time& value, const char* string, char type)
{
	static auto string_is_valid = [](size_t index, char value)->bool
	{
		return isdigit(value);
	};

	if (!aprs_validate_string(string, 6, string_is_valid))
		return false;

	tm time = {};

	switch (type)
	{
		case 'h': // HMS
			time.tm_hour = aprs_decode_int<uint8_t>(&string[0], 2);
			time.tm_min  = aprs_decode_int<uint8_t>(&string[2], 2);
			time.tm_sec  = aprs_decode_int<uint8_t>(&string[4], 2);
			value = { time, APRS_TIME_HMS };
			return (value.tm_hour < 24) && (value.tm_min < 60) && (value.tm_sec < 60);

		case 'z': // DHM
		case '/':
			time.tm_mday = aprs_decode_int<uint8_t>(&string[0], 2);
			time.tm_hour = aprs_decode_int<uint8_t>(&string[2], 2);
			time.tm_min  = aprs_decode_int<uint8_t>(&string[4], 2);
			value = { time, APRS_TIME_DHM };
			return (value.tm_mday <= 31) && (value.tm_hour < 24) && (value.tm_min < 60);

		case 0: // MDHM
			time.tm_mon  = aprs_decode_int<uint8_t>(&string[0], 2);
			time.tm_mday = aprs_decode_int<uint8_t>(&string[2], 2);
			time.tm_hour = aprs_decode_int<uint8_t>(&string[4], 2);
			time.tm_min  = aprs_decode_int<uint8_t>(&string[6], 2);
			value = { time, APRS_TIME_MDHM };
			return (time.tm_mon <= 12) && (value.tm_mday <= 31) && (value.tm_hour < 24) && (value.tm_min < 60);
	}

	return false;
}
bool               aprs_decode_latitude(float& value, const char* string, char hemisphere)
{
	static auto string_is_valid = [](size_t index, char value)->bool
	{
		switch (index)
		{
			case 0:
			case 1:
			case 2:
			case 3:
			case 5:
			case 6:
				return (value == ' ') || ((value >= '0') && (value <= '9'));

			case 4:
				return value == '.';
		}

		return false;
	};

	if (!aprs_validate_string(string, 7, string_is_valid))
		return false;

	static auto get_char = [](size_t index, char value)
	{
		return (value == ' ') ? '0' : value;
	};

	auto hours   = aprs_decode_int_ex<uint8_t>(&string[0], 2, get_char);
	auto minutes = aprs_decode_int_ex<uint8_t>(&string[2], 2, get_char);
	auto seconds = aprs_decode_int_ex<uint8_t>(&string[5], 2, get_char);

	value = hours + (minutes / 60.0f) + (seconds / 6000.0f);

	if (hemisphere == 'S')
		value *= -1;

	return true;
}
bool               aprs_decode_longitude(float& value, const char* string, char hemisphere)
{
	static auto string_is_valid = [](size_t index, char value)->bool
	{
		switch (index)
		{
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 6:
			case 7:
				return isdigit(value);

			case 5:
				return value == '.';
		}

		return false;
	};

	if (!aprs_validate_string(string, 8, string_is_valid))
		return false;

	static auto get_char = [](size_t index, char value)
	{
		return (value == ' ') ? '0' : value;
	};

	auto hours   = aprs_decode_int_ex<uint8_t>(&string[0], 3, get_char);
	auto minutes = aprs_decode_int_ex<uint8_t>(&string[3], 2, get_char);
	auto seconds = aprs_decode_int_ex<uint8_t>(&string[6], 2, get_char);

	value = hours + (minutes / 60.0f) + (seconds / 6000.0f);

	if (hemisphere == 'W')
		value *= -1;

	return true;
}

bool               aprs_decode_compressed_location(aprs_compressed_location& value, const char* string)
{
	static auto string_is_valid = [](size_t index, char value)->bool
	{
		switch (index)
		{
			case 0:
			case 9:
				return true;

			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
				return (value >= '!') && (value <= '{');

			case 10:
			case 11:
			case 12:
				return ((value >= '!') && (value <= '{')) || (value == ' ');
		}

		return false;
	};

	if (!aprs_validate_string(string, 13, string_is_valid))
		return false;

	auto cs        = &string[10];
	auto type      = string[12] - 33;
	auto latitude  = &string[1];
	auto longitude = &string[5];

	value.speed            = 0;
	value.course           = 0;
	value.altitude         = 0;
	value.latitude         = 90 - (((latitude[0] - 33) * 753571) + ((latitude[1] - 33) * 8281) + ((latitude[2] - 33) * 91) + (latitude[3] - 33)) / 380926.0f;
	value.longitude        = -180 + (((longitude[0] - 33) * 753571) + ((longitude[1] - 33) * 8281) + ((longitude[2] - 33) * 91) + (longitude[3] - 33)) / 190463.0f;
	value.symbol_table     = string[0];
	value.symbol_table_key = string[9];

	if (cs[0] != ' ')
	{
		auto c = cs[0] - 33;
		auto s = cs[1] - 33;

		if ((type & 0x10) == 0x10)
			value.altitude = (c * 91) + s;
		else if ((cs[0] >= '!') && (cs[0] <= 'z'))
		{
			value.speed  = s;
			value.course = c;
		}
	}

	return true;
}
void               aprs_encode_compressed_location(const aprs_compressed_location& value, std::stringstream& ss)
{
	static constexpr uint32_t divs[4]      = { 753571, 8281, 91, 1 };
	int32_t                   latlong[]    = { (int32_t)(380926 * (90 - value.latitude)), (int32_t)(190463 * (180 + value.longitude)) };
	char                      altitude[2]  = { (char)((value.altitude / 91) + 33), (char)((value.altitude % 91) + 33) };
	char                      latitude[4]  = {};
	char                      longitude[4] = {};

	for (int i = 0; i < 4; ++i)
	{
		latitude[i] = (latlong[0] / divs[i]) + 33;
		latlong[0] %= divs[i];
	}

	for (int i = 0; i < 4; ++i)
	{
		longitude[i] = (latlong[1] / divs[i]) + 33;
		latlong[1]  %= divs[i];
	}

	ss << value.symbol_table;
	ss << latitude[0] << latitude[1] << latitude[2] << latitude[3];
	ss << longitude[0] << longitude[1] << longitude[2] << longitude[3];
	ss << value.symbol_table_key;
	ss << altitude[0] << altitude[1];
	ss << (uint8_t)0x51;
}

void               aprs_packet_decode_data_extensions(aprs_packet* packet, std::string& string)
{
	static const std::regex regex_dfs("^DFS(\\d)(\\d)(\\d)(\\d)");
	static const std::regex regex_phg("^PHG(\\d)(\\d)(\\d)(\\d)");
	static const std::regex regex_rng("^RNG(\\d{4})");
	static const std::regex regex_altitude("\\/A=(-?\\d{1,6})");
	static const std::regex regex_course_speed("^((\\d{3})\\/(\\d{3}))");

	switch (packet->type)
	{
		case APRS_PACKET_TYPE_ITEM:
		case APRS_PACKET_TYPE_OBJECT:
		case APRS_PACKET_TYPE_POSITION:
		{
			std::cmatch match;

			if (aprs_regex_match(match, regex_course_speed, string.c_str()))
			{
				packet->extensions.speed  = aprs_decode_int<uint16_t>(match[3].first, 3);
				packet->extensions.course = aprs_decode_int<uint16_t>(match[2].first, 3);

				string = string.substr(7);
			}
			else if (aprs_regex_match(match, regex_phg, string.c_str()))
			{
				auto power       = aprs_decode_int<uint8_t>(match[1].first, 1);
				auto height      = aprs_decode_int<uint16_t>(match[2].first, 1);
				auto gain        = aprs_decode_int<uint8_t>(match[3].first, 1);
				auto directivity = aprs_decode_int<uint16_t>(match[4].first, 1);

				if ((power < 10) && (height < 10) && (directivity < 10))
				{
					packet->extensions.phg.power       = APRS_DATA_EXTENSION_POWER[power];
					packet->extensions.phg.height      = APRS_DATA_EXTENSION_HEIGHT[height];
					packet->extensions.phg.gain        = gain;
					packet->extensions.phg.directivity = APRS_DATA_EXTENSION_DIRECTIVITY[directivity];

					string = string.substr(7);
				}
			}
			else if (aprs_regex_match(match, regex_rng, string.c_str()))
			{
				packet->extensions.rng.miles = aprs_decode_int<uint16_t>(match[1].first, 4);

				string = string.substr(7);
			}
			else if (aprs_regex_match(match, regex_dfs, string.c_str()))
			{
				auto strength    = aprs_decode_int<uint8_t>(match[1].first, 1);
				auto height      = aprs_decode_int<uint16_t>(match[2].first, 1);
				auto gain        = aprs_decode_int<uint8_t>(match[3].first, 1);
				auto directivity = aprs_decode_int<uint16_t>(match[4].first, 1);

				if ((height < 10) && (directivity < 10))
				{
					packet->extensions.dfs.strength    = strength;
					packet->extensions.dfs.height      = APRS_DATA_EXTENSION_HEIGHT[height];
					packet->extensions.dfs.gain        = gain;
					packet->extensions.dfs.directivity = APRS_DATA_EXTENSION_DIRECTIVITY[directivity];

					string = string.substr(7);
				}
			}

			if (aprs_regex_match(match, regex_altitude, string.c_str()))
			{
				packet->extensions.altitude = aprs_decode_int<int32_t>(match[1].first, match[1].length());

				if (auto i = string.find(match[0].first, 0, match[0].length()); i != std::string::npos)
					string = string.erase(i, match[0].length());
			}
		}
		break;
	}
}
void               aprs_packet_encode_data_extensions(aprs_packet* packet, std::stringstream& ss)
{
	static auto get_table_index     = [](auto value, const auto* values, size_t count)->size_t
	{
		size_t i = 0;

		for (; i < count; ++i, ++values)
			if (*values > value)
				break;

		return i - 1;
	};
	static auto encode_dfs          = [](const aprs_packet_data_extensions& extensions, std::stringstream& ss)
	{
		if (extensions.dfs.strength || extensions.dfs.height || extensions.dfs.gain || extensions.dfs.directivity)
		{
			ss << "DFS";
			ss << (int)extensions.dfs.strength;
			ss << get_table_index(extensions.dfs.height, APRS_DATA_EXTENSION_HEIGHT, 10);
			ss << (int)extensions.dfs.gain;
			ss << get_table_index(extensions.dfs.directivity, APRS_DATA_EXTENSION_DIRECTIVITY, 9);

			return true;
		}

		return false;
	};
	static auto encode_phg          = [](const aprs_packet_data_extensions& extensions, std::stringstream& ss)
	{
		if (extensions.phg.power || extensions.phg.height || extensions.phg.gain || extensions.phg.directivity)
		{
			ss << "PHG";
			ss << get_table_index(extensions.phg.power, APRS_DATA_EXTENSION_POWER, 10);
			ss << get_table_index(extensions.phg.height, APRS_DATA_EXTENSION_HEIGHT, 10);
			ss << (int)extensions.phg.gain;
			ss << get_table_index(extensions.phg.directivity, APRS_DATA_EXTENSION_DIRECTIVITY, 9);

			return true;
		}

		return false;
	};
	static auto encode_rng          = [](const aprs_packet_data_extensions& extensions, std::stringstream& ss)
	{
		if (extensions.rng.miles)
		{
			ss << "RNG";
			ss << std::setfill('0') << std::setw(4) << extensions.rng.miles;

			return true;
		}

		return false;
	};
	static auto encode_altitude     = [](const aprs_packet_data_extensions& extensions, std::stringstream& ss)
	{
		if (extensions.altitude)
		{
			ss << "/A=" << std::setfill('0') << std::setw(6) << extensions.altitude;

			return true;
		}

		return false;
	};
	static auto encode_course_speed = [](const aprs_packet_data_extensions& extensions, std::stringstream& ss)
	{
		if (extensions.course || extensions.speed)
		{
			ss << std::setfill('0') << std::setw(3) << extensions.course;
			ss << '/';
			ss << std::setfill('0') << std::setw(3) << extensions.speed;

			return true;
		}

		return false;
	};

	switch (packet->type)
	{
		case APRS_PACKET_TYPE_ITEM:
		case APRS_PACKET_TYPE_OBJECT:
		{
			encode_course_speed(packet->extensions, ss) ||
				encode_phg(packet->extensions, ss) ||
				encode_rng(packet->extensions, ss) ||
				encode_dfs(packet->extensions, ss);

			if (!packet->object->is_compressed)
				encode_altitude(packet->extensions, ss);
		}
		break;

		case APRS_PACKET_TYPE_POSITION:
			if (!(packet->position->flags & APRS_POSITION_FLAG_MIC_E))
			{
				encode_course_speed(packet->extensions, ss) ||
					encode_phg(packet->extensions, ss) ||
					encode_rng(packet->extensions, ss) ||
					encode_dfs(packet->extensions, ss);

				if (!(packet->position->flags & APRS_POSITION_FLAG_COMPRESSED))
					encode_altitude(packet->extensions, ss);
			}
			break;
	}
}

bool               aprs_packet_decode_mic_e(aprs_packet* packet)
{
	// packet->type     = APRS_PACKET_TYPE_POSITION;
	// packet->position = new aprs_packet_position { .flags = APRS_POSITION_FLAG_MIC_E };

	// TODO: decode mic-e

	return false;
}
bool               aprs_packet_decode_mic_e_old(aprs_packet* packet)
{
	// packet->type     = APRS_PACKET_TYPE_POSITION;
	// packet->position = new aprs_packet_position { .flags = APRS_POSITION_FLAG_MIC_E };

	// TODO: decode mic-e (old)

	return false;
}
bool               aprs_packet_decode_raw_gps(aprs_packet* packet)
{
	static const std::regex regex("^((\\$[^,]+)(,[^,]*)+,(...))(.*)$");

	std::cmatch match;

	if (!aprs_regex_match(match, regex, packet->content.c_str()))
		return false;

	packet->type = APRS_PACKET_TYPE_GPS;
	packet->gps  = new aprs_packet_gps { .nmea = match[1].str(), .comment = match[5].str() };

	return true;
}
bool               aprs_packet_decode_item(aprs_packet* packet)
{
	static const std::regex regex("^\\)([^ !_]{3,9}) *([!_])([0-9 .]{7})([NS])(.)([0-9 .]{8})([EW])(.)(.*)$");
	static const std::regex regex_compressed("^\\)([^ !_]{3,9}) *([!_])(.{13})(.*)$");

	std::cmatch match;

	if (aprs_regex_match(match, regex, packet->content.c_str()))
	{
		float latitude;
		float longitude;

		if (!aprs_decode_latitude(latitude, match[3].first, *match[4].first))
			return false;

		if (!aprs_decode_longitude(longitude, match[6].first, *match[7].first))
			return false;

		packet->type       = APRS_PACKET_TYPE_ITEM;
		packet->extensions = {};
		packet->item       = new aprs_packet_item
		{
			.is_alive         = *match[2].first == '!',
			.is_compressed    = false,
			.name             = match[1].str(),
			.comment          = match[9].str(),
			.latitude         = latitude,
			.longitude        = longitude,
			.symbol_table     = *match[5].first,
			.symbol_table_key = *match[8].first
		};

		aprs_packet_decode_data_extensions(packet, packet->item->comment);

		return true;
	}

	if (aprs_regex_match(match, regex_compressed, packet->content.c_str()))
	{
		aprs_compressed_location location;

		if (!aprs_decode_compressed_location(location, match[3].first))
			return false;

		packet->type       = APRS_PACKET_TYPE_ITEM;
		packet->extensions = {};
		packet->item       = new aprs_packet_item
		{
			.is_alive         = *match[2].first == '!',
			.is_compressed    = true,
			.name             = match[1].str(),
			.comment          = match[4].str(),
			.latitude         = location.latitude,
			.longitude        = location.longitude,
			.symbol_table     = location.symbol_table,
			.symbol_table_key = location.symbol_table_key
		};

		aprs_packet_decode_data_extensions(packet, packet->item->comment);

		return true;
	}

	return false;
}
bool               aprs_packet_decode_test(aprs_packet* packet)
{
	// packet->type = APRS_PACKET_TYPE_TEST;

	// TODO: decode test

	return false;
}
bool               aprs_packet_decode_query(aprs_packet* packet)
{
	// packet->type = APRS_PACKET_TYPE_QUERY;

	// TODO: decode query

	return false;
}
bool               aprs_packet_decode_object(aprs_packet* packet)
{
	static const std::regex regex("^;([^ *_]{3,9}) *([*_])(\\d{6})([z\\/h])([0-9 .]{7})([NS])(.)([0-9 .]{8})([EW])(.)(.*)$");
	static const std::regex regex_compressed("^;([^ *_]{3,9}) *([*_])(\\d{6})([z\\/h])(.{13})(.*)$");

	aprs_time   time;
	std::cmatch match;

	if (aprs_regex_match(match, regex, packet->content.c_str()))
	{
		float latitude;
		float longitude;

		if (!aprs_decode_time(time, match[3].first, *match[4].first))
			return false;

		if (!aprs_decode_latitude(latitude, match[5].first, *match[6].first))
			return false;

		if (!aprs_decode_longitude(longitude, match[8].first, *match[9].first))
			return false;

		packet->type       = APRS_PACKET_TYPE_OBJECT;
		packet->extensions = {};
		packet->object     = new aprs_packet_object
		{
			.is_alive         = *match[2].first == '*',
			.is_compressed    = false,
			.time             = time,
			.name             = match[1].str(),
			.comment          = match[11].str(),
			.latitude         = latitude,
			.longitude        = longitude,
			.symbol_table     = *match[7].first,
			.symbol_table_key = *match[10].first
		};

		aprs_packet_decode_data_extensions(packet, packet->object->comment);

		return true;
	}

	if (aprs_regex_match(match, regex_compressed, packet->content.c_str()))
	{
		aprs_compressed_location location;

		if (!aprs_decode_time(time, match[3].first, *match[4].first))
			return false;

		if (!aprs_decode_compressed_location(location, match[5].first))
			return false;

		packet->type       = APRS_PACKET_TYPE_OBJECT;
		packet->extensions = {};
		packet->object     = new aprs_packet_object
		{
			.is_alive         = *match[2].first == '*',
			.is_compressed    = true,
			.time             = time,
			.name             = match[1].str(),
			.comment          = match[6].str(),
			.latitude         = location.latitude,
			.longitude        = location.longitude,
			.symbol_table     = location.symbol_table,
			.symbol_table_key = location.symbol_table_key
		};

		aprs_packet_decode_data_extensions(packet, packet->object->comment);

		return true;
	}

	return false;
}
bool               aprs_packet_decode_status(aprs_packet* packet)
{
	static const std::regex regex("^>(.*)$");
	static const std::regex regex_time("^>(\\d{6})([z\\/h])(.*)$");

	aprs_time   time;
	std::cmatch match;

	if (aprs_regex_match(match, regex, packet->content.c_str()))
	{
		packet->type   = APRS_PACKET_TYPE_STATUS;
		packet->status = new aprs_packet_status
		{
			.is_time_set = false,
			.message     = match[1].str()
		};

		return true;
	}

	if (aprs_regex_match(match, regex_time, packet->content.c_str()))
	{
		if (!aprs_decode_time(time, match[1].first, *match[2].first))
			return false;

		packet->type   = APRS_PACKET_TYPE_STATUS;
		packet->status = new aprs_packet_status
		{
			.is_time_set = true,
			.time        = time,
			.message     = match[3].str()
		};

		return true;
	}

	return false;
}
bool               aprs_packet_decode_message_telemetry_params(aprs_packet* packet, const char* content)
{
	packet->type      = APRS_PACKET_TYPE_TELEMETRY;
	packet->telemetry = new aprs_packet_telemetry
	{
		.type = APRS_TELEMETRY_TYPE_PARAMS
	};

	if (auto param = strtok((char*)content, ",")) do
	{
		packet->telemetry->params[packet->telemetry->params_count]   = param;
		packet->telemetry->params_c[packet->telemetry->params_count] = packet->telemetry->params[packet->telemetry->params_count].c_str();
	}
	while ((++packet->telemetry->params_count < 10) && (param = strtok(nullptr, ",")));

	return true;
}
bool               aprs_packet_decode_message_telemetry_units(aprs_packet* packet, const char* content)
{
	packet->type      = APRS_PACKET_TYPE_TELEMETRY;
	packet->telemetry = new aprs_packet_telemetry
	{
		.type = APRS_TELEMETRY_TYPE_UNITS
	};

	if (auto unit = strtok((char*)content, ",")) do
	{
		packet->telemetry->units[packet->telemetry->units_count]   = unit;
		packet->telemetry->units_c[packet->telemetry->units_count] = packet->telemetry->units[packet->telemetry->units_count].c_str();
	}
	while ((++packet->telemetry->units_count < 10) && (unit = strtok(nullptr, ",")));

	return true;
}
bool               aprs_packet_decode_message_telemetry_eqns(aprs_packet* packet, const char* content)
{
	packet->type      = APRS_PACKET_TYPE_TELEMETRY;
	packet->telemetry = new aprs_packet_telemetry
	{
		.type = APRS_TELEMETRY_TYPE_EQNS
	};

	const char* chunks[15]   = {};
	size_t      chunks_count = 0;

	if (auto chunk = strtok((char*)content, ",")) do
		chunks[chunks_count] = chunk;
	while ((++chunks_count < 15) && (chunk = strtok(nullptr, ",")));

	for (size_t i = 0; i < 5; ++i, ++packet->telemetry->eqns_count)
	{
		packet->telemetry->eqns[i]   = { aprs_decode_float(chunks[i * 3]), aprs_decode_float(chunks[(i * 3) + 1]), aprs_decode_float(chunks[(i * 3) + 2]) };
		packet->telemetry->eqns_c[i] = &packet->telemetry->eqns[i];
	}

	return true;
}
bool               aprs_packet_decode_message_telemetry_bits(aprs_packet* packet, const char* content)
{
	packet->type      = APRS_PACKET_TYPE_TELEMETRY;
	packet->telemetry = new aprs_packet_telemetry
	{
		.type    = APRS_TELEMETRY_TYPE_BITS,
		.digital = 0
	};

	if (auto bits = strtok((char*)content, ","))
	{
		for (size_t i = 0; i < 8; ++i)
			if (bits[i] != '0')
				packet->telemetry->digital |= 1 << i;

		if (auto comment = strtok(nullptr, ""))
			packet->telemetry->comment = comment;
	}

	return true;
}
bool               aprs_packet_decode_message_telemetry(aprs_packet* packet, std::cmatch& match)
{
	auto type = match[2].str();

	     if (!type.compare("PARM")) return aprs_packet_decode_message_telemetry_params(packet, match[3].first);
	else if (!type.compare("UNIT")) return aprs_packet_decode_message_telemetry_units(packet, match[3].first);
	else if (!type.compare("EQNS")) return aprs_packet_decode_message_telemetry_eqns(packet, match[3].first);
	else if (!type.compare("BITS")) return aprs_packet_decode_message_telemetry_bits(packet, match[3].first);

	return false;
}
bool               aprs_packet_decode_message(aprs_packet* packet)
{
	static const std::regex regex("^:([^ :]+):(.+?)(\\{(.+))?$");
	static const std::regex regex_ack("^ack\\S{1,5}$");
	static const std::regex regex_rej("^rej\\S{1,5}$");
	static const std::regex regex_bln("^BLN(\\S{1,6})$");
	static const std::regex regex_telemetry("^:([^:]+):(PARM|UNIT|EQNS|BITS).(.*)$");

	std::cmatch match;

	if (aprs_regex_match(match, regex_telemetry, packet->content.c_str()))
		return aprs_packet_decode_message_telemetry(packet, match);
	else if (!aprs_regex_match(match, regex, packet->content.c_str()))
		return false;

	auto id          = (match.size() < 4) ? "" : match[4].str();
	auto content     = match[2].str();
	auto destination = match[1].str();

	if (id.length() > 5)
		return false;

	if (!aprs_validate_name(destination.c_str()))
		return false;

	if (!aprs_validate_comment(content.c_str(), 67))
		return false;

	packet->type    = APRS_PACKET_TYPE_MESSAGE;
	packet->message = new aprs_packet_message
	{
		.id          = std::move(id),
		.type        = APRS_MESSAGE_TYPE_MESSAGE,
		.content     = std::move(content),
		.destination = std::move(destination)
	};

	if (aprs_regex_match(match, regex_ack, packet->message->content.c_str()))
	{
		packet->message->type = APRS_MESSAGE_TYPE_ACK;
		packet->message->content.clear();
	}
	else if (aprs_regex_match(match, regex_rej, packet->message->content.c_str()))
	{
		packet->message->type = APRS_MESSAGE_TYPE_REJECT;
		packet->message->content.clear();
	}
	else if (aprs_regex_match(match, regex_bln, packet->message->destination.c_str()))
	{
		packet->message->type        = APRS_MESSAGE_TYPE_BULLETIN;
		packet->message->destination = match[1].str();
	}

	return true;
}
bool               aprs_packet_decode_weather(aprs_packet* packet)
{
	aprs_time time;

	if (!aprs_decode_time(time, &packet->content[1], 0))
		return false;

	static auto decode_next_chunk = [](const char*& string, char& key, int& value)
	{
		if (!isalpha(*string) || (!isdigit(string[1]) && (string[1] != '.') && (string[1] != ' ')))
			return false;

		key   = *string++;
		value = 0;

		if (*string == '-')
		{
			value = 0x8000;
			++string;
		}

		for (; *string && (isdigit(*string) || (*string == '.') || (*string == ' ')); ++string)
			switch (*string)
			{
				case '.':
				case ' ':
					break;

				default:
					value = 10 * value + (*string - '0');
					break;
			}

		return true;
	};

	packet->type    = APRS_PACKET_TYPE_WEATHER;
	packet->weather = new aprs_packet_weather { .time = time };

	char        key;
	int         value;
	const char* string = &packet->content[9];

	while (decode_next_chunk(string, key, value))
		switch (key)
		{
			case 'c': packet->weather->wind_direction          = value; break;
			case 's': packet->weather->wind_speed              = value; break;
			case 'g': packet->weather->wind_speed_gust         = value; break;
			case 't': packet->weather->temperature             = value; break;
			case 'r': packet->weather->rainfall_last_hour      = value; break;
			case 'p': packet->weather->rainfall_last_24_hours  = value; break;
			case 'P': packet->weather->rainfall_since_midnight = value; break;
			case 'h': packet->weather->humidity                = value; break;
			case 'b': packet->weather->barometric_pressure     = value; break;
		}

	if (*string)
	{
		packet->weather->software = *string;

		if (*(++string))
			packet->weather->type = *string;
	}

	return true;
}
bool               aprs_packet_decode_weather_space(aprs_packet* packet)
{
	// packet->type = APRS_PACKET_TYPE_WEATHER;

	// TODO: decode space weather

	return false;
}
bool               aprs_packet_decode_weather_peet_bros_uii(aprs_packet* packet)
{
	// packet->type = APRS_PACKET_TYPE_WEATHER;

	// TODO: decode peet bros uii weather station

	return false;
}
bool               aprs_packet_decode_position(aprs_packet* packet, int flags)
{
	static const std::regex regex("^[!=]([0-9 .]{7})([NS])(.)([0-9 .]{8})([EW])(.)(.*)$");
	static const std::regex regex_time("^[\\/@](\\d{6})([z\\/h])([0-9 .]{7})([NS])(.)([0-9 .]{8})([EW])(.)(.*)$");
	static const std::regex regex_compressed("^[!=\\/@](.{13})(.*)$");

	std::cmatch match;

	if (aprs_regex_match(match, regex, packet->content.c_str()))
	{
		float latitude;
		float longitude;

		if (!aprs_decode_latitude(latitude, match[1].str().c_str(), *match[2].first))
			return false;

		if (!aprs_decode_longitude(longitude, match[4].str().c_str(), *match[5].first))
			return false;

		packet->type       = APRS_PACKET_TYPE_POSITION;
		packet->extensions = {};
		packet->position   = new aprs_packet_position
		{
			.flags            = flags,
			.latitude         = latitude,
			.longitude        = longitude,
			.comment          = match[7].str(),
			.symbol_table     = *match[3].first,
			.symbol_table_key = *match[6].first
		};

		aprs_packet_decode_data_extensions(packet, packet->position->comment);

		return true;
	}

	if (aprs_regex_match(match, regex_time, packet->content.c_str()))
	{
		aprs_time time;
		float     latitude;
		float     longitude;

		if (!aprs_decode_time(time, match[1].str().c_str(), *match[2].first))
			return false;

		if (!aprs_decode_latitude(latitude, match[3].str().c_str(), *match[4].first))
			return false;

		if (!aprs_decode_longitude(longitude, match[6].str().c_str(), *match[7].first))
			return false;

		packet->type       = APRS_PACKET_TYPE_POSITION;
		packet->extensions = {};
		packet->position   = new aprs_packet_position
		{
			.flags            = flags | APRS_POSITION_FLAG_TIME,
			.time             = time,
			.latitude         = latitude,
			.longitude        = longitude,
			.comment          = match[9].str(),
			.symbol_table     = *match[5].first,
			.symbol_table_key = *match[8].first,
		};

		aprs_packet_decode_data_extensions(packet, packet->position->comment);

		return true;
	}

	if (aprs_regex_match(match, regex_compressed, packet->content.c_str()))
	{
		aprs_compressed_location location;

		if (!aprs_decode_compressed_location(location, match[1].str().c_str()))
			return false;

		packet->type       = APRS_PACKET_TYPE_POSITION;
		packet->extensions = {};
		packet->position   = new aprs_packet_position
		{
			.flags            = flags | APRS_POSITION_FLAG_COMPRESSED,
			.latitude         = location.latitude,
			.longitude        = location.longitude,
			.comment          = match[2].str(),
			.symbol_table     = location.symbol_table,
			.symbol_table_key = location.symbol_table_key
		};

		aprs_packet_decode_data_extensions(packet, packet->position->comment);

		return true;
	}

	return false;
}
bool               aprs_packet_decode_position(aprs_packet* packet)
{
	return aprs_packet_decode_position(packet, 0);
}
bool               aprs_packet_decode_position_time(aprs_packet* packet)
{
	return aprs_packet_decode_position(packet, APRS_POSITION_FLAG_TIME);
}
bool               aprs_packet_decode_position_time_messaging(aprs_packet* packet)
{
	return aprs_packet_decode_position(packet, APRS_POSITION_FLAG_TIME | APRS_POSITION_FLAG_MESSAGING_ENABLED);
}
bool               aprs_packet_decode_position_messaging(aprs_packet* packet)
{
	return aprs_packet_decode_position(packet, APRS_POSITION_FLAG_MESSAGING_ENABLED);
}
bool               aprs_packet_decode_telemetry(aprs_packet* packet)
{
	static const std::regex regex("^T#(\\d{3}|)(,(\\d{0,3}\\.?\\d{0,3}))(,(\\d{0,3}\\.?\\d{0,3}))(,(\\d{0,3}\\.?\\d{0,3}))(,(\\d{0,3}\\.?\\d{0,3}))(,(\\d{0,3}\\.?\\d{0,3}))(,(\\d{8}))(.*)$");

	std::cmatch match;

	if (!aprs_regex_match(match, regex, packet->content.c_str()))
		return false;

	auto& analog_1 = match[3];
	auto& analog_2 = match[5];
	auto& analog_3 = match[7];
	auto& analog_4 = match[9];
	auto& analog_5 = match[11];

	packet->type      = APRS_PACKET_TYPE_TELEMETRY;
	packet->telemetry = new aprs_packet_telemetry
	{
		.digital  = (uint8_t)strtoul(match[13].str().c_str(), nullptr, 10),
		.sequence = (uint16_t)strtoul(match[1].str().c_str(), nullptr, 10),
		.comment  = match[14].str()
	};

	if (!aprs_string_contains(analog_1.first, analog_1.length(), '.') &&
		!aprs_string_contains(analog_2.first, analog_1.length(), '.') &&
		!aprs_string_contains(analog_3.first, analog_1.length(), '.') &&
		!aprs_string_contains(analog_4.first, analog_1.length(), '.') &&
		!aprs_string_contains(analog_5.first, analog_1.length(), '.'))
	{
		packet->telemetry->type           = APRS_TELEMETRY_TYPE_U8;
		packet->telemetry->analog_u8[0]   = aprs_decode_int<uint8_t>(analog_1.first, analog_1.length());
		packet->telemetry->analog_u8[1]   = aprs_decode_int<uint8_t>(analog_2.first, analog_2.length());
		packet->telemetry->analog_u8[2]   = aprs_decode_int<uint8_t>(analog_3.first, analog_3.length());
		packet->telemetry->analog_u8[3]   = aprs_decode_int<uint8_t>(analog_4.first, analog_4.length());
		packet->telemetry->analog_u8[4]   = aprs_decode_int<uint8_t>(analog_5.first, analog_5.length());
		packet->telemetry->analog_u8_c[0] = &packet->telemetry->analog_u8[0];
		packet->telemetry->analog_u8_c[1] = &packet->telemetry->analog_u8[1];
		packet->telemetry->analog_u8_c[2] = &packet->telemetry->analog_u8[2];
		packet->telemetry->analog_u8_c[3] = &packet->telemetry->analog_u8[3];
		packet->telemetry->analog_u8_c[4] = &packet->telemetry->analog_u8[4];
		packet->telemetry->analog_u8_c[5] = nullptr;
	}
	else
	{
		packet->telemetry->type              = APRS_TELEMETRY_TYPE_FLOAT;
		packet->telemetry->analog_float[0]   = strtof(analog_1.str().c_str(), nullptr);
		packet->telemetry->analog_float[1]   = strtof(analog_2.str().c_str(), nullptr);
		packet->telemetry->analog_float[2]   = strtof(analog_3.str().c_str(), nullptr);
		packet->telemetry->analog_float[3]   = strtof(analog_4.str().c_str(), nullptr);
		packet->telemetry->analog_float[4]   = strtof(analog_5.str().c_str(), nullptr);
		packet->telemetry->analog_float_c[0] = &packet->telemetry->analog_float[0];
		packet->telemetry->analog_float_c[1] = &packet->telemetry->analog_float[1];
		packet->telemetry->analog_float_c[2] = &packet->telemetry->analog_float[2];
		packet->telemetry->analog_float_c[3] = &packet->telemetry->analog_float[3];
		packet->telemetry->analog_float_c[4] = &packet->telemetry->analog_float[4];
		packet->telemetry->analog_float_c[5] = nullptr;
	}

	return true;
}
bool               aprs_packet_decode_microfinder(aprs_packet* packet)
{
	// packet->type = APRS_PACKET_TYPE_MICROFINDER;

	// TODO: decode micro finder

	return false;
}
bool               aprs_packet_decode_map_feature(aprs_packet* packet)
{
	// packet->type = APRS_PACKET_TYPE_MAP_FEATURE;

	// TODO: decode map feature

	return false;
}
bool               aprs_packet_decode_third_party(aprs_packet* packet)
{
	packet->type        = APRS_PACKET_TYPE_THIRD_PARTY;
	packet->third_party = new aprs_packet_third_party
	{
		.content = packet->content.substr(1)
	};

	return true;
}
bool               aprs_packet_decode_user_defined(aprs_packet* packet)
{
	if (packet->content.length() < 3)
		return false;

	packet->type         = APRS_PACKET_TYPE_USER_DEFINED;
	packet->user_defined = new aprs_packet_user_defined
	{
		.id   = packet->content[1],
		.type = packet->content[2],
		.data = packet->content.substr(3)
	};

	return true;
}
bool               aprs_packet_decode_shelter_time(aprs_packet* packet)
{
	// packet->type = APRS_PACKET_TYPE_SHELTER_TIME;

	// TODO: decode shelter time

	return false;
}
bool               aprs_packet_decode_station_capabilities(aprs_packet* packet)
{
	// packet->type = APRS_PACKET_TYPE_STATION_CAPABILITIES;

	// TODO: decode station capabilities

	return false;
}
bool               aprs_packet_decode_maidenhead_grid_beacon(aprs_packet* packet)
{
	// packet->type = APRS_PACKET_TYPE_MAIDENHEAD_GRID_BEACON;

	// TODO: decode maidenhead grid beacon

	return false;
}

void               aprs_packet_encode_gps(aprs_packet* packet, std::stringstream& ss)
{
	ss << packet->gps->nmea;
	ss << packet->gps->comment;
}
void               aprs_packet_encode_raw(aprs_packet* packet, std::stringstream& ss)
{
	ss << packet->content;
}
void               aprs_packet_encode_item(aprs_packet* packet, std::stringstream& ss)
{
	ss << ')';
	ss << std::setfill(' ') << std::setw(9) << std::left << packet->item->name;
	ss << (packet->item->is_alive ? '!' : '_');

	if (packet->item->is_compressed)
	{
		aprs_compressed_location location =
		{
			.speed            = packet->extensions.speed,
			.course           = packet->extensions.course,
			.altitude         = packet->extensions.altitude,

			.latitude         = packet->item->latitude,
			.longitude        = packet->item->longitude,

			.symbol_table     = packet->item->symbol_table,
			.symbol_table_key = packet->item->symbol_table_key
		};

		aprs_encode_compressed_location(location, ss);
	}
	else
	{
		auto latitude             = packet->item->latitude;
		auto longitude            = packet->item->longitude;
		char latitude_north_south = (latitude >= 0)  ? 'N' : 'S';
		char longitude_west_east  = (longitude >= 0) ? 'E' : 'W';
		auto latitude_hours       = aprs_from_float<int16_t>(latitude, latitude);
		auto latitude_minutes     = aprs_from_float<uint16_t>(((latitude < 0) ? (latitude * -1) : latitude) * 60, latitude);
		auto latitude_seconds     = aprs_from_float<uint16_t>((latitude * 6000) / 60, latitude);
		auto longitude_hours      = aprs_from_float<int16_t>(longitude, longitude);
		auto longitude_minutes    = aprs_from_float<uint16_t>(((longitude < 0) ? (longitude * -1) : longitude) * 60, longitude);
		auto longitude_seconds    = aprs_from_float<uint16_t>((longitude * 6000) / 60, longitude);

		ss << std::setfill('0') << std::setw(2) << ((latitude_hours >= 0) ? latitude_hours : (latitude_hours * -1));
		ss << std::setfill('0') << std::setw(2) << latitude_minutes;
		ss << '.';
		ss << std::setfill('0') << std::setw(2) << latitude_seconds;
		ss << latitude_north_south << packet->item->symbol_table;

		ss << std::setfill('0') << std::setw(2) << ((longitude_hours >= 0) ? longitude_hours : (longitude_hours * -1));
		ss << std::setfill('0') << std::setw(2) << longitude_minutes;
		ss << '.';
		ss << std::setfill('0') << std::setw(2) << longitude_seconds;
		ss << longitude_west_east << packet->item->symbol_table_key;
	}

	aprs_packet_encode_data_extensions(packet, ss);

	ss << packet->item->comment;
}
void               aprs_packet_encode_object(aprs_packet* packet, std::stringstream& ss)
{
	ss << ';';
	ss << std::setfill(' ') << std::setw(9) << std::left << packet->object->name;
	ss << (packet->object->is_alive ? '*' : '_');

	if (packet->object->time.type & APRS_TIME_DHM)
	{
		ss << std::setfill('0') << std::setw(2) << packet->object->time.tm_mday;
		ss << std::setfill('0') << std::setw(2) << packet->object->time.tm_hour;
		ss << std::setfill('0') << std::setw(2) << packet->object->time.tm_min;
		ss << 'z';
	}
	else if (packet->object->time.type & APRS_TIME_HMS)
	{
		ss << std::setfill('0') << std::setw(2) << packet->object->time.tm_hour;
		ss << std::setfill('0') << std::setw(2) << packet->object->time.tm_min;
		ss << std::setfill('0') << std::setw(2) << packet->object->time.tm_sec;
		ss << 'h';
	}

	if (packet->object->is_compressed)
	{
		aprs_compressed_location location =
		{
			.speed            = packet->extensions.speed,
			.course           = packet->extensions.course,
			.altitude         = packet->extensions.altitude,

			.latitude         = packet->object->latitude,
			.longitude        = packet->object->longitude,

			.symbol_table     = packet->object->symbol_table,
			.symbol_table_key = packet->object->symbol_table_key
		};

		aprs_encode_compressed_location(location, ss);
	}
	else
	{
		auto latitude             = packet->object->latitude;
		auto longitude            = packet->object->longitude;
		char latitude_north_south = (latitude >= 0)  ? 'N' : 'S';
		char longitude_west_east  = (longitude >= 0) ? 'E' : 'W';
		auto latitude_hours       = aprs_from_float<int16_t>(latitude, latitude);
		auto latitude_minutes     = aprs_from_float<uint16_t>(((latitude < 0) ? (latitude * -1) : latitude) * 60, latitude);
		auto latitude_seconds     = aprs_from_float<uint16_t>((latitude * 6000) / 60, latitude);
		auto longitude_hours      = aprs_from_float<int16_t>(longitude, longitude);
		auto longitude_minutes    = aprs_from_float<uint16_t>(((longitude < 0) ? (longitude * -1) : longitude) * 60, longitude);
		auto longitude_seconds    = aprs_from_float<uint16_t>((longitude * 6000) / 60, longitude);

		ss << std::setfill('0') << std::setw(2) << ((latitude_hours >= 0) ? latitude_hours : (latitude_hours * -1));
		ss << std::setfill('0') << std::setw(2) << latitude_minutes;
		ss << '.';
		ss << std::setfill('0') << std::setw(2) << latitude_seconds;
		ss << latitude_north_south << packet->object->symbol_table;

		ss << std::setfill('0') << std::setw(2) << ((longitude_hours >= 0) ? longitude_hours : (longitude_hours * -1));
		ss << std::setfill('0') << std::setw(2) << longitude_minutes;
		ss << '.';
		ss << std::setfill('0') << std::setw(2) << longitude_seconds;
		ss << longitude_west_east << packet->object->symbol_table_key;
	}

	aprs_packet_encode_data_extensions(packet, ss);

	ss << packet->object->comment;
}
void               aprs_packet_encode_status(aprs_packet* packet, std::stringstream& ss)
{
	ss << '>';

	if (packet->status->is_time_set)
		if (packet->status->time.type & APRS_TIME_DHM)
		{
			ss << std::setfill('0') << std::setw(2) << packet->status->time.tm_mday;
			ss << std::setfill('0') << std::setw(2) << packet->status->time.tm_hour;
			ss << std::setfill('0') << std::setw(2) << packet->status->time.tm_min;
			ss << 'z';
		}
		else if (packet->status->time.type & APRS_TIME_HMS)
		{
			ss << std::setfill('0') << std::setw(2) << packet->status->time.tm_hour;
			ss << std::setfill('0') << std::setw(2) << packet->status->time.tm_min;
			ss << std::setfill('0') << std::setw(2) << packet->status->time.tm_sec;
			ss << 'h';
		}

	ss << packet->status->message;
}
void               aprs_packet_encode_message(aprs_packet* packet, std::stringstream& ss)
{
	switch (packet->message->type)
	{
		case APRS_MESSAGE_TYPE_ACK:
		case APRS_MESSAGE_TYPE_REJECT:
		case APRS_MESSAGE_TYPE_MESSAGE:
			ss << ':' << std::setfill(' ') << std::setw(9) << std::left << packet->message->destination << ':';
			break;

		case APRS_MESSAGE_TYPE_BULLETIN:
			ss << ":BLN" << std::setfill(' ') << std::setw(6) << std::left << packet->message->destination << ':';
			break;
	}

	switch (packet->message->type)
	{
		case APRS_MESSAGE_TYPE_ACK:
			ss << "ack" << packet->message->id;
			break;

		case APRS_MESSAGE_TYPE_REJECT:
			ss << "rej" << packet->message->id;
			break;

		case APRS_MESSAGE_TYPE_MESSAGE:
			ss << packet->message->content;

			if (auto& id = packet->message->id; id.length())
				ss << '{' << id;
			break;

		case APRS_MESSAGE_TYPE_BULLETIN:
			ss << packet->message->content;
			break;
	}
}
void               aprs_packet_encode_weather(aprs_packet* packet, std::stringstream& ss)
{
	auto humidity = packet->weather->humidity;

	switch (humidity)
	{
		case 0:
			humidity = 1;
			break;

		case 100:
			humidity = 0;
			break;
	}

	ss << '_';
	ss << std::setfill('0') << std::setw(2) << packet->weather->time.tm_mon;
	ss << std::setfill('0') << std::setw(2) << packet->weather->time.tm_mday;
	ss << std::setfill('0') << std::setw(2) << packet->weather->time.tm_hour;
	ss << std::setfill('0') << std::setw(2) << packet->weather->time.tm_min;
	ss << 'c' << std::setfill('0') << std::setw(3) << packet->weather->wind_direction;
	ss << 's' << std::setfill('0') << std::setw(3) << packet->weather->wind_speed;
	ss << 'g' << std::setfill('0') << std::setw(3) << packet->weather->wind_speed_gust;
	ss << 't' << std::setfill('0') << std::setw(3) << packet->weather->temperature;
	ss << 'r' << std::setfill('0') << std::setw(3) << packet->weather->rainfall_last_hour;
	ss << 'p' << std::setfill('0') << std::setw(3) << packet->weather->rainfall_last_24_hours;
	ss << 'P' << std::setfill('0') << std::setw(3) << packet->weather->rainfall_since_midnight;
	ss << 'h' << std::setfill('0') << std::setw(2) << humidity;
	ss << 'b' << std::setfill('0') << std::setw(4) << packet->weather->barometric_pressure;
	ss << packet->weather->software;
	ss << packet->weather->type;
}
void               aprs_packet_encode_position(aprs_packet* packet, std::stringstream& ss)
{
	// if (packet->position->flags & APRS_POSITION_FLAG_MIC_E)
	// {
	// 	// TODO: encode mic-e position
	// }
	// else
	{
		if (packet->position->flags & APRS_POSITION_FLAG_TIME)
		{
			ss << ((packet->position->flags & APRS_POSITION_FLAG_MESSAGING_ENABLED) ? '@' : '/');

			if (packet->position->time.type & APRS_TIME_DHM)
			{
				ss << std::setfill('0') << std::setw(2) << packet->position->time.tm_mday;
				ss << std::setfill('0') << std::setw(2) << packet->position->time.tm_hour;
				ss << std::setfill('0') << std::setw(2) << packet->position->time.tm_min;
				ss << 'z';
			}
			else if (packet->position->time.type & APRS_TIME_HMS)
			{
				ss << std::setfill('0') << std::setw(2) << packet->position->time.tm_hour;
				ss << std::setfill('0') << std::setw(2) << packet->position->time.tm_min;
				ss << std::setfill('0') << std::setw(2) << packet->position->time.tm_sec;
				ss << 'h';
			}
		}
		else
			ss << ((packet->position->flags & APRS_POSITION_FLAG_MESSAGING_ENABLED) ? '=' : '!');

		if (packet->position->flags & APRS_POSITION_FLAG_COMPRESSED)
		{
			aprs_compressed_location location =
			{
				.speed            = packet->extensions.speed,
				.course           = packet->extensions.course,
				.altitude         = packet->extensions.altitude,

				.latitude         = packet->position->latitude,
				.longitude        = packet->position->longitude,

				.symbol_table     = packet->position->symbol_table,
				.symbol_table_key = packet->position->symbol_table_key
			};

			aprs_encode_compressed_location(location, ss);
		}
		else
		{
			auto latitude             = packet->position->latitude;
			auto longitude            = packet->position->longitude;
			char latitude_north_south = (latitude >= 0)  ? 'N' : 'S';
			char longitude_west_east  = (longitude >= 0) ? 'E' : 'W';
			auto latitude_hours       = aprs_from_float<int16_t>(latitude, latitude);
			auto latitude_minutes     = aprs_from_float<uint16_t>(((latitude < 0) ? (latitude * -1) : latitude) * 60, latitude);
			auto latitude_seconds     = aprs_from_float<uint16_t>((latitude * 6000) / 60, latitude);
			auto longitude_hours      = aprs_from_float<int16_t>(longitude, longitude);
			auto longitude_minutes    = aprs_from_float<uint16_t>(((longitude < 0) ? (longitude * -1) : longitude) * 60, longitude);
			auto longitude_seconds    = aprs_from_float<uint16_t>((longitude * 6000) / 60, longitude);

			ss << std::setfill('0') << std::setw(2) << ((latitude_hours >= 0) ? latitude_hours : (latitude_hours * -1));
			ss << std::setfill('0') << std::setw(2) << latitude_minutes;
			ss << '.';
			ss << std::setfill('0') << std::setw(2) << latitude_seconds;
			ss << latitude_north_south << packet->position->symbol_table;

			ss << std::setfill('0') << std::setw(2) << ((longitude_hours >= 0) ? longitude_hours : (longitude_hours * -1));
			ss << std::setfill('0') << std::setw(2) << longitude_minutes;
			ss << '.';
			ss << std::setfill('0') << std::setw(2) << longitude_seconds;
			ss << longitude_west_east << packet->position->symbol_table_key;
		}
	}

	aprs_packet_encode_data_extensions(packet, ss);

	ss << packet->position->comment;
}
void               aprs_packet_encode_telemetry(aprs_packet* packet, std::stringstream& ss)
{
	switch (packet->telemetry->type)
	{
		case APRS_TELEMETRY_TYPE_U8:
			ss << "T#" << std::setfill('0') << std::setw(3) << packet->telemetry->sequence << ',';
			for (auto analog : packet->telemetry->analog_u8)
				ss << (int)analog << ',';
			for (uint8_t i = 0; i < 8; ++i)
				ss << (((packet->telemetry->digital & (1 << i)) == (1 << i)) ? 1 : 0);
			break;

		case APRS_TELEMETRY_TYPE_FLOAT:
			ss << "T#" << std::setfill('0') << std::setw(3) << packet->telemetry->sequence << ',';
			for (auto analog : packet->telemetry->analog_float)
				ss << analog << ',';
			for (uint8_t i = 0; i < 8; ++i)
				ss << (((packet->telemetry->digital & (1 << i)) == (1 << i)) ? 1 : 0);
			break;

		case APRS_TELEMETRY_TYPE_PARAMS:
		case APRS_TELEMETRY_TYPE_UNITS:
		case APRS_TELEMETRY_TYPE_EQNS:
		case APRS_TELEMETRY_TYPE_BITS:
			// TODO: implement
			break;
	}

	ss << packet->telemetry->comment;
}
void               aprs_packet_encode_third_party(aprs_packet* packet, std::stringstream& ss)
{
	ss << '}' << packet->third_party->content;
}
void               aprs_packet_encode_user_defined(aprs_packet* packet, std::stringstream& ss)
{
	ss << '{' << packet->user_defined->id << packet->user_defined->type << packet->user_defined->data;
}

constexpr const aprs_packet_decoder_context aprs_packet_decoders[] =
{
	{ 0x1C, &aprs_packet_decode_mic_e                   }, // Current Mic-E Data
	{ 0x1D, &aprs_packet_decode_mic_e_old               }, // Old Mic-E Data
	{ '!',  &aprs_packet_decode_position                }, // Position without timestamp (no APRS messaging), or Ultimeter 2000 WX Station
	{ '#',  &aprs_packet_decode_weather_peet_bros_uii   }, // Peet Bros U-II Weather Station
	{ '$',  &aprs_packet_decode_raw_gps                 }, // Raw GPS data or Ultimeter 2000 
	{ '%',  &aprs_packet_decode_microfinder             }, // Agrelo DFJr / MicroFinder
	{ '&',  &aprs_packet_decode_map_feature             }, // [Reserved — Map Feature]
	{ '\'', &aprs_packet_decode_mic_e_old               }, // Old Mic-E Data (but Current data for TM-D700) 
	{ ')',  &aprs_packet_decode_item                    }, // Item
	{ '*',  &aprs_packet_decode_weather_peet_bros_uii   }, // Peet Bros U-II Weather Station
	{ '+',  &aprs_packet_decode_shelter_time            }, // [Reserved — Shelter data with time] 
	{ ',',  &aprs_packet_decode_test                    }, // Invalid data or test data 
	{ '.',  &aprs_packet_decode_weather_space           }, // [Reserved — Space weather] 
	{ '/',  &aprs_packet_decode_position_time           }, // Position with timestamp (no APRS messaging) 
	{ ':',  &aprs_packet_decode_message                 }, // Message
	{ ';',  &aprs_packet_decode_object                  }, // Object
	{ '<',  &aprs_packet_decode_station_capabilities    }, // Station Capabilities
	{ '=',  &aprs_packet_decode_position_messaging      }, // Position without timestamp (with APRS messaging)
	{ '>',  &aprs_packet_decode_status                  }, // Status
	{ '?',  &aprs_packet_decode_query                   }, // Query
	{ '@',  &aprs_packet_decode_position_time_messaging }, // Position with timestamp (with APRS messaging)
	{ 'T',  &aprs_packet_decode_telemetry               }, // Telemetry data
	{ '[',  &aprs_packet_decode_maidenhead_grid_beacon  }, // Maidenhead grid locator beacon (obsolete)
	{ '_',  &aprs_packet_decode_weather                 }, // Weather Report (without position)
	{ '`',  &aprs_packet_decode_mic_e                   }, // Current Mic-E Data (not used in TM-D700)
	{ '{',  &aprs_packet_decode_user_defined            }, // User-Defined APRS packet format
	{ '}',  &aprs_packet_decode_third_party             }  // Third-party traffic
};

constexpr const aprs_packet_encoder_context aprs_packet_encoders[APRS_PACKET_TYPES_COUNT] =
{
	{ APRS_PACKET_TYPE_GPS,                    &aprs_packet_encode_gps                    },
	{ APRS_PACKET_TYPE_RAW,                    &aprs_packet_encode_raw                    },
	{ APRS_PACKET_TYPE_ITEM,                   &aprs_packet_encode_item                   },
	// { APRS_PACKET_TYPE_TEST,                   &aprs_packet_encode_test                   },
	// { APRS_PACKET_TYPE_QUERY,                  &aprs_packet_encode_query                  },
	{ APRS_PACKET_TYPE_OBJECT,                 &aprs_packet_encode_object                 },
	{ APRS_PACKET_TYPE_STATUS,                 &aprs_packet_encode_status                 },
	{ APRS_PACKET_TYPE_MESSAGE,                &aprs_packet_encode_message                },
	{ APRS_PACKET_TYPE_WEATHER,                &aprs_packet_encode_weather                },
	{ APRS_PACKET_TYPE_POSITION,               &aprs_packet_encode_position               },
	{ APRS_PACKET_TYPE_TELEMETRY,              &aprs_packet_encode_telemetry              },
	// { APRS_PACKET_TYPE_MAP_FEATURE,            &aprs_packet_encode_map_feature            },
	// { APRS_PACKET_TYPE_GRID_BEACON,            &aprs_packet_encode_grid_beacon            },
	{ APRS_PACKET_TYPE_THIRD_PARTY,            &aprs_packet_encode_third_party            },
	// { APRS_PACKET_TYPE_MICROFINDER,            &aprs_packet_encode_microfinder            },
	{ APRS_PACKET_TYPE_USER_DEFINED,           &aprs_packet_encode_user_defined           },
	// { APRS_PACKET_TYPE_SHELTER_TIME,           &aprs_packet_encode_shelter_time           },
	// { APRS_PACKET_TYPE_STATION_CAPABILITIES,   &aprs_packet_encode_station_capabilities   },
	// { APRS_PACKET_TYPE_MAIDENHEAD_GRID_BEACON, &aprs_packet_encode_maidenhead_grid_beacon }
};

template<size_t ... I>
consteval bool static_assert_aprs_packet_decoders(std::index_sequence<I ...>)
{
	return ((aprs_packet_decoders[I].ident && aprs_packet_decoders[I].function) && ...);
}
static_assert(static_assert_aprs_packet_decoders(std::make_index_sequence<sizeof(aprs_packet_decoders) / sizeof(aprs_packet_decoder_context)> {}));

template<size_t ... I>
consteval bool static_assert_aprs_packet_encoders(std::index_sequence<I ...>)
{
	return (((aprs_packet_encoders[I].type == I) && aprs_packet_encoders[I].function) && ...);
}
static_assert(static_assert_aprs_packet_encoders(std::make_index_sequence<sizeof(aprs_packet_encoders) / sizeof(aprs_packet_encoder_context)> {}));

struct aprs_path*                 APRSERVICE_CALL aprs_path_init()
{
	auto path = new aprs_path
	{
		.size            = 0,

		.reference_count = 1
	};

	return path;
}
struct aprs_path*                 APRSERVICE_CALL aprs_path_init_from_string(const char* string)
{
	if (!string)
		return nullptr;

	auto path = new aprs_path
	{
		.size            = 0,

		.reference_count = 1
	};

	std::string buffer(string);
	size_t      chunk_length;

	if (auto chunk = strtok(&buffer[0], ",")) do
	{
		if (path->size == path->chunks.max_size())
		{
			delete path;

			return nullptr;
		}

		if (!(chunk_length = aprs_validate_path(chunk)))
		{
			delete path;

			return nullptr;
		}

		path->chunks[path->size++].assign(chunk, chunk_length);
		path->chunks_c[path->size - 1] = path->chunks[path->size - 1].c_str();
	} while (chunk = strtok(nullptr, ","));

	return path;
}
void                              APRSERVICE_CALL aprs_path_deinit(struct aprs_path* path)
{
	if (!--path->reference_count)
		delete path;
}
const char**                      APRSERVICE_CALL aprs_path_get(struct aprs_path* path)
{
	return path->chunks_c.data();
}
uint8_t                           APRSERVICE_CALL aprs_path_get_length(struct aprs_path* path)
{
	return path->size;
}
uint8_t                           APRSERVICE_CALL aprs_path_get_capacity(struct aprs_path* path)
{
	return path->chunks.max_size();
}
bool                              APRSERVICE_CALL aprs_path_set(struct aprs_path* path, uint8_t index, const char* value)
{
	if (index >= path->size)
		return false;

	if (!aprs_validate_path(value))
		return false;

	path->chunks[index] = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_path_pop(struct aprs_path* path)
{
	if (path->size == 0)
		return false;

	if (path->size == path->chunks.max_size())
		return false;

	path->chunks[--path->size].clear();
	path->chunks_c[path->size + 1] = nullptr;

	return true;
}
bool                              APRSERVICE_CALL aprs_path_push(struct aprs_path* path, const char* value)
{
	if (!aprs_validate_path(value))
		return false;

	if (path->size == path->chunks.max_size())
		return false;

	path->chunks[path->size++] = value;
	path->chunks_c[path->size - 1] = path->chunks[path->size - 1].c_str();
	path->chunks_c[path->size] = nullptr;

	return true;
}
void                              APRSERVICE_CALL aprs_path_clear(struct aprs_path* path)
{
	for (size_t i = 0; i < path->size; ++i)
		path->chunks[i].clear();

	path->size = 0;
	path->chunks_c.fill(nullptr);
}
const char*                       APRSERVICE_CALL aprs_path_to_string(struct aprs_path* path)
{
	std::stringstream ss;

	if (path->size)
	{
		ss << path->chunks[0];

		for (size_t i = 1; i < path->size; ++i)
			ss << ',' << path->chunks[i];
	}

	path->string = ss.str();

	return path->string.c_str();
}
void                              APRSERVICE_CALL aprs_path_add_reference(struct aprs_path* path)
{
	++path->reference_count;
}

struct aprs_time*                 APRSERVICE_CALL aprs_time_now()
{
	static aprs_time time;

	time_t now;
	::time(&now);

	time = { *localtime(&now), APRS_TIME_DHM | APRS_TIME_HMS | APRS_TIME_MDHM };

	return &time;
}
int                               APRSERVICE_CALL aprs_time_get_type(const struct aprs_time* time)
{
	return time->type;
}
bool                              APRSERVICE_CALL aprs_time_get_dms(const struct aprs_time* time, uint8_t* day, uint8_t* minute, uint8_t* second)
{
	if (!(time->type & APRS_TIME_DHM))
		return false;

	*day    = time->tm_mday;
	*minute = time->tm_min;
	*second = time->tm_sec;

	return true;
}
bool                              APRSERVICE_CALL aprs_time_get_hms(const struct aprs_time* time, uint8_t* hour, uint8_t* minute, uint8_t* second)
{
	if (!(time->type & APRS_TIME_HMS))
		return false;

	*hour   = time->tm_hour;
	*minute = time->tm_min;
	*second = time->tm_sec;

	return true;
}
bool                              APRSERVICE_CALL aprs_time_get_mdhm(const struct aprs_time* time, uint8_t* month, uint8_t* day, uint8_t* hour, uint8_t* minute)
{
	if (!(time->type & APRS_TIME_MDHM))
		return false;

	*month  = time->tm_mon;
	*day    = time->tm_mday;
	*hour   = time->tm_hour;
	*minute = time->tm_min;

	return true;
}

bool                                              aprs_packet_decode(aprs_packet* packet)
{
	if (auto content = aprs_packet_get_content(packet))
		for (auto& decoder : aprs_packet_decoders)
			if (decoder.ident == *content)
				return decoder.function(packet);

	return false;
}
bool                                              aprs_packet_encode(aprs_packet* packet, std::stringstream& ss)
{
	if (auto type = aprs_packet_get_type(packet); type < APRS_PACKET_TYPES_COUNT)
	{
		aprs_packet_encoders[type].function(packet, ss);

		return true;
	}

	return false;
}

struct aprs_packet*               APRSERVICE_CALL aprs_packet_init(const char* sender, const char* tocall, struct aprs_path* path)
{
	if (!tocall || !path)
		return nullptr;

	if (!aprs_validate_station(sender))
		return nullptr;

	auto packet = new aprs_packet
	{
		.type            = APRS_PACKET_TYPE_RAW,
		.path            = path,
		.tocall          = tocall,
		.sender          = sender,
		.reference_count = 1
	};

	aprs_path_add_reference(path);

	return packet;
}
struct aprs_packet*                               aprs_packet_init_ex(const char* sender, const char* tocall, struct aprs_path* path, enum APRS_PACKET_TYPES type)
{
	auto packet = new aprs_packet
	{
		.type            = type,
		.path            = path,
		.tocall          = tocall,
		.sender          = sender,
		.reference_count = 1
	};

	aprs_path_add_reference(path);

	return packet;
}
struct aprs_packet*               APRSERVICE_CALL aprs_packet_init_from_string(const char* string)
{
	static const std::regex regex("^([^>]{3,9})>([^,]+),([^:]+):(.+)$");
	static const std::regex regex_path("^((\\S+?(?=,qA\\w)),(qA\\w),(.+))|(\\S+)$");

	aprs_path*  path;
	std::cmatch path_match;
	std::string path_string;

	std::cmatch match;

	if (!aprs_regex_match(match, regex, string))
		return nullptr;

	path_string = match[3].str();

	if (!aprs_regex_match(path_match, regex_path, path_string.c_str()))
		return nullptr;

	path_string = path_match[2].str();

	if (!(path = aprs_path_init_from_string(path_string.c_str())))
		return nullptr;

	auto packet = new aprs_packet
	{
		.path            = path,
		.igate           = path_match[4].str(),
		.tocall          = match[2].str(),
		.sender          = match[1].str(),
		.content         = match[4].str(),
		.qconstruct      = path_match[3].str(),
		.reference_count = 1
	};

	if (!aprs_packet_decode(packet))
		packet->type = APRS_PACKET_TYPE_RAW;

	return packet;
}
void                              APRSERVICE_CALL aprs_packet_deinit(struct aprs_packet* packet)
{
	if (!--packet->reference_count)
	{
		aprs_path_deinit(packet->path);

		switch (packet->type)
		{
			case APRS_PACKET_TYPE_GPS:
				delete packet->gps;
				break;

			case APRS_PACKET_TYPE_RAW:
				break;

			case APRS_PACKET_TYPE_ITEM:
				delete packet->item;
				break;

			// case APRS_PACKET_TYPE_TEST:
				// break;

			// case APRS_PACKET_TYPE_QUERY:
				// break;

			case APRS_PACKET_TYPE_OBJECT:
				delete packet->object;
				break;

			case APRS_PACKET_TYPE_STATUS:
				delete packet->status;
				break;

			case APRS_PACKET_TYPE_MESSAGE:
				delete packet->message;
				break;

			case APRS_PACKET_TYPE_WEATHER:
				delete packet->weather;
				break;

			case APRS_PACKET_TYPE_POSITION:
				delete packet->position;
			break;

			case APRS_PACKET_TYPE_TELEMETRY:
				delete packet->telemetry;
				break;

			// case APRS_PACKET_TYPE_MAP_FEATURE:
				// break;

			// case APRS_PACKET_TYPE_GRID_BEACON:
				// break;

			case APRS_PACKET_TYPE_THIRD_PARTY:
				delete packet->third_party;
				break;

			// case APRS_PACKET_TYPE_MICROFINDER:
				// break;

			case APRS_PACKET_TYPE_USER_DEFINED:
				delete packet->user_defined;
				break;

			// case APRS_PACKET_TYPE_SHELTER_TIME:
				// break;

			// case APRS_PACKET_TYPE_STATION_CAPABILITIES:
				// break;

			// case APRS_PACKET_TYPE_MAIDENHEAD_GRID_BEACON:
				// break;
		}

		delete packet;
	}
}
const char*                       APRSERVICE_CALL aprs_packet_get_q(struct aprs_packet* packet)
{
	return packet->qconstruct.c_str();
}
enum APRS_PACKET_TYPES            APRSERVICE_CALL aprs_packet_get_type(struct aprs_packet* packet)
{
	return packet->type;
}
struct aprs_path*                 APRSERVICE_CALL aprs_packet_get_path(struct aprs_packet* packet)
{
	return packet->path;
}
const char*                       APRSERVICE_CALL aprs_packet_get_igate(struct aprs_packet* packet)
{
	return packet->igate.c_str();
}
const char*                       APRSERVICE_CALL aprs_packet_get_tocall(struct aprs_packet* packet)
{
	return packet->tocall.c_str();
}
const char*                       APRSERVICE_CALL aprs_packet_get_sender(struct aprs_packet* packet)
{
	return packet->sender.c_str();
}
const char*                       APRSERVICE_CALL aprs_packet_get_content(struct aprs_packet* packet)
{
	return packet->content.c_str();
}
bool                              APRSERVICE_CALL aprs_packet_set_path(struct aprs_packet* packet, struct aprs_path* value)
{
	if (!value)
		return false;

	aprs_path_deinit(packet->path);

	packet->path = value;

	aprs_path_add_reference(value);

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_set_tocall(struct aprs_packet* packet, const char* value)
{
	if (auto length = aprs_validate_name(value))
	{
		packet->tocall.assign(value, length);

		return true;
	}

	return false;
}
bool                              APRSERVICE_CALL aprs_packet_set_sender(struct aprs_packet* packet, const char* value)
{
	if (auto length = aprs_validate_station(value))
	{
		packet->sender.assign(value, length);

		return true;
	}

	return false;
}
bool                              APRSERVICE_CALL aprs_packet_set_content(struct aprs_packet* packet, const char* value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_RAW)
		return false;

	if (auto length = aprs_string_length(value); length && (length <= 256))
	{
		packet->content.assign(value, length);

		return true;
	}

	return false;
}
const char*                       APRSERVICE_CALL aprs_packet_to_string(struct aprs_packet* packet)
{
	{
		std::stringstream ss;

		if (!aprs_packet_encode(packet, ss))
			return nullptr;

		packet->content = ss.str();
	}

	{
		std::stringstream ss;
		ss << aprs_packet_get_sender(packet) << '>' << aprs_packet_get_tocall(packet) << ',' << aprs_path_to_string(packet->path) << ':' << packet->content;

		packet->string = ss.str();
	}

	return packet->string.c_str();
}
void                              APRSERVICE_CALL aprs_packet_add_reference(struct aprs_packet* packet)
{
	++packet->reference_count;
}

struct aprs_packet*               APRSERVICE_CALL aprs_packet_gps_init(const char* sender, const char* tocall, struct aprs_path* path, const char* nmea)
{
	if (auto packet = aprs_packet_init_ex(sender, tocall, path, APRS_PACKET_TYPE_GPS))
	{
		packet->gps = new aprs_packet_gps {};

		if (!aprs_packet_gps_set_nmea(packet, nmea))
		{
			aprs_packet_deinit(packet);

			return nullptr;
		}

		return packet;
	}

	return nullptr;
}
const char*                       APRSERVICE_CALL aprs_packet_gps_get_nmea(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_GPS)
		return nullptr;

	return packet->gps->nmea.c_str();
}
const char*                       APRSERVICE_CALL aprs_packet_gps_get_comment(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_GPS)
		return nullptr;

	return packet->gps->comment.c_str();
}
bool                              APRSERVICE_CALL aprs_packet_gps_set_nmea(struct aprs_packet* packet, const char* value)
{
	if (!*value || (*value != '$'))
		return false;

	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_GPS)
		return false;

	packet->gps->nmea = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_gps_set_comment(struct aprs_packet* packet, const char* value)
{
	if (!*value)
		return false;

	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_GPS)
		return false;

	packet->gps->comment = value;

	return true;
}

struct aprs_packet*               APRSERVICE_CALL aprs_packet_item_init(const char* sender, const char* tocall, struct aprs_path* path, const char* name, char symbol_table, char symbol_table_key)
{
	if (auto packet = aprs_packet_init_ex(sender, tocall, path, APRS_PACKET_TYPE_ITEM))
	{
		packet->item = new aprs_packet_item
		{
			.is_alive      = true,
			.is_compressed = false,
			.latitude      = 0,
			.longitude     = 0
		};

		if (!aprs_packet_object_set_name(packet, name) ||
			!aprs_packet_object_set_symbol(packet, symbol_table, symbol_table_key))
		{
			aprs_packet_deinit(packet);

			return nullptr;
		}

		return packet;
	}

	return nullptr;
}
bool                              APRSERVICE_CALL aprs_packet_item_is_alive(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_ITEM)
		return false;

	return packet->item->is_alive;
}
bool                              APRSERVICE_CALL aprs_packet_item_is_compressed(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_ITEM)
		return false;

	return packet->item->is_compressed;
}
const char*                       APRSERVICE_CALL aprs_packet_item_get_name(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_ITEM)
		return nullptr;

	return packet->item->name.c_str();
}
const char*                       APRSERVICE_CALL aprs_packet_item_get_comment(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_ITEM)
		return nullptr;

	return packet->item->comment.c_str();
}
uint16_t                          APRSERVICE_CALL aprs_packet_item_get_speed(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_ITEM)
		return 0;

	return packet->extensions.speed;
}
uint16_t                          APRSERVICE_CALL aprs_packet_item_get_course(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_ITEM)
		return 0;

	return packet->extensions.course;
}
int32_t                           APRSERVICE_CALL aprs_packet_item_get_altitude(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_ITEM)
		return 0;

	return packet->extensions.altitude;
}
float                             APRSERVICE_CALL aprs_packet_item_get_latitude(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_ITEM)
		return 0;

	return packet->item->latitude;
}
float                             APRSERVICE_CALL aprs_packet_item_get_longitude(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_ITEM)
		return 0;

	return packet->item->longitude;
}
char                              APRSERVICE_CALL aprs_packet_item_get_symbol_table(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_ITEM)
		return '\0';

	return packet->item->symbol_table;
}
char                              APRSERVICE_CALL aprs_packet_item_get_symbol_table_key(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_ITEM)
		return '\0';

	return packet->item->symbol_table_key;
}
bool                              APRSERVICE_CALL aprs_packet_item_set_alive(struct aprs_packet* packet, bool value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_ITEM)
		return false;

	packet->item->is_alive = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_item_set_compressed(struct aprs_packet* packet, bool value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_ITEM)
		return false;

	packet->item->is_compressed = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_item_set_name(struct aprs_packet* packet, const char* value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_ITEM)
		return false;

	if (auto length = aprs_validate_name(value))
	{
		packet->item->name.assign(value, length);

		return true;
	}

	return false;
}
bool                              APRSERVICE_CALL aprs_packet_item_set_comment(struct aprs_packet* packet, const char* value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_ITEM)
		return false;

	if (!value)
	{
		packet->item->comment.clear();

		return true;
	}
	else if (auto length = aprs_validate_comment(value, 36))
	{
		packet->item->comment.assign(value, length);

		return true;
	}

	return false;
}
bool                              APRSERVICE_CALL aprs_packet_item_set_speed(struct aprs_packet* packet, uint16_t value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_ITEM)
		return false;

	packet->extensions.speed = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_item_set_course(struct aprs_packet* packet, uint16_t value)
{
	if (value > 359)
		return false;

	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_ITEM)
		return false;

	packet->extensions.course = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_item_set_altitude(struct aprs_packet* packet, int32_t value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_ITEM)
		return false;

	packet->extensions.altitude = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_item_set_latitude(struct aprs_packet* packet, float value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_ITEM)
		return false;

	packet->item->latitude = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_item_set_longitude(struct aprs_packet* packet, float value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_ITEM)
		return false;

	packet->item->longitude = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_item_set_symbol(struct aprs_packet* packet, char table, char key)
{
	if (!aprs_validate_symbol(table, key))
		return false;

	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_ITEM)
		return false;

	packet->item->symbol_table     = table;
	packet->item->symbol_table_key = key;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_item_set_symbol_table(struct aprs_packet* packet, char value)
{
	return aprs_packet_item_set_symbol(packet, value, aprs_packet_item_get_symbol_table_key(packet));
}
bool                              APRSERVICE_CALL aprs_packet_item_set_symbol_table_key(struct aprs_packet* packet, char value)
{
	return aprs_packet_item_set_symbol(packet, aprs_packet_item_get_symbol_table(packet), value);
}

struct aprs_packet*               APRSERVICE_CALL aprs_packet_object_init(const char* sender, const char* tocall, struct aprs_path* path, const char* name, char symbol_table, char symbol_table_key)
{
	if (auto packet = aprs_packet_init_ex(sender, tocall, path, APRS_PACKET_TYPE_OBJECT))
	{
		packet->object = new aprs_packet_object
		{
			.is_alive      = true,
			.is_compressed = false,
			.time          = *aprs_time_now(),
			.latitude      = 0,
			.longitude     = 0
		};

		if (!aprs_packet_object_set_name(packet, name) ||
			!aprs_packet_object_set_symbol(packet, symbol_table, symbol_table_key))
		{
			aprs_packet_deinit(packet);

			return nullptr;
		}

		return packet;
	}

	return nullptr;
}
bool                              APRSERVICE_CALL aprs_packet_object_is_alive(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_OBJECT)
		return false;

	return packet->object->is_alive;
}
bool                              APRSERVICE_CALL aprs_packet_object_is_compressed(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_OBJECT)
		return false;

	return packet->object->is_compressed;
}
const struct aprs_time*           APRSERVICE_CALL aprs_packet_object_get_time(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_OBJECT)
		return nullptr;

	return &packet->object->time;
}
const char*                       APRSERVICE_CALL aprs_packet_object_get_name(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_OBJECT)
		return nullptr;

	return packet->object->name.c_str();
}
const char*                       APRSERVICE_CALL aprs_packet_object_get_comment(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_OBJECT)
		return nullptr;

	return packet->object->comment.c_str();
}
uint16_t                          APRSERVICE_CALL aprs_packet_object_get_speed(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_OBJECT)
		return 0;

	return packet->extensions.speed;
}
uint16_t                          APRSERVICE_CALL aprs_packet_object_get_course(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_OBJECT)
		return 0;

	return packet->extensions.course;
}
int32_t                           APRSERVICE_CALL aprs_packet_object_get_altitude(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_OBJECT)
		return 0;

	return packet->extensions.altitude;
}
float                             APRSERVICE_CALL aprs_packet_object_get_latitude(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_OBJECT)
		return 0;

	return packet->object->latitude;
}
float                             APRSERVICE_CALL aprs_packet_object_get_longitude(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_OBJECT)
		return 0;

	return packet->object->longitude;
}
char                              APRSERVICE_CALL aprs_packet_object_get_symbol_table(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_OBJECT)
		return '\0';

	return packet->object->symbol_table;
}
char                              APRSERVICE_CALL aprs_packet_object_get_symbol_table_key(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_OBJECT)
		return '\0';

	return packet->object->symbol_table_key;
}
bool                              APRSERVICE_CALL aprs_packet_object_set_time(struct aprs_packet* packet, const struct aprs_time* value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_OBJECT)
		return false;

	if (!aprs_validate_time(value))
		return false;

	packet->object->time = *value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_object_set_alive(struct aprs_packet* packet, bool value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_OBJECT)
		return false;

	packet->object->is_alive = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_object_set_compressed(struct aprs_packet* packet, bool value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_OBJECT)
		return false;

	packet->object->is_compressed = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_object_set_name(struct aprs_packet* packet, const char* value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_OBJECT)
		return false;

	if (auto length = aprs_validate_name(value))
	{
		packet->object->name.assign(value, length);

		return true;
	}

	return false;
}
bool                              APRSERVICE_CALL aprs_packet_object_set_comment(struct aprs_packet* packet, const char* value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_OBJECT)
		return false;

	if (!value)
	{
		packet->object->comment.clear();

		return true;
	}
	else if (auto length = aprs_validate_comment(value, 36))
	{
		packet->object->comment.assign(value, length);

		return true;
	}

	return false;
}
bool                              APRSERVICE_CALL aprs_packet_object_set_speed(struct aprs_packet* packet, uint16_t value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_OBJECT)
		return false;

	packet->extensions.speed = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_object_set_course(struct aprs_packet* packet, uint16_t value)
{
	if (value > 359)
		return false;

	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_OBJECT)
		return false;

	packet->extensions.course = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_object_set_altitude(struct aprs_packet* packet, int32_t value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_OBJECT)
		return false;

	packet->extensions.altitude = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_object_set_latitude(struct aprs_packet* packet, float value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_OBJECT)
		return false;

	packet->object->latitude = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_object_set_longitude(struct aprs_packet* packet, float value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_OBJECT)
		return false;

	packet->object->longitude = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_object_set_symbol(struct aprs_packet* packet, char table, char key)
{
	if (!aprs_validate_symbol(table, key))
		return false;

	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_OBJECT)
		return false;

	packet->object->symbol_table     = table;
	packet->object->symbol_table_key = key;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_object_set_symbol_table(struct aprs_packet* packet, char value)
{
	return aprs_packet_object_set_symbol(packet, value, aprs_packet_object_get_symbol_table_key(packet));
}
bool                              APRSERVICE_CALL aprs_packet_object_set_symbol_table_key(struct aprs_packet* packet, char value)
{
	return aprs_packet_object_set_symbol(packet, aprs_packet_object_get_symbol_table(packet), value);
}

struct aprs_packet*               APRSERVICE_CALL aprs_packet_status_init(const char* sender, const char* tocall, struct aprs_path* path, const char* message)
{
	if (auto packet = aprs_packet_init_ex(sender, tocall, path, APRS_PACKET_TYPE_STATUS))
	{
		packet->status = new aprs_packet_status {};

		if (!aprs_packet_status_set_time(packet, nullptr) ||
			!aprs_packet_status_set_message(packet, message))
		{
			aprs_packet_deinit(packet);

			return nullptr;
		}

		return packet;
	}

	return nullptr;
}
struct aprs_time*                 APRSERVICE_CALL aprs_packet_status_get_time(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_STATUS)
		return nullptr;

	if (!packet->status->is_time_set)
		return nullptr;

	return &packet->status->time;
}
const char*                       APRSERVICE_CALL aprs_packet_status_get_message(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_STATUS)
		return nullptr;

	return packet->status->message.c_str();
}
bool                              APRSERVICE_CALL aprs_packet_status_set_time(struct aprs_packet* packet, struct aprs_time* value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_STATUS)
		return false;

	if (!value)
	{
		packet->status->is_time_set = false;

		return true;
	}

	if (aprs_validate_time(value))
	{
		packet->status->is_time_set = true;
		packet->status->time        = *value;

		return true;
	}

	return false;
}
bool                              APRSERVICE_CALL aprs_packet_status_set_message(struct aprs_packet* packet, const char* value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_STATUS)
		return false;

	if (!aprs_validate_status(value, packet->status->is_time_set ? 55 : 62))
		return false;

	packet->status->message = value;

	return true;
}

struct aprs_packet*               APRSERVICE_CALL aprs_packet_message_init(const char* sender, const char* tocall, struct aprs_path* path, const char* destination, const char* content)
{
	if (auto packet = aprs_packet_init_ex(sender, tocall, path, APRS_PACKET_TYPE_MESSAGE))
	{
		packet->message = new aprs_packet_message {};

		if (!aprs_packet_message_set_type(packet, APRS_MESSAGE_TYPE_MESSAGE) ||
			!aprs_packet_message_set_content(packet, content) ||
			!aprs_packet_message_set_destination(packet, destination))
		{
			aprs_packet_deinit(packet);

			return nullptr;
		}

		return packet;
	}

	return nullptr;
}
struct aprs_packet*               APRSERVICE_CALL aprs_packet_message_init_ack(const char* sender, const char* tocall, struct aprs_path* path, const char* destination, const char* id)
{
	if (auto packet = aprs_packet_init_ex(sender, tocall, path, APRS_PACKET_TYPE_MESSAGE))
	{
		packet->message = new aprs_packet_message {};

		if (!aprs_packet_message_set_type(packet, APRS_MESSAGE_TYPE_ACK) ||
			!aprs_packet_message_set_id(packet, id) ||
			!aprs_packet_message_set_destination(packet, destination))
		{
			aprs_packet_deinit(packet);

			return nullptr;
		}

		return packet;
	}

	return nullptr;
}
struct aprs_packet*               APRSERVICE_CALL aprs_packet_message_init_reject(const char* sender, const char* tocall, struct aprs_path* path, const char* destination, const char* id)
{
	if (auto packet = aprs_packet_init_ex(sender, tocall, path, APRS_PACKET_TYPE_MESSAGE))
	{
		packet->message = new aprs_packet_message {};

		if (!aprs_packet_message_set_type(packet, APRS_MESSAGE_TYPE_REJECT) ||
			!aprs_packet_message_set_id(packet, id) ||
			!aprs_packet_message_set_destination(packet, destination))
		{
			aprs_packet_deinit(packet);

			return nullptr;
		}

		return packet;
	}

	return nullptr;
}
struct aprs_packet*               APRSERVICE_CALL aprs_packet_message_init_bulletin(const char* sender, const char* tocall, struct aprs_path* path, const char* destination)
{
	if (auto packet = aprs_packet_init_ex(sender, tocall, path, APRS_PACKET_TYPE_MESSAGE))
	{
		packet->message = new aprs_packet_message {};

		if (!aprs_packet_message_set_type(packet, APRS_MESSAGE_TYPE_BULLETIN) ||
			!aprs_packet_message_set_destination(packet, destination))
		{
			aprs_packet_deinit(packet);

			return nullptr;
		}

		return packet;
	}

	return nullptr;
}
const char*                       APRSERVICE_CALL aprs_packet_message_get_id(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_MESSAGE)
		return nullptr;

	if (!packet->message->id.length())
		return nullptr;

	return packet->message->id.c_str();
}
enum APRS_MESSAGE_TYPES           APRSERVICE_CALL aprs_packet_message_get_type(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_MESSAGE)
		return APRS_MESSAGE_TYPES_COUNT;

	return packet->message->type;
}
const char*                       APRSERVICE_CALL aprs_packet_message_get_content(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_MESSAGE)
		return nullptr;

	return packet->message->content.c_str();
}
const char*                       APRSERVICE_CALL aprs_packet_message_get_destination(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_MESSAGE)
		return nullptr;

	return packet->message->destination.c_str();
}
bool                              APRSERVICE_CALL aprs_packet_message_set_id(struct aprs_packet* packet, const char* value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_MESSAGE)
		return false;

	if (aprs_packet_message_get_type(packet) == APRS_MESSAGE_TYPE_BULLETIN)
		return false;

	if (!value)
	{
		if (aprs_packet_message_get_type(packet) != APRS_MESSAGE_TYPE_MESSAGE)
			return false;

		packet->message->id.clear();

		return true;
	}
	else if (auto length = aprs_string_length(value, true); length && (length >= 1) && (length <= 5))
	{
		static auto is_string_valid = [](size_t index, char value)->bool
		{
			return value && (value != ' ');
		};

		if (!aprs_validate_string(value, length, is_string_valid))
			return false;

		packet->message->id.assign(value, length);

		return true;
	}

	return false;
}
bool                              APRSERVICE_CALL aprs_packet_message_set_type(struct aprs_packet* packet, enum APRS_MESSAGE_TYPES value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_MESSAGE)
		return false;

	switch (value)
	{
		case APRS_MESSAGE_TYPE_ACK:
		case APRS_MESSAGE_TYPE_REJECT:
			if (!aprs_packet_message_get_id(packet))
				aprs_packet_message_set_id(packet, "0");
		case APRS_MESSAGE_TYPE_MESSAGE:
			packet->message->type = value;
			return true;

		case APRS_MESSAGE_TYPE_BULLETIN:
			packet->message->id.clear();
			packet->message->type = value;
			return true;
	}

	return false;
}
bool                              APRSERVICE_CALL aprs_packet_message_set_content(struct aprs_packet* packet, const char* value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_MESSAGE)
		return false;

	switch (aprs_packet_message_get_type(packet))
	{
		case APRS_MESSAGE_TYPE_MESSAGE:
		case APRS_MESSAGE_TYPE_BULLETIN:
			break;

		default:
			return false;
	}

	if (!value)
	{
		packet->message->content.clear();

		return true;
	}
	else if (auto length = aprs_string_length(value); length && (length <= 67))
	{
		packet->message->type = APRS_MESSAGE_TYPE_MESSAGE;
		packet->message->content.assign(value, length);

		return true;
	}

	return false;
}
bool                              APRSERVICE_CALL aprs_packet_message_set_destination(struct aprs_packet* packet, const char* value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_MESSAGE)
		return false;

	if (auto length = aprs_validate_name(value))
	{
		packet->message->destination.assign(value, length);

		return true;
	}

	return false;
}

struct aprs_packet*               APRSERVICE_CALL aprs_packet_weather_init(const char* sender, const char* tocall, struct aprs_path* path, const char* type, char software)
{
	auto type_length = aprs_string_length(type, true);

	if (!type_length || (type_length < 2) || (type_length > 4))
		return nullptr;

	if (auto packet = aprs_packet_init_ex(sender, tocall, path, APRS_PACKET_TYPE_WEATHER))
	{
		packet->weather = new aprs_packet_weather
		{
			.time     = *aprs_time_now(),
			.type     = std::string(type, type_length),
			.software = software
		};

		return packet;
	}

	return nullptr;
}
const struct aprs_time*           APRSERVICE_CALL aprs_packet_weather_get_time(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_WEATHER)
		return nullptr;

	return &packet->weather->time;
}
const char*                       APRSERVICE_CALL aprs_packet_weather_get_type(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_WEATHER)
		return nullptr;

	return packet->weather->type.c_str();
}
char                              APRSERVICE_CALL aprs_packet_weather_get_software(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_WEATHER)
		return 0;

	return packet->weather->software;
}
uint16_t                          APRSERVICE_CALL aprs_packet_weather_get_wind_speed(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_WEATHER)
		return 0;

	return packet->weather->wind_speed;
}
uint16_t                          APRSERVICE_CALL aprs_packet_weather_get_wind_speed_gust(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_WEATHER)
		return 0;

	return packet->weather->wind_speed_gust;
}
uint16_t                          APRSERVICE_CALL aprs_packet_weather_get_wind_direction(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_WEATHER)
		return 0;

	return packet->weather->wind_direction;
}
uint16_t                          APRSERVICE_CALL aprs_packet_weather_get_rainfall_last_hour(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_WEATHER)
		return 0;

	return packet->weather->rainfall_last_hour;
}
uint16_t                          APRSERVICE_CALL aprs_packet_weather_get_rainfall_last_24_hours(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_WEATHER)
		return 0;

	return packet->weather->rainfall_last_24_hours;
}
uint16_t                          APRSERVICE_CALL aprs_packet_weather_get_rainfall_since_midnight(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_WEATHER)
		return 0;

	return packet->weather->rainfall_since_midnight;
}
uint8_t                           APRSERVICE_CALL aprs_packet_weather_get_humidity(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_WEATHER)
		return 0;

	return packet->weather->humidity;
}
int16_t                           APRSERVICE_CALL aprs_packet_weather_get_temperature(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_WEATHER)
		return 0;

	return packet->weather->temperature;
}
uint32_t                          APRSERVICE_CALL aprs_packet_weather_get_barometric_pressure(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_WEATHER)
		return 0;

	return packet->weather->barometric_pressure;
}
bool                              APRSERVICE_CALL aprs_packet_weather_set_time(struct aprs_packet* packet, const struct aprs_time* value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_WEATHER)
		return false;

	if (!aprs_validate_time(value))
		return false;

	if (!(value->type & APRS_TIME_MDHM))
		return false;

	packet->weather->time = *value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_weather_set_wind_speed(struct aprs_packet* packet, uint16_t value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_WEATHER)
		return false;

	if (value > 9999)
		return false;

	packet->weather->wind_speed = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_weather_set_wind_speed_gust(struct aprs_packet* packet, uint16_t value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_WEATHER)
		return false;

	if (value > 9999)
		return false;

	packet->weather->wind_speed_gust = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_weather_set_wind_direction(struct aprs_packet* packet, uint16_t value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_WEATHER)
		return false;

	if (value > 359)
		return false;

	packet->weather->wind_direction = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_weather_set_rainfall_last_hour(struct aprs_packet* packet, uint16_t value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_WEATHER)
		return false;

	if (value > 9999)
		return false;

	packet->weather->rainfall_last_hour = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_weather_set_rainfall_last_24_hours(struct aprs_packet* packet, uint16_t value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_WEATHER)
		return false;

	if (value > 9999)
		return false;

	packet->weather->rainfall_last_24_hours = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_weather_set_rainfall_since_midnight(struct aprs_packet* packet, uint16_t value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_WEATHER)
		return false;

	if (value > 9999)
		return false;

	packet->weather->rainfall_since_midnight = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_weather_set_humidity(struct aprs_packet* packet, uint8_t value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_WEATHER)
		return false;

	if (value > 100)
		return false;

	packet->weather->humidity = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_weather_set_temperature(struct aprs_packet* packet, int16_t value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_WEATHER)
		return false;

	if ((value > 0) && (value > 9999))
		return false;

	if ((value < 0) && (value < -999))
		return false;

	packet->weather->temperature = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_weather_set_barometric_pressure(struct aprs_packet* packet, uint32_t value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_WEATHER)
		return false;

	if (value > 99999)
		return false;

	packet->weather->barometric_pressure = value;

	return true;
}

aprs_packet*                                      aprs_packet_position_init(const char* sender, const char* tocall, struct aprs_path* path, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course, const char* comment, char symbol_table, char symbol_table_key, int flags)
{
	if ((flags & APRS_POSITION_FLAG_MIC_E) && (flags & APRS_POSITION_FLAG_TIME))
		return nullptr;

	if ((flags & APRS_POSITION_FLAG_MIC_E) && (flags & APRS_POSITION_FLAG_COMPRESSED))
		return nullptr;

	if (auto packet = aprs_packet_init_ex(sender, tocall, path, APRS_PACKET_TYPE_POSITION))
	{
		packet->position = new aprs_packet_position
		{
			.flags = flags,
			.time  = *aprs_time_now()
		};

		if (!aprs_packet_position_set_speed(packet, speed) ||
			!aprs_packet_position_set_course(packet, course) ||
			!aprs_packet_position_set_comment(packet, comment) ||
			!aprs_packet_position_set_altitude(packet, altitude) ||
			!aprs_packet_position_set_latitude(packet, latitude) ||
			!aprs_packet_position_set_longitude(packet, longitude) ||
			!aprs_packet_position_set_symbol(packet, symbol_table, symbol_table_key))
		{
			aprs_packet_deinit(packet);

			return nullptr;
		}

		return packet;
	}

	return nullptr;
}
struct aprs_packet*               APRSERVICE_CALL aprs_packet_position_init(const char* sender, const char* tocall, struct aprs_path* path, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course, const char* comment, char symbol_table, char symbol_table_key)
{
	return aprs_packet_position_init(sender, tocall, path, latitude, longitude, altitude, speed, course, comment, symbol_table, symbol_table_key, 0);
}
struct aprs_packet*               APRSERVICE_CALL aprs_packet_position_init_mic_e(const char* sender, const char* tocall, struct aprs_path* path, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course, const char* comment, char symbol_table, char symbol_table_key)
{
	return aprs_packet_position_init(sender, tocall, path, latitude, longitude, altitude, speed, course, comment, symbol_table, symbol_table_key, APRS_POSITION_FLAG_MIC_E);
}
struct aprs_packet*               APRSERVICE_CALL aprs_packet_position_init_compressed(const char* sender, const char* tocall, struct aprs_path* path, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course, const char* comment, char symbol_table, char symbol_table_key)
{
	return aprs_packet_position_init(sender, tocall, path, latitude, longitude, altitude, speed, course, comment, symbol_table, symbol_table_key, APRS_POSITION_FLAG_COMPRESSED);
}
bool                              APRSERVICE_CALL aprs_packet_position_is_mic_e(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_POSITION)
		return false;

	return packet->position->flags & APRS_POSITION_FLAG_MIC_E;
}
bool                              APRSERVICE_CALL aprs_packet_position_is_compressed(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_POSITION)
		return false;

	return packet->position->flags & APRS_POSITION_FLAG_COMPRESSED;
}
bool                              APRSERVICE_CALL aprs_packet_position_is_messaging_enabled(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_POSITION)
		return false;

	return packet->position->flags & APRS_POSITION_FLAG_MESSAGING_ENABLED;
}
const struct aprs_time*           APRSERVICE_CALL aprs_packet_position_get_time(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_POSITION)
		return nullptr;

	if (!(packet->position->flags & APRS_POSITION_FLAG_TIME))
		return nullptr;

	return &packet->position->time;
}
int                               APRSERVICE_CALL aprs_packet_position_get_flags(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_POSITION)
		return 0;

	return packet->position->flags;
}
const char*                       APRSERVICE_CALL aprs_packet_position_get_comment(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_POSITION)
		return nullptr;

	return packet->position->comment.c_str();
}
uint16_t                          APRSERVICE_CALL aprs_packet_position_get_speed(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_POSITION)
		return 0;

	return packet->extensions.speed;
}
uint16_t                          APRSERVICE_CALL aprs_packet_position_get_course(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_POSITION)
		return 0;

	return packet->extensions.course;
}
int32_t                           APRSERVICE_CALL aprs_packet_position_get_altitude(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_POSITION)
		return 0;

	return packet->extensions.altitude;
}
float                             APRSERVICE_CALL aprs_packet_position_get_latitude(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_POSITION)
		return 0;

	return packet->position->latitude;
}
float                             APRSERVICE_CALL aprs_packet_position_get_longitude(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_POSITION)
		return 0;

	return packet->position->longitude;
}
char                              APRSERVICE_CALL aprs_packet_position_get_symbol_table(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_POSITION)
		return '\0';

	return packet->position->symbol_table;
}
char                              APRSERVICE_CALL aprs_packet_position_get_symbol_table_key(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_POSITION)
		return '\0';

	return packet->position->symbol_table_key;
}
bool                              APRSERVICE_CALL aprs_packet_position_set_time(struct aprs_packet* packet, const struct aprs_time* value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_POSITION)
		return false;

	if (!value)
	{
		packet->position->flags &= ~APRS_POSITION_FLAG_TIME;

		return true;
	}

	if (aprs_validate_time(value))
	{
		packet->position->time   = *value;
		packet->position->flags |= APRS_POSITION_FLAG_TIME;

		return true;
	}

	return false;
}
bool                              APRSERVICE_CALL aprs_packet_position_set_comment(struct aprs_packet* packet, const char* value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_POSITION)
		return false;

	if (!value)
	{
		packet->position->comment.clear();

		return true;
	}
	else if (auto length = aprs_validate_comment(value, 36))
	{
		packet->position->comment.assign(value, length);

		return true;
	}

	return false;
}
bool                              APRSERVICE_CALL aprs_packet_position_set_speed(struct aprs_packet* packet, uint16_t value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_POSITION)
		return false;

	packet->extensions.speed = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_position_set_course(struct aprs_packet* packet, uint16_t value)
{
	if (value > 359)
		return false;

	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_POSITION)
		return false;

	packet->extensions.course = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_position_set_altitude(struct aprs_packet* packet, int32_t value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_POSITION)
		return false;

	packet->extensions.altitude = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_position_set_latitude(struct aprs_packet* packet, float value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_POSITION)
		return false;

	packet->position->latitude = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_position_set_longitude(struct aprs_packet* packet, float value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_POSITION)
		return false;

	packet->position->longitude = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_position_set_symbol(struct aprs_packet* packet, char table, char key)
{
	if (!aprs_validate_symbol(table, key))
		return false;

	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_POSITION)
		return false;

	packet->position->symbol_table     = table;
	packet->position->symbol_table_key = key;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_position_set_symbol_table(struct aprs_packet* packet, char value)
{
	return aprs_packet_position_set_symbol(packet, value, aprs_packet_position_get_symbol_table_key(packet));
}
bool                              APRSERVICE_CALL aprs_packet_position_set_symbol_table_key(struct aprs_packet* packet, char value)
{
	return aprs_packet_position_set_symbol(packet, aprs_packet_position_get_symbol_table(packet), value);
}
bool                              APRSERVICE_CALL aprs_packet_position_enable_mic_e(struct aprs_packet* packet, bool value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_POSITION)
		return false;

	if (value)
	{
		packet->position->flags |= APRS_POSITION_FLAG_MIC_E;
		packet->position->flags &= ~APRS_POSITION_FLAG_COMPRESSED;
	}
	else
		packet->position->flags &= ~APRS_POSITION_FLAG_MIC_E;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_position_enable_messaging(struct aprs_packet* packet, bool value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_POSITION)
		return false;

	if (value)
		packet->position->flags |= APRS_POSITION_FLAG_MESSAGING_ENABLED;
	else
		packet->position->flags &= ~APRS_POSITION_FLAG_MESSAGING_ENABLED;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_position_enable_compression(struct aprs_packet* packet, bool value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_POSITION)
		return false;

	if (aprs_packet_position_is_mic_e(packet))
		return false;

	if (value)
		packet->position->flags |= APRS_POSITION_FLAG_COMPRESSED;
	else
		packet->position->flags &= ~APRS_POSITION_FLAG_COMPRESSED;

	return true;
}

struct aprs_packet*               APRSERVICE_CALL aprs_packet_telemetry_init(const char* sender, const char* tocall, struct aprs_path* path, uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t digital, uint16_t sequence)
{
	if (auto packet = aprs_packet_init_ex(sender, tocall, path, APRS_PACKET_TYPE_TELEMETRY))
	{
		packet->telemetry = new aprs_packet_telemetry
		{
			.type      = APRS_TELEMETRY_TYPE_U8,
			.analog_u8 = { a1, a2, a3, a4, a5 },
			.digital   = digital
		};

		packet->telemetry->analog_u8_c[0] = &packet->telemetry->analog_u8[0];
		packet->telemetry->analog_u8_c[1] = &packet->telemetry->analog_u8[1];
		packet->telemetry->analog_u8_c[2] = &packet->telemetry->analog_u8[2];
		packet->telemetry->analog_u8_c[3] = &packet->telemetry->analog_u8[3];
		packet->telemetry->analog_u8_c[4] = &packet->telemetry->analog_u8[4];
		packet->telemetry->analog_u8_c[5] = nullptr;

		if (!aprs_packet_telemetry_set_sequence(packet, sequence))
		{
			aprs_packet_deinit(packet);

			return nullptr;
		}

		return packet;
	}

	return nullptr;
}
struct aprs_packet*               APRSERVICE_CALL aprs_packet_telemetry_init_float(const char* sender, const char* tocall, struct aprs_path* path, float a1, float a2, float a3, float a4, float a5, uint8_t digital, uint16_t sequence)
{
	if (auto packet = aprs_packet_init_ex(sender, tocall, path, APRS_PACKET_TYPE_TELEMETRY))
	{
		packet->telemetry = new aprs_packet_telemetry
		{
			.type         = APRS_TELEMETRY_TYPE_FLOAT,
			.analog_float = { a1, a2, a3, a4, a5 },
			.digital      = digital
		};

		packet->telemetry->analog_float_c[0] = &packet->telemetry->analog_float[0];
		packet->telemetry->analog_float_c[1] = &packet->telemetry->analog_float[1];
		packet->telemetry->analog_float_c[2] = &packet->telemetry->analog_float[2];
		packet->telemetry->analog_float_c[3] = &packet->telemetry->analog_float[3];
		packet->telemetry->analog_float_c[4] = &packet->telemetry->analog_float[4];
		packet->telemetry->analog_float_c[5] = nullptr;

		if (!aprs_packet_telemetry_set_sequence(packet, sequence))
		{
			aprs_packet_deinit(packet);

			return nullptr;
		}

		return packet;
	}

	return nullptr;
}
enum APRS_TELEMETRY_TYPES         APRSERVICE_CALL aprs_packet_telemetry_get_type(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_TELEMETRY)
		return APRS_TELEMETRY_TYPES_COUNT;

	return packet->telemetry->type;
}
const uint8_t**                   APRSERVICE_CALL aprs_packet_telemetry_get_analog(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_TELEMETRY)
		return nullptr;

	if (aprs_packet_telemetry_get_type(packet) != APRS_TELEMETRY_TYPE_U8)
		return nullptr;

	return packet->telemetry->analog_u8_c.data();
}
const float**                     APRSERVICE_CALL aprs_packet_telemetry_get_analog_float(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_TELEMETRY)
		return nullptr;

	if (aprs_packet_telemetry_get_type(packet) != APRS_TELEMETRY_TYPE_FLOAT)
		return nullptr;

	return packet->telemetry->analog_float_c.data();
}
uint8_t                           APRSERVICE_CALL aprs_packet_telemetry_get_bits(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_TELEMETRY)
		return 0;

	if (aprs_packet_telemetry_get_type(packet) != APRS_TELEMETRY_TYPE_BITS)
		return 0;

	return packet->telemetry->digital;
}
const struct aprs_telemetry_eqn** APRSERVICE_CALL aprs_packet_telemetry_get_eqns(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_TELEMETRY)
		return nullptr;

	if (aprs_packet_telemetry_get_type(packet) != APRS_TELEMETRY_TYPE_EQNS)
		return nullptr;

	return packet->telemetry->eqns_c.data();
}
const char**                      APRSERVICE_CALL aprs_packet_telemetry_get_units(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_TELEMETRY)
		return nullptr;

	if (aprs_packet_telemetry_get_type(packet) != APRS_TELEMETRY_TYPE_UNITS)
		return nullptr;

	return packet->telemetry->units_c.data();
}
const char**                      APRSERVICE_CALL aprs_packet_telemetry_get_params(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_TELEMETRY)
		return nullptr;

	if (aprs_packet_telemetry_get_type(packet) != APRS_TELEMETRY_TYPE_PARAMS)
		return nullptr;

	return packet->telemetry->params_c.data();
}
uint8_t                           APRSERVICE_CALL aprs_packet_telemetry_get_digital(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_TELEMETRY)
		return 0;

	switch (aprs_packet_telemetry_get_type(packet))
	{
		case APRS_TELEMETRY_TYPE_U8:
		case APRS_TELEMETRY_TYPE_FLOAT:
			return packet->telemetry->digital;
	}

	return 0;
}
uint16_t                          APRSERVICE_CALL aprs_packet_telemetry_get_sequence(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_TELEMETRY)
		return 0;

	switch (aprs_packet_telemetry_get_type(packet))
	{
		case APRS_TELEMETRY_TYPE_U8:
		case APRS_TELEMETRY_TYPE_FLOAT:
			return packet->telemetry->sequence;
	}

	return 0;
}
const char*                       APRSERVICE_CALL aprs_packet_telemetry_get_comment(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_TELEMETRY)
		return nullptr;

	switch (aprs_packet_telemetry_get_type(packet))
	{
		case APRS_TELEMETRY_TYPE_U8:
		case APRS_TELEMETRY_TYPE_FLOAT:
		case APRS_TELEMETRY_TYPE_BITS:
			return packet->telemetry->comment.c_str();
	}

	return nullptr;
}
bool                              APRSERVICE_CALL aprs_packet_telemetry_set_analog(struct aprs_packet* packet, uint8_t value, uint8_t index)
{
	if (index >= 5)
		return false;

	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_TELEMETRY)
		return false;

	if (aprs_packet_telemetry_get_type(packet) != APRS_TELEMETRY_TYPE_U8)
		return false;

	packet->telemetry->analog_u8[index] = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_telemetry_set_analog_float(struct aprs_packet* packet, float value, uint8_t index)
{
	if (index >= 5)
		return false;

	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_TELEMETRY)
		return false;

	if (aprs_packet_telemetry_get_type(packet) != APRS_TELEMETRY_TYPE_FLOAT)
		return false;

	packet->telemetry->analog_float[index] = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_telemetry_set_digital(struct aprs_packet* packet, uint8_t value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_TELEMETRY)
		return false;

	switch (aprs_packet_telemetry_get_type(packet))
	{
		case APRS_TELEMETRY_TYPE_U8:
		case APRS_TELEMETRY_TYPE_FLOAT:
			packet->telemetry->digital = value;
			return true;
	}

	return false;
}
bool                              APRSERVICE_CALL aprs_packet_telemetry_set_sequence(struct aprs_packet* packet, uint16_t value)
{
	if (value > 999)
		return false;

	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_TELEMETRY)
		return false;

	switch (aprs_packet_telemetry_get_type(packet))
	{
		case APRS_TELEMETRY_TYPE_U8:
		case APRS_TELEMETRY_TYPE_FLOAT:
			packet->telemetry->sequence = value;
			return true;
	}

	return false;
}
bool                              APRSERVICE_CALL aprs_packet_telemetry_set_comment(struct aprs_packet* packet, const char* value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_TELEMETRY)
		return false;

	switch (aprs_packet_telemetry_get_type(packet))
	{
		case APRS_TELEMETRY_TYPE_U8:
		case APRS_TELEMETRY_TYPE_FLOAT:
		case APRS_TELEMETRY_TYPE_BITS:
			if (auto length = aprs_validate_comment(value, 67))
			{
				packet->telemetry->comment.assign(value, length);

				return true;
			}
			break;
	}

	return false;
}

struct aprs_packet*               APRSERVICE_CALL aprs_packet_user_defined_init(const char* sender, const char* tocall, struct aprs_path* path, char id, char type, const char* data)
{
	if (auto packet = aprs_packet_init_ex(sender, tocall, path, APRS_PACKET_TYPE_USER_DEFINED))
	{
		packet->user_defined = new aprs_packet_user_defined {};

		if (!aprs_packet_user_defined_set_id(packet, id) ||
			!aprs_packet_user_defined_set_type(packet, type) ||
			!aprs_packet_user_defined_set_data(packet, data))
		{
			aprs_packet_deinit(packet);

			return nullptr;
		}

		return packet;
	}

	return nullptr;
}
char                              APRSERVICE_CALL aprs_packet_user_defined_get_id(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_USER_DEFINED)
		return 0;

	return packet->user_defined->id;
}
char                              APRSERVICE_CALL aprs_packet_user_defined_get_type(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_USER_DEFINED)
		return 0;

	return packet->user_defined->type;
}
const char*                       APRSERVICE_CALL aprs_packet_user_defined_get_data(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_USER_DEFINED)
		return nullptr;

	return packet->user_defined->data.c_str();
}
bool                              APRSERVICE_CALL aprs_packet_user_defined_set_id(struct aprs_packet* packet, char value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_USER_DEFINED)
		return false;

	if (!isprint(value))
		return false;

	packet->user_defined->id = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_user_defined_set_type(struct aprs_packet* packet, char value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_USER_DEFINED)
		return false;

	if (!isprint(value))
		return false;

	packet->user_defined->type = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_user_defined_set_data(struct aprs_packet* packet, const char* value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_USER_DEFINED)
		return false;

	if (auto length = aprs_validate_user_defined_data(value))
	{
		packet->user_defined->data.assign(value, length);

		return true;
	}

	return false;
}

struct aprs_packet*               APRSERVICE_CALL aprs_packet_third_party_init(const char* sender, const char* tocall, struct aprs_path* path)
{
	if (auto packet = aprs_packet_init_ex(sender, tocall, path, APRS_PACKET_TYPE_THIRD_PARTY))
	{
		packet->third_party = new aprs_packet_third_party {};

		return packet;
	}

	return nullptr;
}
const char*                       APRSERVICE_CALL aprs_packet_third_party_get_content(struct aprs_packet* packet)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_THIRD_PARTY)
		return nullptr;

	return packet->third_party->content.c_str();
}
bool                              APRSERVICE_CALL aprs_packet_third_party_set_content(struct aprs_packet* packet, const char* value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_THIRD_PARTY)
		return false;

	packet->third_party->content = value;

	return true;
}

float                                             aprs_distance(double value, APRS_DISTANCES type)
{
	switch (type)
	{
		case APRS_DISTANCE_FEET:       return value * 20903251;
		case APRS_DISTANCE_MILES:      return value * 3959.6f;
		case APRS_DISTANCE_METERS:     return value * 6371e3;
		case APRS_DISTANCE_KILOMETERS: return value * 6371;
	}

	return 0;
}
float                             APRSERVICE_CALL aprs_distance(float latitude1, float longitude1, float latitude2, float longitude2, enum APRS_DISTANCES type)
{
	double radians[] =
	{
		APRS_DEG2RAD * (latitude2 - latitude1),
		APRS_DEG2RAD * (longitude2 - longitude1),
		APRS_DEG2RAD * latitude1,
		APRS_DEG2RAD * latitude2
	};

	double a = sin(radians[0] / 2) * sin(radians[0] / 2) + cos(radians[2]) * cos(radians[3]) * sin(radians[1] / 2) * sin(radians[1] / 2);
	double b = 2 * atan2(sqrt(a), sqrt(1 - a));

	return aprs_distance(b, type);
}
float                             APRSERVICE_CALL aprs_distance_3d(float latitude1, float longitude1, int32_t altitude1, float latitude2, float longitude2, int32_t altitude2, enum APRS_DISTANCES type)
{
	double distance   = aprs_distance(latitude1, longitude1, latitude2, longitude2, type);
	double distance_z = (altitude1 > altitude2) ? (altitude1 - altitude2) : (altitude2 - altitude1);

	return distance + aprs_distance(distance_z / 20903251, type);
}
