#include "aprservice.hpp"

#include <AL/APRS/Client.hpp>

#include <AL/Network/DNS.hpp>

struct aprservice_aprs
{
	bool             is_connected;

	aprservice*      service;

	AL::APRS::Client client;

	AL::String       is_filter;

	AL::uint32       message_counter;   // XXXXX
	AL::uint16       telemetry_counter; // 999-0
};

AL::String aprservice_aprs_convert_path(const AL::APRS::DigiPath& value)
{
	AL::StringBuilder sb;

	for (AL::size_t i = 0, j = 0; i < value.GetSize(); ++i)
	{
		if (value[i].GetLength() == 0)
			break;

		if (j++ != 0)
			sb << ',';

		sb << value[i];
	}

	return sb.ToString();
}
AL::uint8  aprservice_aprs_convert_packet_type(AL::APRS::PacketTypes value)
{
	switch (value)
	{
		case AL::Serialization::APRS::PacketTypes::Unknown:   return APRSERVICE_APRS_PACKET_TYPE_UNKNOWN;
		case AL::Serialization::APRS::PacketTypes::Message:   return APRSERVICE_APRS_PACKET_TYPE_MESSAGE;
		case AL::Serialization::APRS::PacketTypes::Position:  return APRSERVICE_APRS_PACKET_TYPE_POSITION;
		case AL::Serialization::APRS::PacketTypes::Telemetry: return APRSERVICE_APRS_PACKET_TYPE_TELEMETRY;
	}

	return APRSERVICE_APRS_PACKET_TYPE_UNKNOWN;
}
AL::uint8  aprservice_aprs_convert_position_flags(const AL::APRS::Position& value)
{
	AL::uint8 flags = APRSERVICE_APRS_POSITION_FLAG_NONE;

	if (value.IsMessagingEnabled())
		flags |= APRSERVICE_APRS_POSITION_FLAG_MESSAGING_ENABLED;

	if (value.IsCompressionEnabled())
		flags |= APRSERVICE_APRS_POSITION_FLAG_COMPRESSED;

	return flags;
}
AL::uint8  aprservice_aprs_convert_connection_type(AL::APRS::ClientConnectionTypes value)
{
	switch (value)
	{
		case AL::APRS::ClientConnectionTypes::None:        return APRSERVICE_APRS_CONNECTION_TYPE_NONE;
		case AL::APRS::ClientConnectionTypes::APRS_IS:     return APRSERVICE_APRS_CONNECTION_TYPE_APRS_IS;
		case AL::APRS::ClientConnectionTypes::KISS_Tcp:    return APRSERVICE_APRS_CONNECTION_TYPE_KISS_TCP;
		case AL::APRS::ClientConnectionTypes::KISS_Serial: return APRSERVICE_APRS_CONNECTION_TYPE_KISS_SERIAL;
	}

	return APRSERVICE_APRS_CONNECTION_TYPE_NONE;
}
AL::uint8  aprservice_aprs_convert_disconnect_reason(AL::APRS::ClientDisconnectReasons value)
{
	switch (value)
	{
		case AL::APRS::ClientDisconnectReasons::UserRequested:        return APRSERVICE_APRS_DISCONNECT_REASON_USER_REQUESTED;
		case AL::APRS::ClientDisconnectReasons::ConnectionLost:       return APRSERVICE_APRS_DISCONNECT_REASON_CONNECTION_LOST;
		case AL::APRS::ClientDisconnectReasons::AuthenticationFailed: return APRSERVICE_APRS_DISCONNECT_REASON_AUTHENTICATION_FAILED;
	}

	return APRSERVICE_APRS_DISCONNECT_REASON_UNDEFINED;
}

