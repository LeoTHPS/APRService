#include "aprservice.hpp"

#include <AL/Game/Loop.hpp>

struct aprservice_aprs;
struct aprservice_events;
struct aprservice_commands;

struct aprservice
{
	bool                        is_running;
	bool                        is_stopping;

	aprservice_aprs*            aprs;
	aprservice_config           config;
	aprservice_events*          events;
	aprservice_commands*        commands;
	AL::uint8                   runtime_flags;
};

aprservice_aprs* aprservice_aprs_init(aprservice* service, const aprservice_aprs_config& config);
void             aprservice_aprs_deinit(aprservice_aprs* aprs);
bool             aprservice_aprs_is_connected(aprservice_aprs* aprs);
bool             aprservice_aprs_connect_is(aprservice_aprs* aprs, const AL::String& remote_host, AL::uint16 remote_port, AL::uint16 passcode);
bool             aprservice_aprs_connect_kiss_tcp(aprservice_aprs* aprs, const AL::String& remote_host, AL::uint16 remote_port);
bool             aprservice_aprs_connect_kiss_serial(aprservice_aprs* aprs, const AL::String& device, AL::uint32 speed);
void             aprservice_aprs_disconnect(aprservice_aprs* aprs);
bool             aprservice_aprs_update(aprservice_aprs* aprs, AL::TimeSpan delta);
void             aprservice_aprs_add_packet_monitor(aprservice_aprs* aprs, aprservice_aprs_packet_filter_callback filter, aprservice_aprs_packet_monitor_callback callback, void* param);
// @return 0 on connection closed
// @return -1 on encoding error
int              aprservice_aprs_send_message(aprservice_aprs* aprs, const AL::String& destination, const AL::String& content);
// @return 0 on connection closed
// @return -1 on encoding error
int              aprservice_aprs_send_position(aprservice_aprs* aprs, AL::int32 altitude, AL::Float latitude, AL::Float longitude, const AL::String& comment);
// @return 0 on connection closed
// @return -1 on encoding error
int              aprservice_aprs_send_telemetry(aprservice_aprs* aprs, const AL::uint8(&analog)[5], const bool(&digital)[8]);
// @return 0 on connection closed
// @return -1 on encoding error
int              aprservice_aprs_begin_send_message(aprservice_aprs* aprs, const AL::String& destination, const AL::String& content, aprservice_aprs_message_callback callback, void* param);

aprservice_events* aprservice_events_init(aprservice* service);
void               aprservice_events_deinit(aprservice_events* events);
AL::uint32         aprservice_events_get_count(aprservice_events* events);
void               aprservice_events_clear(aprservice_events* events);
bool               aprservice_events_update(aprservice_events* events, AL::TimeSpan delta);
void               aprservice_events_schedule(aprservice_events* events, AL::TimeSpan delay, aprservice_event_handler handler, void* param);

aprservice_commands* aprservice_commands_init(aprservice* service);
void                 aprservice_commands_deinit(aprservice_commands* commands);
bool                 aprservice_commands_execute(aprservice_commands* commands, const AL::String& sender, const AL::String& message);
void                 aprservice_commands_register(aprservice_commands* commands, const AL::String& name, aprservice_command_handler handler, void* param);

