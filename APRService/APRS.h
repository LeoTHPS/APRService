#ifndef APRSERVICE_APRS_H
#define APRSERVICE_APRS_H

#include "api.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

enum APRS_TIME
{
	APRS_TIME_DHM  = 0x1,
	APRS_TIME_HMS  = 0x2,
	APRS_TIME_MDHM = 0x4 | APRS_TIME_DHM
};

enum APRS_DISTANCES
{
	APRS_DISTANCE_FEET,
	APRS_DISTANCE_MILES,
	APRS_DISTANCE_METERS,
	APRS_DISTANCE_KILOMETERS,

	APRS_DISTANCES_COUNT
};

enum APRS_PACKET_TYPES
{
	APRS_PACKET_TYPE_RAW,
	APRS_PACKET_TYPE_ITEM,
	APRS_PACKET_TYPE_OBJECT,
	APRS_PACKET_TYPE_STATUS,
	APRS_PACKET_TYPE_MESSAGE,
	APRS_PACKET_TYPE_WEATHER,
	APRS_PACKET_TYPE_POSITION,
	APRS_PACKET_TYPE_TELEMETRY,
	APRS_PACKET_TYPE_USER_DEFINED,

	APRS_PACKET_TYPES_COUNT
};

enum APRS_MESSAGE_TYPES
{
	APRS_MESSAGE_TYPE_ACK,
	APRS_MESSAGE_TYPE_REJECT,
	APRS_MESSAGE_TYPE_MESSAGE,
	APRS_MESSAGE_TYPE_BULLETIN,

	APRS_MESSAGE_TYPES_COUNT
};

enum APRS_POSITION_FLAGS
{
	APRS_POSITION_FLAG_TIME              = 0x1,
	APRS_POSITION_FLAG_MIC_E             = 0x2,
	APRS_POSITION_FLAG_COMPRESSED        = 0x4,
	APRS_POSITION_FLAG_MESSAGING_ENABLED = 0x8
};

enum APRS_TELEMETRY_TYPES
{
	APRS_TELEMETRY_TYPE_U8,
	APRS_TELEMETRY_TYPE_FLOAT,

	APRS_TELEMETRY_TYPES_COUNT
};

struct aprs_path;
struct aprs_time;
struct aprs_packet;

