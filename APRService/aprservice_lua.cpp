#include "aprservice.hpp"
#include "aprservice_lua.hpp"

#include <AL/Lua54/Lua.hpp>

#include <AL/OS/Console.hpp>

#include <AL/Collections/UnorderedSet.hpp>

struct aprservice_lua
{
	AL::Lua54::Lua state;
};

struct lua_aprservice;

typedef AL::Lua54::Function<bool(lua_aprservice* lua_service)>                                                                                                                                                                                                                                                                                                                                                                                                                                                lua_aprservice_aprs_connection_is_blocking;
typedef AL::Lua54::Function<bool(lua_aprservice* lua_service)>                                                                                                                                                                                                                                                                                                                                                                                                                                                lua_aprservice_aprs_connection_is_connected;
typedef AL::Lua54::Function<bool(lua_aprservice* lua_service)>                                                                                                                                                                                                                                                                                                                                                                                                                                                lua_aprservice_aprs_connection_connect;
typedef AL::Lua54::Function<void(lua_aprservice* lua_service)>                                                                                                                                                                                                                                                                                                                                                                                                                                                lua_aprservice_aprs_connection_disconnect;
typedef AL::Lua54::Function<bool(lua_aprservice* lua_service, bool set)>                                                                                                                                                                                                                                                                                                                                                                                                                                      lua_aprservice_aprs_connection_set_blocking;
// @return 0 on connection closed
// @return -1 if would block
typedef AL::Lua54::Function<int(lua_aprservice* lua_service, AL::String& value)>                                                                                                                                                                                                                                                                                                                                                                                                                              lua_aprservice_aprs_connection_read;
// @return false on connection closed
typedef AL::Lua54::Function<bool(lua_aprservice* lua_service, const AL::String& value)>                                                                                                                                                                                                                                                                                                                                                                                                                       lua_aprservice_aprs_connection_write;

struct lua_aprservice_aprs_connection
{
	lua_aprservice_aprs_connection_is_blocking  is_blocking;
	lua_aprservice_aprs_connection_is_connected is_connected;

	lua_aprservice_aprs_connection_connect      connect;
	lua_aprservice_aprs_connection_disconnect   disconnect;

	lua_aprservice_aprs_connection_set_blocking set_blocking;

	lua_aprservice_aprs_connection_read         read;
	lua_aprservice_aprs_connection_write        write;
};

typedef AL::Lua54::Function<void(lua_aprservice* lua_service)>                                                                                                                                                                                                                                                                                                                                                                                                                                                lua_aprservice_event_handler;
typedef AL::Lua54::Function<void(lua_aprservice* lua_service, const AL::String& sender, const AL::String& command_name, const AL::String& command_params)>                                                                                                                                                                                                                                                                                                                                                    lua_aprservice_command_handler;

typedef AL::Lua54::Function<void(lua_aprservice* lua_service)>                                                                                                                                                                                                                                                                                                                                                                                                                                                lua_aprservice_aprs_message_callback;
typedef AL::Lua54::Function<bool(lua_aprservice* lua_service, const AL::String& station, const AL::String& tocall, const AL::String& path, const AL::String& igate, const AL::String& content)>                                                                                                                                                                                                                                                                                                               lua_aprservice_aprs_packet_filter_callback;
typedef AL::Lua54::Function<void(lua_aprservice* lua_service, const AL::String& station, const AL::String& tocall, const AL::String& path, const AL::String& igate, const AL::String& content)>                                                                                                                                                                                                                                                                                                               lua_aprservice_aprs_packet_monitor_callback;

typedef AL::Lua54::Function<void(lua_aprservice* lua_service, AL::uint8 type)>                                                                                                                                                                                                                                                                                                                                                                                                                                lua_aprservice_aprs_on_connect;
typedef AL::Lua54::Function<void(lua_aprservice* lua_service, AL::uint8 reason)>                                                                                                                                                                                                                                                                                                                                                                                                                              lua_aprservice_aprs_on_disconnect;

typedef AL::Lua54::Function<void(lua_aprservice* lua_service, const AL::String& value)>                                                                                                                                                                                                                                                                                                                                                                                                                       lua_aprservice_aprs_on_send;
typedef AL::Lua54::Function<void(lua_aprservice* lua_service, const AL::String& value)>                                                                                                                                                                                                                                                                                                                                                                                                                       lua_aprservice_aprs_on_receive;

typedef AL::Lua54::Function<void(lua_aprservice* lua_service, const AL::String& station, const AL::String& tocall, const AL::String& path, const AL::String& igate, const AL::String& content)>                                                                                                                                                                                                                                                                                                               lua_aprservice_aprs_on_send_packet;
typedef AL::Lua54::Function<void(lua_aprservice* lua_service, const AL::String& station, const AL::String& tocall, const AL::String& path, const AL::String& igate, const AL::String& content)>                                                                                                                                                                                                                                                                                                               lua_aprservice_aprs_on_receive_packet;

typedef AL::Lua54::Function<void(lua_aprservice* lua_service, const AL::String& station, const AL::String& path, const AL::String& igate, const AL::String& destination, const AL::String& content)>                                                                                                                                                                                                                                                                                                          lua_aprservice_aprs_on_send_message;
typedef AL::Lua54::Function<void(lua_aprservice* lua_service, const AL::String& station, const AL::String& path, const AL::String& igate, const AL::String& destination, const AL::String& content)>                                                                                                                                                                                                                                                                                                          lua_aprservice_aprs_on_receive_message;

typedef AL::Lua54::Function<void(lua_aprservice* lua_service, const AL::String& station, const AL::String& path, const AL::String& igate, AL::uint8 month, AL::uint8 day, AL::uint8 hours, AL::uint8 minutes, AL::Float wind_speed_mph, AL::Float wind_speed_gust_mph, AL::uint16 wind_direction, AL::uint16 rainfall_last_hour_inches, AL::uint16 rainfall_last_24_hours_inches, AL::uint16 rainfall_since_midnight_inches, AL::uint8 humidity, AL::int16 temperature_f, AL::uint32 barometric_pressure_pa)> lua_aprservice_aprs_on_send_weather;
typedef AL::Lua54::Function<void(lua_aprservice* lua_service, const AL::String& station, const AL::String& path, const AL::String& igate, AL::uint8 month, AL::uint8 day, AL::uint8 hours, AL::uint8 minutes, AL::Float wind_speed_mph, AL::Float wind_speed_gust_mph, AL::uint16 wind_direction, AL::uint16 rainfall_last_hour_inches, AL::uint16 rainfall_last_24_hours_inches, AL::uint16 rainfall_since_midnight_inches, AL::uint8 humidity, AL::int16 temperature_f, AL::uint32 barometric_pressure_pa)> lua_aprservice_aprs_on_receive_weather;

