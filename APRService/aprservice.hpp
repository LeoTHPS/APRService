#pragma once
#include <AL/Common.hpp>

enum APRSERVICE_FLAGS : AL::uint8
{
	APRSERVICE_FLAG_NONE                    = 0x00,
	APRSERVICE_FLAG_STOP_ON_APRS_DISCONNECT = 0x01
};

enum APRSERVICE_MEASUREMENT_TYPES : AL::uint8
{
	APRSERVICE_MEASUREMENT_TYPE_FEET,
	APRSERVICE_MEASUREMENT_TYPE_MILES,
	APRSERVICE_MEASUREMENT_TYPE_METERS,
	APRSERVICE_MEASUREMENT_TYPE_KILOMETERS
};

enum APRSERVICE_APRS_PACKET_TYPES : AL::uint8
{
	APRSERVICE_APRS_PACKET_TYPE_UNKNOWN,
	APRSERVICE_APRS_PACKET_TYPE_MESSAGE,
	APRSERVICE_APRS_PACKET_TYPE_POSITION,
	APRSERVICE_APRS_PACKET_TYPE_TELEMETRY
};

enum APRSERVICE_APRS_POSITION_FLAGS : AL::uint8
{
	APRSERVICE_APRS_POSITION_FLAG_NONE              = 0x00,
	APRSERVICE_APRS_POSITION_FLAG_COMPRESSED        = 0x01,
	APRSERVICE_APRS_POSITION_FLAG_MESSAGING_ENABLED = 0x02
};

enum APRSERVICE_APRS_CONNECTION_TYPES : AL::uint8
{
	APRSERVICE_APRS_CONNECTION_TYPE_NONE,
	APRSERVICE_APRS_CONNECTION_TYPE_APRS_IS,
	APRSERVICE_APRS_CONNECTION_TYPE_KISS_TCP,
	APRSERVICE_APRS_CONNECTION_TYPE_KISS_SERIAL
};

enum APRSERVICE_APRS_DISCONNECT_REASONS : AL::uint8
{
	APRSERVICE_APRS_DISCONNECT_REASON_UNDEFINED,

	APRSERVICE_APRS_DISCONNECT_REASON_USER_REQUESTED,
	APRSERVICE_APRS_DISCONNECT_REASON_CONNECTION_LOST,
	APRSERVICE_APRS_DISCONNECT_REASON_AUTHENTICATION_FAILED
};

struct aprservice;

typedef void(*aprservice_event_handler)(aprservice* service, void* param);
typedef void(*aprservice_command_handler)(aprservice* service, const AL::String& sender, const AL::String& command_name, const AL::String& command_params, void* param);

typedef void(*aprservice_aprs_message_callback)(aprservice* service, void* param);
typedef bool(*aprservice_aprs_packet_filter_callback)(aprservice* service, const AL::String& station, const AL::String& tocall, const AL::String& path, const AL::String& content, void* param);
typedef void(*aprservice_aprs_packet_monitor_callback)(aprservice* service, const AL::String& station, const AL::String& tocall, const AL::String& path, const AL::String& content, void* param);

typedef void(*aprservice_aprs_on_connect)(aprservice* service, AL::uint8 type, void* param);
typedef void(*aprservice_aprs_on_disconnect)(aprservice* service, AL::uint8 reason, void* param);

typedef void(*aprservice_aprs_on_send)(aprservice* service, const AL::String& value, void* param);
typedef void(*aprservice_aprs_on_receive)(aprservice* service, const AL::String& value, void* param);

typedef void(*aprservice_aprs_on_send_packet)(aprservice* service, const AL::String& station, const AL::String& tocall, const AL::String& path, const AL::String& content, void* param);
typedef void(*aprservice_aprs_on_receive_packet)(aprservice* service, const AL::String& station, const AL::String& tocall, const AL::String& path, const AL::String& content, void* param);

typedef void(*aprservice_aprs_on_send_message)(aprservice* service, const AL::String& station, const AL::String& path, const AL::String& destination, const AL::String& content, void* param);
typedef void(*aprservice_aprs_on_receive_message)(aprservice* service, const AL::String& station, const AL::String& path, const AL::String& destination, const AL::String& content, void* param);

typedef void(*aprservice_aprs_on_send_position)(aprservice* service, const AL::String& station, const AL::String& path, AL::int32 altitude, AL::Float latitude, AL::Float longitude, char symbol_table, char symbol_table_key, const AL::String& comment, AL::uint8 flags, void* param);
typedef void(*aprservice_aprs_on_receive_position)(aprservice* service, const AL::String& station, const AL::String& path, AL::int32 altitude, AL::Float latitude, AL::Float longitude, char symbol_table, char symbol_table_key, const AL::String& comment, AL::uint8 flags, void* param);