bool       aprservice_aprs_is_connected(aprservice_aprs* aprs);
AL::String aprservice_aprs_build_is_filter(const aprservice_aprs_config& config)
{
	AL::StringBuilder sb;

	if (config.events.on_receive_message || config.events.on_receive_position || config.events.on_receive_telemetry)
	{
		sb << "t/";

		if (config.events.on_receive_message)
			sb << 'm';

		if (config.events.on_receive_position)
			sb << 'p';

		if (config.events.on_receive_telemetry)
			sb << 't';
	}

	return sb.ToString();
}
void       aprservice_aprs_register_events(aprservice_aprs* aprs, const aprservice_aprs_event_handlers& events)
{
	if (events.on_connect)                aprs->client.OnConnect.Register([aprs, &events](AL::APRS::ClientConnectionTypes connection_type) { events.on_connect(aprs->service, aprservice_aprs_convert_connection_type(connection_type), events.param); });
	if (events.on_disconnect)             aprs->client.OnDisconnect.Register([aprs, &events](AL::APRS::ClientDisconnectReasons disconnect_reason) { events.on_disconnect(aprs->service, aprservice_aprs_convert_disconnect_reason(disconnect_reason), events.param); });

	if (events.on_send)                   aprs->client.OnSend.Register([aprs, &events](const AL::String& value) { events.on_send(aprs->service, value, events.param); });
	if (events.on_receive)                aprs->client.OnReceive.Register([aprs, &events](const AL::String& value) { events.on_receive(aprs->service, value, events.param); });

	if (events.on_send_packet)            aprs->client.OnSendPacket.Register([aprs, &events](const AL::APRS::Packet& value) { events.on_send_packet(aprs->service, value.GetSender(), value.GetToCall(), aprservice_aprs_convert_path(value.GetPath()), value.GetContent(), events.param); });
	if (events.on_receive_packet)         aprs->client.OnReceivePacket.Register([aprs, &events](const AL::APRS::Packet& value) { events.on_receive_packet(aprs->service, value.GetSender(), value.GetToCall(), aprservice_aprs_convert_path(value.GetPath()), value.GetContent(), events.param); });

	if (events.on_send_message)           aprs->client.OnSendMessage.Register([aprs, &events](const AL::APRS::Message& value) { events.on_send_message(aprs->service, value.GetSender(), aprservice_aprs_convert_path(value.GetPath()), value.GetDestination(), value.GetContent(), events.param); });
	if (events.on_receive_message)        aprs->client.OnReceiveMessage.Register([aprs, &events](const AL::APRS::Message& value) { if (!value.GetDestination().Compare(aprs->client.GetStation(), AL::True)) events.on_receive_message(aprs->service, value.GetSender(), aprservice_aprs_convert_path(value.GetPath()), value.GetDestination(), value.GetContent(), events.param); else if (!aprservice_commands_execute(aprs->service, value.GetSender(), value.GetContent())) events.on_receive_message(aprs->service, value.GetSender(), aprservice_aprs_convert_path(value.GetPath()), value.GetDestination(), value.GetContent(), events.param); });

	if (events.on_send_position)          aprs->client.OnSendPosition.Register([aprs, &events](const AL::APRS::Position& value) { events.on_send_position(aprs->service, value.GetSender(), aprservice_aprs_convert_path(value.GetPath()), value.GetAltitude(), value.GetLatitude(), value.GetLongitude(), value.GetSymbolTable(), value.GetSymbolTableKey(), value.GetComment(), aprservice_aprs_convert_position_flags(value), events.param); });
	if (events.on_receive_position)       aprs->client.OnReceivePosition.Register([aprs, &events](const AL::APRS::Position& value) { events.on_receive_position(aprs->service, value.GetSender(), aprservice_aprs_convert_path(value.GetPath()), value.GetAltitude(), value.GetLatitude(), value.GetLongitude(), value.GetSymbolTable(), value.GetSymbolTableKey(), value.GetComment(), aprservice_aprs_convert_position_flags(value), events.param); });

	if (events.on_send_telemetry)         aprs->client.OnSendTelemetry.Register([aprs, &events](const AL::APRS::Telemetry& value) { AL::uint8 a[5]; bool d[8]; value.GetValues(a, d); events.on_send_telemetry(aprs->service, value.GetSender(), value.GetToCall(), aprservice_aprs_convert_path(value.GetPath()), a, d, events.param); });
	if (events.on_receive_telemetry)      aprs->client.OnReceiveTelemetry.Register([aprs, &events](const AL::APRS::Telemetry& value) { AL::uint8 a[5]; bool d[8]; value.GetValues(a, d); events.on_receive_telemetry(aprs->service, value.GetSender(), value.GetToCall(), aprservice_aprs_convert_path(value.GetPath()), a, d, events.param); });

	if (events.on_receive_invalid_packet) aprs->client.OnReceiveInvalidPacket.Register([aprs, &events](const AL::APRS::Packet& packet) { events.on_receive_invalid_packet(aprs->service, packet.GetSender(), packet.GetToCall(), aprservice_aprs_convert_path(packet.GetPath()), packet.GetContent(), aprservice_aprs_convert_packet_type(packet.GetType()), events.param); });
}
void       aprservice_aprs_disconnect(aprservice_aprs* aprs);