typedef AL::Lua54::Function<void(lua_aprservice* lua_service, const AL::String& station, const AL::String& path, const AL::String& igate, AL::int32 altitude_ft, float latitude, float longitude, AL::Float speed_mph, AL::uint16 course, const AL::String& symbol_table, const AL::String& symbol_table_key, const AL::String& comment, AL::uint8 flags)>                                                                                                                                                    lua_aprservice_aprs_on_send_position;
typedef AL::Lua54::Function<void(lua_aprservice* lua_service, const AL::String& station, const AL::String& path, const AL::String& igate, AL::int32 altitude_ft, float latitude, float longitude, AL::Float speed_mph, AL::uint16 course, const AL::String& symbol_table, const AL::String& symbol_table_key, const AL::String& comment, AL::uint8 flags)>                                                                                                                                                    lua_aprservice_aprs_on_receive_position;

typedef AL::Lua54::Function<void(lua_aprservice* lua_service, const AL::String& station, const AL::String& tocall, const AL::String& path, const AL::String& igate, AL::uint8 analog_1, AL::uint8 analog_2, AL::uint8 analog_3, AL::uint8 analog_4, AL::uint8 analog_5, bool digital_1, bool digital_2, bool digital_3, bool digital_4, bool digital_5, bool digital_6, bool digital_7, bool digital_8)>                                                                                                      lua_aprservice_aprs_on_send_telemetry;
typedef AL::Lua54::Function<void(lua_aprservice* lua_service, const AL::String& station, const AL::String& tocall, const AL::String& path, const AL::String& igate, AL::uint8 analog_1, AL::uint8 analog_2, AL::uint8 analog_3, AL::uint8 analog_4, AL::uint8 analog_5, bool digital_1, bool digital_2, bool digital_3, bool digital_4, bool digital_5, bool digital_6, bool digital_7, bool digital_8)>                                                                                                      lua_aprservice_aprs_on_receive_telemetry;

typedef AL::Lua54::Function<void(lua_aprservice* lua_service, const AL::String& station, const AL::String& tocall, const AL::String& path, const AL::String& igate, const AL::String& content, AL::uint8 type)>                                                                                                                                                                                                                                                                                               lua_aprservice_aprs_on_receive_invalid_packet;

struct lua_aprservice_aprs_event_handlers
{
	lua_aprservice_aprs_on_connect                on_connect;
	lua_aprservice_aprs_on_disconnect             on_disconnect;

	lua_aprservice_aprs_on_send                   on_send;
	lua_aprservice_aprs_on_receive                on_receive;

	lua_aprservice_aprs_on_send_packet            on_send_packet;
	lua_aprservice_aprs_on_receive_packet         on_receive_packet;

	lua_aprservice_aprs_on_send_message           on_send_message;
	lua_aprservice_aprs_on_receive_message        on_receive_message;

	lua_aprservice_aprs_on_send_weather           on_send_weather;
	lua_aprservice_aprs_on_receive_weather        on_receive_weather;

	lua_aprservice_aprs_on_send_position          on_send_position;
	lua_aprservice_aprs_on_receive_position       on_receive_position;

	lua_aprservice_aprs_on_send_telemetry         on_send_telemetry;
	lua_aprservice_aprs_on_receive_telemetry      on_receive_telemetry;

	lua_aprservice_aprs_on_receive_invalid_packet on_receive_invalid_packet;
};

struct lua_aprservice_aprs_config
{
	lua_aprservice_aprs_event_handlers events;

	AL::String                         path;
	AL::String                         station;
	AL::String                         symbol_table;
	AL::String                         symbol_table_key;
	bool                               monitor_mode_enabled;
};

struct lua_aprservice_config
{
	lua_aprservice_aprs_config aprs;
};

struct lua_aprservice_aprs_add_packet_monitor_context
{
	lua_aprservice_aprs_packet_filter_callback  filter;
	lua_aprservice_aprs_packet_monitor_callback callback;
	lua_aprservice*                             lua_service;
};

struct lua_aprservice_aprs_begin_send_message_context
{
	lua_aprservice_aprs_message_callback callback;
	lua_aprservice*                      lua_service;
};

struct lua_aprservice_events_schedule_context
{
	lua_aprservice_event_handler handler;
	lua_aprservice*              lua_service;
};

struct lua_aprservice_commands_register_context
{
	lua_aprservice_command_handler handler;
	lua_aprservice*                lua_service;
};

typedef AL::Collections::UnorderedSet<lua_aprservice_aprs_add_packet_monitor_context*> lua_aprservice_aprs_add_packet_monitor_contexts;
typedef AL::Collections::UnorderedSet<lua_aprservice_aprs_begin_send_message_context*> lua_aprservice_aprs_begin_send_message_contexts;
typedef AL::Collections::UnorderedSet<lua_aprservice_events_schedule_context*>         lua_aprservice_events_schedule_contexts;
typedef AL::Collections::UnorderedSet<lua_aprservice_commands_register_context*>       lua_aprservice_commands_register_contexts;

struct lua_aprservice
{
	aprservice*                                     service;
	lua_aprservice_aprs_event_handlers              aprs_events;
	lua_aprservice_aprs_connection                  aprs_connection;
	lua_aprservice_aprs_add_packet_monitor_contexts aprs_add_packet_monitor_contexts;
	lua_aprservice_aprs_begin_send_message_contexts aprs_begin_send_message_contexts;
	lua_aprservice_events_schedule_contexts         events_schedule_contexts;
	lua_aprservice_commands_register_contexts       commands_register_contexts;
};

