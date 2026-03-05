#include "APRS.hpp"

#include <array>
#include <cmath>
#include <ctime>
#include <regex>
#include <string>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <charconv>
#include <iostream>
#include <type_traits>

// TODO: replace strtok with something like std::views::split

// make sure this is never changed
// logic depends on DHM being a subset of MDHM
static_assert(APRS_TIME_MDHM & APRS_TIME_DHM);

constexpr double   APRS_DEG2RAD                      = 3.14159265358979323846 / 180;

constexpr uint8_t  APRS_DATA_EXTENSION_POWER[]       = { 0,  1,  4,  9,   16,  25,  36,  49,   64,   81 };
constexpr uint16_t APRS_DATA_EXTENSION_HEIGHT[]      = { 10, 20, 40, 80,  160, 320, 640, 1280, 2560, 5120 };
constexpr uint16_t APRS_DATA_EXTENSION_DIRECTIVITY[] = { 0,  45, 90, 135, 180, 225, 270, 315,  360 };

struct aprs_mic_e_message
{
	APRS_MIC_E_MESSAGES message;
	const char*         string;
};

constexpr const aprs_mic_e_message aprs_mic_e_messages[APRS_MIC_E_MESSAGES_COUNT] =
{
	{ APRS_MIC_E_MESSAGE_EMERGENCY,  "Emergency"  },
	{ APRS_MIC_E_MESSAGE_PRIORITY,   "Priority"   },
	{ APRS_MIC_E_MESSAGE_SPECIAL,    "Special"    },
	{ APRS_MIC_E_MESSAGE_COMMITTED,  "Committed"  },
	{ APRS_MIC_E_MESSAGE_RETURNING,  "Returning"  },
	{ APRS_MIC_E_MESSAGE_IN_SERVICE, "In Service" },
	{ APRS_MIC_E_MESSAGE_EN_ROUTE,   "En Route"   },
	{ APRS_MIC_E_MESSAGE_OFF_DUTY,   "Off Duty"   },

	{ APRS_MIC_E_MESSAGE_CUSTOM_0,   "Custom 0"   },
	{ APRS_MIC_E_MESSAGE_CUSTOM_1,   "Custom 1"   },
	{ APRS_MIC_E_MESSAGE_CUSTOM_2,   "Custom 2"   },
	{ APRS_MIC_E_MESSAGE_CUSTOM_3,   "Custom 3"   },
	{ APRS_MIC_E_MESSAGE_CUSTOM_4,   "Custom 4"   },
	{ APRS_MIC_E_MESSAGE_CUSTOM_5,   "Custom 5"   },
	{ APRS_MIC_E_MESSAGE_CUSTOM_6,   "Custom 6"   }
};

template<size_t ... I>
constexpr bool static_assert_aprs_mic_e_messages(std::index_sequence<I ...>)
{
	return ((aprs_mic_e_messages[I].message == I) && ...);
}
static_assert(static_assert_aprs_mic_e_messages(std::make_index_sequence<APRS_MIC_E_MESSAGES_COUNT> {}));

struct aprs_path
{
	uint8_t                       size;
	std::array<aprs_path_node, 8> chunks;
	std::array<std::string, 8>    chunks_stations;

	std::string                   string;

	size_t                        reference_count;
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
	int                    flags;

	aprs_time              time;

	float                  latitude;
	float                  longitude;

	std::string            comment;

	char                   symbol_table;
	char                   symbol_table_key;

	APRS_MIC_E_MESSAGES    mic_e_message;
	std::array<uint8_t, 5> mic_e_telemetry;
	uint8_t                mic_e_telemetry_channels;
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

typedef std::regex                                           aprs_regex_pattern;
typedef std::match_results<std::string_view::const_iterator> aprs_regex_match_result;

template<typename T>
constexpr T        aprs_from_float(float value, float& fraction)
{
	fraction = modff(value, &value);

	return value;
}

bool               aprs_regex_match(aprs_regex_match_result& match, const aprs_regex_pattern& regex, std::string_view string)
{
	try
	{
		if (!std::regex_match(string.begin(), string.end(), match, regex))
			return false;
	}
	catch (const std::regex_error& exception)
	{
		std::cerr << exception.what() << std::endl;

		return false;
	}

	return true;
}

size_t             aprs_string_length(std::string_view string, bool stop_at_whitespace = false)
{
	if (!stop_at_whitespace)
		return string.length();

	size_t value = 0;

	for (auto c : string)
	{
		if (isspace(c))
			break;

		++value;
	}

	return value;
}
bool               aprs_string_contains(std::string_view string, char value)
{
	for (auto c : string)
		if (c == value)
			return true;

	return false;
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
constexpr bool     aprs_validate_base91(char value)
{
	return (value >= 33) & (value <= 124);
}
bool               aprs_validate_base91(std::string_view string)
{
	for (auto c : string)
		if (!aprs_validate_base91(c))
			return false;

	return true;
}
bool               aprs_validate_string(std::string_view value, bool(*is_char_valid)(size_t index, char value))
{
	size_t i = 0;

	for (auto c : value)
	{
		if (!is_char_valid(i, c))
			return false;

		++i;
	}

	return true;
}
bool               aprs_validate_name(std::string_view value)
{
	if (auto length = aprs_string_length(value, true); length && (length <= 9))
		return true;

	return false;
}
bool               aprs_validate_station(std::string_view value)
{
	int i           = 0;
	int ssid_offset = -1;

	for (auto c : value)
	{
		if (c == '-')
			ssid_offset = i;
		else if ((c < '0') || (c > '9'))
			if ((c < 'A') || (c > 'Z'))
				return false;

		if (++i == 10)
			return false;
	}

	if (!i)
		return false;

	if ((ssid_offset != -1) && (ssid_offset < (i - 3)))
		return false;

	return true;
}
bool               aprs_validate_status(std::string_view value, size_t max_length)
{
	size_t i = 0;

	for (auto c : value)
	{
		if (++i > max_length)
			return false;

		if (!isprint(c) || (c == '|') || (c == '~'))
			return false;
	}

	return true;
}
bool               aprs_validate_comment(std::string_view value, size_t max_length)
{
	if (auto length = aprs_string_length(value); length <= max_length)
		return true;

	return false;
}
bool               aprs_validate_user_defined_data(std::string_view value)
{
	for (auto c : value)
		if (!isprint(c))
			return false;

	return true;
}

bool               aprs_extract_path_node(std::string& station, bool& repeated, const char* value)
{
	size_t length      = 0;
	       repeated    = false;
	int    ssid_offset = -1;

	for (int i = 0; *value && (length < 10); ++i, ++length, ++value)
		if ((*value == '-') && (ssid_offset == -1))
			ssid_offset = i;
		else if ((*value < '0') || (*value > '9'))
			if ((*value < 'A') || (*value > 'Z'))
			{
				if ((*value == '*') && !value[1])
				{
					repeated = true;

					break;
				}

				return false;
			}

	if (!length || (length > 9))
		return false;

	if ((ssid_offset != -1) && (ssid_offset < (length - 3)))
		return false;

	station.assign(&value[-length], length);

	return true;
}

template<typename T>
T                  aprs_decode_int_ex(std::string_view string, size_t max_length, char(*get_char)(size_t index, char value))
{
	size_t i     = 0;
	T      value = 0;

	if (!string.empty())
	{
		if (string.length() < max_length)
			max_length = string.length();

		if (string[0] == '-')
		{
			++i;

			value = 1 << ((sizeof(T) * 8) - 1);
		}

		for (; i < max_length; ++i)
			value = 10 * value + (get_char(i, string[i]) - '0');
	}

	return value;
}
bool               aprs_decode_hex_1(uint8_t& value, char c)
{
	if (c == '0')
	{
		value = 0;

		return true;
	}

	if (c == '1')
	{
		value = 1;

		return true;
	}

	return false;
}
bool               aprs_decode_hex_4(uint8_t& value, char c)
{
	if ((c >= '0') && (c <= '9'))
	{
		value = c - '0';

		return true;
	}

	if ((c >= 'a') && (c <= 'f'))
	{
		value = 10 + (c - 'a');

		return true;
	}

	if ((c >= 'A') && (c <= 'F'))
	{
		value = 10 + (c - 'A');

		return true;
	}

	return false;
}
bool               aprs_decode_hex_8(uint8_t& value, char c1, char c2)
{
	uint8_t tmp[2];

	if (!aprs_decode_hex_4(tmp[0], c1) || !aprs_decode_hex_4(tmp[1], c2))
		return false;

	value = (tmp[0] << 4) | tmp[1];

	return true;
}
template<typename T>
bool               aprs_decode_base91(T& value, std::string_view string)
{
	value = 0;

	for (size_t i = 0, j = string.length() - 1; i < string.length(); ++i, --j)
	{
		if (!aprs_validate_base91(string[i]))
			return false;

		if (j)
			value += std::pow(91, j) * (string[i] - 33);
		else
			value += string[i] - 33;
	}

	return true;
}
bool               aprs_decode_time(aprs_time& value, std::string_view string, char type)
{
	static auto string_is_valid = [](size_t index, char value)->bool
	{
		return isdigit(value);
	};

	tm time = {};

	switch (type)
	{
		case 'h': // HMS
		{
			if (!aprs_validate_string(string.substr(0, 6), string_is_valid))
				return false;

			auto hour = string.substr(0, 2);
			auto min  = string.substr(2, 2);
			auto sec  = string.substr(4, 2);

			std::from_chars(hour.data(), hour.data() + hour.length(), time.tm_hour);
			std::from_chars(min.data(), min.data() + min.length(), time.tm_min);
			std::from_chars(sec.data(), sec.data() + sec.length(), time.tm_sec);

			value = { time, APRS_TIME_HMS };
		}
		return (value.tm_hour < 24) && (value.tm_min < 60) && (value.tm_sec < 60);

		case 'z': // DHM
		case '/':
		{
			if (!aprs_validate_string(string.substr(0, 6), string_is_valid))
				return false;

			auto mday = string.substr(0, 2);
			auto hour = string.substr(2, 2);
			auto min  = string.substr(4, 2);

			std::from_chars(mday.data(), mday.data() + mday.length(), time.tm_mday);
			std::from_chars(hour.data(), hour.data() + hour.length(), time.tm_hour);
			std::from_chars(min.data(), min.data() + min.length(), time.tm_min);

			value = { time, APRS_TIME_DHM };
		}
		return (value.tm_mday <= 31) && (value.tm_hour < 24) && (value.tm_min < 60);

		case 0: // MDHM
		{
			if (!aprs_validate_string(string.substr(0, 8), string_is_valid))
				return false;

			auto mon  = string.substr(0, 2);
			auto mday = string.substr(2, 2);
			auto hour = string.substr(4, 2);
			auto min  = string.substr(6, 2);

			std::from_chars(mon.data(), mon.data() + mon.length(), time.tm_mon);
			std::from_chars(mday.data(), mday.data() + mday.length(), time.tm_mday);
			std::from_chars(hour.data(), hour.data() + hour.length(), time.tm_hour);
			std::from_chars(min.data(), min.data() + min.length(), time.tm_min);

			value = { time, APRS_TIME_MDHM };
		}
		return (time.tm_mon <= 12) && (value.tm_mday <= 31) && (value.tm_hour < 24) && (value.tm_min < 60);
	}

	return false;
}
bool               aprs_decode_latitude(float& value, std::string_view string, char hemisphere)
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

	if (!aprs_validate_string(string, string_is_valid))
		return false;

	static auto get_char = [](size_t index, char value)
	{
		return (value == ' ') ? '0' : value;
	};

	auto hours   = aprs_decode_int_ex<uint8_t>(string.substr(0, 2), 2, get_char);
	auto minutes = aprs_decode_int_ex<uint8_t>(string.substr(2, 2), 2, get_char);
	auto seconds = aprs_decode_int_ex<uint8_t>(string.substr(5, 2), 2, get_char);

	value = hours + (minutes / 60.0f) + (seconds / 6000.0f);

	if (hemisphere == 'S')
		value *= -1;

	return true;
}
bool               aprs_decode_longitude(float& value, std::string_view string, char hemisphere)
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
				return (value == ' ') || ((value >= '0') && (value <= '9'));

