#ifndef APRSERVICE_H
#define APRSERVICE_H

#include "api.h"
#include "APRS.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

enum APRSERVICE_EVENTS
{
	APRSERVICE_EVENT_CONNECT,
	APRSERVICE_EVENT_DISCONNECT,
	APRSERVICE_EVENT_AUTHENTICATE,
	APRSERVICE_EVENT_RECEIVE_PACKET,
	APRSERVICE_EVENT_RECEIVE_MESSAGE,
	APRSERVICE_EVENT_RECEIVE_SERVER_MESSAGE,

	APRSERVICE_EVENTS_COUNT
};

enum APRSERVICE_MESSAGE_ERRORS
{
	APRSERVICE_MESSAGE_ERROR_SUCCESS,
	APRSERVICE_MESSAGE_ERROR_TIMEOUT,
	APRSERVICE_MESSAGE_ERROR_REJECTED,
	APRSERVICE_MESSAGE_ERROR_DISCONNECTED,

	APRSERVICE_MESSAGE_ERRORS_COUNT
};

struct aprservice;
struct aprservice_task;
struct aprservice_object;
struct aprservice_command;

struct aprservice_task_information
{
	const bool is_canceled;

	uint32_t   seconds;
	bool       reschedule;
};

struct aprservice_event_information
{
	enum APRSERVICE_EVENTS type;
};
struct aprservice_event_information_connect
{
	enum APRSERVICE_EVENTS type;
};
struct aprservice_event_information_disconnect
{
	enum APRSERVICE_EVENTS type;
};
struct aprservice_event_information_authenticate
{
	enum APRSERVICE_EVENTS type;
	const char*            message;
	bool                   success;
	bool                   verified;
};
struct aprservice_event_information_receive_packet
{
	enum APRSERVICE_EVENTS type;
	struct aprs_packet*    packet;
};
struct aprservice_event_information_receive_message
{
	enum APRSERVICE_EVENTS type;
	struct aprs_packet*    packet;
	const char*            sender;
	const char*            content;
	const char*            destination;
};
struct aprservice_event_information_receive_server_message
{
	enum APRSERVICE_EVENTS type;
	const char*            message;
};

typedef void(APRSERVICE_CALL *aprservice_task_handler)(struct aprservice* service, struct aprservice_task_information* task, void* param);
typedef void(APRSERVICE_CALL *aprservice_event_handler)(struct aprservice* service, struct aprservice_event_information* event, void* param);
typedef void(APRSERVICE_CALL *aprservice_command_handler)(struct aprservice* service, struct aprservice_command* command, const char* name, const char* args, void* param);

typedef void(APRSERVICE_CALL *aprservice_message_callback)(struct aprservice* service, enum APRSERVICE_MESSAGE_ERRORS error, void* param);

APRSERVICE_EXPORT struct aprservice*         APRSERVICE_CALL aprservice_init(const char* station, struct aprs_path* path, char symbol_table, char symbol_table_key);
APRSERVICE_EXPORT void                       APRSERVICE_CALL aprservice_deinit(struct aprservice* service);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_is_read_only(struct aprservice* service);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_is_connected(struct aprservice* service);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_is_authenticated(struct aprservice* service);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_is_authenticating(struct aprservice* service);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_is_monitoring_enabled(struct aprservice* service);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_is_compression_enabled(struct aprservice* service);
APRSERVICE_EXPORT struct aprs_path*          APRSERVICE_CALL aprservice_get_path(struct aprservice* service);
APRSERVICE_EXPORT uint32_t                   APRSERVICE_CALL aprservice_get_time(struct aprservice* service);
APRSERVICE_EXPORT const char*                APRSERVICE_CALL aprservice_get_station(struct aprservice* service);
APRSERVICE_EXPORT char                       APRSERVICE_CALL aprservice_get_symbol_table(struct aprservice* service);
APRSERVICE_EXPORT char                       APRSERVICE_CALL aprservice_get_symbol_table_key(struct aprservice* service);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_set_path(struct aprservice* service, struct aprs_path* value);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_set_symbol(struct aprservice* service, char table, char key);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_set_comment(struct aprservice* service, const char* value);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_set_position(struct aprservice* service, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course);
APRSERVICE_EXPORT void                       APRSERVICE_CALL aprservice_set_event_handler(struct aprservice* service, enum APRSERVICE_EVENTS event, aprservice_event_handler handler, void* param);
APRSERVICE_EXPORT void                       APRSERVICE_CALL aprservice_set_default_event_handler(struct aprservice* service, aprservice_event_handler handler, void* param);
APRSERVICE_EXPORT void                       APRSERVICE_CALL aprservice_enable_monitoring(struct aprservice* service, bool value);
APRSERVICE_EXPORT void                       APRSERVICE_CALL aprservice_enable_compression(struct aprservice* service, bool value);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_poll(struct aprservice* service);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_send(struct aprservice* service, const char* raw);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_send_packet(struct aprservice* service, const char* content);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_send_object(struct aprservice* service, const char* name, const char* comment, char symbol_table, char symbol_table_key, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course, bool live);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_send_status(struct aprservice* service, const char* message);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_send_message(struct aprservice* service, const char* destination, const char* content, uint32_t timeout, aprservice_message_callback callback, void* param);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_send_message_ex(struct aprservice* service, const char* destination, const char* content, const char* id, uint32_t timeout, aprservice_message_callback callback, void* param);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_send_weather(struct aprservice* service, uint16_t wind_speed, uint16_t wind_speed_gust, uint16_t wind_direction, uint16_t rainfall_last_hour, uint16_t rainfall_last_24_hours, uint16_t rainfall_since_midnight, uint8_t humidity, int16_t temperature, uint32_t barometric_pressure, const char* type);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_send_position(struct aprservice* service);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_send_position_ex(struct aprservice* service, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course, const char* comment);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_send_telemetry(struct aprservice* service, uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t digital);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_send_telemetry_ex(struct aprservice* service, uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t digital, const char* comment, uint16_t sequence);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_send_telemetry_float(struct aprservice* service, float a1, float a2, float a3, float a4, float a5, uint8_t digital);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_send_telemetry_float_ex(struct aprservice* service, float a1, float a2, float a3, float a4, float a5, uint8_t digital, const char* comment, uint16_t sequence);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_send_user_defined(struct aprservice* service, char id, char type, const char* data);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_connect(struct aprservice* service, const char* host, uint16_t port, uint16_t passwd);
APRSERVICE_EXPORT void                       APRSERVICE_CALL aprservice_disconnect(struct aprservice* service);