lua_aprservice*         lua_aprservice_init(lua_aprservice_config* lua_config)
{
	aprservice_aprs_on_connect on_connect_detour([](aprservice* service, AL::uint8 type, void* param)
	{
		if (auto lua_service = reinterpret_cast<lua_aprservice*>(param); lua_service->aprs_events.on_connect)
			lua_service->aprs_events.on_connect(lua_service, type);
	});
	aprservice_aprs_on_disconnect on_disconnect_detour([](aprservice* service, AL::uint8 reason, void* param)
	{
		if (auto lua_service = reinterpret_cast<lua_aprservice*>(param))
		{
			for (auto it = lua_service->aprs_begin_send_message_contexts.begin(); it != lua_service->aprs_begin_send_message_contexts.end(); )
			{ delete *it; lua_service->aprs_begin_send_message_contexts.Erase(it++); }

			if (lua_service->aprs_events.on_disconnect)
				lua_service->aprs_events.on_disconnect(lua_service, reason);
		}
	});

	aprservice_aprs_on_send on_send_detour([](aprservice* service, const AL::String& value, void* param)
	{
		if (auto lua_service = reinterpret_cast<lua_aprservice*>(param); lua_service->aprs_events.on_send)
			lua_service->aprs_events.on_send(lua_service, value);
	});
	aprservice_aprs_on_receive on_receive_detour([](aprservice* service, const AL::String& value, void* param)
	{
		if (auto lua_service = reinterpret_cast<lua_aprservice*>(param); lua_service->aprs_events.on_receive)
			lua_service->aprs_events.on_receive(lua_service, value);
	});

	aprservice_aprs_on_send_packet on_send_packet_detour([](aprservice* service, const AL::String& station, const AL::String& tocall, const AL::String& path, const AL::String& igate, const AL::String& content, void* param)
	{
		if (auto lua_service = reinterpret_cast<lua_aprservice*>(param); lua_service->aprs_events.on_send_packet)
			lua_service->aprs_events.on_send_packet(lua_service, station, tocall, path, igate, content);
	});
	aprservice_aprs_on_receive_packet on_receive_packet_detour([](aprservice* service, const AL::String& station, const AL::String& tocall, const AL::String& path, const AL::String& igate, const AL::String& content, void* param)
	{
		if (auto lua_service = reinterpret_cast<lua_aprservice*>(param); lua_service->aprs_events.on_receive_packet)
			lua_service->aprs_events.on_receive_packet(lua_service, station, tocall, path, igate, content);
	});

	aprservice_aprs_on_send_message on_send_message_detour([](aprservice* service, const AL::String& station, const AL::String& path, const AL::String& igate, const AL::String& destination, const AL::String& content, void* param)
	{
		if (auto lua_service = reinterpret_cast<lua_aprservice*>(param); lua_service->aprs_events.on_send_message)
			lua_service->aprs_events.on_send_message(lua_service, station, path, igate, destination, content);
	});
	aprservice_aprs_on_receive_message on_receive_message_detour([](aprservice* service, const AL::String& station, const AL::String& path, const AL::String& igate, const AL::String& destination, const AL::String& content, void* param)
	{
		if (auto lua_service = reinterpret_cast<lua_aprservice*>(param); lua_service->aprs_events.on_receive_message)
			lua_service->aprs_events.on_receive_message(lua_service, station, path, igate, destination, content);
	});

	aprservice_aprs_on_send_weather on_send_weather_detour([](aprservice* service, const AL::String& station, const AL::String& path, const AL::String& igate, const AL::DateTime& time, AL::Float wind_speed_mph, AL::Float wind_speed_gust_mph, AL::uint16 wind_direction, AL::uint16 rainfall_last_hour_inches, AL::uint16 rainfall_last_24_hours_inches, AL::uint16 rainfall_since_midnight_inches, AL::uint8 humidity, AL::int16 temperature_f, AL::uint32 barometric_pressure_pa, void* param)
	{
		if (auto lua_service = reinterpret_cast<lua_aprservice*>(param); lua_service->aprs_events.on_send_message)
			lua_service->aprs_events.on_send_weather(lua_service, station, path, igate, static_cast<AL::uint8>(time.Month), static_cast<AL::uint8>(time.Day), static_cast<AL::uint8>(time.Hour), static_cast<AL::uint8>(time.Minutes), wind_speed_mph, wind_speed_gust_mph, wind_direction, rainfall_last_hour_inches, rainfall_last_24_hours_inches, rainfall_since_midnight_inches, humidity, temperature_f, barometric_pressure_pa);
	});
	aprservice_aprs_on_receive_weather on_receive_weather_detour([](aprservice* service, const AL::String& station, const AL::String& path, const AL::String& igate, const AL::DateTime& time, AL::Float wind_speed_mph, AL::Float wind_speed_gust_mph, AL::uint16 wind_direction, AL::uint16 rainfall_last_hour_inches, AL::uint16 rainfall_last_24_hours_inches, AL::uint16 rainfall_since_midnight_inches, AL::uint8 humidity, AL::int16 temperature_f, AL::uint32 barometric_pressure_pa, void* param)
	{
		if (auto lua_service = reinterpret_cast<lua_aprservice*>(param); lua_service->aprs_events.on_receive_message)
			lua_service->aprs_events.on_receive_weather(lua_service, station, path, igate, static_cast<AL::uint8>(time.Month), static_cast<AL::uint8>(time.Day), static_cast<AL::uint8>(time.Hour), static_cast<AL::uint8>(time.Minutes), wind_speed_mph, wind_speed_gust_mph, wind_direction, rainfall_last_hour_inches, rainfall_last_24_hours_inches, rainfall_since_midnight_inches, humidity, temperature_f, barometric_pressure_pa);
	});

	aprservice_aprs_on_send_position on_send_position_detour([](aprservice* service, const AL::String& station, const AL::String& path, const AL::String& igate, AL::int32 altitude_ft, AL::Float latitude, AL::Float longitude, AL::Float speed_mph, AL::uint16 course, char symbol_table, char symbol_table_key, const AL::String& comment, AL::uint8 flags, void* param)
	{
		if (auto lua_service = reinterpret_cast<lua_aprservice*>(param); lua_service->aprs_events.on_send_position)
			lua_service->aprs_events.on_send_position(lua_service, station, path, igate, altitude_ft, latitude, longitude, speed_mph, course, AL::ToString(symbol_table), AL::ToString(symbol_table_key), comment, flags);
	});
	aprservice_aprs_on_receive_position on_receive_position_detour([](aprservice* service, const AL::String& station, const AL::String& path, const AL::String& igate, AL::int32 altitude_ft, AL::Float latitude, AL::Float longitude, AL::Float speed_mph, AL::uint16 course, char symbol_table, char symbol_table_key, const AL::String& comment, AL::uint8 flags, void* param)
	{
		if (auto lua_service = reinterpret_cast<lua_aprservice*>(param); lua_service->aprs_events.on_receive_position)
			lua_service->aprs_events.on_receive_position(lua_service, station, path, igate, altitude_ft, latitude, longitude, speed_mph, course, AL::ToString(symbol_table), AL::ToString(symbol_table_key), comment, flags);
	});

	aprservice_aprs_on_send_telemetry on_send_telemetry_detour([](aprservice* service, const AL::String& station, const AL::String& tocall, const AL::String& path, const AL::String& igate, const AL::uint8(&analog)[5], const bool(&digital)[8], void* param)
	{
		if (auto lua_service = reinterpret_cast<lua_aprservice*>(param); lua_service->aprs_events.on_send_telemetry)
			lua_service->aprs_events.on_send_telemetry(lua_service, station, tocall, path, igate, analog[0], analog[1], analog[2], analog[3], analog[4], digital[0], digital[1], digital[2], digital[3], digital[4], digital[5], digital[6], digital[7]);
	});
	aprservice_aprs_on_receive_telemetry on_receive_telemetry_detour([](aprservice* service, const AL::String& station, const AL::String& tocall, const AL::String& path, const AL::String& igate, const AL::uint8(&analog)[5], const bool(&digital)[8], void* param)
	{
		if (auto lua_service = reinterpret_cast<lua_aprservice*>(param); lua_service->aprs_events.on_receive_telemetry)
			lua_service->aprs_events.on_receive_telemetry(lua_service, station, tocall, path, igate, analog[0], analog[1], analog[2], analog[3], analog[4], digital[0], digital[1], digital[2], digital[3], digital[4], digital[5], digital[6], digital[7]);
	});

	aprservice_aprs_on_receive_invalid_packet on_receive_invalid_packet_detour([](aprservice* service, const AL::String& station, const AL::String& tocall, const AL::String& path, const AL::String& igate, const AL::String& content, AL::uint8 packet_type, void* param)
	{
		if (auto lua_service = reinterpret_cast<lua_aprservice*>(param); lua_service->aprs_events.on_receive_invalid_packet)
			lua_service->aprs_events.on_receive_invalid_packet(lua_service, station, tocall, path, igate, content, packet_type);
	});

	auto lua_service = new lua_aprservice
	{
		.aprs_events = lua_config->aprs.events
	};

	aprservice_config config =
	{
		.aprs =
		{
			.events =
			{
				.param                     = lua_service,

				.on_connect                = lua_config->aprs.events.on_connect ? on_connect_detour : nullptr,
				.on_disconnect             = lua_config->aprs.events.on_disconnect ? on_disconnect_detour : nullptr,

				.on_send                   = lua_config->aprs.events.on_send ? on_send_detour : nullptr,
				.on_receive                = lua_config->aprs.events.on_receive ? on_receive_detour : nullptr,

				.on_send_packet            = lua_config->aprs.events.on_send_packet ? on_send_packet_detour : nullptr,
				.on_receive_packet         = lua_config->aprs.events.on_receive_packet ? on_receive_packet_detour : nullptr,

				.on_send_message           = lua_config->aprs.events.on_send_message ? on_send_message_detour : nullptr,
				.on_receive_message        = lua_config->aprs.events.on_receive_message ? on_receive_message_detour : nullptr,

				.on_send_weather           = lua_config->aprs.events.on_send_weather ? on_send_weather_detour : nullptr,
				.on_receive_weather        = lua_config->aprs.events.on_receive_weather ? on_receive_weather_detour : nullptr,

				.on_send_position          = lua_config->aprs.events.on_send_position ? on_send_position_detour : nullptr,
				.on_receive_position       = lua_config->aprs.events.on_receive_position ? on_receive_position_detour : nullptr,

				.on_send_telemetry         = lua_config->aprs.events.on_send_telemetry ? on_send_telemetry_detour : nullptr,
				.on_receive_telemetry      = lua_config->aprs.events.on_receive_telemetry ? on_receive_telemetry_detour : nullptr,

				.on_receive_invalid_packet = lua_config->aprs.events.on_receive_invalid_packet ? on_receive_invalid_packet_detour : nullptr
			},

			.path                 = lua_config->aprs.path,
			.station              = lua_config->aprs.station,
			.symbol_table         = *lua_config->aprs.symbol_table.GetCString(),
			.symbol_table_key     = *lua_config->aprs.symbol_table_key.GetCString(),
			.monitor_mode_enabled = lua_config->aprs.monitor_mode_enabled
		}
	};

	if ((lua_service->service = aprservice_init(config)) == nullptr)
	{
		delete lua_service;

		return nullptr;
	}

	return lua_service;
}
void                    lua_aprservice_deinit(lua_aprservice* lua_service)
{
	aprservice_deinit(lua_service->service);

	for (auto it = lua_service->events_schedule_contexts.begin(); it != lua_service->events_schedule_contexts.end(); )
	{ delete *it; lua_service->events_schedule_contexts.Erase(it++); }

	for (auto it = lua_service->commands_register_contexts.begin(); it != lua_service->commands_register_contexts.end(); )
	{ delete *it; lua_service->commands_register_contexts.Erase(it++); }

	for (auto it = lua_service->aprs_begin_send_message_contexts.begin(); it != lua_service->aprs_begin_send_message_contexts.end(); )
	{ delete *it; lua_service->aprs_begin_send_message_contexts.Erase(it++); }

	for (auto it = lua_service->aprs_add_packet_monitor_contexts.begin(); it != lua_service->aprs_add_packet_monitor_contexts.end(); )
	{ delete *it; lua_service->aprs_add_packet_monitor_contexts.Erase(it++); }

	delete lua_service;
}
bool                    lua_aprservice_is_running(lua_aprservice* lua_service)
{
	return aprservice_is_running(lua_service->service);
}
void                    lua_aprservice_run(lua_aprservice* lua_service, AL::uint32 tick_rate, AL::uint8 flags)
{
	aprservice_run(lua_service->service, tick_rate, flags);
}
void                    lua_aprservice_stop(lua_aprservice* lua_service)
{
	aprservice_stop(lua_service->service);
}