APRSERVICE_EXPORT struct aprs_path*         APRSERVICE_CALL aprs_path_init();
APRSERVICE_EXPORT struct aprs_path*         APRSERVICE_CALL aprs_path_init_from_string(const char* string);
APRSERVICE_EXPORT void                      APRSERVICE_CALL aprs_path_deinit(struct aprs_path* path);
APRSERVICE_EXPORT const char*               APRSERVICE_CALL aprs_path_get(struct aprs_path* path, uint8_t index);
APRSERVICE_EXPORT uint8_t                   APRSERVICE_CALL aprs_path_get_length(struct aprs_path* path);
APRSERVICE_EXPORT uint8_t                   APRSERVICE_CALL aprs_path_get_capacity(struct aprs_path* path);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_path_set(struct aprs_path* path, uint8_t index, const char* value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_path_pop(struct aprs_path* path);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_path_push(struct aprs_path* path, const char* value);
APRSERVICE_EXPORT void                      APRSERVICE_CALL aprs_path_clear(struct aprs_path* path);
APRSERVICE_EXPORT const char*               APRSERVICE_CALL aprs_path_to_string(struct aprs_path* path);
APRSERVICE_EXPORT void                      APRSERVICE_CALL aprs_path_add_reference(struct aprs_path* path);

APRSERVICE_EXPORT struct aprs_time*         APRSERVICE_CALL aprs_time_now();
APRSERVICE_EXPORT int                       APRSERVICE_CALL aprs_time_get_type(const struct aprs_time* time);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_time_get_dms(const struct aprs_time* time, uint8_t* day, uint8_t* minute, uint8_t* second);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_time_get_hms(const struct aprs_time* time, uint8_t* hour, uint8_t* minute, uint8_t* second);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_time_get_mdhm(const struct aprs_time* time, uint8_t* month, uint8_t* day, uint8_t* hour, uint8_t* minute);

APRSERVICE_EXPORT struct aprs_packet*       APRSERVICE_CALL aprs_packet_init(const char* sender, const char* tocall, struct aprs_path* path);
APRSERVICE_EXPORT struct aprs_packet*       APRSERVICE_CALL aprs_packet_init_from_string(const char* string);
APRSERVICE_EXPORT void                      APRSERVICE_CALL aprs_packet_deinit(struct aprs_packet* packet);
APRSERVICE_EXPORT const char*               APRSERVICE_CALL aprs_packet_get_q(struct aprs_packet* packet);
APRSERVICE_EXPORT enum APRS_PACKET_TYPES    APRSERVICE_CALL aprs_packet_get_type(struct aprs_packet* packet);
APRSERVICE_EXPORT struct aprs_path*         APRSERVICE_CALL aprs_packet_get_path(struct aprs_packet* packet);
APRSERVICE_EXPORT const char*               APRSERVICE_CALL aprs_packet_get_igate(struct aprs_packet* packet);
APRSERVICE_EXPORT const char*               APRSERVICE_CALL aprs_packet_get_tocall(struct aprs_packet* packet);
APRSERVICE_EXPORT const char*               APRSERVICE_CALL aprs_packet_get_sender(struct aprs_packet* packet);
APRSERVICE_EXPORT const char*               APRSERVICE_CALL aprs_packet_get_content(struct aprs_packet* packet);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_set_path(struct aprs_packet* packet, struct aprs_path* value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_set_tocall(struct aprs_packet* packet, const char* value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_set_sender(struct aprs_packet* packet, const char* value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_set_content(struct aprs_packet* packet, const char* value);
APRSERVICE_EXPORT const char*               APRSERVICE_CALL aprs_packet_to_string(struct aprs_packet* packet);
APRSERVICE_EXPORT void                      APRSERVICE_CALL aprs_packet_add_reference(struct aprs_packet* packet);

APRSERVICE_EXPORT struct aprs_packet*       APRSERVICE_CALL aprs_packet_item_init(const char* sender, const char* tocall, struct aprs_path* path, const char* name, char symbol_table, char symbol_table_key);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_item_is_alive(struct aprs_packet* packet);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_item_is_compressed(struct aprs_packet* packet);
APRSERVICE_EXPORT const char*               APRSERVICE_CALL aprs_packet_item_get_name(struct aprs_packet* packet);
APRSERVICE_EXPORT const char*               APRSERVICE_CALL aprs_packet_item_get_comment(struct aprs_packet* packet);
APRSERVICE_EXPORT uint16_t                  APRSERVICE_CALL aprs_packet_item_get_speed(struct aprs_packet* packet);
APRSERVICE_EXPORT uint16_t                  APRSERVICE_CALL aprs_packet_item_get_course(struct aprs_packet* packet);
APRSERVICE_EXPORT int32_t                   APRSERVICE_CALL aprs_packet_item_get_altitude(struct aprs_packet* packet);
APRSERVICE_EXPORT float                     APRSERVICE_CALL aprs_packet_item_get_latitude(struct aprs_packet* packet);
APRSERVICE_EXPORT float                     APRSERVICE_CALL aprs_packet_item_get_longitude(struct aprs_packet* packet);
APRSERVICE_EXPORT char                      APRSERVICE_CALL aprs_packet_item_get_symbol_table(struct aprs_packet* packet);
APRSERVICE_EXPORT char                      APRSERVICE_CALL aprs_packet_item_get_symbol_table_key(struct aprs_packet* packet);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_item_set_alive(struct aprs_packet* packet, bool value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_item_set_compressed(struct aprs_packet* packet, bool value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_item_set_name(struct aprs_packet* packet, const char* value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_item_set_comment(struct aprs_packet* packet, const char* value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_item_set_speed(struct aprs_packet* packet, uint16_t value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_item_set_course(struct aprs_packet* packet, uint16_t value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_item_set_altitude(struct aprs_packet* packet, int32_t value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_item_set_latitude(struct aprs_packet* packet, float value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_item_set_longitude(struct aprs_packet* packet, float value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_item_set_symbol(struct aprs_packet* packet, char table, char key);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_item_set_symbol_table(struct aprs_packet* packet, char value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_item_set_symbol_table_key(struct aprs_packet* packet, char value);

APRSERVICE_EXPORT struct aprs_packet*       APRSERVICE_CALL aprs_packet_object_init(const char* sender, const char* tocall, struct aprs_path* path, const char* name, char symbol_table, char symbol_table_key);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_object_is_alive(struct aprs_packet* packet);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_object_is_compressed(struct aprs_packet* packet);
APRSERVICE_EXPORT const struct aprs_time*   APRSERVICE_CALL aprs_packet_object_get_time(struct aprs_packet* packet);
APRSERVICE_EXPORT const char*               APRSERVICE_CALL aprs_packet_object_get_name(struct aprs_packet* packet);
APRSERVICE_EXPORT const char*               APRSERVICE_CALL aprs_packet_object_get_comment(struct aprs_packet* packet);
APRSERVICE_EXPORT uint16_t                  APRSERVICE_CALL aprs_packet_object_get_speed(struct aprs_packet* packet);
APRSERVICE_EXPORT uint16_t                  APRSERVICE_CALL aprs_packet_object_get_course(struct aprs_packet* packet);
APRSERVICE_EXPORT int32_t                   APRSERVICE_CALL aprs_packet_object_get_altitude(struct aprs_packet* packet);
APRSERVICE_EXPORT float                     APRSERVICE_CALL aprs_packet_object_get_latitude(struct aprs_packet* packet);
APRSERVICE_EXPORT float                     APRSERVICE_CALL aprs_packet_object_get_longitude(struct aprs_packet* packet);
APRSERVICE_EXPORT char                      APRSERVICE_CALL aprs_packet_object_get_symbol_table(struct aprs_packet* packet);
APRSERVICE_EXPORT char                      APRSERVICE_CALL aprs_packet_object_get_symbol_table_key(struct aprs_packet* packet);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_object_set_time(struct aprs_packet* packet, const struct aprs_time* value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_object_set_alive(struct aprs_packet* packet, bool value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_object_set_compressed(struct aprs_packet* packet, bool value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_object_set_name(struct aprs_packet* packet, const char* value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_object_set_comment(struct aprs_packet* packet, const char* value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_object_set_speed(struct aprs_packet* packet, uint16_t value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_object_set_course(struct aprs_packet* packet, uint16_t value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_object_set_altitude(struct aprs_packet* packet, int32_t value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_object_set_latitude(struct aprs_packet* packet, float value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_object_set_longitude(struct aprs_packet* packet, float value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_object_set_symbol(struct aprs_packet* packet, char table, char key);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_object_set_symbol_table(struct aprs_packet* packet, char value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_object_set_symbol_table_key(struct aprs_packet* packet, char value);

APRSERVICE_EXPORT struct aprs_packet*       APRSERVICE_CALL aprs_packet_status_init(const char* sender, const char* tocall, struct aprs_path* path, const char* message);
APRSERVICE_EXPORT struct aprs_time*         APRSERVICE_CALL aprs_packet_status_get_time(struct aprs_packet* packet);
APRSERVICE_EXPORT const char*               APRSERVICE_CALL aprs_packet_status_get_message(struct aprs_packet* packet);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_status_set_time(struct aprs_packet* packet, struct aprs_time* value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_status_set_message(struct aprs_packet* packet, const char* value);

APRSERVICE_EXPORT struct aprs_packet*       APRSERVICE_CALL aprs_packet_message_init(const char* sender, const char* tocall, struct aprs_path* path, const char* destination, const char* content);
APRSERVICE_EXPORT struct aprs_packet*       APRSERVICE_CALL aprs_packet_message_init_ack(const char* sender, const char* tocall, struct aprs_path* path, const char* destination, const char* id);
APRSERVICE_EXPORT struct aprs_packet*       APRSERVICE_CALL aprs_packet_message_init_reject(const char* sender, const char* tocall, struct aprs_path* path, const char* destination, const char* id);
APRSERVICE_EXPORT struct aprs_packet*       APRSERVICE_CALL aprs_packet_message_init_bulletin(const char* sender, const char* tocall, struct aprs_path* path, const char* destination);
APRSERVICE_EXPORT const char*               APRSERVICE_CALL aprs_packet_message_get_id(struct aprs_packet* packet);
APRSERVICE_EXPORT enum APRS_MESSAGE_TYPES   APRSERVICE_CALL aprs_packet_message_get_type(struct aprs_packet* packet);
APRSERVICE_EXPORT const char*               APRSERVICE_CALL aprs_packet_message_get_content(struct aprs_packet* packet);
APRSERVICE_EXPORT const char*               APRSERVICE_CALL aprs_packet_message_get_destination(struct aprs_packet* packet);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_message_set_id(struct aprs_packet* packet, const char* value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_message_set_type(struct aprs_packet* packet, enum APRS_MESSAGE_TYPES value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_message_set_content(struct aprs_packet* packet, const char* value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_message_set_destination(struct aprs_packet* packet, const char* value);

APRSERVICE_EXPORT struct aprs_packet*       APRSERVICE_CALL aprs_packet_weather_init(const char* sender, const char* tocall, struct aprs_path* path, const char* type);
APRSERVICE_EXPORT const struct aprs_time*   APRSERVICE_CALL aprs_packet_weather_get_time(struct aprs_packet* packet);
APRSERVICE_EXPORT const char*               APRSERVICE_CALL aprs_packet_weather_get_type(struct aprs_packet* packet);
APRSERVICE_EXPORT uint16_t                  APRSERVICE_CALL aprs_packet_weather_get_wind_speed(struct aprs_packet* packet);
APRSERVICE_EXPORT uint16_t                  APRSERVICE_CALL aprs_packet_weather_get_wind_speed_gust(struct aprs_packet* packet);
APRSERVICE_EXPORT uint16_t                  APRSERVICE_CALL aprs_packet_weather_get_wind_direction(struct aprs_packet* packet);
APRSERVICE_EXPORT uint16_t                  APRSERVICE_CALL aprs_packet_weather_get_rainfall_last_hour(struct aprs_packet* packet);
APRSERVICE_EXPORT uint16_t                  APRSERVICE_CALL aprs_packet_weather_get_rainfall_last_24_hours(struct aprs_packet* packet);
APRSERVICE_EXPORT uint16_t                  APRSERVICE_CALL aprs_packet_weather_get_rainfall_since_midnight(struct aprs_packet* packet);
APRSERVICE_EXPORT uint8_t                   APRSERVICE_CALL aprs_packet_weather_get_humidity(struct aprs_packet* packet);
APRSERVICE_EXPORT int16_t                   APRSERVICE_CALL aprs_packet_weather_get_temperature(struct aprs_packet* packet);
APRSERVICE_EXPORT uint32_t                  APRSERVICE_CALL aprs_packet_weather_get_barometric_pressure(struct aprs_packet* packet);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_weather_set_time(struct aprs_packet* packet, const struct aprs_time* value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_weather_set_wind_speed(struct aprs_packet* packet, uint16_t value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_weather_set_wind_speed_gust(struct aprs_packet* packet, uint16_t value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_weather_set_wind_direction(struct aprs_packet* packet, uint16_t value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_weather_set_rainfall_last_hour(struct aprs_packet* packet, uint16_t value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_weather_set_rainfall_last_24_hours(struct aprs_packet* packet, uint16_t value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_weather_set_rainfall_since_midnight(struct aprs_packet* packet, uint16_t value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_weather_set_humidity(struct aprs_packet* packet, uint8_t value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_weather_set_temperature(struct aprs_packet* packet, int16_t value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_weather_set_barometric_pressure(struct aprs_packet* packet, uint32_t value);

APRSERVICE_EXPORT struct aprs_packet*       APRSERVICE_CALL aprs_packet_position_init(const char* sender, const char* tocall, struct aprs_path* path, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course, const char* comment, char symbol_table, char symbol_table_key);
APRSERVICE_EXPORT struct aprs_packet*       APRSERVICE_CALL aprs_packet_position_init_mic_e(const char* sender, const char* tocall, struct aprs_path* path, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course, const char* comment, char symbol_table, char symbol_table_key);
APRSERVICE_EXPORT struct aprs_packet*       APRSERVICE_CALL aprs_packet_position_init_compressed(const char* sender, const char* tocall, struct aprs_path* path, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course, const char* comment, char symbol_table, char symbol_table_key);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_position_is_mic_e(struct aprs_packet* packet);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_position_is_compressed(struct aprs_packet* packet);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_position_is_messaging_enabled(struct aprs_packet* packet);
APRSERVICE_EXPORT const struct aprs_time*   APRSERVICE_CALL aprs_packet_position_get_time(struct aprs_packet* packet);
APRSERVICE_EXPORT int                       APRSERVICE_CALL aprs_packet_position_get_flags(struct aprs_packet* packet);
APRSERVICE_EXPORT const char*               APRSERVICE_CALL aprs_packet_position_get_comment(struct aprs_packet* packet);
APRSERVICE_EXPORT uint16_t                  APRSERVICE_CALL aprs_packet_position_get_speed(struct aprs_packet* packet);
APRSERVICE_EXPORT uint16_t                  APRSERVICE_CALL aprs_packet_position_get_course(struct aprs_packet* packet);
APRSERVICE_EXPORT int32_t                   APRSERVICE_CALL aprs_packet_position_get_altitude(struct aprs_packet* packet);
APRSERVICE_EXPORT float                     APRSERVICE_CALL aprs_packet_position_get_latitude(struct aprs_packet* packet);
APRSERVICE_EXPORT float                     APRSERVICE_CALL aprs_packet_position_get_longitude(struct aprs_packet* packet);
APRSERVICE_EXPORT char                      APRSERVICE_CALL aprs_packet_position_get_symbol_table(struct aprs_packet* packet);
APRSERVICE_EXPORT char                      APRSERVICE_CALL aprs_packet_position_get_symbol_table_key(struct aprs_packet* packet);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_position_set_time(struct aprs_packet* packet, const struct aprs_time* value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_position_set_comment(struct aprs_packet* packet, const char* value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_position_set_speed(struct aprs_packet* packet, uint16_t value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_position_set_course(struct aprs_packet* packet, uint16_t value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_position_set_altitude(struct aprs_packet* packet, int32_t value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_position_set_latitude(struct aprs_packet* packet, float value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_position_set_longitude(struct aprs_packet* packet, float value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_position_set_symbol(struct aprs_packet* packet, char table, char key);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_position_set_symbol_table(struct aprs_packet* packet, char value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_position_set_symbol_table_key(struct aprs_packet* packet, char value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_position_enable_mic_e(struct aprs_packet* packet, bool value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_position_enable_messaging(struct aprs_packet* packet, bool value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_position_enable_compression(struct aprs_packet* packet, bool value);

APRSERVICE_EXPORT struct aprs_packet*       APRSERVICE_CALL aprs_packet_telemetry_init(const char* sender, const char* tocall, struct aprs_path* path, uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t digital, uint16_t sequence);
APRSERVICE_EXPORT struct aprs_packet*       APRSERVICE_CALL aprs_packet_telemetry_init_float(const char* sender, const char* tocall, struct aprs_path* path, float a1, float a2, float a3, float a4, float a5, uint8_t digital, uint16_t sequence);
APRSERVICE_EXPORT enum APRS_TELEMETRY_TYPES APRSERVICE_CALL aprs_packet_telemetry_get_type(struct aprs_packet* packet);
APRSERVICE_EXPORT uint8_t                   APRSERVICE_CALL aprs_packet_telemetry_get_analog(struct aprs_packet* packet, uint8_t index);
APRSERVICE_EXPORT float                     APRSERVICE_CALL aprs_packet_telemetry_get_analog_float(struct aprs_packet* packet, uint8_t index);
APRSERVICE_EXPORT uint8_t                   APRSERVICE_CALL aprs_packet_telemetry_get_digital(struct aprs_packet* packet);
APRSERVICE_EXPORT uint16_t                  APRSERVICE_CALL aprs_packet_telemetry_get_sequence(struct aprs_packet* packet);
APRSERVICE_EXPORT const char*               APRSERVICE_CALL aprs_packet_telemetry_get_comment(struct aprs_packet* packet);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_telemetry_set_analog(struct aprs_packet* packet, uint8_t value, uint8_t index);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_telemetry_set_analog_float(struct aprs_packet* packet, float value, uint8_t index);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_telemetry_set_digital(struct aprs_packet* packet, uint8_t value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_telemetry_set_sequence(struct aprs_packet* packet, uint16_t value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_telemetry_set_comment(struct aprs_packet* packet, const char* value);

APRSERVICE_EXPORT struct aprs_packet*       APRSERVICE_CALL aprs_packet_user_defined_init(const char* sender, const char* tocall, struct aprs_path* path, char id, char type, const char* data);
APRSERVICE_EXPORT char                      APRSERVICE_CALL aprs_packet_user_defined_get_id(struct aprs_packet* packet);
APRSERVICE_EXPORT char                      APRSERVICE_CALL aprs_packet_user_defined_get_type(struct aprs_packet* packet);
APRSERVICE_EXPORT const char*               APRSERVICE_CALL aprs_packet_user_defined_get_data(struct aprs_packet* packet);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_user_defined_set_id(struct aprs_packet* packet, char value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_user_defined_set_type(struct aprs_packet* packet, char value);
APRSERVICE_EXPORT bool                      APRSERVICE_CALL aprs_packet_user_defined_set_data(struct aprs_packet* packet, const char* value);

APRSERVICE_EXPORT float                     APRSERVICE_CALL aprs_distance(float latitude1, float longitude1, float latitude2, float longitude2, enum APRS_DISTANCES type);
APRSERVICE_EXPORT float                     APRSERVICE_CALL aprs_distance_3d(float latitude1, float longitude1, int32_t altitude1, float latitude2, float longitude2, int32_t altitude2, enum APRS_DISTANCES type);

#endif