APRSERVICE_EXPORT struct aprservice_task*    APRSERVICE_CALL aprservice_task_schedule(struct aprservice* service, uint32_t seconds, aprservice_task_handler handler, void* param);
APRSERVICE_EXPORT void                       APRSERVICE_CALL aprservice_task_cancel(struct aprservice_task* task);
APRSERVICE_EXPORT struct aprservice*         APRSERVICE_CALL aprservice_task_get_service(struct aprservice_task* task);

APRSERVICE_EXPORT struct aprservice_object*  APRSERVICE_CALL aprservice_object_create(struct aprservice* service, const char* name, const char* comment, char symbol_table, char symbol_table_key, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course);
APRSERVICE_EXPORT void                       APRSERVICE_CALL aprservice_object_destroy(struct aprservice_object* object);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_object_is_alive(struct aprservice_object* object);
APRSERVICE_EXPORT struct aprservice*         APRSERVICE_CALL aprservice_object_get_service(struct aprservice_object* object);
APRSERVICE_EXPORT const char*                APRSERVICE_CALL aprservice_object_get_name(struct aprservice_object* object);
APRSERVICE_EXPORT const char*                APRSERVICE_CALL aprservice_object_get_comment(struct aprservice_object* object);
APRSERVICE_EXPORT uint16_t                   APRSERVICE_CALL aprservice_object_get_speed(struct aprservice_object* object);
APRSERVICE_EXPORT uint16_t                   APRSERVICE_CALL aprservice_object_get_course(struct aprservice_object* object);
APRSERVICE_EXPORT int32_t                    APRSERVICE_CALL aprservice_object_get_altitude(struct aprservice_object* object);
APRSERVICE_EXPORT float                      APRSERVICE_CALL aprservice_object_get_latitude(struct aprservice_object* object);
APRSERVICE_EXPORT float                      APRSERVICE_CALL aprservice_object_get_longitude(struct aprservice_object* object);
APRSERVICE_EXPORT char                       APRSERVICE_CALL aprservice_object_get_symbol_table(struct aprservice_object* object);
APRSERVICE_EXPORT char                       APRSERVICE_CALL aprservice_object_get_symbol_table_key(struct aprservice_object* object);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_object_set_symbol(struct aprservice_object* object, char table, char key);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_object_set_comment(struct aprservice_object* object, const char* value);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_object_set_position(struct aprservice_object* object, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_object_kill(struct aprservice_object* object);
APRSERVICE_EXPORT bool                       APRSERVICE_CALL aprservice_object_announce(struct aprservice_object* object);

APRSERVICE_EXPORT struct aprservice_command* APRSERVICE_CALL aprservice_command_register(struct aprservice* service, const char* name, const char* help, aprservice_command_handler handler, void* param);
APRSERVICE_EXPORT void                       APRSERVICE_CALL aprservice_command_unregister(struct aprservice_command* command);
APRSERVICE_EXPORT struct aprservice*         APRSERVICE_CALL aprservice_command_get_service(struct aprservice_command* command);

#endif