void                    lua_aprservice_aprs_disconnect(lua_aprservice* lua_service);

bool                    lua_aprservice_aprs_is_connected(lua_aprservice* lua_service)
{
	return aprservice_aprs_is_connected(lua_service->service);
}
bool                    lua_aprservice_aprs_connect(lua_aprservice* lua_service, lua_aprservice_aprs_connection_is_blocking is_blocking, lua_aprservice_aprs_connection_is_connected is_connected, lua_aprservice_aprs_connection_connect connect, lua_aprservice_aprs_connection_disconnect disconnect, lua_aprservice_aprs_connection_set_blocking set_blocking, lua_aprservice_aprs_connection_read read, lua_aprservice_aprs_connection_write write)
{
	if (lua_aprservice_aprs_is_connected(lua_service))
		lua_aprservice_aprs_disconnect(lua_service);

	lua_service->aprs_connection.is_blocking  = AL::Move(is_blocking);
	lua_service->aprs_connection.is_connected = AL::Move(is_connected);
	lua_service->aprs_connection.connect      = AL::Move(connect);
	lua_service->aprs_connection.disconnect   = AL::Move(disconnect);
	lua_service->aprs_connection.set_blocking = AL::Move(set_blocking);
	lua_service->aprs_connection.read         = AL::Move(read);
	lua_service->aprs_connection.write        = AL::Move(write);

	aprservice_aprs_connection_is_blocking is_blocking_detour([](void* param)
	{
		auto lua_service = reinterpret_cast<lua_aprservice*>(param);
		return lua_service->aprs_connection.is_blocking(lua_service);
	});
	aprservice_aprs_connection_is_connected is_connected_detour([](void* param)
	{
		auto lua_service = reinterpret_cast<lua_aprservice*>(param);
		return lua_service->aprs_connection.is_connected(lua_service);
	});
	aprservice_aprs_connection_connect connect_detour([](void* param)
	{
		auto lua_service = reinterpret_cast<lua_aprservice*>(param);
		return lua_service->aprs_connection.connect(lua_service);
	});
	aprservice_aprs_connection_disconnect disconnect_detour([](void* param)
	{
		auto lua_service = reinterpret_cast<lua_aprservice*>(param);
		return lua_service->aprs_connection.disconnect(lua_service);
	});
	aprservice_aprs_connection_set_blocking set_blocking_detour([](bool set, void* param)
	{
		auto lua_service = reinterpret_cast<lua_aprservice*>(param);
		return lua_service->aprs_connection.set_blocking(lua_service, set);
	});
	aprservice_aprs_connection_read read_detour([](AL::String& value, void* param)
	{
		auto lua_service = reinterpret_cast<lua_aprservice*>(param);
		return lua_service->aprs_connection.read(lua_service, value);
	});
	aprservice_aprs_connection_write write_detour([](const AL::String& value, void* param)
	{
		auto lua_service = reinterpret_cast<lua_aprservice*>(param);
		return lua_service->aprs_connection.write(lua_service, value);
	});

	aprservice_aprs_connection connection =
	{
		.is_blocking  = is_blocking_detour,
		.is_connected = is_connected_detour,
		.connect      = connect_detour,
		.disconnect   = disconnect_detour,
		.set_blocking = set_blocking_detour,
		.read         = read_detour,
		.write        = write_detour
	};

	switch (aprservice_aprs_connect(lua_service->service, connection, lua_service))
	{
		case 0:
		case -1:
			return false;
	}

	return true;
}
bool                    lua_aprservice_aprs_connect_is(lua_aprservice* lua_service, const AL::String& remote_host, AL::uint16 remote_port, AL::uint16 passcode)
{
	if (lua_aprservice_aprs_is_connected(lua_service))
		lua_aprservice_aprs_disconnect(lua_service);

	switch (aprservice_aprs_connect_is(lua_service->service, remote_host, remote_port, passcode))
	{
		case 0:
		case -1:
			return false;
	}

	return true;
}
bool                    lua_aprservice_aprs_connect_kiss_tcp(lua_aprservice* lua_service, const AL::String& remote_host, AL::uint16 remote_port)
{
	if (lua_aprservice_aprs_is_connected(lua_service))
		lua_aprservice_aprs_disconnect(lua_service);

	switch (aprservice_aprs_connect_kiss_tcp(lua_service->service, remote_host, remote_port))
	{
		case 0:
		case -1:
			return false;
	}

	return true;
}
bool                    lua_aprservice_aprs_connect_kiss_serial(lua_aprservice* lua_service, const AL::String& device, AL::uint32 speed)
{
	if (lua_aprservice_aprs_is_connected(lua_service))
		lua_aprservice_aprs_disconnect(lua_service);

	switch (aprservice_aprs_connect_kiss_serial(lua_service->service, device, speed))
	{
		case 0:
		case -1:
			return false;
	}

	return true;
}
void                    lua_aprservice_aprs_disconnect(lua_aprservice* lua_service)
{
	aprservice_aprs_disconnect(lua_service->service);
}
void                    lua_aprservice_aprs_add_packet_monitor(lua_aprservice* lua_service, lua_aprservice_aprs_packet_filter_callback filter, lua_aprservice_aprs_packet_monitor_callback callback)
{
	auto context = new lua_aprservice_aprs_add_packet_monitor_context
	{
		.filter      = AL::Move(filter),
		.callback    = AL::Move(callback),
		.lua_service = lua_service
	};

	aprservice_aprs_packet_filter_callback filter_detour([](aprservice* service, const AL::String& station, const AL::String& tocall, const AL::String& path, const AL::String& igate, const AL::String& content, void* param)
	{
		auto context = reinterpret_cast<const lua_aprservice_aprs_add_packet_monitor_context*>(param);

		return context->filter(context->lua_service, station, tocall, path, igate, content);
	});

	aprservice_aprs_packet_monitor_callback callback_detour([](aprservice* service, const AL::String& station, const AL::String& tocall, const AL::String& path, const AL::String& igate, const AL::String& content, void* param)
	{
		auto context = reinterpret_cast<const lua_aprservice_aprs_add_packet_monitor_context*>(param);

		context->callback(context->lua_service, station, tocall, path, igate, content);
	});

	aprservice_aprs_add_packet_monitor(lua_service->service, filter_detour, callback_detour, context);
}
// @return encoding_failed, connection_closed
auto                    lua_aprservice_aprs_send_message(lua_aprservice* lua_service, const AL::String& destination, const AL::String& content)
{
	AL::Collections::Tuple<bool, bool> value(false, false);

	switch (aprservice_aprs_send_message(lua_service->service, destination, content))
	{
		case 0:  value.Set<1>(true); break;
		case -1: value.Set<0>(true); break;
	}

	return value;
}
// @return encoding_failed, connection_closed
auto                    lua_aprservice_aprs_send_weather(lua_aprservice* lua_service, AL::uint8 month, AL::uint8 day, AL::uint8 hours, AL::uint8 minutes, AL::Float wind_speed_mph, AL::Float wind_speed_gust_mph, AL::uint16 wind_direction, AL::uint16 rainfall_last_hour_inches, AL::uint16 rainfall_last_24_hours_inches, AL::uint16 rainfall_since_midnight_inches, AL::uint8 humidity, AL::int16 temperature_f, AL::uint32 barometric_pressure_pa)
{
	AL::Collections::Tuple<bool, bool> value(false, false);

	AL::DateTime time;
	time.Month   = static_cast<AL::DateTime::Months>(month);
	time.Day     = day;
	time.Hour    = hours;
	time.Minutes = minutes;

	switch (aprservice_aprs_send_weather(lua_service->service, time, wind_speed_mph, wind_speed_gust_mph, wind_direction, rainfall_last_hour_inches, rainfall_last_24_hours_inches, rainfall_since_midnight_inches, humidity, temperature_f, barometric_pressure_pa))
	{
		case 0:  value.Set<1>(true); break;
		case -1: value.Set<0>(true); break;
	}

	return value;
}
// @return encoding_failed, connection_closed
auto                    lua_aprservice_aprs_send_position(lua_aprservice* lua_service, AL::int32 altitude_ft, AL::Float latitude, AL::Float longitude, AL::uint16 speed_mph, AL::uint16 course, const AL::String& comment)
{
	AL::Collections::Tuple<bool, bool> value(false, false);

	switch (aprservice_aprs_send_position(lua_service->service, altitude_ft, latitude, longitude, speed_mph, course, comment))
	{
		case 0:  value.Set<1>(true); break;
		case -1: value.Set<0>(true); break;
	}

	return value;
}
// @return encoding_failed, connection_closed
auto                    lua_aprservice_aprs_send_telemetry(lua_aprservice* lua_service, AL::uint8 analog_1, AL::uint8 analog_2, AL::uint8 analog_3, AL::uint8 analog_4, AL::uint8 analog_5, bool digital_1, bool digital_2, bool digital_3, bool digital_4, bool digital_5, bool digital_6, bool digital_7, bool digital_8)
{
	AL::Collections::Tuple<bool, bool> value(false, false);

	switch (aprservice_aprs_send_telemetry(lua_service->service, { analog_1, analog_2, analog_3, analog_4, analog_5 }, { digital_1, digital_2, digital_3, digital_4, digital_5, digital_6, digital_7, digital_8 }))
	{
		case 0:  value.Set<1>(true); break;
		case -1: value.Set<0>(true); break;
	}

	return value;
}
// @return encoding_failed, connection_closed
auto                    lua_aprservice_aprs_begin_send_message(lua_aprservice* lua_service, const AL::String& destination, const AL::String& content, lua_aprservice_aprs_message_callback callback)
{
	AL::Collections::Tuple<bool, bool> value(false, false);

	aprservice_aprs_message_callback callback_detour([](aprservice* service, void* param)
	{
		if (auto context = reinterpret_cast<lua_aprservice_aprs_begin_send_message_context*>(param))
		{
			context->callback(context->lua_service);
			context->lua_service->aprs_begin_send_message_contexts.Remove(context);

			delete context;
		}
	});

	auto context = new lua_aprservice_aprs_begin_send_message_context
	{
		.callback    = AL::Move(callback),
		.lua_service = lua_service
	};

	switch (aprservice_aprs_begin_send_message(lua_service->service, destination, content, callback_detour, context))
	{
		case 0:  value.Set<1>(true); delete context; break;
		case -1: value.Set<0>(true); delete context; break;
	}

	lua_service->aprs_begin_send_message_contexts.Add(context);

	return value;
}