aprservice* aprservice_init(const aprservice_config& config)
{
	auto service = new aprservice
	{
		.is_running  = false,
		.is_stopping = false,

		.config      = config
	};

	if (!(service->aprs = aprservice_aprs_init(service, service->config.aprs)))
	{
		delete service;

		return nullptr;
	}

	if (!(service->events = aprservice_events_init(service)))
	{
		aprservice_aprs_deinit(service->aprs);

		delete service;

		return nullptr;
	}

	if (!(service->commands = aprservice_commands_init(service)))
	{
		aprservice_events_deinit(service->events);
		aprservice_aprs_deinit(service->aprs);

		delete service;

		return nullptr;
	}

	aprservice_events_init(service);

	return service;
}
void        aprservice_deinit(aprservice* service)
{
	aprservice_aprs_deinit(service->aprs);
	aprservice_events_deinit(service->events);
	aprservice_commands_deinit(service->commands);

	delete service;
}
bool        aprservice_is_running(aprservice* service)
{
	return service->is_running;
}
bool        aprservice_run_once(aprservice* service, AL::TimeSpan delta, AL::uint8 flags)
{
	if (!aprservice_events_update(service->events, delta))
		return false;

	if (aprservice_aprs_is_connected(service) && !aprservice_aprs_update(service->aprs, delta))
		if ((flags & APRSERVICE_FLAG_STOP_ON_APRS_DISCONNECT) == APRSERVICE_FLAG_STOP_ON_APRS_DISCONNECT)
			return false;

	return true;
}
void        aprservice_run(aprservice* service, AL::uint32 tick_rate, AL::uint8 flags)
{
	if (!service->is_running)
	{
		service->runtime_flags = flags;
		service->is_running    = true;

		AL::Game::Loop::Run(tick_rate, [service, flags](AL::TimeSpan delta)
		{
			return !service->is_stopping && aprservice_run_once(service, delta, flags);
		});

		service->is_running = false;
	}
}
void        aprservice_stop(aprservice* service)
{
	if (service->is_running)
		service->is_stopping = true;
}

bool        aprservice_aprs_is_connected(aprservice* service)
{
	return aprservice_aprs_is_connected(service->aprs);
}
bool        aprservice_aprs_connect_is(aprservice* service, const AL::String& remote_host, AL::uint16 remote_port, AL::uint16 passcode)
{
	return aprservice_aprs_connect_is(service->aprs, remote_host, remote_port, passcode);
}
bool        aprservice_aprs_connect_kiss_tcp(aprservice* service, const AL::String& remote_host, AL::uint16 remote_port)
{
	return aprservice_aprs_connect_kiss_tcp(service->aprs, remote_host, remote_port);
}
bool        aprservice_aprs_connect_kiss_serial(aprservice* service, const AL::String& device, AL::uint32 speed)
{
	return aprservice_aprs_connect_kiss_serial(service->aprs, device, speed);
}
void        aprservice_aprs_disconnect(aprservice* service)
{
	aprservice_aprs_disconnect(service->aprs);
}
void        aprservice_aprs_add_packet_monitor(aprservice* service, aprservice_aprs_packet_filter_callback filter, aprservice_aprs_packet_monitor_callback callback, void* param)
{
	aprservice_aprs_add_packet_monitor(service->aprs, filter, callback, param);
}
// @return 0 on connection closed
// @return -1 on encoding error
int         aprservice_aprs_send_message(aprservice* service, const AL::String& destination, const AL::String& content)
{
	return aprservice_aprs_send_message(service->aprs, destination, content);
}
// @return 0 on connection closed
// @return -1 on encoding error
int         aprservice_aprs_send_position(aprservice* service, AL::int32 altitude, AL::Float latitude, AL::Float longitude, const AL::String& comment)
{
	return aprservice_aprs_send_position(service->aprs, altitude, latitude, longitude, comment);
}
// @return 0 on connection closed
// @return -1 on encoding error
int         aprservice_aprs_send_telemetry(aprservice* service, const AL::uint8(&analog)[5], const bool(&digital)[8])
{
	return aprservice_aprs_send_telemetry(service->aprs, analog, digital);
}
// @return 0 on connection closed
// @return -1 on encoding error
int         aprservice_aprs_begin_send_message(aprservice* service, const AL::String& destination, const AL::String& content, aprservice_aprs_message_callback callback, void* param)
{
	return aprservice_aprs_begin_send_message(service->aprs, destination, content, callback, param);
}

AL::uint32  aprservice_events_get_count(aprservice* service)
{
	return aprservice_events_get_count(service->events);
}
void        aprservice_events_clear(aprservice* service)
{
	aprservice_events_clear(service->events);
}
void        aprservice_events_schedule(aprservice* service, AL::TimeSpan delay, aprservice_event_handler handler, void* param)
{
	aprservice_events_schedule(service->events, delay, handler, param);
}

bool        aprservice_commands_execute(aprservice* service, const AL::String& sender, const AL::String& message)
{
	return aprservice_commands_execute(service->commands, sender, message);
}
void        aprservice_commands_register(aprservice* service, const AL::String& name, aprservice_command_handler handler, void* param)
{
	aprservice_commands_register(service->commands, name, handler, param);
}