			case 5:
				return value == '.';
		}

		return false;
	};

	if (!aprs_validate_string(string, string_is_valid))
		return false;

	static auto get_char = [](size_t index, char value)
	{
		return (value == ' ') ? '0' : value;
	};

	auto hours   = aprs_decode_int_ex<uint8_t>(string.substr(0, 3), 3, get_char);
	auto minutes = aprs_decode_int_ex<uint8_t>(string.substr(3, 2), 2, get_char);
	auto seconds = aprs_decode_int_ex<uint8_t>(string.substr(6, 2), 2, get_char);

	value = hours + (minutes / 60.0f) + (seconds / 6000.0f);

	if (hemisphere == 'W')
		value *= -1;

	return true;
}

bool               aprs_decode_compressed_location(aprs_compressed_location& value, std::string_view string)
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

	if ((string.length() != 13) || !aprs_validate_string(string, string_is_valid))
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

void               aprs_packet_decode_comment_weather(aprs_packet* packet, std::string& string)
{
	// TODO: implement
}
void               aprs_packet_decode_comment_position(aprs_packet* packet, std::string& string)
{
	// TODO: implement
}
void               aprs_packet_decode_comment_data_extensions(aprs_packet* packet, std::string& string)
{
	static const aprs_regex_pattern regex_dfs("DFS(\\d)(\\d)(\\d)(\\d)");
	static const aprs_regex_pattern regex_phg("PHG(\\d)(\\d)(\\d)(\\d)");
	static const aprs_regex_pattern regex_rng("RNG(\\d{4})");
	static const aprs_regex_pattern regex_altitude("\\/A=(-?\\d{1,6})");
	static const aprs_regex_pattern regex_course_speed("^((\\d{3})\\/(\\d{3}))");

	switch (packet->type)
	{
		case APRS_PACKET_TYPE_ITEM:
		case APRS_PACKET_TYPE_OBJECT:
		case APRS_PACKET_TYPE_POSITION:
		{
			aprs_regex_match_result match;

			if (aprs_regex_match(match, regex_course_speed, string))
			{
				auto& speed  = match[3];
				auto& course = match[2];

				std::from_chars(&*speed.first, &*speed.first + speed.length(), packet->extensions.speed);
				std::from_chars(&*course.first, &*course.first + course.length(), packet->extensions.course);

				string = string.substr(7);
			}
			else if (aprs_regex_match(match, regex_phg, string))
			{
				uint8_t  gain              = 0;
				auto&    gain_match        = match[3];

				uint8_t  power             = 0;
				auto&    power_match       = match[1];

				uint16_t height            = 0;
				auto&    height_match      = match[2];

				uint16_t directivity       = 0;
				auto&    directivity_match = match[4];

				std::from_chars(&*gain_match.first, &*gain_match.first + gain_match.length(), gain);
				std::from_chars(&*power_match.first, &*power_match.first + power_match.length(), power);
				std::from_chars(&*height_match.first, &*height_match.first + height_match.length(), height);
				std::from_chars(&*directivity_match.first, &*directivity_match.first + directivity_match.length(), directivity);

				if ((power < 10) && (height < 10) && (directivity < 10))
				{
					packet->extensions.phg.power       = APRS_DATA_EXTENSION_POWER[power];
					packet->extensions.phg.height      = APRS_DATA_EXTENSION_HEIGHT[height];
					packet->extensions.phg.gain        = gain;
					packet->extensions.phg.directivity = APRS_DATA_EXTENSION_DIRECTIVITY[directivity];

					string = string.substr(7);
				}
			}
			else if (aprs_regex_match(match, regex_rng, string))
			{
				auto& miles = match[1];

				std::from_chars(&*miles.first, &*miles.first + miles.length(), packet->extensions.rng.miles);

				string = string.substr(7);
			}
			else if (aprs_regex_match(match, regex_dfs, string))
			{
				uint8_t  gain              = 0;
				auto&    gain_match        = match[3];

				uint16_t height            = 0;
				auto&    height_match      = match[2];

				uint8_t  strength          = 0;
				auto&    strength_match    = match[1];

				uint16_t directivity       = 0;
				auto&    directivity_match = match[4];

				std::from_chars(&*gain_match.first, &*gain_match.first + gain_match.length(), gain);
				std::from_chars(&*height_match.first, &*height_match.first + height_match.length(), height);
				std::from_chars(&*strength_match.first, &*strength_match.first + strength_match.length(), strength);
				std::from_chars(&*directivity_match.first, &*directivity_match.first + directivity_match.length(), directivity);

				if ((height < 10) && (directivity < 10))
				{
					packet->extensions.dfs.strength    = strength;
					packet->extensions.dfs.height      = APRS_DATA_EXTENSION_HEIGHT[height];
					packet->extensions.dfs.gain        = gain;
					packet->extensions.dfs.directivity = APRS_DATA_EXTENSION_DIRECTIVITY[directivity];

					string = string.substr(7);
				}
			}

			if (aprs_regex_match(match, regex_altitude, string))
			{
				auto& match0 = match[0];

				int32_t altitude       = 0;
				auto&   altitude_match = match[1];

				std::from_chars(&*altitude_match.first, &*altitude_match.first + altitude_match.length(), packet->extensions.altitude);

				if (auto i = string.find(&*match0.first, 0, match0.length()); i != std::string::npos)
					string = string.erase(i, match0.length());
			}
		}
		break;
	}
}
void               aprs_packet_decode_comment(aprs_packet* packet, std::string& string)
{
	aprs_packet_decode_comment_weather(packet, string);
	aprs_packet_decode_comment_position(packet, string);
	aprs_packet_decode_comment_data_extensions(packet, string);
}