lua_aprservice_config*  lua_aprservice_config_init()
{
	return new lua_aprservice_config
	{
		.aprs =
		{
			.path                 = "WIDE1-1",
			.station              = "N0CALL",
			.symbol_table         = "/",
			.symbol_table_key     = "y",
			.monitor_mode_enabled = false
		}
	};
}
void                    lua_aprservice_config_deinit(lua_aprservice_config* lua_config)
{
	delete lua_config;
}
const AL::String&       lua_aprservice_config_get_path(lua_aprservice_config* lua_config)
{
	return lua_config->aprs.path;
}
void                    lua_aprservice_config_set_path(lua_aprservice_config* lua_config, const AL::String& value)
{
	lua_config->aprs.path = value;
}
const AL::String&       lua_aprservice_config_get_station(lua_aprservice_config* lua_config)
{
	return lua_config->aprs.station;
}
void                    lua_aprservice_config_set_station(lua_aprservice_config* lua_config, const AL::String& value)
{
	lua_config->aprs.station = value;
}
const AL::String&       lua_aprservice_config_get_symbol_table(lua_aprservice_config* lua_config)
{
	return lua_config->aprs.symbol_table;
}
void                    lua_aprservice_config_set_symbol_table(lua_aprservice_config* lua_config, const AL::String& value)
{
	lua_config->aprs.symbol_table = value;
}
const AL::String&       lua_aprservice_config_get_symbol_table_key(lua_aprservice_config* lua_config)
{
	return lua_config->aprs.symbol_table_key;
}
void                    lua_aprservice_config_set_symbol_table_key(lua_aprservice_config* lua_config, const AL::String& value)
{
	lua_config->aprs.symbol_table_key = value;
}
bool                    lua_aprservice_config_get_monitor_mode(lua_aprservice_config* lua_config)
{
	return lua_config->aprs.monitor_mode_enabled;
}
void                    lua_aprservice_config_set_monitor_mode(lua_aprservice_config* lua_config, bool value)
{
	lua_config->aprs.monitor_mode_enabled = value;
}
void                    lua_aprservice_config_events_set_on_connect(lua_aprservice_config* lua_config, lua_aprservice_aprs_on_connect handler)
{
	lua_config->aprs.events.on_connect = AL::Move(handler);
}
void                    lua_aprservice_config_events_set_on_disconnect(lua_aprservice_config* lua_config, lua_aprservice_aprs_on_disconnect handler)
{
	lua_config->aprs.events.on_disconnect = AL::Move(handler);
}
void                    lua_aprservice_config_events_set_on_send(lua_aprservice_config* lua_config, lua_aprservice_aprs_on_send handler)
{
	lua_config->aprs.events.on_send = AL::Move(handler);
}
void                    lua_aprservice_config_events_set_on_receive(lua_aprservice_config* lua_config, lua_aprservice_aprs_on_receive handler)
{
	lua_config->aprs.events.on_receive = AL::Move(handler);
}
void                    lua_aprservice_config_events_set_on_send_packet(lua_aprservice_config* lua_config, lua_aprservice_aprs_on_send_packet handler)
{
	lua_config->aprs.events.on_send_packet = AL::Move(handler);
}
void                    lua_aprservice_config_events_set_on_receive_packet(lua_aprservice_config* lua_config, lua_aprservice_aprs_on_receive_packet handler)
{
	lua_config->aprs.events.on_receive_packet = AL::Move(handler);
}
void                    lua_aprservice_config_events_set_on_send_message(lua_aprservice_config* lua_config, lua_aprservice_aprs_on_send_message handler)
{
	lua_config->aprs.events.on_send_message = AL::Move(handler);
}
void                    lua_aprservice_config_events_set_on_receive_message(lua_aprservice_config* lua_config, lua_aprservice_aprs_on_receive_message handler)
{
	lua_config->aprs.events.on_receive_message = AL::Move(handler);
}
void                    lua_aprservice_config_events_set_on_send_weather(lua_aprservice_config* lua_config, lua_aprservice_aprs_on_send_weather handler)
{
	lua_config->aprs.events.on_send_weather = AL::Move(handler);
}
void                    lua_aprservice_config_events_set_on_receive_weather(lua_aprservice_config* lua_config, lua_aprservice_aprs_on_receive_weather handler)
{
	lua_config->aprs.events.on_receive_weather = AL::Move(handler);
}
void                    lua_aprservice_config_events_set_on_send_position(lua_aprservice_config* lua_config, lua_aprservice_aprs_on_send_position handler)
{
	lua_config->aprs.events.on_send_position = AL::Move(handler);
}
void                    lua_aprservice_config_events_set_on_receive_position(lua_aprservice_config* lua_config, lua_aprservice_aprs_on_receive_position handler)
{
	lua_config->aprs.events.on_receive_position = AL::Move(handler);
}
void                    lua_aprservice_config_events_set_on_send_telemetry(lua_aprservice_config* lua_config, lua_aprservice_aprs_on_send_telemetry handler)
{
	lua_config->aprs.events.on_send_telemetry = AL::Move(handler);
}
void                    lua_aprservice_config_events_set_on_receive_telemetry(lua_aprservice_config* lua_config, lua_aprservice_aprs_on_receive_telemetry handler)
{
	lua_config->aprs.events.on_receive_telemetry = AL::Move(handler);
}
void                    lua_aprservice_config_events_set_on_receive_invalid_packet(lua_aprservice_config* lua_config, lua_aprservice_aprs_on_receive_invalid_packet handler)
{
	lua_config->aprs.events.on_receive_invalid_packet = AL::Move(handler);
}