typedef void(*aprservice_aprs_on_send_telemetry)(aprservice* service, const AL::String& station, const AL::String& tocall, const AL::String& path, const AL::uint8(&analog)[5], const bool(&digital)[8], void* param);
typedef void(*aprservice_aprs_on_receive_telemetry)(aprservice* service, const AL::String& station, const AL::String& tocall, const AL::String& path, const AL::uint8(&analog)[5], const bool(&digital)[8], void* param);

typedef void(*aprservice_aprs_on_receive_invalid_packet)(aprservice* service, const AL::String& station, const AL::String& tocall, const AL::String& path, const AL::String& content, AL::uint8 packet_type, void* param);

struct aprservice_aprs_event_handlers
{
	void*                                     param;

	aprservice_aprs_on_connect                on_connect;
	aprservice_aprs_on_disconnect             on_disconnect;

	aprservice_aprs_on_send                   on_send;
	aprservice_aprs_on_receive                on_receive;

	aprservice_aprs_on_send_packet            on_send_packet;
	aprservice_aprs_on_receive_packet         on_receive_packet;

	aprservice_aprs_on_send_message           on_send_message;
	aprservice_aprs_on_receive_message        on_receive_message;

	aprservice_aprs_on_send_position          on_send_position;
	aprservice_aprs_on_receive_position       on_receive_position;

	aprservice_aprs_on_send_telemetry         on_send_telemetry;
	aprservice_aprs_on_receive_telemetry      on_receive_telemetry;

	aprservice_aprs_on_receive_invalid_packet on_receive_invalid_packet;
};

struct aprservice_aprs_config
{
	aprservice_aprs_event_handlers events;

	AL::String                     path;
	AL::String                     station;
	char                           symbol_table;
	char                           symbol_table_key;
	bool                           monitor_mode_enabled;
};

struct aprservice_config
{
	aprservice_aprs_config aprs;
};

aprservice* aprservice_init(const aprservice_config& config);
void        aprservice_deinit(aprservice* service);
bool        aprservice_is_running(aprservice* service);
void        aprservice_run(aprservice* service, AL::uint32 tick_rate, AL::uint8 flags);
void        aprservice_stop(aprservice* service);

bool        aprservice_aprs_is_connected(aprservice* service);
bool        aprservice_aprs_connect_is(aprservice* service, const AL::String& remote_host, AL::uint16 remote_port, AL::uint16 passcode);
bool        aprservice_aprs_connect_kiss_tcp(aprservice* service, const AL::String& remote_host, AL::uint16 remote_port);
bool        aprservice_aprs_connect_kiss_serial(aprservice* service, const AL::String& device, AL::uint32 speed);
void        aprservice_aprs_disconnect(aprservice* service);
void        aprservice_aprs_add_packet_monitor(aprservice* service, aprservice_aprs_packet_filter_callback filter, aprservice_aprs_packet_monitor_callback callback, void* param);
// @return 0 on connection closed
// @return -1 on encoding error
int         aprservice_aprs_send_message(aprservice* service, const AL::String& destination, const AL::String& content);
// @return 0 on connection closed
// @return -1 on encoding error
int         aprservice_aprs_send_position(aprservice* service, AL::int32 altitude, AL::Float latitude, AL::Float longitude, const AL::String& comment);
// @return 0 on connection closed
// @return -1 on encoding error
int         aprservice_aprs_send_telemetry(aprservice* service, const AL::uint8(&analog)[5], const bool(&digital)[8]);
// @return 0 on connection closed
// @return -1 on encoding error
int         aprservice_aprs_begin_send_message(aprservice* service, const AL::String& destination, const AL::String& content, aprservice_aprs_message_callback callback, void* param);

AL::Float   aprservice_math_get_distance_between_points(AL::Float latitude1, AL::Float longitude1, AL::Float latitude2, AL::Float longitude2, AL::uint8 measurement_type);

AL::uint32  aprservice_events_get_count(aprservice* service);
void        aprservice_events_clear(aprservice* service);
void        aprservice_events_schedule(aprservice* service, AL::TimeSpan delay, aprservice_event_handler handler, void* param);

bool        aprservice_console_set_title(const AL::String& value);
bool        aprservice_console_read(char& value);
bool        aprservice_console_read_line(AL::String& value);
bool        aprservice_console_write(const AL::String& value);
bool        aprservice_console_write_line(const AL::String& value);
bool        aprservice_console_write_exception(const AL::Exception& value);

bool        aprservice_commands_execute(aprservice* service, const AL::String& sender, const AL::String& message);
void        aprservice_commands_register(aprservice* service, const AL::String& name, aprservice_command_handler handler, void* param);