aprservice_aprs* aprservice_aprs_init(aprservice* service, const aprservice_aprs_config& config)
{
	AL::APRS::DigiPath path;

	if (!AL::APRS::DigiPath_FromString(path, config.path))
	{
		aprservice_console_write_line("Error parsing path");

		return nullptr;
	}

	auto aprs = new aprservice_aprs
	{
		.is_connected      = false,

		.service           = service,

		.client            = AL::APRS::Client(AL::String(config.station), AL::Move(path), config.symbol_table, config.symbol_table_key),

		.is_filter         = aprservice_aprs_build_is_filter(config),

		.message_counter   = 0,
		.telemetry_counter = 1000
	};

	aprservice_aprs_register_events(aprs, config.events);
	aprs->client.SetBlocking(AL::False);
	aprs->client.EnableMonitorMode(config.monitor_mode_enabled);

	return aprs;
}
void             aprservice_aprs_deinit(aprservice_aprs* aprs)
{
	if (aprservice_aprs_is_connected(aprs))
		aprservice_aprs_disconnect(aprs);

	delete aprs;
}
bool             aprservice_aprs_is_connected(aprservice_aprs* aprs)
{
	return aprs->is_connected;
}
bool             aprservice_aprs_connect_is(aprservice_aprs* aprs, const AL::String& remote_host, AL::uint16 remote_port, AL::uint16 passcode)
{
	if (aprservice_aprs_is_connected(aprs))
		aprservice_aprs_disconnect(aprs);

	AL::Network::IPEndPoint remote_end_point =
	{
		.Port = remote_port
	};

	try
	{
		if (!AL::Network::DNS::Resolve(remote_end_point.Host, remote_host))
			throw AL::Exception("Host '%s' not found", remote_host.GetCString());
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line(AL::String::Format("Error resolving remote host '%s'", remote_host.GetCString()));
		aprservice_console_write_exception(exception);

		return false;
	}

	try
	{
		if (!aprs->client.ConnectIS(remote_end_point, passcode, aprs->is_filter))
			throw AL::Exception("Connection timed out");
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error connecting AL::APRS::Client to APRS-IS");
		aprservice_console_write_exception(exception);

		return false;
	}

	aprs->is_connected = true;

	return true;
}
bool             aprservice_aprs_connect_kiss_tcp(aprservice_aprs* aprs, const AL::String& remote_host, AL::uint16 remote_port)
{
	if (aprservice_aprs_is_connected(aprs))
		aprservice_aprs_disconnect(aprs);

	AL::Network::IPEndPoint remote_end_point =
	{
		.Port = remote_port
	};

	try
	{
		if (!AL::Network::DNS::Resolve(remote_end_point.Host, remote_host))
			throw AL::Exception("Host '%s' not found", remote_host.GetCString());
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line(AL::String::Format("Error resolving remote host '%s'", remote_host.GetCString()));
		aprservice_console_write_exception(exception);

		return false;
	}

	try
	{
		if (!aprs->client.ConnectKISS(remote_end_point))
			throw AL::Exception("Connection timed out");
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error connecting AL::APRS::Client to tcp KISS TNC");
		aprservice_console_write_exception(exception);

		return false;
	}

	aprs->is_connected = true;

	return true;
}
bool             aprservice_aprs_connect_kiss_serial(aprservice_aprs* aprs, const AL::String& device)
{
	if (aprservice_aprs_is_connected(aprs))
		aprservice_aprs_disconnect(aprs);

	try
	{
		if (!aprs->client.ConnectKISS(device))
			throw AL::Exception("Connection timed out");
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error connecting AL::APRS::Client to serial KISS TNC");
		aprservice_console_write_exception(exception);

		return false;
	}

	aprs->is_connected = true;

	return true;
}
void             aprservice_aprs_disconnect(aprservice_aprs* aprs)
{
	if (aprservice_aprs_is_connected(aprs))
	{
		aprs->client.Disconnect();

		aprs->is_connected = false;
	}
}
bool             aprservice_aprs_update(aprservice_aprs* aprs, AL::TimeSpan delta)
{
	if (!aprservice_aprs_is_connected(aprs))
		return false;

	try
	{
		if (!aprs->client.Update(delta))
		{
			aprservice_aprs_disconnect(aprs);

			return false;
		}
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error updating AL::APRS::Client");
		aprservice_console_write_exception(exception);

		if (!aprs->client.IsConnected())
		{
			aprservice_aprs_disconnect(aprs);

			return false;
		}
	}

	return true;
}
// @return 0 on connection closed
// @return -1 on encoding error
int              aprservice_aprs_send_message(aprservice_aprs* aprs, const AL::String& destination, const AL::String& content)
{
	if (!aprservice_aprs_is_connected(aprs))
		return 0;

	try
	{
		if (!aprs->client.SendMessage(destination, content))
		{
			aprservice_aprs_disconnect(aprs);

			return 0;
		}
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error sending message");
		aprservice_console_write_exception(exception);

		if (!aprs->client.IsConnected())
		{
			aprservice_aprs_disconnect(aprs);

			return 0;
		}

		return -1;
	}

	return 1;
}
// @return 0 on connection closed
// @return -1 on encoding error
int              aprservice_aprs_send_position(aprservice_aprs* aprs, AL::int32 altitude, AL::Float latitude, AL::Float longitude, const AL::String& comment)
{
	if (!aprservice_aprs_is_connected(aprs))
		return 0;

	try
	{
		if (!aprs->client.SendPosition(altitude, latitude, longitude, comment))
		{
			aprservice_aprs_disconnect(aprs);

			return 0;
		}
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error sending position");
		aprservice_console_write_exception(exception);

		if (!aprs->client.IsConnected())
		{
			aprservice_aprs_disconnect(aprs);

			return 0;
		}

		return -1;
	}

	return 1;
}
// @return 0 on connection closed
// @return -1 on encoding error
int              aprservice_aprs_send_telemetry(aprservice_aprs* aprs, const AL::uint8(&analog)[5], const bool(&digital)[8])
{
	if (!aprservice_aprs_is_connected(aprs))
		return 0;

	if (aprs->telemetry_counter-- == 0)
		aprs->telemetry_counter = 999;

	try
	{
		if (!aprs->client.SendTelemetry(analog, digital, aprs->telemetry_counter))
		{
			aprservice_aprs_disconnect(aprs);

			return 0;
		}
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error sending telemetry");
		aprservice_console_write_exception(exception);

		if (!aprs->client.IsConnected())
		{
			aprservice_aprs_disconnect(aprs);

			return 0;
		}

		return -1;
	}

	return 1;
}
// @return 0 on connection closed
// @return -1 on encoding error
int              aprservice_aprs_begin_send_message(aprservice_aprs* aprs, const AL::String& destination, const AL::String& content, aprservice_aprs_message_callback callback, void* param)
{
	if (!aprservice_aprs_is_connected(aprs))
		return 0;

	if (aprs->message_counter++ > 0xFFFFF)
		aprs->message_counter = 0;

	AL::APRS::ClientMessageCallback callback_detour([aprs, callback, param]()
	{
		callback(aprs->service, param);
	});

	try
	{
		if (!aprs->client.SendMessage(destination, content, AL::ToString(aprs->message_counter), AL::Move(callback_detour)))
		{
			aprservice_aprs_disconnect(aprs);

			return 0;
		}
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error sending message");
		aprservice_console_write_exception(exception);

		if (!aprs->client.IsConnected())
		{
			aprservice_aprs_disconnect(aprs);

			return 0;
		}

		return -1;
	}

	return 0;
}