AL::Float               lua_aprservice_math_convert_speed(AL::Float value, AL::uint8 measurement_type_input, AL::uint8 measurement_type_output)
{
	return aprservice_math_convert_speed(value, measurement_type_input, measurement_type_output);
}
AL::Float               lua_aprservice_math_get_distance_between_points(AL::Float latitude1, AL::Float longitude1, AL::Float latitude2, AL::Float longitude2, AL::uint8 measurement_type)
{
	return aprservice_math_get_distance_between_points(latitude1, longitude1, latitude2, longitude2, measurement_type);
}

AL::uint32              lua_aprservice_events_get_count(lua_aprservice* lua_service)
{
	return aprservice_events_get_count(lua_service->service);
}
void                    lua_aprservice_events_clear(lua_aprservice* lua_service)
{
	aprservice_events_clear(lua_service->service);

	for (auto it = lua_service->events_schedule_contexts.begin(); it != lua_service->events_schedule_contexts.end(); )
	{ delete *it; lua_service->events_schedule_contexts.Erase(it++); }
}
void                    lua_aprservice_events_schedule(lua_aprservice* lua_service, AL::uint32 seconds, lua_aprservice_event_handler handler)
{
	aprservice_event_handler handler_detour([](aprservice* service, void* param)
	{
		if (auto context = reinterpret_cast<lua_aprservice_events_schedule_context*>(param))
		{
			context->handler(context->lua_service);
			context->lua_service->events_schedule_contexts.Remove(context);

			delete context;
		}
	});

	auto context = new lua_aprservice_events_schedule_context
	{
		.handler     = AL::Move(handler),
		.lua_service = lua_service
	};

	lua_service->events_schedule_contexts.Add(context);
	aprservice_events_schedule(lua_service->service, AL::TimeSpan::FromSeconds(seconds), handler_detour, context);
}