void               aprs_packet_encode_data_weather(aprs_packet* packet, std::stringstream& ss)
{
	// TODO: implement
}
void               aprs_packet_encode_data_position(aprs_packet* packet, std::stringstream& ss)
{
	// TODO: implement
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
void               aprs_packet_encode_comment(aprs_packet* packet, std::stringstream& ss)
{
	aprs_packet_encode_data_weather(packet, ss);
	aprs_packet_encode_data_position(packet, ss);
	aprs_packet_encode_data_extensions(packet, ss);
}

bool               aprs_packet_decode_mic_e(aprs_packet* packet, std::string& tocall, std::string& content, bool gps_is_new)
{
	if ((tocall.length() < 6) || (content.length() < 9))
		return false;

	auto destination = tocall.c_str();
	auto information = content.c_str();

	auto     comment          = &information[9];
	int8_t   message          = 0;
	int8_t   lat_long[2]      = {};
	int8_t   latitude[6]      = {};
	uint32_t longitude[3]     = {};
	int8_t   longitude_offset = 0;

	for (size_t i = 0; i < 6; ++i)
	{
		auto c = destination[i];

		if ((c >= '0') && (c <= '9'))
			latitude[i] = c - '0';
		else if ((c >= 'A') && (c <= 'J'))
			latitude[i] = c - 'A';
		else if ((c >= 'P') && (c <= 'Y'))
			latitude[i] = c - 'P';
		else if ((c == 'K') || (c == 'L') || (c == 'Z'))
			lat_long[i] = 0;
		else
			return false;

		switch (i)
		{
			case 0:
			case 1:
			case 2:
				if ((c >= '0') && (c <= '9'))
					; // do nothing
				else if ((c >= 'A') && (c <= 'K'))
					message |= 0x80 | (1 << (2 - i));
				else if ((c >= 'P') && (c <= 'Z'))
					message |= 1 << (2 - i);
				else
					return false;
				break;

			case 3:
				if (((c >= '0') && (c <= '9')) || (c == 'L'))
					lat_long[0] = -1;
				else if ((c >= 'P') && (c <= 'Z'))
					lat_long[0] = 1;
				else
					return false;
				break;

			case 4:
				if (((c >= '0') && (c <= '9')) || (c == 'L'))
					longitude_offset = 0;
				else if ((c >= 'P') && (c <= 'Z'))
					longitude_offset = 100;
				else
					return false;
				break;

			case 5:
				if (((c >= '0') && (c <= '9')) || (c == 'L'))
					lat_long[1] = 1;
				else if ((c >= 'P') && (c <= 'Z'))
					lat_long[1] = -1;
				else
					return false;
				break;
		}
	}

	if (((longitude[0] = ((information[1] - 28) + longitude_offset)) >= 180) && (longitude[0] < 189))
		longitude[0] -= 80;
	else if ((longitude[0] >= 190) && (longitude[0] <= 199))
		longitude[0] -= 190;
	if ((longitude[1] = (information[2] - 28)) >= 60)
		longitude[1] -= 60;
	longitude[2] = information[3] - 28;

	packet->type     = APRS_PACKET_TYPE_POSITION;
	packet->position = new aprs_packet_position
	{
		.flags            = APRS_POSITION_FLAG_MIC_E,

		.latitude         = (((latitude[0] * 10) + latitude[1]) + (((latitude[2] * 10) + latitude[3]) / 60.0f) + (((latitude[4] * 10) + latitude[5]) / 6000.0f)) * lat_long[0],
		.longitude        = (longitude[0] + (longitude[1] / 60.0f) + (longitude[2] / 6000.0f)) * lat_long[1],

		.comment          = comment,

		.symbol_table     = information[8],
		.symbol_table_key = information[7],

		.mic_e_message    = (APRS_MIC_E_MESSAGES)(message & 0x7F)
	};

	if (!aprs_decode_base91(packet->extensions.altitude, packet->position->comment) || (comment[3] != '}'))
		packet->extensions.altitude = 0;
	else
	{
		comment                     += 4;
		packet->position->comment    = comment;
		packet->extensions.altitude -= 10000;
		packet->extensions.altitude *= 3.28084f;
	}

	// TODO: decode telemetry
	// TODO: decode maidenhead

	aprs_packet_decode_comment(packet, packet->position->comment);

	if (message & 0x80)
		packet->position->mic_e_message = (APRS_MIC_E_MESSAGES)(packet->position->mic_e_message + 7);

	packet->extensions.speed  = ((information[4] - 28) * 10) + ((information[5] - 28) / 10);
	packet->extensions.course = (((information[5] - 28) % 10) * 100) + (information[6] - 28);

	if (packet->extensions.speed >= 800)
		packet->extensions.speed -= 800;

	if (packet->extensions.course >= 400)
		packet->extensions.course -= 400;

	if (packet->extensions.course == 360)
		packet->extensions.course = 0;

	packet->extensions.speed *= 1.151f; // knots -> mph

	return true;
}
bool               aprs_packet_decode_mic_e(aprs_packet* packet)
{
	return aprs_packet_decode_mic_e(packet, packet->tocall, packet->content, true);
}
bool               aprs_packet_decode_mic_e_old(aprs_packet* packet)
{
	return aprs_packet_decode_mic_e(packet, packet->tocall, packet->content, false);
}
bool               aprs_packet_decode_raw_gps(aprs_packet* packet)
{
	static const aprs_regex_pattern regex("^((\\$[^,]+)(,[^,]*)+,(...))(.*)$");

	aprs_regex_match_result match;

	if (!aprs_regex_match(match, regex, packet->content))
		return false;

	packet->type = APRS_PACKET_TYPE_GPS;
	packet->gps  = new aprs_packet_gps { .nmea = match[1].str(), .comment = match[5].str() };

	return true;
}
bool               aprs_packet_decode_item(aprs_packet* packet)
{
	static const aprs_regex_pattern regex("^\\)([^!_]{3,9})([!_])([0-9 .]{7})([NS])(.)([0-9 .]{8})([EW])(.)(.*)$");
	static const aprs_regex_pattern regex_compressed("^\\)([^!_]{3,9})([!_])(.{13})(.*)$");

	aprs_regex_match_result match;

	if (aprs_regex_match(match, regex, packet->content))
	{
		auto&            name_match = match[1];
		std::string_view name(&*name_match.first, name_match.length());

		float            latitude;
		auto&            latitude_match = match[3];

		float            longitude;
		auto&            longitude_match = match[6];

		if (!aprs_decode_latitude(latitude, std::string_view(&*latitude_match.first, latitude_match.length()), *match[4].first))
			return false;

		if (!aprs_decode_longitude(longitude, std::string_view(&*longitude_match.first, longitude_match.length()), *match[7].first))
			return false;

		if (auto i = name.find_last_not_of(' '); i != std::string_view::npos)
			name = name.substr(0, i + 1);

		packet->type       = APRS_PACKET_TYPE_ITEM;
		packet->extensions = {};
		packet->item       = new aprs_packet_item
		{
			.is_alive         = *match[2].first == '!',
			.is_compressed    = false,
			.name             = std::string(name.data(), name.length()),
			.comment          = match[9].str(),
			.latitude         = latitude,
			.longitude        = longitude,
			.symbol_table     = *match[5].first,
			.symbol_table_key = *match[8].first
		};

		aprs_packet_decode_comment(packet, packet->item->comment);

		return true;
	}

	if (aprs_regex_match(match, regex_compressed, packet->content))
	{
		auto&                    name_match = match[1];
		std::string_view         name(&*name_match.first, name_match.length());

		aprs_compressed_location location;
		auto&                    location_match = match[3];

		if (auto i = name.find_last_not_of(' '); i != std::string_view::npos)
			name = name.substr(0, i + 1);

		if (!aprs_decode_compressed_location(location, std::string_view(&*location_match.first, location_match.length())))
			return false;

		packet->type       = APRS_PACKET_TYPE_ITEM;
		packet->extensions = {};
		packet->item       = new aprs_packet_item
		{
			.is_alive         = *match[2].first == '!',
			.is_compressed    = true,
			.name             = std::string(name.data(), name.length()),
			.comment          = match[4].str(),
			.latitude         = location.latitude,
			.longitude        = location.longitude,
			.symbol_table     = location.symbol_table,
			.symbol_table_key = location.symbol_table_key
		};

		aprs_packet_decode_comment(packet, packet->item->comment);

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
	static const aprs_regex_pattern regex("^;(.{9})([*_])(\\d{6})([z\\/h])([0-9 .]{7})([NS])(.)([0-9 .]{8})([EW])(.)(.*)$");
	static const aprs_regex_pattern regex_compressed("^;(.{9})([*_])(\\d{6})([z\\/h])(.{13})(.*)$");

	aprs_time               time;
	aprs_regex_match_result match;

	if (aprs_regex_match(match, regex, packet->content))
	{
		auto&            name_match = match[1];
		std::string_view name(&*name_match.first, name_match.length());

		auto             time_match = match[3];

		float            latitude;
		auto&            latitude_match = match[5];

		float            longitude;
		auto&            longitude_match = match[8];

		if (auto i = name.find_last_not_of(' '); i != std::string_view::npos)
			name = name.substr(0, i + 1);

		if (!aprs_decode_time(time, std::string_view(&*time_match.first, time_match.length()), *match[4].first))
			return false;

		if (!aprs_decode_latitude(latitude, std::string_view(&*latitude_match.first, latitude_match.length()), *match[6].first))
			return false;

		if (!aprs_decode_longitude(longitude, std::string_view(&*longitude_match.first, longitude_match.length()), *match[9].first))
			return false;

		packet->type       = APRS_PACKET_TYPE_OBJECT;
		packet->extensions = {};
		packet->object     = new aprs_packet_object
		{
			.is_alive         = *match[2].first == '*',
			.is_compressed    = false,
			.time             = time,
			.name             = std::string(name.data(), name.length()),
			.comment          = match[11].str(),
			.latitude         = latitude,
			.longitude        = longitude,
			.symbol_table     = *match[7].first,
			.symbol_table_key = *match[10].first
		};

		aprs_packet_decode_comment(packet, packet->object->comment);

		return true;
	}

	if (aprs_regex_match(match, regex_compressed, packet->content))
	{
		auto&                    name_match = match[1];
		std::string_view         name(&*name_match.first, name_match.length());

		auto&                    time_match = match[3];

		aprs_compressed_location location;
		auto&                    location_match = match[5];

		if (auto i = name.find_last_not_of(' '); i != std::string_view::npos)
			name = name.substr(0, i + 1);

		if (!aprs_decode_time(time, std::string_view(&*time_match.first, time_match.length()), *match[4].first))
			return false;

		if (!aprs_decode_compressed_location(location, std::string_view(&*location_match.first, location_match.length())))
			return false;

		packet->type       = APRS_PACKET_TYPE_OBJECT;
		packet->extensions = {};
		packet->object     = new aprs_packet_object
		{
			.is_alive         = *match[2].first == '*',
			.is_compressed    = true,
			.time             = time,
			.name             = std::string(name.data(), name.length()),
			.comment          = match[6].str(),
			.latitude         = location.latitude,
			.longitude        = location.longitude,
			.symbol_table     = location.symbol_table,
			.symbol_table_key = location.symbol_table_key
		};

		aprs_packet_decode_comment(packet, packet->object->comment);

		return true;
	}

	return false;
}
bool               aprs_packet_decode_status(aprs_packet* packet)
{
	static const aprs_regex_pattern regex("^>(.*)$");
	static const aprs_regex_pattern regex_time("^>(\\d{6})([z\\/h])(.*)$");

	aprs_time               time;
	aprs_regex_match_result match;

	if (aprs_regex_match(match, regex, packet->content))
	{
		packet->type   = APRS_PACKET_TYPE_STATUS;
		packet->status = new aprs_packet_status
		{
			.is_time_set = false,
			.message     = match[1].str()
		};

		return true;
	}

	if (aprs_regex_match(match, regex_time, packet->content))
	{
		auto& time_match = match[1];

		if (!aprs_decode_time(time, std::string_view(&*time_match.first, time_match.length()), *match[2].first))
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
bool               aprs_packet_decode_message_telemetry_params(aprs_packet* packet, std::string_view content)
{
	packet->type      = APRS_PACKET_TYPE_TELEMETRY;
	packet->telemetry = new aprs_packet_telemetry
	{
		.type = APRS_TELEMETRY_TYPE_PARAMS
	};

	if (auto param = strtok((char*)content.data(), ",")) do
	{
		packet->telemetry->params[packet->telemetry->params_count]   = param;
		packet->telemetry->params_c[packet->telemetry->params_count] = packet->telemetry->params[packet->telemetry->params_count].c_str();
	}
	while ((++packet->telemetry->params_count < 10) && (param = strtok(nullptr, ",")));

	return true;
}
bool               aprs_packet_decode_message_telemetry_units(aprs_packet* packet, std::string_view content)
{
	packet->type      = APRS_PACKET_TYPE_TELEMETRY;
	packet->telemetry = new aprs_packet_telemetry
	{
		.type = APRS_TELEMETRY_TYPE_UNITS
	};

	if (auto unit = strtok((char*)content.data(), ",")) do
	{
		packet->telemetry->units[packet->telemetry->units_count]   = unit;
		packet->telemetry->units_c[packet->telemetry->units_count] = packet->telemetry->units[packet->telemetry->units_count].c_str();
	}
	while ((++packet->telemetry->units_count < 10) && (unit = strtok(nullptr, ",")));

	return true;
}
bool               aprs_packet_decode_message_telemetry_eqns(aprs_packet* packet, std::string_view content)
{
	packet->type      = APRS_PACKET_TYPE_TELEMETRY;
	packet->telemetry = new aprs_packet_telemetry
	{
		.type = APRS_TELEMETRY_TYPE_EQNS
	};

	const char* chunks[15]   = {};
	size_t      chunks_count = 0;

	if (auto chunk = strtok((char*)content.data(), ",")) do
		chunks[chunks_count] = chunk;
	while ((++chunks_count < 15) && (chunk = strtok(nullptr, ",")));

	for (size_t i = 0; i < 5; ++i, ++packet->telemetry->eqns_count)
	{
		if (auto chunk = chunks[i * 3])
			packet->telemetry->eqns[i].a = strtof(chunk, nullptr);

		if (auto chunk = chunks[(i * 3) + 1])
			packet->telemetry->eqns[i].b = strtof(chunk, nullptr);

		if (auto chunk = chunks[(i * 3) + 2])
			packet->telemetry->eqns[i].c = strtof(chunk, nullptr);

		packet->telemetry->eqns_c[i] = &packet->telemetry->eqns[i];
	}

	return true;
}
bool               aprs_packet_decode_message_telemetry_bits(aprs_packet* packet, std::string_view content)
{
	packet->type      = APRS_PACKET_TYPE_TELEMETRY;
	packet->telemetry = new aprs_packet_telemetry
	{
		.type    = APRS_TELEMETRY_TYPE_BITS,
		.digital = 0
	};

	if (auto bits = strtok((char*)content.data(), ","))
	{
		for (size_t i = 0; i < 8; ++i)
			if (bits[i] != '0')
				packet->telemetry->digital |= 1 << i;

		if (auto comment = strtok(nullptr, ""))
			packet->telemetry->comment = comment;
	}

	return true;
}
bool               aprs_packet_decode_message_telemetry(aprs_packet* packet, aprs_regex_match_result& match)
{
	auto&            type_match = match[2];
	std::string_view type(&*type_match.first, type_match.length());

	auto&            data_match = match[3];
	std::string_view data(&*data_match.first, data_match.length());

	     if (!type.compare("PARM")) return aprs_packet_decode_message_telemetry_params(packet, data);
	else if (!type.compare("UNIT")) return aprs_packet_decode_message_telemetry_units(packet, data);
	else if (!type.compare("EQNS")) return aprs_packet_decode_message_telemetry_eqns(packet, data);
	else if (!type.compare("BITS")) return aprs_packet_decode_message_telemetry_bits(packet, data);

	return false;
}
bool               aprs_packet_decode_message(aprs_packet* packet)
{
	static const aprs_regex_pattern regex("^:([^ :]+) *:(.+?)(\\{(.+))?$");
	static const aprs_regex_pattern regex_ack("^ack\\S{1,5}$");
	static const aprs_regex_pattern regex_rej("^rej\\S{1,5}$");
	static const aprs_regex_pattern regex_bln("^BLN(\\S{1,6})$");
	static const aprs_regex_pattern regex_telemetry("^:([^:]+):(PARM|UNIT|EQNS|BITS).(.*)$");

	aprs_regex_match_result match;

	if (aprs_regex_match(match, regex_telemetry, packet->content))
		return aprs_packet_decode_message_telemetry(packet, match);
	else if (!aprs_regex_match(match, regex, packet->content))
		return false;

	std::string_view id;

	if (match.size() >= 4)
	{
		auto& match_id = match[4];

		if (match_id.length() > 5)
			return false;

		id = std::string_view(&*match_id.first, match_id.length());
	}

	auto&            content_match = match[2];
	std::string_view content(&*content_match.first, content_match.length());

	auto&            destination_match = match[1];
	std::string_view destination(&*destination_match.first, destination_match.length());

	if (!aprs_validate_name(destination))
		return false;

	if (!aprs_validate_comment(content, 67))
		return false;

	packet->type    = APRS_PACKET_TYPE_MESSAGE;
	packet->message = new aprs_packet_message
	{
		.id          = std::string(id.data(), id.length()),
		.type        = APRS_MESSAGE_TYPE_MESSAGE,
		.content     = std::string(content.data(), content.length()),
		.destination = std::string(destination.data(), destination.length())
	};

	if (aprs_regex_match(match, regex_ack, packet->message->content))
	{
		packet->message->type = APRS_MESSAGE_TYPE_ACK;
		packet->message->content.clear();
	}
	else if (aprs_regex_match(match, regex_rej, packet->message->content))
	{
		packet->message->type = APRS_MESSAGE_TYPE_REJECT;
		packet->message->content.clear();
	}
	else if (aprs_regex_match(match, regex_bln, packet->message->destination))
	{
		packet->message->type        = APRS_MESSAGE_TYPE_BULLETIN;
		packet->message->destination = match[1].str();
	}

	return true;
}
bool               aprs_packet_decode_weather(aprs_packet* packet)
{
	aprs_time time;

	if (!aprs_decode_time(time, std::string_view(&packet->content[1], packet->content.length() - 1), 0))
		return false;

	static auto decode_next_chunk = [](std::string_view& string, char& key, int& value)
	{
		if ((string.length() < 2) || !isalpha(string[0]) || (!isdigit(string[1]) && (string[1] != '.') && (string[1] != ' ')))
			return false;

		size_t i     = 1;
		       key   = string[0];
		       value = 0;

		if (string[i] == '-')
		{
			++i;
			value = 0x8000;
		}

		for (; (i < string.length()) && (isdigit(string[i]) || (string[i] == '.') || (string[i] == ' ')); ++i)
			switch (string[i])
			{
				case '.':
				case ' ':
					break;

				default:
					value = 10 * value + (string[i] - '0');
					break;
			}

		string = string.substr(i);

		return true;
	};

	packet->type    = APRS_PACKET_TYPE_WEATHER;
	packet->weather = new aprs_packet_weather { .time = time };

	char             key;
	int              value;
	std::string_view string(&packet->content[9], packet->content.length() - 9);

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

	if (!string.empty())
	{
		packet->weather->software = string[0];

		if (string.length() > 1)
			packet->weather->type = string[1];
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
	static const aprs_regex_pattern regex("^[!=]([0-9 .]{7})([NS])(.)([0-9 .]{8})([EW])(.)(.*)$");
	static const aprs_regex_pattern regex_time("^[\\/@](\\d{6})([z\\/h])([0-9 .]{7})([NS])(.)([0-9 .]{8})([EW])(.)(.*)$");
	static const aprs_regex_pattern regex_compressed("^[!=\\/@](.{13})(.*)$");

	aprs_regex_match_result match;

	if (aprs_regex_match(match, regex, packet->content))
	{
		float latitude;
		auto& latitude_match  = match[1];

		float longitude;
		auto& longitude_match = match[4];

		if (!aprs_decode_latitude(latitude, std::string_view(&*latitude_match.first, latitude_match.length()), *match[2].first))
			return false;

		if (!aprs_decode_longitude(longitude, std::string_view(&*longitude_match.first, longitude_match.length()), *match[5].first))
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

		aprs_packet_decode_comment(packet, packet->position->comment);

		return true;
	}

	if (aprs_regex_match(match, regex_time, packet->content))
	{
		aprs_time time;
		auto&     time_match      = match[1];

		float     latitude;
		auto&     latitude_match  = match[3];

		float     longitude;
		auto&     longitude_match = match[6];

		if (!aprs_decode_time(time, std::string_view(&*time_match.first, time_match.length()), *match[2].first))
			return false;

		if (!aprs_decode_latitude(latitude, std::string_view(&*latitude_match.first, latitude_match.length()), *match[4].first))
			return false;

		if (!aprs_decode_longitude(longitude, std::string_view(&*longitude_match.first, longitude_match.length()), *match[7].first))
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

		aprs_packet_decode_comment(packet, packet->position->comment);

		return true;
	}

	if (aprs_regex_match(match, regex_compressed, packet->content))
	{
		aprs_compressed_location location;
		auto&                    location_match = match[1];

		if (!aprs_decode_compressed_location(location, std::string_view(&*location_match.first, location_match.length())))
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

		aprs_packet_decode_comment(packet, packet->position->comment);

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
	static const aprs_regex_pattern regex("^T#(\\d+)(,(-?\\d*\\.?\\d*))(,(-?\\d*\\.?\\d*))(,(-?\\d*\\.?\\d*))(,(-?\\d*\\.?\\d*))(,(-?\\d*\\.?\\d*))(,(\\d{8}))(.*)$");

	aprs_regex_match_result match;

	if (!aprs_regex_match(match, regex, packet->content))
		return false;

	auto&            analog_1_match = match[3];
	std::string_view analog_1(&*analog_1_match.first, analog_1_match.length());
	auto&            analog_2_match = match[5];
	std::string_view analog_2(&*analog_2_match.first, analog_2_match.length());
	auto&            analog_3_match = match[7];
	std::string_view analog_3(&*analog_3_match.first, analog_3_match.length());
	auto&            analog_4_match = match[9];
	std::string_view analog_4(&*analog_4_match.first, analog_4_match.length());
	auto&            analog_5_match = match[11];
	std::string_view analog_5(&*analog_5_match.first, analog_5_match.length());
	auto&            digital_match  = match[13];
	auto&            sequence_match = match[1];

	packet->type      = APRS_PACKET_TYPE_TELEMETRY;
	packet->telemetry = new aprs_packet_telemetry
	{
		.comment = match[14].str()
	};

	std::from_chars(&*digital_match.first, &*digital_match.first + digital_match.length(), packet->telemetry->digital);
	std::from_chars(&*sequence_match.first, &*sequence_match.first + sequence_match.length(), packet->telemetry->sequence);

	if (!analog_1.starts_with('-') && !analog_2.starts_with('.') && !analog_3.starts_with('.') && !analog_4.starts_with('.') && !analog_5.starts_with('.') &&
		!aprs_string_contains(analog_1, '.') && !aprs_string_contains(analog_2, '.') && !aprs_string_contains(analog_3, '.') && !aprs_string_contains(analog_4, '.') && !aprs_string_contains(analog_5, '.'))
	{
		packet->telemetry->type           = APRS_TELEMETRY_TYPE_U8;
		packet->telemetry->analog_u8_c[0] = &packet->telemetry->analog_u8[0];
		packet->telemetry->analog_u8_c[1] = &packet->telemetry->analog_u8[1];
		packet->telemetry->analog_u8_c[2] = &packet->telemetry->analog_u8[2];
		packet->telemetry->analog_u8_c[3] = &packet->telemetry->analog_u8[3];
		packet->telemetry->analog_u8_c[4] = &packet->telemetry->analog_u8[4];
		packet->telemetry->analog_u8_c[5] = nullptr;

		std::from_chars(&*analog_1_match.first, &*analog_1_match.first + analog_1_match.length(), packet->telemetry->analog_u8[0]);
		std::from_chars(&*analog_2_match.first, &*analog_2_match.first + analog_2_match.length(), packet->telemetry->analog_u8[1]);
		std::from_chars(&*analog_3_match.first, &*analog_3_match.first + analog_3_match.length(), packet->telemetry->analog_u8[2]);
		std::from_chars(&*analog_4_match.first, &*analog_4_match.first + analog_4_match.length(), packet->telemetry->analog_u8[3]);
		std::from_chars(&*analog_5_match.first, &*analog_5_match.first + analog_5_match.length(), packet->telemetry->analog_u8[4]);
	}
	else
	{
		packet->telemetry->type              = APRS_TELEMETRY_TYPE_FLOAT;
		packet->telemetry->analog_float_c[0] = &packet->telemetry->analog_float[0];
		packet->telemetry->analog_float_c[1] = &packet->telemetry->analog_float[1];
		packet->telemetry->analog_float_c[2] = &packet->telemetry->analog_float[2];
		packet->telemetry->analog_float_c[3] = &packet->telemetry->analog_float[3];
		packet->telemetry->analog_float_c[4] = &packet->telemetry->analog_float[4];
		packet->telemetry->analog_float_c[5] = nullptr;

		std::from_chars(&*analog_1_match.first, &*analog_1_match.first + analog_1_match.length(), packet->telemetry->analog_float[0]);
		std::from_chars(&*analog_2_match.first, &*analog_2_match.first + analog_2_match.length(), packet->telemetry->analog_float[1]);
		std::from_chars(&*analog_3_match.first, &*analog_3_match.first + analog_3_match.length(), packet->telemetry->analog_float[2]);
		std::from_chars(&*analog_4_match.first, &*analog_4_match.first + analog_4_match.length(), packet->telemetry->analog_float[3]);
		std::from_chars(&*analog_5_match.first, &*analog_5_match.first + analog_5_match.length(), packet->telemetry->analog_float[4]);
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
void               aprs_packet_encode_test(aprs_packet* packet, std::stringstream& ss)
{
	// TODO: encode test
}
void               aprs_packet_encode_query(aprs_packet* packet, std::stringstream& ss)
{
	// TODO: encode query
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

		aprs_packet_encode_data_extensions(packet, ss);
	}

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
			ss << packet->telemetry->comment;
			break;

		case APRS_TELEMETRY_TYPE_FLOAT:
			ss << "T#" << std::setfill('0') << std::setw(3) << packet->telemetry->sequence << ',';
			for (auto analog : packet->telemetry->analog_float)
				ss << analog << ',';
			for (uint8_t i = 0; i < 8; ++i)
				ss << (((packet->telemetry->digital & (1 << i)) == (1 << i)) ? 1 : 0);
			ss << packet->telemetry->comment;
			break;

		case APRS_TELEMETRY_TYPE_BITS:
			ss << ':' << std::setfill(' ') << std::setw(9) << std::left << packet->sender << ":BITS.";
			for (uint8_t i = 0; i < 8; ++i)
				ss << (((packet->telemetry->digital & (1 << i)) == (1 << i)) ? 1 : 0);
			ss << packet->telemetry->comment;
			break;

		case APRS_TELEMETRY_TYPE_EQNS:
			ss << ':' << std::setfill(' ') << std::setw(9) << std::left << packet->sender << ":EQNS.";
			if (packet->telemetry->eqns_count)
			{
				auto eqn = packet->telemetry->eqns.data();

				ss << eqn->a << ',' << eqn->b << ',' << eqn->c;

				++eqn;

				for (size_t i = 1; i < packet->telemetry->eqns_count; ++i, ++eqn)
					ss << ',' << eqn->a << ',' << eqn->b << ',' << eqn->c;
			}
			break;

		case APRS_TELEMETRY_TYPE_UNITS:
			ss << ':' << std::setfill(' ') << std::setw(9) << std::left << packet->sender << ":UNIT.";
			if (packet->telemetry->units_count)
			{
				ss << packet->telemetry->units[0];

				for (size_t i = 1; i < packet->telemetry->units_count; ++i)
					ss << ',' << packet->telemetry->units[i];
			}
			break;

		case APRS_TELEMETRY_TYPE_PARAMS:
			ss << ':' << std::setfill(' ') << std::setw(9) << std::left << packet->sender << ":PARM.";
			if (packet->telemetry->params_count)
			{
				ss << packet->telemetry->params[0];

				for (size_t i = 1; i < packet->telemetry->params_count; ++i)
					ss << ',' << packet->telemetry->params[i];
			}
			break;
	}
}
void               aprs_packet_encode_map_feature(aprs_packet* packet, std::stringstream& ss)
{
	// TODO: encode map feature
}
void               aprs_packet_encode_grid_beacon(aprs_packet* packet, std::stringstream& ss)
{
	// TODO: encode grid beacon
}
void               aprs_packet_encode_third_party(aprs_packet* packet, std::stringstream& ss)
{
	ss << '}' << packet->third_party->content;
}
void               aprs_packet_encode_microfinder(aprs_packet* packet, std::stringstream& ss)
{
	// TODO: encode microfinder
}
void               aprs_packet_encode_user_defined(aprs_packet* packet, std::stringstream& ss)
{
	ss << '{' << packet->user_defined->id << packet->user_defined->type << packet->user_defined->data;
}
void               aprs_packet_encode_shelter_time(aprs_packet* packet, std::stringstream& ss)
{
	// TODO: encode shelter time
}
void               aprs_packet_encode_station_capabilities(aprs_packet* packet, std::stringstream& ss)
{
	// TODO: encode station capabilities
}
void               aprs_packet_encode_maidenhead_grid_beacon(aprs_packet* packet, std::stringstream& ss)
{
	// TODO: encode maidenhead grid beacon
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
	{ APRS_PACKET_TYPE_TEST,                   &aprs_packet_encode_test                   },
	{ APRS_PACKET_TYPE_QUERY,                  &aprs_packet_encode_query                  },
	{ APRS_PACKET_TYPE_OBJECT,                 &aprs_packet_encode_object                 },
	{ APRS_PACKET_TYPE_STATUS,                 &aprs_packet_encode_status                 },
	{ APRS_PACKET_TYPE_MESSAGE,                &aprs_packet_encode_message                },
	{ APRS_PACKET_TYPE_WEATHER,                &aprs_packet_encode_weather                },
	{ APRS_PACKET_TYPE_POSITION,               &aprs_packet_encode_position               },
	{ APRS_PACKET_TYPE_TELEMETRY,              &aprs_packet_encode_telemetry              },
	{ APRS_PACKET_TYPE_MAP_FEATURE,            &aprs_packet_encode_map_feature            },
	{ APRS_PACKET_TYPE_GRID_BEACON,            &aprs_packet_encode_grid_beacon            },
	{ APRS_PACKET_TYPE_THIRD_PARTY,            &aprs_packet_encode_third_party            },
	{ APRS_PACKET_TYPE_MICROFINDER,            &aprs_packet_encode_microfinder            },
	{ APRS_PACKET_TYPE_USER_DEFINED,           &aprs_packet_encode_user_defined           },
	{ APRS_PACKET_TYPE_SHELTER_TIME,           &aprs_packet_encode_shelter_time           },
	{ APRS_PACKET_TYPE_STATION_CAPABILITIES,   &aprs_packet_encode_station_capabilities   },
	{ APRS_PACKET_TYPE_MAIDENHEAD_GRID_BEACON, &aprs_packet_encode_maidenhead_grid_beacon }
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
struct aprs_path*                 APRSERVICE_CALL aprs_path_init_from_copy(struct aprs_path* path)
{
	auto p = new aprs_path
	{
		.size            = path->size,
		.chunks_stations = path->chunks_stations,

		.string          = path->string,

		.reference_count = 1
	};

	for (size_t i = 0; i < path->size; ++i)
		p->chunks[i] = { .station = p->chunks_stations[i].c_str(), .repeated = path->chunks[i].repeated };

	return p;
}
struct aprs_path*                                 aprs_path_init_from_string(std::string_view string)
{
	auto path = new aprs_path
	{
		.size            = 0,

		.reference_count = 1
	};

	if (auto chunk = strtok((char*)string.data(), ",")) do
	{
		if (path->size == path->chunks.max_size())
		{
			delete path;

			return nullptr;
		}

		if (!aprs_extract_path_node(path->chunks_stations[path->size], path->chunks[path->size].repeated, chunk))
		{
			delete path;

			return nullptr;
		}

		path->chunks[path->size].station = path->chunks_stations[path->size].c_str();

		++path->size;
	} while (chunk = strtok(nullptr, ","));

	return path;
}
struct aprs_path*                 APRSERVICE_CALL aprs_path_init_from_string(const char* string)
{
	if (!string)
		return nullptr;

	// TODO: remove copy
	std::string buffer(string);

	return aprs_path_init_from_string(buffer);
}
void                              APRSERVICE_CALL aprs_path_deinit(struct aprs_path* path)
{
	if (!--path->reference_count)
		delete path;
}
const struct aprs_path_node*      APRSERVICE_CALL aprs_path_get(struct aprs_path* path)
{
	return path->chunks.data();
}
uint8_t                           APRSERVICE_CALL aprs_path_get_length(struct aprs_path* path)
{
	return path->size;
}
uint8_t                           APRSERVICE_CALL aprs_path_get_capacity(struct aprs_path* path)
{
	return path->chunks.max_size();
}
bool                              APRSERVICE_CALL aprs_path_set(struct aprs_path* path, uint8_t index, const char* station, bool repeated)
{
	if (!station)
		return false;

	if (index >= path->size)
		return false;

	if (!aprs_validate_station(station))
		return false;

	path->chunks_stations[index].assign(station);
	path->chunks[index].station  = path->chunks_stations[index].c_str();
	path->chunks[index].repeated = repeated;

	return true;
}
bool                              APRSERVICE_CALL aprs_path_pop(struct aprs_path* path)
{
	if (path->size == 0)
		return false;

	path->chunks_stations[path->size].clear();
	path->chunks[path->size].station  = nullptr;
	path->chunks[path->size].repeated = false;

	--path->size;

	return true;
}
bool                              APRSERVICE_CALL aprs_path_push(struct aprs_path* path, const char* station, bool repeated)
{
	if (!station)
		return false;

	if (!aprs_validate_station(station))
		return false;

	if (path->size == path->chunks.max_size())
		return false;

	path->chunks_stations[path->size].assign(station);
	path->chunks[path->size].station  = path->chunks_stations[path->size].c_str();
	path->chunks[path->size].repeated = repeated;

	++path->size;

	return true;
}
void                              APRSERVICE_CALL aprs_path_clear(struct aprs_path* path)
{
	for (size_t i = 0; i < path->size; ++i)
	{
		path->chunks_stations[i].clear();
		path->chunks[i].repeated = false;
	}

	path->size = 0;
}
bool                              APRSERVICE_CALL aprs_path_compare(struct aprs_path* path, struct aprs_path* path2)
{
	if (!path2)
		return false;

	if (path->size != path2->size)
		return false;

	for (size_t i = 0; i < path->size; ++i)
	{
		if (!path->chunks[i].repeated != path2->chunks[i].repeated)
			return false;

		if (!path->chunks_stations[i].compare(path2->chunks_stations[i]))
			return false;
	}

	return true;
}
const char*                       APRSERVICE_CALL aprs_path_to_string(struct aprs_path* path)
{
	std::stringstream ss;

	if (path->size)
	{
		ss << path->chunks[0].station;

		if (path->chunks[0].repeated)
			ss << '*';

		for (size_t i = 1; i < path->size; ++i)
		{
			ss << ',' << path->chunks[i].station;

			if (path->chunks[i].repeated)
				ss << '*';
		}
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
bool                              APRSERVICE_CALL aprs_time_compare(const struct aprs_time* time, const struct aprs_time* time2)
{
	if (!time2)
		return false;

	if (time->type != time2->type)
		return false;

	if (time->type & APRS_TIME_DHM)
	{
		if (time->tm_mday != time2->tm_mday) return false;
		if (time->tm_min  != time2->tm_min)  return false;
		if (time->tm_sec  != time2->tm_sec)  return false;
	}

	if (time->type & APRS_TIME_HMS)
	{
		if (time->tm_hour != time2->tm_hour) return false;
		if (time->tm_min  != time2->tm_min)  return false;
		if (time->tm_sec  != time2->tm_sec)  return false;
	}

	if (time->type & APRS_TIME_MDHM)
	{
		if (time->tm_mon  != time2->tm_mon)  return false;
		if (time->tm_mday != time2->tm_mday) return false;
		if (time->tm_hour != time2->tm_hour) return false;
		if (time->tm_min  != time2->tm_min)  return false;
	}

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
	if (!sender || !tocall || !path)
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
	if (!sender || !tocall || !path)
		return nullptr;

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
struct aprs_packet*               APRSERVICE_CALL aprs_packet_init_from_copy(struct aprs_packet* packet)
{
	auto p = new aprs_packet
	{
		.type            = packet->type,
		.path            = aprs_path_init_from_copy(packet->path),
		.igate           = packet->igate,
		.tocall          = packet->tocall,
		.sender          = packet->sender,
		.content         = packet->content,
		.qconstruct      = packet->qconstruct,
		.extensions      = packet->extensions,

		.string          = packet->string,

		.reference_count = 1
	};

	switch (packet->type)
	{
		case APRS_PACKET_TYPE_GPS:
			p->gps = new aprs_packet_gps
			{
				.nmea    = packet->gps->nmea,
				.comment = packet->gps->comment
			};
			break;

		case APRS_PACKET_TYPE_RAW:
			break;

		case APRS_PACKET_TYPE_ITEM:
			p->item = new aprs_packet_item
			{
				.is_alive         = packet->item->is_alive,
				.is_compressed    = packet->item->is_compressed,

				.time             = packet->item->time,

				.name             = packet->item->name,
				.comment          = packet->item->comment,

				.latitude         = packet->item->latitude,
				.longitude        = packet->item->longitude,

				.symbol_table     = packet->item->symbol_table,
				.symbol_table_key = packet->item->symbol_table_key
			};
			break;

		case APRS_PACKET_TYPE_TEST:
			// TODO: copy test
			break;

		case APRS_PACKET_TYPE_QUERY:
			// TODO: copy query
			break;

		case APRS_PACKET_TYPE_OBJECT:
			p->object = new aprs_packet_object
			{
				.is_alive         = packet->object->is_alive,
				.is_compressed    = packet->object->is_compressed,

				.time             = packet->object->time,

				.name             = packet->object->name,
				.comment          = packet->object->comment,

				.latitude         = packet->object->latitude,
				.longitude        = packet->object->longitude,

				.symbol_table     = packet->object->symbol_table,
				.symbol_table_key = packet->object->symbol_table_key
			};
			break;

		case APRS_PACKET_TYPE_STATUS:
			p->status = new aprs_packet_status
			{
				.is_time_set = packet->status->is_time_set,

				.time        = packet->status->time,
				.message     = packet->status->message
			};
			break;

		case APRS_PACKET_TYPE_MESSAGE:
			p->message = new aprs_packet_message
			{
				.id          = packet->message->id,
				.type        = packet->message->type,
				.content     = packet->message->content,
				.destination = packet->message->destination
			};
			break;

		case APRS_PACKET_TYPE_WEATHER:
			p->weather = new aprs_packet_weather
			{
				.time                    = packet->weather->time,

				.wind_speed              = packet->weather->wind_speed,
				.wind_speed_gust         = packet->weather->wind_speed_gust,
				.wind_direction          = packet->weather->wind_direction,

				.rainfall_last_hour      = packet->weather->rainfall_last_hour,
				.rainfall_last_24_hours  = packet->weather->rainfall_last_24_hours,
				.rainfall_since_midnight = packet->weather->rainfall_since_midnight,

				.humidity                = packet->weather->humidity,
				.temperature             = packet->weather->temperature,
				.barometric_pressure     = packet->weather->barometric_pressure,

				.type                    = packet->weather->type,
				.software                = packet->weather->software
			};
			break;

		case APRS_PACKET_TYPE_POSITION:
			p->position = new aprs_packet_position
			{
				.flags                    = packet->position->flags,

				.time                     = packet->position->time,

				.latitude                 = packet->position->latitude,
				.longitude                = packet->position->longitude,

				.comment                  = packet->position->comment,

				.symbol_table             = packet->position->symbol_table,
				.symbol_table_key         = packet->position->symbol_table_key,

				.mic_e_message            = packet->position->mic_e_message,
				.mic_e_telemetry          = packet->position->mic_e_telemetry,
				.mic_e_telemetry_channels = packet->position->mic_e_telemetry_channels
			};
		break;

		case APRS_PACKET_TYPE_TELEMETRY:
			p->telemetry = new aprs_packet_telemetry
			{
				.type           = packet->telemetry->type,

				.eqns           = packet->telemetry->eqns,
				.eqns_count     = packet->telemetry->eqns_count,

				.units          = packet->telemetry->units,
				.units_count    = packet->telemetry->units_count,

				.params         = packet->telemetry->params,
				.params_count   = packet->telemetry->params_count,

				.analog_u8      = packet->telemetry->analog_u8,
				.analog_float   = packet->telemetry->analog_float,
				.digital        = packet->telemetry->digital,
				.sequence       = packet->telemetry->sequence,
				.comment        = packet->telemetry->comment
			};

			for (size_t i = 0; i < p->telemetry->eqns_count; ++i)
				p->telemetry->eqns_c[i] = &p->telemetry->eqns[i];
			for (size_t i = 0; i < p->telemetry->units_count; ++i)
				p->telemetry->units_c[i] = p->telemetry->units[i].c_str();
			for (size_t i = 0; i < p->telemetry->params_count; ++i)
				p->telemetry->params_c[i] = p->telemetry->params[i].c_str();
			for (size_t i = 0; i < p->telemetry->analog_u8.size(); ++i)
				p->telemetry->analog_u8_c[i] = &p->telemetry->analog_u8[i];
			for (size_t i = 0; i < p->telemetry->analog_float.size(); ++i)
				p->telemetry->analog_float_c[i] = &p->telemetry->analog_float[i];
			break;

		case APRS_PACKET_TYPE_MAP_FEATURE:
			// TODO: copy map feature
			break;

		case APRS_PACKET_TYPE_GRID_BEACON:
			// TODO: copy grid beacon
			break;

		case APRS_PACKET_TYPE_THIRD_PARTY:
			p->third_party = new aprs_packet_third_party
			{
				.content = packet->third_party->content
			};
			break;

		case APRS_PACKET_TYPE_MICROFINDER:
			// TODO: copy microfinder
			break;

		case APRS_PACKET_TYPE_USER_DEFINED:
			p->user_defined = new aprs_packet_user_defined
			{
				.id   = packet->user_defined->id,
				.type = packet->user_defined->type,
				.data = packet->user_defined->data
			};
			break;

		case APRS_PACKET_TYPE_SHELTER_TIME:
			// TODO: copy shelter time
			break;

		case APRS_PACKET_TYPE_STATION_CAPABILITIES:
			// TODO: copy station capabilities
			break;

		case APRS_PACKET_TYPE_MAIDENHEAD_GRID_BEACON:
			// TODO: copy maidenhead grid beacon
			break;
	}

	return p;
}
struct aprs_packet*               APRSERVICE_CALL aprs_packet_init_from_string(const char* string)
{
	if (!string)
		return nullptr;

	static const aprs_regex_pattern regex("^([^>]{3,9})>([^,]+),([^:]+):(.*)$");
	static const aprs_regex_pattern regex_path_is("(.*?),?(qA\\w),(\\S+)$");

	aprs_regex_match_result match;

	if (!aprs_regex_match(match, regex, string))
		return nullptr;

	aprs_path*              path;
	aprs_regex_match_result path_match;
	auto&                   path_match3 = match[3];
	std::string_view        path_string(&*path_match3.first, path_match3.length());
	std::string             path_q_igate[2] = { "", "" };

	if (aprs_regex_match(path_match, regex_path_is, path_string))
	{
		path_q_igate[0] = path_match[2].str();
		path_q_igate[1] = path_match[3].str();
		path_string     = path_match[1].str();
	}

	if (!(path = aprs_path_init_from_string(path_string)))
		return nullptr;

	auto packet = new aprs_packet
	{
		.path            = path,
		.igate           = std::move(path_q_igate[1]),
		.tocall          = match[2].str(),
		.sender          = match[1].str(),
		.content         = match[4].str(),
		.qconstruct      = std::move(path_q_igate[0]),
		.reference_count = 1
	};

	for (auto& c : packet->sender)
	{
		if (c == '-')
			break;

		if ((c >= 'a') && (c <= 'z'))
			c = 'A' + (c - 'a');
	}

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

			case APRS_PACKET_TYPE_TEST:
				// TODO: deinit test
				break;

			case APRS_PACKET_TYPE_QUERY:
				// TODO: deinit query
				break;

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

			case APRS_PACKET_TYPE_MAP_FEATURE:
				// TODO: deinit map feature
				break;

			case APRS_PACKET_TYPE_GRID_BEACON:
				// TODO: deinit grid beacon
				break;

			case APRS_PACKET_TYPE_THIRD_PARTY:
				delete packet->third_party;
				break;

			case APRS_PACKET_TYPE_MICROFINDER:
				// TODO: deinit microfinder
				break;

			case APRS_PACKET_TYPE_USER_DEFINED:
				delete packet->user_defined;
				break;

			case APRS_PACKET_TYPE_SHELTER_TIME:
				// TODO: deinit shelter time
				break;

			case APRS_PACKET_TYPE_STATION_CAPABILITIES:
				// TODO: deinit station capabilities
				break;

			case APRS_PACKET_TYPE_MAIDENHEAD_GRID_BEACON:
				// TODO: deinit maidenhead grid beacon
				break;
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
	if (!value)
		return false;

	if (!aprs_validate_name(value))
		return false;

	packet->tocall.assign(value);

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_set_sender(struct aprs_packet* packet, const char* value)
{
	if (!value)
		return false;

	if (!aprs_validate_station(value))
		return false;

	packet->sender.assign(value);

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_set_content(struct aprs_packet* packet, const char* value)
{
	if (!value)
		return false;

	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_RAW)
		return false;

	if (auto length = aprs_string_length(value); length && (length <= 256))
	{
		packet->content.assign(value, length);

		return true;
	}

	return false;
}
bool                              APRSERVICE_CALL aprs_packet_compare(struct aprs_packet* packet, struct aprs_packet* packet2)
{
	if (!packet2)
		return false;

	if (packet->type != packet2->type)
		return false;

	if (packet->type                       != packet2->type)                       return false;
	if (!aprs_path_compare(packet->path, packet2->path))                           return false;
	if (packet->igate                      != packet2->igate)                      return false;
	if (packet->tocall                     != packet2->tocall)                     return false;
	if (packet->sender                     != packet2->sender)                     return false;
	if (packet->content                    != packet2->content)                    return false;
	if (packet->qconstruct                 != packet2->qconstruct)                 return false;
	if (packet->extensions.speed           != packet2->extensions.speed)           return false;
	if (packet->extensions.course          != packet2->extensions.course)          return false;
	if (packet->extensions.altitude        != packet2->extensions.altitude)        return false;
	if (packet->extensions.dfs.strength    != packet2->extensions.dfs.strength)    return false;
	if (packet->extensions.dfs.height      != packet2->extensions.dfs.height)      return false;
	if (packet->extensions.dfs.gain        != packet2->extensions.dfs.gain)        return false;
	if (packet->extensions.dfs.directivity != packet2->extensions.dfs.directivity) return false;
	if (packet->extensions.phg.power       != packet2->extensions.phg.power)       return false;
	if (packet->extensions.phg.height      != packet2->extensions.phg.height)      return false;
	if (packet->extensions.phg.gain        != packet2->extensions.phg.gain)        return false;
	if (packet->extensions.phg.directivity != packet2->extensions.phg.directivity) return false;
	if (packet->extensions.rng.miles       != packet2->extensions.rng.miles)       return false;

	switch (packet->type)
	{
		case APRS_PACKET_TYPE_GPS:
			if (packet->gps->nmea    != packet2->gps->nmea)    return false;
			if (packet->gps->comment != packet2->gps->comment) return false;
			break;

		case APRS_PACKET_TYPE_RAW:
			break;

		case APRS_PACKET_TYPE_ITEM:
			if (packet->item->is_alive         != packet2->item->is_alive)         return false;
			if (packet->item->is_compressed    != packet2->item->is_compressed)    return false;
			if (!aprs_time_compare(&packet->item->time, &packet2->item->time))     return false;
			if (packet->item->name             != packet2->item->name)             return false;
			if (packet->item->comment          != packet2->item->comment)          return false;
			if (packet->item->latitude         != packet2->item->latitude)         return false;
			if (packet->item->longitude        != packet2->item->longitude)        return false;
			if (packet->item->symbol_table     != packet2->item->symbol_table)     return false;
			if (packet->item->symbol_table_key != packet2->item->symbol_table_key) return false;
			break;

		case APRS_PACKET_TYPE_TEST:
			// TODO: compare test
			break;

		case APRS_PACKET_TYPE_QUERY:
			// TODO: compare query
			break;

		case APRS_PACKET_TYPE_OBJECT:
			if (packet->object->is_alive         != packet2->object->is_alive)         return false;
			if (packet->object->is_compressed    != packet2->object->is_compressed)    return false;
			if (!aprs_time_compare(&packet->object->time, &packet2->object->time))     return false;
			if (packet->object->name             != packet2->object->name)             return false;
			if (packet->object->comment          != packet2->object->comment)          return false;
			if (packet->object->latitude         != packet2->object->latitude)         return false;
			if (packet->object->longitude        != packet2->object->longitude)        return false;
			if (packet->object->symbol_table     != packet2->object->symbol_table)     return false;
			if (packet->object->symbol_table_key != packet2->object->symbol_table_key) return false;
			break;

		case APRS_PACKET_TYPE_STATUS:
			if (packet->status->is_time_set != packet2->status->is_time_set)       return false;
			if (!aprs_time_compare(&packet->status->time, &packet2->status->time)) return false;
			if (packet->status->message != packet2->status->message)               return false;
			break;

		case APRS_PACKET_TYPE_MESSAGE:
			if (packet->message->id          != packet2->message->id)          return false;
			if (packet->message->type        != packet2->message->type)        return false;
			if (packet->message->content     != packet2->message->content)     return false;
			if (packet->message->destination != packet2->message->destination) return false;
			break;

		case APRS_PACKET_TYPE_WEATHER:
			if (!aprs_time_compare(&packet->weather->time, &packet2->weather->time))                   return false;
			if (packet->weather->wind_speed              != packet2->weather->wind_speed)              return false;
			if (packet->weather->wind_speed_gust         != packet2->weather->wind_speed_gust)         return false;
			if (packet->weather->wind_direction          != packet2->weather->wind_direction)          return false;
			if (packet->weather->rainfall_last_hour      != packet2->weather->rainfall_last_hour)      return false;
			if (packet->weather->rainfall_last_24_hours  != packet2->weather->rainfall_last_24_hours)  return false;
			if (packet->weather->rainfall_since_midnight != packet2->weather->rainfall_since_midnight) return false;
			if (packet->weather->humidity                != packet2->weather->humidity)                return false;
			if (packet->weather->temperature             != packet2->weather->temperature)             return false;
			if (packet->weather->barometric_pressure     != packet2->weather->barometric_pressure)     return false;
			if (packet->weather->type                    != packet2->weather->type)                    return false;
			if (packet->weather->software                != packet2->weather->software)                return false;
			break;

		case APRS_PACKET_TYPE_POSITION:
			if (packet->position->flags                    != packet2->position->flags)                    return false;
			if (!aprs_time_compare(&packet->position->time, &packet2->position->time))                     return false;
			if (packet->position->latitude                 != packet2->position->latitude)                 return false;
			if (packet->position->longitude                != packet2->position->longitude)                return false;
			if (packet->position->comment                  != packet2->position->comment)                  return false;
			if (packet->position->symbol_table             != packet2->position->symbol_table)             return false;
			if (packet->position->symbol_table_key         != packet2->position->symbol_table_key)         return false;
			if (packet->position->mic_e_message            != packet2->position->mic_e_message)            return false;
			if (packet->position->mic_e_telemetry          != packet2->position->mic_e_telemetry)          return false;
			if (packet->position->mic_e_telemetry_channels != packet2->position->mic_e_telemetry_channels) return false;
		break;

		case APRS_PACKET_TYPE_TELEMETRY:
			if (packet->telemetry->type         != packet2->telemetry->type)         return false;
			if (packet->telemetry->eqns_count   != packet2->telemetry->eqns_count)   return false;
			if (packet->telemetry->units        != packet2->telemetry->units)        return false;
			if (packet->telemetry->units_count  != packet2->telemetry->units_count)  return false;
			if (packet->telemetry->params       != packet2->telemetry->params)       return false;
			if (packet->telemetry->params_count != packet2->telemetry->params_count) return false;
			if (packet->telemetry->analog_u8    != packet2->telemetry->analog_u8)    return false;
			if (packet->telemetry->analog_float != packet2->telemetry->analog_float) return false;
			if (packet->telemetry->digital      != packet2->telemetry->digital)      return false;
			if (packet->telemetry->sequence     != packet2->telemetry->sequence)     return false;
			if (packet->telemetry->comment      != packet2->telemetry->comment)      return false;

			for (size_t i = 0; i < packet->telemetry->eqns_count; ++i)
			{
				if (packet->telemetry->eqns[i].a != packet2->telemetry->eqns[i].a) return false;
				if (packet->telemetry->eqns[i].b != packet2->telemetry->eqns[i].b) return false;
				if (packet->telemetry->eqns[i].c != packet2->telemetry->eqns[i].c) return false;
			}
			break;

		case APRS_PACKET_TYPE_MAP_FEATURE:
			// TODO: compare map feature
			break;

		case APRS_PACKET_TYPE_GRID_BEACON:
			// TODO: compare grid beacon
			break;

		case APRS_PACKET_TYPE_THIRD_PARTY:
			if (packet->third_party->content != packet2->third_party->content) return false;
			break;

		case APRS_PACKET_TYPE_MICROFINDER:
			// TODO: compare microfinder
			break;

		case APRS_PACKET_TYPE_USER_DEFINED:
			if (packet->user_defined->id   != packet2->user_defined->id)   return false;
			if (packet->user_defined->type != packet2->user_defined->type) return false;
			if (packet->user_defined->data != packet2->user_defined->data) return false;
			break;

		case APRS_PACKET_TYPE_SHELTER_TIME:
			// TODO: compare shelter time
			break;

		case APRS_PACKET_TYPE_STATION_CAPABILITIES:
			// TODO: compare station capabilities
			break;

		case APRS_PACKET_TYPE_MAIDENHEAD_GRID_BEACON:
			// TODO: compare maidenhead grid beacon
			break;
	}

	return true;
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
	if (!nmea)
		return nullptr;

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
	if (value && (*value != '$'))
		return false;

	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_GPS)
		return false;

	if (!value)
		packet->gps->nmea.clear();
	else
		packet->gps->nmea = value;

	return true;
}
bool                              APRSERVICE_CALL aprs_packet_gps_set_comment(struct aprs_packet* packet, const char* value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_GPS)
		return false;

	if (!value)
		packet->gps->comment.clear();
	else
		packet->gps->comment = value;

	return true;
}

struct aprs_packet*               APRSERVICE_CALL aprs_packet_item_init(const char* sender, const char* tocall, struct aprs_path* path, const char* name, char symbol_table, char symbol_table_key)
{
	if (!name)
		return nullptr;

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
	if (!value)
		return false;

	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_ITEM)
		return false;

	if (!aprs_validate_name(value))
		return false;

	packet->item->name.assign(value);

	return true;
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
	else if (aprs_validate_comment(value, 36))
	{
		packet->item->comment.assign(value);

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
	if (!name)
		return nullptr;

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
	if (!value)
		return false;

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
	if (!value)
		return false;

	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_OBJECT)
		return false;

	if (!aprs_validate_name(value))
		return false;

	packet->object->name.assign(value);

	return true;
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
	else if (aprs_validate_comment(value, 36))
	{
		packet->object->comment.assign(value);

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

		if (message && !aprs_packet_status_set_message(packet, message))
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

	if (!value)
		packet->status->message.clear();
	else
		packet->status->message = value;

	return true;
}

struct aprs_packet*               APRSERVICE_CALL aprs_packet_message_init(const char* sender, const char* tocall, struct aprs_path* path, const char* destination, const char* content)
{
	if (!destination)
		return nullptr;

	if (auto packet = aprs_packet_init_ex(sender, tocall, path, APRS_PACKET_TYPE_MESSAGE))
	{
		packet->message = new aprs_packet_message {};

		if (!aprs_packet_message_set_type(packet, APRS_MESSAGE_TYPE_MESSAGE) ||
			(content && !aprs_packet_message_set_content(packet, content)) ||
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
	if (!id || !destination)
		return nullptr;

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
	if (!id || !destination)
		return nullptr;

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
	if (!destination)
		return nullptr;

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

		if (!aprs_validate_string(value, is_string_valid))
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
	if (!value)
		return false;

	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_MESSAGE)
		return false;

	if (!aprs_validate_name(value))
		return false;

	packet->message->destination.assign(value);

	return true;
}

struct aprs_packet*               APRSERVICE_CALL aprs_packet_weather_init(const char* sender, const char* tocall, struct aprs_path* path, const char* type, char software)
{
	if (!type)
		return nullptr;

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

aprs_packet*                                      aprs_packet_position_init(const char* sender, const char* tocall, struct aprs_path* path, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course, const char* comment, char symbol_table, char symbol_table_key, int flags, int mic_e_message)
{
	if (flags & APRS_POSITION_FLAG_MIC_E)
	{
		if (flags & APRS_POSITION_FLAG_TIME)
			return nullptr;

		if (flags & APRS_POSITION_FLAG_COMPRESSED)
			return nullptr;

		if ((mic_e_message < 0) || (mic_e_message > 7))
			return nullptr;
	}

	if (auto packet = aprs_packet_init_ex(sender, tocall, path, APRS_PACKET_TYPE_POSITION))
	{
		packet->position = new aprs_packet_position
		{
			.flags         = flags,
			.time          = *aprs_time_now(),
			.mic_e_message = (APRS_MIC_E_MESSAGES)mic_e_message
		};

		if (!aprs_packet_position_set_speed(packet, speed) ||
			!aprs_packet_position_set_course(packet, course) ||
			(comment && !aprs_packet_position_set_comment(packet, comment)) ||
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
	return aprs_packet_position_init(sender, tocall, path, latitude, longitude, altitude, speed, course, comment, symbol_table, symbol_table_key, 0, -1);
}
struct aprs_packet*               APRSERVICE_CALL aprs_packet_position_init_mic_e(const char* sender, const char* tocall, struct aprs_path* path, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course, const char* comment, char symbol_table, char symbol_table_key, enum APRS_MIC_E_MESSAGES message)
{
	return aprs_packet_position_init(sender, tocall, path, latitude, longitude, altitude, speed, course, comment, symbol_table, symbol_table_key, APRS_POSITION_FLAG_MIC_E, message);
}
struct aprs_packet*               APRSERVICE_CALL aprs_packet_position_init_compressed(const char* sender, const char* tocall, struct aprs_path* path, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course, const char* comment, char symbol_table, char symbol_table_key)
{
	return aprs_packet_position_init(sender, tocall, path, latitude, longitude, altitude, speed, course, comment, symbol_table, symbol_table_key, APRS_POSITION_FLAG_COMPRESSED, -1);
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
int                               APRSERVICE_CALL aprs_packet_position_get_mic_e_message(struct aprs_packet* packet)
{
	if (!aprs_packet_position_is_mic_e(packet))
		return -1;

	return packet->position->mic_e_message;
}
bool                              APRSERVICE_CALL aprs_packet_position_set_time(struct aprs_packet* packet, const struct aprs_time* value)
{
	if (!value)
		return false;

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
	else if (aprs_validate_comment(value, 36))
	{
		packet->position->comment.assign(value);

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
bool                              APRSERVICE_CALL aprs_packet_position_set_mic_e_message(struct aprs_packet* packet, enum APRS_MIC_E_MESSAGES value)
{
	if (value >= APRS_MIC_E_MESSAGES_COUNT)
		return false;

	if (!aprs_packet_position_is_mic_e(packet))
		return false;

	packet->position->mic_e_message = value;

	return true;
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

		for (size_t i = 0; i < 5; ++i)
			packet->telemetry->analog_u8_c[i] = &packet->telemetry->analog_u8[i];
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

		for (size_t i = 0; i < 5; ++i)
			packet->telemetry->analog_float_c[i] = &packet->telemetry->analog_float[i];
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
struct aprs_packet*               APRSERVICE_CALL aprs_packet_telemetry_init_bits(const char* sender, const char* tocall, struct aprs_path* path, uint8_t value)
{
	if (auto packet = aprs_packet_init_ex(sender, tocall, path, APRS_PACKET_TYPE_TELEMETRY))
	{
		packet->telemetry = new aprs_packet_telemetry
		{
			.type    = APRS_TELEMETRY_TYPE_BITS,
			.digital = value
		};

		return packet;
	}

	return nullptr;
}
struct aprs_packet*               APRSERVICE_CALL aprs_packet_telemetry_init_eqns(const char* sender, const char* tocall, struct aprs_path* path)
{
	if (auto packet = aprs_packet_init_ex(sender, tocall, path, APRS_PACKET_TYPE_TELEMETRY))
	{
		packet->telemetry = new aprs_packet_telemetry
		{
			.type = APRS_TELEMETRY_TYPE_EQNS
		};

		// TODO: init eqns

		for (size_t i = 0; i < packet->telemetry->eqns_count; ++i)
			packet->telemetry->eqns_c[i] = &packet->telemetry->eqns[i];
		packet->telemetry->eqns_c[packet->telemetry->eqns_count] = nullptr;

		return packet;
	}

	return nullptr;
}
struct aprs_packet*               APRSERVICE_CALL aprs_packet_telemetry_init_units(const char* sender, const char* tocall, struct aprs_path* path)
{
	if (auto packet = aprs_packet_init_ex(sender, tocall, path, APRS_PACKET_TYPE_TELEMETRY))
	{
		packet->telemetry = new aprs_packet_telemetry
		{
			.type = APRS_TELEMETRY_TYPE_UNITS
		};

		// TODO: init units

		for (size_t i = 0; i < packet->telemetry->units_count; ++i)
			packet->telemetry->units_c[i] = packet->telemetry->units[i].c_str();
		packet->telemetry->units_c[packet->telemetry->units_count] = nullptr;

		return packet;
	}

	return nullptr;
}
struct aprs_packet*               APRSERVICE_CALL aprs_packet_telemetry_init_params(const char* sender, const char* tocall, struct aprs_path* path)
{
	if (auto packet = aprs_packet_init_ex(sender, tocall, path, APRS_PACKET_TYPE_TELEMETRY))
	{
		packet->telemetry = new aprs_packet_telemetry
		{
			.type = APRS_TELEMETRY_TYPE_PARAMS
		};

		// TODO: init params

		for (size_t i = 0; i < packet->telemetry->params_count; ++i)
			packet->telemetry->params_c[i] = packet->telemetry->params[i].data();
		packet->telemetry->params_c[packet->telemetry->params_count] = nullptr;

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
bool                              APRSERVICE_CALL aprs_packet_telemetry_set_bits(struct aprs_packet* packet, uint8_t value)
{
	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_TELEMETRY)
		return false;

	if (aprs_packet_telemetry_get_type(packet) != APRS_TELEMETRY_TYPE_BITS)
		return false;

	packet->telemetry->digital = value;

	return true;
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
			if (!value)
			{
				packet->telemetry->comment.clear();

				return true;
			}
			else if (aprs_validate_comment(value, 67))
			{
				packet->telemetry->comment.assign(value);

				return true;
			}
			break;
	}

	return false;
}

struct aprs_packet*               APRSERVICE_CALL aprs_packet_user_defined_init(const char* sender, const char* tocall, struct aprs_path* path, char id, char type, const char* data)
{
	if (!data)
		return nullptr;

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
	if (!value)
		return false;

	if (aprs_packet_get_type(packet) != APRS_PACKET_TYPE_USER_DEFINED)
		return false;

	if (!aprs_validate_user_defined_data(value))
		return false;

	packet->user_defined->data.assign(value);

	return true;
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

	if (!value)
		packet->third_party->content.clear();
	else
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

const char*                       APRSERVICE_CALL aprs_mic_e_message_to_string(enum APRS_MIC_E_MESSAGES value)
{
	if (value >= APRS_MIC_E_MESSAGES_COUNT)
		return nullptr;

	return aprs_mic_e_messages[value].string;
}