bool                    lua_aprservice_console_set_title(const AL::String& value)
{
	return aprservice_console_set_title(value);
}
// @return success, char
auto                    lua_aprservice_console_read()
{
	AL::Collections::Tuple<bool, AL::String> value(false, AL::String(AL::String::END, 2));
	value.Set<0>(AL::OS::Console::Read(value.Get<1>()[0]));

	return value;
}
// @return success, string
auto                    lua_aprservice_console_read_line()
{
	AL::Collections::Tuple<bool, AL::String> value;
	value.Set<0>(AL::OS::Console::ReadLine(value.Get<1>()));

	return value;
}
bool                    lua_aprservice_console_write(const AL::String& value)
{
	return aprservice_console_write(value);
}
bool                    lua_aprservice_console_write_line(const AL::String& value)
{
	return aprservice_console_write_line(value);
}
bool                    lua_aprservice_console_write_exception(const AL::Exception& value)
{
	return aprservice_console_write_exception(value);
}

bool                    lua_aprservice_commands_execute(lua_aprservice* lua_service, const AL::String& sender, const AL::String& message)
{
	return aprservice_commands_execute(lua_service->service, sender, message);
}
void                    lua_aprservice_commands_register(lua_aprservice* lua_service, const AL::String& name, lua_aprservice_command_handler handler)
{
	aprservice_command_handler handler_detour([](aprservice* service, const AL::String& sender, const AL::String& command_name, const AL::String& command_params, void* param)
	{
		if (auto context = reinterpret_cast<lua_aprservice_commands_register_context*>(param))
		{
			context->handler(context->lua_service, sender, command_name, command_params);
			context->lua_service->commands_register_contexts.Remove(context);

			delete context;
		}
	});

	auto context = new lua_aprservice_commands_register_context
	{
		.handler     = AL::Move(handler),
		.lua_service = lua_service
	};

	lua_service->commands_register_contexts.Add(context);
	aprservice_commands_register(lua_service->service, name, handler_detour, context);
}

#define               aprservice_lua_register_global(lua, value)                      aprservice_lua_state_register_global((&lua->state), value)
#define               aprservice_lua_register_global_ex(lua, value, name)             aprservice_lua_state_register_global_ex((&lua->state), value, name)
#define               aprservice_lua_register_global_function(lua, function)          aprservice_lua_state_register_global_function((&lua->state), function)
#define               aprservice_lua_register_global_function_ex(lua, function, name) aprservice_lua_state_register_global_function_ex((&lua->state), function, name)
void                  aprservice_lua_register_globals(aprservice_lua* lua)
{
	aprservice_lua_register_global(lua, APRSERVICE_FLAG_NONE);
	aprservice_lua_register_global(lua, APRSERVICE_FLAG_STOP_ON_APRS_DISCONNECT);

	aprservice_lua_register_global(lua, APRSERVICE_MEASUREMENT_TYPE_FEET);
	aprservice_lua_register_global(lua, APRSERVICE_MEASUREMENT_TYPE_MILES);
	aprservice_lua_register_global(lua, APRSERVICE_MEASUREMENT_TYPE_METERS);
	aprservice_lua_register_global(lua, APRSERVICE_MEASUREMENT_TYPE_KILOMETERS);

	aprservice_lua_register_global(lua, APRSERVICE_APRS_PACKET_TYPE_UNKNOWN);
	aprservice_lua_register_global(lua, APRSERVICE_APRS_PACKET_TYPE_MESSAGE);
	aprservice_lua_register_global(lua, APRSERVICE_APRS_PACKET_TYPE_WEATHER);
	aprservice_lua_register_global(lua, APRSERVICE_APRS_PACKET_TYPE_POSITION);
	aprservice_lua_register_global(lua, APRSERVICE_APRS_PACKET_TYPE_TELEMETRY);

	aprservice_lua_register_global(lua, APRSERVICE_APRS_POSITION_FLAG_NONE);
	aprservice_lua_register_global(lua, APRSERVICE_APRS_POSITION_FLAG_COMPRESSED);
	aprservice_lua_register_global(lua, APRSERVICE_APRS_POSITION_FLAG_MESSAGING_ENABLED);

	aprservice_lua_register_global(lua, APRSERVICE_APRS_CONNECTION_TYPE_NONE);
	aprservice_lua_register_global(lua, APRSERVICE_APRS_CONNECTION_TYPE_APRS_IS);
	aprservice_lua_register_global(lua, APRSERVICE_APRS_CONNECTION_TYPE_KISS_TCP);
	aprservice_lua_register_global(lua, APRSERVICE_APRS_CONNECTION_TYPE_KISS_SERIAL);
	aprservice_lua_register_global(lua, APRSERVICE_APRS_CONNECTION_TYPE_USER_DEFINED);

	aprservice_lua_register_global(lua, APRSERVICE_APRS_DISCONNECT_REASON_UNDEFINED);
	aprservice_lua_register_global(lua, APRSERVICE_APRS_DISCONNECT_REASON_USER_REQUESTED);
	aprservice_lua_register_global(lua, APRSERVICE_APRS_DISCONNECT_REASON_CONNECTION_LOST);
	aprservice_lua_register_global(lua, APRSERVICE_APRS_DISCONNECT_REASON_AUTHENTICATION_FAILED);

	aprservice_lua_register_global_function_ex(lua, lua_aprservice_init,                                        "aprservice_init");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_deinit,                                      "aprservice_deinit");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_is_running,                                  "aprservice_is_running");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_run,                                         "aprservice_run");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_stop,                                        "aprservice_stop");

	aprservice_lua_register_global_function_ex(lua, lua_aprservice_aprs_is_connected,                           "aprservice_aprs_is_connected");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_aprs_connect,                                "aprservice_aprs_connect");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_aprs_connect_is,                             "aprservice_aprs_connect_is");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_aprs_connect_kiss_tcp,                       "aprservice_aprs_connect_kiss_tcp");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_aprs_connect_kiss_serial,                    "aprservice_aprs_connect_kiss_serial");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_aprs_disconnect,                             "aprservice_aprs_disconnect");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_aprs_add_packet_monitor,                     "aprservice_aprs_add_packet_monitor");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_aprs_send_message,                           "aprservice_aprs_send_message");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_aprs_send_weather,                           "aprservice_aprs_send_weather");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_aprs_send_position,                          "aprservice_aprs_send_position");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_aprs_send_telemetry,                         "aprservice_aprs_send_telemetry");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_aprs_begin_send_message,                     "aprservice_aprs_begin_send_message");

	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_init,                                 "aprservice_config_init");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_deinit,                               "aprservice_config_deinit");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_get_path,                             "aprservice_config_get_path");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_set_path,                             "aprservice_config_set_path");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_get_station,                          "aprservice_config_get_station");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_set_station,                          "aprservice_config_set_station");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_get_symbol_table,                     "aprservice_config_get_symbol_table");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_set_symbol_table,                     "aprservice_config_set_symbol_table");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_get_symbol_table_key,                 "aprservice_config_get_symbol_table_key");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_set_symbol_table_key,                 "aprservice_config_set_symbol_table_key");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_get_monitor_mode,                     "aprservice_config_get_monitor_mode");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_set_monitor_mode,                     "aprservice_config_set_monitor_mode");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_events_set_on_connect,                "aprservice_config_events_set_on_connect");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_events_set_on_disconnect,             "aprservice_config_events_set_on_disconnect");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_events_set_on_send,                   "aprservice_config_events_set_on_send");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_events_set_on_receive,                "aprservice_config_events_set_on_receive");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_events_set_on_send_packet,            "aprservice_config_events_set_on_send_packet");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_events_set_on_receive_packet,         "aprservice_config_events_set_on_receive_packet");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_events_set_on_send_message,           "aprservice_config_events_set_on_send_message");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_events_set_on_receive_message,        "aprservice_config_events_set_on_receive_message");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_events_set_on_send_weather,           "aprservice_config_events_set_on_send_weather");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_events_set_on_receive_weather,        "aprservice_config_events_set_on_receive_weather");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_events_set_on_send_position,          "aprservice_config_events_set_on_send_position");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_events_set_on_receive_position,       "aprservice_config_events_set_on_receive_position");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_events_set_on_send_telemetry,         "aprservice_config_events_set_on_send_telemetry");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_events_set_on_receive_telemetry,      "aprservice_config_events_set_on_receive_telemetry");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_config_events_set_on_receive_invalid_packet, "aprservice_config_events_set_on_receive_invalid_packet");

	aprservice_lua_register_global_function_ex(lua, lua_aprservice_math_convert_speed,                          "aprservice_math_convert_speed");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_math_get_distance_between_points,            "aprservice_math_get_distance_between_points");

	aprservice_lua_register_global_function_ex(lua, lua_aprservice_events_get_count,                            "aprservice_events_get_count");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_events_clear,                                "aprservice_events_clear");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_events_schedule,                             "aprservice_events_schedule");

	aprservice_lua_register_global_function_ex(lua, lua_aprservice_console_set_title,                           "aprservice_console_set_title");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_console_read,                                "aprservice_console_read");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_console_write,                               "aprservice_console_write");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_console_write_line,                          "aprservice_console_write_line");

	aprservice_lua_register_global_function_ex(lua, lua_aprservice_commands_execute,                            "aprservice_commands_execute");
	aprservice_lua_register_global_function_ex(lua, lua_aprservice_commands_register,                           "aprservice_commands_register");
}

void                  aprservice_lua_modules_register_globals(aprservice_lua* lua);

aprservice_lua*       aprservice_lua_init()
{
	auto lua = new aprservice_lua
	{
	};

	try
	{
		lua->state.Create();
	}
	catch (const AL::Exception& exception)
	{
		delete lua;

		aprservice_console_write_line("Error creating AL::Lua54::Lua");
		aprservice_console_write_exception(exception);

		return nullptr;
	}

	try
	{
		lua->state.LoadLibrary(AL::Lua54::Libraries::All);
	}
	catch (const AL::Exception& exception)
	{
		lua->state.Destroy();

		delete lua;

		aprservice_console_write_line("Error loading AL::Lua54::Lua libraries");
		aprservice_console_write_exception(exception);

		return nullptr;
	}

	aprservice_lua_register_globals(lua);
	aprservice_lua_modules_register_globals(lua);

	return lua;
}
void                  aprservice_lua_deinit(aprservice_lua* lua)
{
	lua->state.Destroy();

	delete lua;
}

aprservice_lua_state* aprservice_lua_get_state(aprservice_lua* lua)
{
	return &lua->state;
}

bool                  aprservice_lua_run(aprservice_lua* lua, const AL::String& script)
{
	try
	{
		lua->state.Run(script);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error running script");
		aprservice_console_write_exception(exception);

		return false;
	}

	return true;
}
bool                  aprservice_lua_run_file(aprservice_lua* lua, const AL::String& script_path)
{
	try
	{
		if (!lua->state.RunFile(script_path))
			throw AL::Exception("File not found");
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line(AL::String::Format("Error running script [Path: %s]", script_path.GetCString()));
		aprservice_console_write_exception(exception);

		return false;
	}

	return true;
}
