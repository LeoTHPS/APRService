#include "APRService.hpp"

#include <cmath>
#include <regex>
#include <cassert>
#include <sstream>

#if defined(APRSERVICE_UNIX)
	#include <fcntl.h>
	#include <netdb.h>
	#include <unistd.h>

	#include <arpa/inet.h>

	#include <sys/types.h>
	#include <sys/socket.h>

	#include <netinet/tcp.h>
#elif defined(APRSERVICE_WIN32)
	#include <WS2tcpip.h>
	#include <MSWSock.h>
#endif

// https://www.aprs.org/doc/APRS101.PDF#page=27
// https://www.aprs.org/doc/APRS101.PDF#page=67

template<size_t ... I>
constexpr bool assert_tocall(const char* value, std::index_sequence<I ...>)
{
	return ((value[I] != ' ') && ...);
}
template<size_t S>
constexpr bool assert_tocall(const char(&value)[S])
{
	return assert_tocall(value, std::make_index_sequence<S - 1> {});
}

template<size_t ... I>
constexpr bool assert_software_name_version(const char* value, std::index_sequence<I ...>)
{
	return ((value[I] != ' ') && ...);
}
template<size_t S>
constexpr bool assert_software_name_version(const char(&value)[S])
{
	return assert_software_name_version(value, std::make_index_sequence<S - 1> {});
}

#define        static_assert_tocall(value) \
	static_assert(assert_tocall(value), #value " contains a space")

#define        static_assert_software_name_version(value) \
	static_assert(assert_software_name_version(value), #value " contains a space")

static_assert_tocall(APRSERVICE_TOCALL);
static_assert_software_name_version(APRSERVICE_SOFTWARE_NAME);
static_assert_software_name_version(APRSERVICE_SOFTWARE_VERSION);

#if defined(APRSERVICE_WIN32)
static WSADATA aprservice_winsock_data;
static size_t  aprservice_winsock_load_count = 0;
#endif

static constexpr double APRSERVICE_DEG2RAD = 3.14159265358979323846 / 180;

template<typename T>
constexpr T aprservice_from_float(float value, float& fraction)
{
	fraction = std::modff(value, &value);

	return value;
}

float aprservice_convert_distance(double value, APRService::DISTANCES type)
{
	switch (type)
	{
		case APRService::DISTANCE_FEET:       return value * 20903251;
		case APRService::DISTANCE_MILES:      return value * 3959.6f;
		case APRService::DISTANCE_METERS:     return value * 6371e3;
		case APRService::DISTANCE_KILOMETERS: return value * 6371;
	}

	return 0;
}

float aprservice_calculate_distance(const float(&latitude)[2], const float(&longitude)[2], APRService::DISTANCES type)
{
	double radians[] =
	{
		APRSERVICE_DEG2RAD * (latitude[1] - latitude[0]),
		APRSERVICE_DEG2RAD * (longitude[1] - longitude[0]),
		APRSERVICE_DEG2RAD * latitude[0],
		APRSERVICE_DEG2RAD * latitude[1]
	};

	double a = sin(radians[0] / 2) * sin(radians[0] / 2) + cos(radians[2]) * cos(radians[3]) * sin(radians[1] / 2) * sin(radians[1] / 2);
	double b = 2 * atan2(sqrt(a), sqrt(1 - a));

	return aprservice_convert_distance(b, type);
}
float aprservice_calculate_distance_3d(const float(&latitude)[2], const float(&longitude)[2], const int32_t(&altitude)[2], APRService::DISTANCES type)
{
	double distance   = aprservice_calculate_distance(latitude, longitude, type);
	double distance_z = (altitude[0] > altitude[1]) ? (altitude[0] - altitude[1]) : (altitude[1] - altitude[0]);

	return distance + aprservice_convert_distance(distance_z / 20903251, type);
}

bool             APRService::Path_IsValid(const Path& path)
{
	static const std::regex regex("^([A-Za-z0-9\\-]{2,9}\\*?)$");

	if (!path[0].length())
		return false;

	for (size_t i = 1; i < path.size(); ++i)
	{
		if (!path[i].length())
		{
			for (size_t j = i + 1; j < path.size(); ++j)
				if (path[j].length())
					return false;

			break;
		}

		try
		{
			if (!std::regex_match(path[i], regex))
				return false;
		}
		catch (const std::regex_error& error)
		{

			return false;
		}
	}

	return true;
}
std::string      APRService::Path_ToString(const Path& path)
{
	std::stringstream ss;

	if (path[0].length())
	{
		ss << path[0];

		for (size_t i = 1; (i < path.size()) && path[i].length(); ++i)
			ss << ',' << path[i];
	}

	return ss.str();
}
APRService::Path APRService::Path_FromString(const std::string& string)
{
	Path        path;
	size_t path_i     = 0;
	size_t path_end   = 0;
	size_t path_start = 0;

	do
	{
		if ((path_end = string.find_first_of(',', path_start)) == std::string::npos)
			path[path_i] = string.substr(path_start);
		else
		{
			path[path_i++] = string.substr(path_start, path_end - path_start);
			path_start     = path_end + 1;
		}
	}
	while ((path_end != std::string::npos) && (path_i < path.size()));

	return path;
}

float APRService::Object::CalculateDistance(float latitude, float longitude, DISTANCES type) const
{
	return aprservice_calculate_distance({ Latitude, latitude }, { Longitude, longitude }, type);
}
float APRService::Object::CalculateDistance3D(float latitude, float longitude, int32_t altitude, DISTANCES type) const
{
	return aprservice_calculate_distance_3d({ Latitude, latitude }, { Longitude, longitude }, { Altitude, altitude }, type);
}

float APRService::Position::CalculateDistance(float latitude, float longitude, DISTANCES type) const
{
	return aprservice_calculate_distance({ Latitude, latitude }, { Longitude, longitude }, type);
}
float APRService::Position::CalculateDistance3D(float latitude, float longitude, int32_t altitude, DISTANCES type) const
{
	return aprservice_calculate_distance_3d({ Latitude, latitude }, { Longitude, longitude }, { Altitude, altitude }, type);
}

APRService::SystemException::SystemException(const std::string& function)
#if defined(APRSERVICE_UNIX)
	: Exception("Error calling '" + function + "': " + std::to_string(errno))
#elif defined(APRSERVICE_WIN32)
	: Exception("Error calling '" + function + "': " + std::to_string(GetLastError()))
#endif
{
}
APRService::SystemException::SystemException(const std::string& function, int error)
	: Exception("Error calling '" + function + "': " + std::to_string(error))
{
}

std::string APRService::InvalidPathException::Path_ToString(const Path& path)
{
	std::stringstream ss;
	ss << path[0];

	for (auto& chunk : path)
		ss << ", " << chunk;

	return ss.str();
}

std::string APRService::InvalidPacketException::PacketTypesToString(PacketTypes type)
{
	switch (type)
	{
		case PacketTypes::Object:    return "Object";
		case PacketTypes::Message:   return "Message";
		case PacketTypes::Weather:   return "Weather";
		case PacketTypes::Position:  return "Position";
		case PacketTypes::Telemetry: return "Telemetry";
	}

	return "Undefined";
}

struct APRService::Client::DNS::Entry
{
	int Family;

	union
	{
		sockaddr_in  IPv4;
		sockaddr_in6 IPv6;
	};
};

// @throw Exception
// @return false on not found
bool APRService::Client::DNS::Resolve(const std::string& host, Entry& entry)
{
	addrinfo  hint = {};
	addrinfo* result;
	hint.ai_family = AF_UNSPEC;

	if (auto error = getaddrinfo(host.c_str(), "", &hint, &result))
	{
		switch (error)
		{
			case EAI_FAIL:
			case EAI_AGAIN:
				return false;
		}

		throw SystemException("getaddrinfo", error);
	}

	entry.Family = AF_UNSPEC;

	switch (result->ai_family)
	{
		case AF_INET:
			entry.Family = AF_INET;
			entry.IPv4 = *reinterpret_cast<const sockaddr_in*>(result->ai_addr);
			break;

		case AF_INET6:
			entry.Family = AF_INET6;
			entry.IPv6   = *reinterpret_cast<const sockaddr_in6*>(result->ai_addr);
			break;
	}

	freeaddrinfo(result);

	return entry.Family != AF_UNSPEC;
}

#if defined(APRSERVICE_WIN32)
// @throw Exception
void APRService::Client::WinSock::Load()
{
	if (!aprservice_winsock_load_count++)
	{
		if (WSAStartup(MAKEWORD(2, 2), &aprservice_winsock_data) != NO_ERROR)
		{
			aprservice_winsock_load_count = 0;

			throw SystemException("WSAStartup");
		}
	}
}
void APRService::Client::WinSock::Unload()
{
	if (aprservice_winsock_load_count && !--aprservice_winsock_load_count)
		WSACleanup();
}
#endif

// @throw Exception
void APRService::Client::TcpSocket::Connect(const std::string& host, uint16_t port)
{
	assert(!IsConnected());

	DNS::Entry dns_entry;

	if (!DNS::Resolve(host, dns_entry))
		throw Exception("Host '" + host + "' not found");

	sockaddr* socket_address;
	int       socket_address_length;

	switch (dns_entry.Family)
	{
		case AF_INET:
			dns_entry.IPv4.sin_port = htons(port);
			socket_address          = reinterpret_cast<sockaddr*>(&dns_entry.IPv4);
			socket_address_length   = sizeof(sockaddr_in);
			break;

		case AF_INET6:
			dns_entry.IPv6.sin6_port = htons(port);
			socket_address           = reinterpret_cast<sockaddr*>(&dns_entry.IPv6);
			socket_address_length    = sizeof(sockaddr_in6);
			break;
	}

#if defined(APRSERVICE_UNIX)
	if ((handle = socket(dns_entry.Family, SOCK_STREAM, IPPROTO_TCP)) == -1)
		throw SystemException("socket");

	if (::connect(handle, socket_address, socket_address_length) == -1)
	{
		close(handle);

		throw SystemException("connect");
	}

	int flags;

	if ((flags = fcntl(handle, F_GETFL, 0)) == -1)
	{
		close(handle);

		throw SystemException("fcntl");
	}

	if (fcntl(handle, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		close(handle);

		throw SystemException("fcntl");
	}
#elif defined(APRSERVICE_WIN32)
	if ((handle = WSASocketW(dns_entry.Family, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, 0)) == INVALID_SOCKET)
		throw SystemException("WSASocketW");

	if (connect(handle, socket_address, socket_address_length) == SOCKET_ERROR)
	{
		closesocket(handle);

		throw SystemException("connect");
	}

	u_long arg = 1;

	if (ioctlsocket(handle, FIONBIO, &arg) == SOCKET_ERROR)
	{
		closesocket(handle);

		throw SystemException("ioctlsocket");
	}
#endif

	is_connected = true;
}
void APRService::Client::TcpSocket::Disconnect()
{
	if (IsConnected())
	{
#if defined(APRSERVICE_UNIX)
		close(handle);
#elif defined(APRSERVICE_WIN32)
		closesocket(handle);
#endif

		is_connected = false;
	}
}

// @throw Exception
// @return false on connection closed
bool APRService::Client::TcpSocket::Send(const void* buffer, size_t size, size_t& number_of_bytes_sent)
{
	if (!IsConnected())
		return false;

#if defined(APRSERVICE_UNIX)
	std::ssize_t bytes_sent;

	if ((bytes_sent = send(handle, buffer, size, 0)) == -1)
	{
		auto error = errno;

		if ((error == EAGAIN) || (error == EWOULDBLOCK))
		{
			number_of_bytes_sent = 0;

			return true;
		}

		Disconnect();

		if ((error == EHOSTDOWN) || (error == ECONNRESET) || (error == EHOSTUNREACH))
			return false;

		throw SystemException("recv", error);
	}

	number_of_bytes_sent = bytes_sent;
#elif defined(APRSERVICE_WIN32)
	int bytes_sent;

	if ((bytes_sent = send(handle, reinterpret_cast<const char*>(buffer), static_cast<int>(size), 0)) == SOCKET_ERROR)
	{
		auto error = GetLastError();

		switch (error)
		{
			case WSAEWOULDBLOCK:
				number_of_bytes_sent = 0;
				return true;

			case WSAENETDOWN:
			case WSAENETRESET:
			case WSAETIMEDOUT:
			case WSAECONNRESET:
			case WSAECONNABORTED:
			case WSAEHOSTUNREACH:
				Disconnect();
				return false;
		}

		throw SystemException("send", error);
	}

	number_of_bytes_sent = bytes_sent;
#endif

	return true;
}
// @throw Exception
// @return false on connection closed
bool APRService::Client::TcpSocket::Receive(void* buffer, size_t size, size_t& number_of_bytes_received)
{
	if (!IsConnected())
		return false;

#if defined(APRSERVICE_UNIX)
	std::ssize_t bytes_received;

	if ((bytes_received = recv(handle, buffer, size, 0)) == -1)
	{
		auto error = GetLastError();

		if ((error == EAGAIN) || (error == EWOULDBLOCK))
		{
			number_of_bytes_received = 0;

			return true;
		}

		Disconnect();

		if ((error == EHOSTDOWN) || (error == ECONNRESET) || (error == EHOSTUNREACH))
			return false;

		throw SystemException("recv", error);
	}

	number_of_bytes_received = bytes_received;
#elif defined(APRSERVICE_WIN32)
	int bytes_received;

	if ((bytes_received = recv(handle, reinterpret_cast<char*>(buffer), static_cast<int>(size), 0)) == SOCKET_ERROR)
	{
		auto error = GetLastError();

		switch (error)
		{
			case WSAEWOULDBLOCK:
				number_of_bytes_received = 0;
				return true;

			case WSAENETDOWN:
			case WSAENETRESET:
			case WSAETIMEDOUT:
			case WSAECONNRESET:
			case WSAECONNABORTED:
				Disconnect();
				return false;
		}

		throw SystemException("recv", error);
	}

	number_of_bytes_received = bytes_received;
#endif

	return true;
}

// @throw Exception
APRService::Client::Client(std::string&& station, Path&& path, char symbol_table, char symbol_table_key)
	: path(std::move(path)),
	station(std::move(station)),
	symbol_table(symbol_table),
	symbol_table_key(symbol_table_key)
{
	if (!Path_IsValid(GetPath()))
		throw InvalidPathException(GetPath());

	if (!Station_IsValid(GetStation()))
		throw InvalidStationException(GetStation());

	if (!SymbolTable_IsValid(GetSymbolTable()))
		throw InvalidSymbolTableException(GetSymbolTable());

	if (!SymbolTableKey_IsValid(GetSymbolTable(), GetSymbolTableKey()))
		throw InvalidSymbolTableKeyException(GetSymbolTableKey());
}

// @throw Exception
void APRService::Client::Connect(const std::string& host, uint16_t port, int32_t passcode)
{
	if (IsConnected())
		throw Exception("Already connected");

#if defined(APRSERVICE_WIN32)
	WinSock::Load();
#endif

	socket = new TcpSocket();

	try
	{
		socket->Connect(host, port);
	}
	catch (Exception&)
	{
		delete socket;

#if defined(APRSERVICE_WIN32)
		WinSock::Unload();
#endif

		throw;
	}

	is_read_only = false;
	is_connected = true;

	auth_state   = AUTH_STATE_NONE;

	try
	{
		std::stringstream ss;
		ss << "user " << GetStation() << " pass " << (IsReadOnly() ? -1 : passcode);
		ss << " vers " << APRSERVICE_SOFTWARE_NAME << ' ' << APRSERVICE_SOFTWARE_VERSION;
		ss << " filter t/poimqstunw";

		Send(ss.str());

		is_read_only = passcode < 0;

		OnConnect.Execute();
	}
	catch (...)
	{
		Disconnect();

		throw;
	}
}
void APRService::Client::Disconnect()
{
	if (IsConnected())
	{
		socket->Disconnect();
		delete socket;

#if defined(APRSERVICE_WIN32)
		WinSock::Unload();
#endif

		auth.Success = false;
		auth_state   = AUTH_STATE_NONE;

		while (!send_queue.empty())
			send_queue.pop();

		receive_buffer_string.clear();

		is_connected = false;

		OnDisconnect.Execute();
	}
}

void APRService::Client::Send(std::string&& raw)
{
	if (IsConnected() && !IsReadOnly())
		send_queue.push({ .Buffer = std::move(raw), .Offset = 0, .OffsetEOL = 0 });
}

// @throw Exception
void APRService::Client::SendPacket(const std::string& content)
{
	Send(Packet_ToString(GetPath(), GetStation(), APRSERVICE_TOCALL, content));
}

// @throw Exception
void APRService::Client::SendObject(const std::string& name, const std::string& comment, float latitude, float longitude, char symbol_table, char symbol_table_key, bool live)
{
	auto time     = ::time(nullptr);
	auto datetime = *localtime(&time);
	int  flags    = live ? OBJECT_FLAG_LIVE : OBJECT_FLAG_KILLED;

	if (IsCompressionEnabled())
		flags |= OBJECT_FLAG_COMPRESSED;

	Send(Object_ToString(GetPath(), GetStation(), APRSERVICE_TOCALL, datetime, name, comment, latitude, longitude, symbol_table, symbol_table_key, flags));
}

// @throw Exception
void APRService::Client::SendMessage(const std::string& destination, const std::string& message)
{
	if (message_ack_counter++ == 0xFFFFF)
		message_ack_counter = 0;

	SendMessage(destination, message, sprintf("%05X", message_ack_counter));
}
// @throw Exception
void APRService::Client::SendMessage(const std::string& destination, const std::string& message, const std::string& id)
{
	if (!Station_IsValid(destination))
		throw InvalidPacketException(PacketTypes::Message, "destination", destination);

	Send(Message_ToString(GetPath(), GetStation(), APRSERVICE_TOCALL, destination, message, id));
}

// @throw Exception
void APRService::Client::SendMessageNoAck(const std::string& destination, const std::string& message)
{
	SendMessage(destination, message, "");
}

// @throw Exception
void APRService::Client::SendWeather(uint16_t wind_speed, uint16_t wind_speed_gust, uint16_t wind_direction, uint16_t rainfall_last_hour, uint16_t rainfall_last_24_hours, uint16_t rainfall_since_midnight, uint8_t humidity, int16_t temperature, uint32_t barometric_pressure, const std::string& type)
{
	auto time     = ::time(nullptr);
	auto datetime = *localtime(&time);

	Send(Weather_ToString(GetPath(), GetStation(), APRSERVICE_TOCALL, datetime, wind_speed, wind_speed_gust, wind_direction, rainfall_last_hour, rainfall_last_24_hours, rainfall_since_midnight, humidity, temperature, barometric_pressure, type));
}

// @throw Exception
void APRService::Client::SendPosition(uint16_t speed, uint16_t course, int32_t altitude, float latitude, float longitude, const std::string& comment)
{
	int flags = 0;

	if (IsMessagingEnabled())
		flags |= POSITION_FLAG_MESSAGING_ENABLED;

	if (IsCompressionEnabled())
		flags |= POSITION_FLAG_COMPRESSED;

	Send(Position_ToString(GetPath(), GetStation(), APRSERVICE_TOCALL, speed, course, altitude, latitude, longitude, comment, GetSymbolTable(), GetSymbolTableKey(), flags));
}

// @throw Exception
void APRService::Client::SendTelemetry(const TelemetryAnalog& analog, TelemetryDigital digital)
{
	if (telemetry_sequence_counter++ == 999)
		telemetry_sequence_counter = 0;

	SendTelemetry(analog, digital, telemetry_sequence_counter);
}
// @throw Exception
void APRService::Client::SendTelemetry(const TelemetryAnalog& analog, TelemetryDigital digital, uint16_t sequence)
{
	Send(Telemetry_ToString(GetPath(), GetStation(), APRSERVICE_TOCALL, analog, digital, sequence));
}

// @throw Exception
// @return false on connection closed
bool APRService::Client::Update()
{
	if (!IsConnected())
		return false;

	if (IsAuthenticated() || !IsAuthenticating())
	{
send_once:
		switch (SendOnce())
		{
			case IO_RESULT_SUCCESS:
				if (!IsAuthenticated())
					auth_state = AUTH_STATE_SENT;
				goto send_once;
	
			case IO_RESULT_DISCONNECT:
				Disconnect();
				return false;
		}
	}

receive_once:
	switch (ReceiveOnce())
	{
		case IO_RESULT_SUCCESS:
		{
			if (receive_buffer_string.starts_with("# "))
			{
				receive_buffer_string = receive_buffer_string.substr(2);

				HandleServerMessage(receive_buffer_string);
			}
			else
			{
				bool receive_buffer_packet_decoded = false;

				try
				{
					receive_buffer_packet_decoded = Packet_FromString(receive_buffer_packet, receive_buffer_string);
				}
				catch (Exception& exception)
				{
					HandleDecodeError(receive_buffer_string, &exception);

					break;
				}

				if (receive_buffer_packet_decoded)
					HandlePacket(receive_buffer_string, receive_buffer_packet);
				else
					HandleDecodeError(receive_buffer_string, nullptr);
			}
		}
		goto receive_once;

		case IO_RESULT_DISCONNECT:
			Disconnect();
			return false;
	}

	return true;
}

// @throw Exception
void APRService::Client::HandlePacket(const std::string& raw, Packet& packet)
{
	OnReceivePacket.Execute(packet);

	switch (packet.Type)
	{
		case PacketTypes::Object:
		{
			Object object;
			bool   object_is_decoded = false;

			try
			{
				object_is_decoded = Object_FromPacket(object, std::move(packet));
			}
			catch (Exception& exception)
			{
				HandleDecodeError(raw, &exception);

				break;
			}

			if (!object_is_decoded)
				HandleDecodeError(raw, nullptr);
			else
			{
				if (!object_is_decoded)
					HandleDecodeError(raw, nullptr);
				else
					HandleObject(raw, object);
			}
		}
		break;

		case PacketTypes::Message:
		{
			Message message;
			bool    message_is_decoded = false;

			try
			{
				message_is_decoded = Message_FromPacket(message, std::move(packet));
			}
			catch (Exception& exception)
			{
				HandleDecodeError(raw, &exception);

				break;
			}

			if (!message_is_decoded)
				HandleDecodeError(raw, nullptr);
			else
			{
				bool message_is_for_station = !stricmp(message.Destination.c_str(), GetStation().c_str());

				if (message_is_for_station || IsMonitorModeEnabled())
				{
					if (message_is_for_station && message.ID.length() && IsAutoAckEnabled())
						Send(Message_ToString_Ack(GetPath(), GetStation(), APRSERVICE_TOCALL, message.Sender, message.ID));

					HandleMessage(raw, message);
				}
			}
		}
		break;

		case PacketTypes::Weather:
		{
			Weather weather;
			bool    weather_is_decoded = false;

			try
			{
				weather_is_decoded = Weather_FromPacket(weather, std::move(packet));
			}
			catch (Exception& exception)
			{
				HandleDecodeError(raw, &exception);

				break;
			}

			if (!weather_is_decoded)
				HandleDecodeError(raw, nullptr);
			else
				HandleWeather(raw, weather);
		}
		break;

		case PacketTypes::Position:
		{
			Position position;
			bool     position_is_decoded = false;

			try
			{
				position_is_decoded = Position_FromPacket(position, std::move(packet));
			}
			catch (Exception& exception)
			{
				HandleDecodeError(raw, &exception);

				break;
			}

			if (!position_is_decoded)
				HandleDecodeError(raw, nullptr);
			else
				HandlePosition(raw, position);
		}
		break;

		case PacketTypes::Telemetry:
		{
			Telemetry telemetry;
			bool      telemetry_is_decoded = false;

			try
			{
				telemetry_is_decoded = Telemetry_FromPacket(telemetry, std::move(packet));
			}
			catch (Exception& exception)
			{
				HandleDecodeError(raw, &exception);

				break;
			}

			if (!telemetry_is_decoded)
				HandleDecodeError(raw, nullptr);
			else
				HandleTelemetry(raw, telemetry);
		}
		break;
	}
}
// @throw Exception
void APRService::Client::HandleObject(const std::string& raw, Object& object)
{
	OnReceiveObject.Execute(object);
}
// @throw Exception
void APRService::Client::HandleMessage(const std::string& raw, Message& message)
{
	OnReceiveMessage.Execute(message);
}
// @throw Exception
void APRService::Client::HandleWeather(const std::string& raw, Weather& weather)
{
	OnReceiveWeather.Execute(weather);
}
// @throw Exception
void APRService::Client::HandlePosition(const std::string& raw, Position& position)
{
	OnReceivePosition.Execute(position);
}
// @throw Exception
void APRService::Client::HandleTelemetry(const std::string& raw, Telemetry& telemetry)
{
	OnReceiveTelemetry.Execute(telemetry);
}
// @throw Exception
void APRService::Client::HandleServerMessage(const std::string& message)
{
	if (IsAuthenticating() && Auth_FromString(auth, receive_buffer_string))
	{
		if (!auth.Success)
		{
			Disconnect();

			throw AuthFailedException(auth.Message);
		}

		is_read_only = !auth.Verified;
		auth_state   = AUTH_STATE_RECEIVED;

		OnAuthenticate.Execute(auth.Message);

		return;
	}

	OnReceiveServerMessage.Execute(message);
}

// @throw Exception
void APRService::Client::HandleDecodeError(const std::string& raw, Exception* exception)
{
	OnDecodeError.Execute(raw, exception);
}

// @throw Exception
APRService::Client::IO_RESULTS APRService::Client::SendOnce()
{
	if (send_queue.empty())
		return IO_RESULT_WOULD_BLOCK;

	auto        entry = &send_queue.front();
	size_t number_of_bytes_sent;

	if (entry->Buffer.length() - entry->Offset)
	{
		auto entry_buffer_size = entry->Buffer.length();

		if (!socket->Send(&entry->Buffer[entry->Offset], entry_buffer_size - entry->Offset, number_of_bytes_sent))
			return IO_RESULT_DISCONNECT;

		if ((entry->Offset += number_of_bytes_sent) < entry_buffer_size)
			return IO_RESULT_WOULD_BLOCK;
	}

	if (!socket->Send(&EOL[entry->OffsetEOL], EOL_SIZE - entry->OffsetEOL, number_of_bytes_sent))
		return IO_RESULT_DISCONNECT;

	if ((entry->OffsetEOL += number_of_bytes_sent) == EOL_SIZE)
	{
		auto raw = std::move(send_queue.front().Buffer);

		send_queue.pop();

		OnSend.Execute(raw);

		return IO_RESULT_SUCCESS;
	}

	return IO_RESULT_WOULD_BLOCK;
}

// @throw Exception
APRService::Client::IO_RESULTS APRService::Client::ReceiveOnce()
{
	if (recieve_buffer_is_complete)
	{
		recieve_buffer_is_complete = false;
		receive_buffer_string.clear();
	}

	size_t number_of_bytes_received;

	if (!socket->Receive(&receive_buffer[receive_buffer_offset], receive_buffer.size() - receive_buffer_offset, number_of_bytes_received))
		return IO_RESULT_DISCONNECT;

	if (number_of_bytes_received == 0)
		return IO_RESULT_WOULD_BLOCK;

	receive_buffer_offset += number_of_bytes_received;

	// @return 0 if not found
	// @return 1 if CR is found
	// @return 2 if CRLF is found
	auto receive_buffer_find_eol = [](const char* buffer, size_t length, size_t& index)->int
	{
		for (size_t i = 0; i < length; ++i, ++buffer)
		{
			if (buffer[0] == EOL[0])
			{
				index = i;

				if ((++i < length) && (buffer[1] == EOL[1]))
				{
					index = i - 1;

					return 2;
				}

				return 1;
			}
		}

		return 0;
	};

	size_t receive_buffer_eol;

	switch (receive_buffer_find_eol(&receive_buffer[0], receive_buffer_offset, receive_buffer_eol))
	{
		case 0:
			receive_buffer_string.append(&receive_buffer[0], receive_buffer_offset);
			receive_buffer_offset = 0;
			break;

		case 1:
			if (receive_buffer_eol)
				receive_buffer_string.append(&receive_buffer[0], receive_buffer_eol);
			if (receive_buffer_eol < receive_buffer_offset)
				std::memmove(&receive_buffer[0], &receive_buffer[receive_buffer_eol], receive_buffer_offset - receive_buffer_eol);
			receive_buffer_offset -= receive_buffer_eol;
			break;

		case 2:
			if (receive_buffer_eol)
				receive_buffer_string.append(&receive_buffer[0], receive_buffer_eol);
			if ((receive_buffer_eol + 2) < receive_buffer_offset)
				std::memmove(&receive_buffer[0], &receive_buffer[receive_buffer_eol + 2], receive_buffer_offset - (receive_buffer_eol + 2));
			receive_buffer_offset -= receive_buffer_eol + 2;
			recieve_buffer_is_complete = true;
			OnReceive.Execute(receive_buffer_string);
			return IO_RESULT_SUCCESS;
	}

	return IO_RESULT_WOULD_BLOCK;
}

// @throw Exception
bool APRService::Client::Auth_FromString(Auth& auth, const std::string& string)
{
	static const std::regex regex("^logresp ([^ ]+) ([^ ,]+)[^ ]* ?(.*)$");

	std::smatch match;

	try
	{
		if (!std::regex_match(string, match, regex))
			return false;
	}
	catch (const std::regex_error& error)
	{

		throw RegexException(error.what());
	}

	auto status = match[2].str();

	if (auth.Success = !stricmp(status.c_str(), "verified"))
		auth.Verified = true;
	else if (auth.Success = !stricmp(status.c_str(), "unverified"))
		auth.Verified = false;

	auth.Message = match[3].str();

	return true;
}

// @throw Exception
bool APRService::Client::Path_IsValid(const Path& path)
{
	if (!path[0].length())
		return false;

	for (size_t i = 1; i < path.size(); ++i)
	{
		if (!path[i].length())
		{
			for (size_t j = i + 1; j < path.size(); ++j)
				if (path[j].length())
					return false;

			break;
		}

		if (!Station_IsValid(path[i]))
			return false;
	}

	return true;
}

// @throw Exception
bool APRService::Client::Station_IsValid(const std::string& station)
{
	static const std::regex regex("^([A-Za-z0-9\\-]{2,9}\\*?)$");

	try
	{
		if (!std::regex_match(station, regex))
			return false;
	}
	catch (const std::regex_error& error)
	{

		throw RegexException(error.what());
	}

	return true;
}

// @throw Exception
bool APRService::Client::SymbolTable_IsValid(char table)
{
	return (table == '/') || (table == '\\');
}

// @throw Exception
bool APRService::Client::SymbolTableKey_IsValid(char table, char key)
{
	if (!SymbolTable_IsValid(table))
		return false;

	return (key >= '!') && (key <= '}');
}

// @throw Exception
std::string APRService::Client::Packet_ToString(const Path& path, const std::string& sender, const std::string& tocall, const std::string& content)
{
	std::stringstream ss;
	ss << sender << '>' << tocall;

	for (size_t i = 0; (i < path.size()) && path[i].length(); ++i)
		ss << ',' << path[i];

	ss << ':' << content;

	return ss.str();
}
// @throw Exception
bool        APRService::Client::Packet_FromString(Packet& packet, const std::string& string)
{
	static const std::regex regex("^([^>]{3,9})>([^,]+),([^:]+):(.+)$");
	static const std::regex regex_path("^((\\S+?(?=,qA\\w)),(qA\\w),(.+))|(\\S+)$");

	std::string data;
	std::string path;
	std::smatch match;
	std::smatch match_path;

	try
	{
		if (!std::regex_match(string, match, regex))
			return false;

		data = match[3].str();

		if (!std::regex_match(data, match_path, regex_path))
			return false;

		path = match_path[2].str();
	}
	catch (const std::regex_error& error)
	{

		throw RegexException(error.what());
	}

	packet.Type       = (PacketTypes)(-1);
	packet.Path       = Path_FromString(path);
	packet.IGate      = match_path[4].str();
	packet.Sender     = match[1].str();
	packet.ToCall     = match[2].str();
	packet.Content    = match[4].str();
	packet.QConstruct = match_path[3].str();

	if (packet.Content.starts_with(';'))
		packet.Type = PacketTypes::Object;
	else if (packet.Content.starts_with(':'))
		packet.Type = PacketTypes::Message;
	else if ((packet.Content.starts_with('!') || packet.Content.starts_with('=')) || (packet.Content.starts_with('/') || packet.Content.starts_with('@')) || (packet.Content.starts_with('!') || packet.Content.starts_with('=')) || (packet.Content.starts_with('/') || packet.Content.starts_with('@')))
		packet.Type = PacketTypes::Position;
	else if (packet.Content.starts_with("T#"))
		packet.Type = PacketTypes::Telemetry;
	else if (packet.Content.starts_with('_')) // TODO: other weather types
		packet.Type = PacketTypes::Weather;

	return true;
}

// @throw Exception
std::string APRService::Client::Object_ToString(const Path& path, const std::string& sender, const std::string& tocall, const tm& time, const std::string& name, const std::string& comment, float latitude, float longitude, char symbol_table, char symbol_table_key, int flags)
{
	std::stringstream ss;
	ss << ';';
	ss << sprintf("%-9s", name.c_str());
	ss << ((flags & OBJECT_FLAG_LIVE) ? '*' : '_');
	ss << sprintf("%02u%02u%02uz", time.tm_mday, time.tm_hour, time.tm_min);

	// if (!(flags & OBJECT_FLAG_COMPRESSED))
	{
		char latitude_north_south = (latitude >= 0)  ? 'N' : 'S';
		char longitude_west_east  = (longitude >= 0) ? 'E' : 'W';
		auto latitude_hours       = aprservice_from_float<int16_t>(latitude, latitude);
		auto latitude_minutes     = aprservice_from_float<uint16_t>(((latitude < 0) ? (latitude * -1) : latitude) * 60, latitude);
		auto latitude_seconds     = aprservice_from_float<uint16_t>((latitude * 6000) / 60, latitude);
		auto longitude_hours      = aprservice_from_float<int16_t>(longitude, longitude);
		auto longitude_minutes    = aprservice_from_float<uint16_t>(((longitude < 0) ? (longitude * -1) : longitude) * 60, longitude);
		auto longitude_seconds    = aprservice_from_float<uint16_t>((longitude * 6000) / 60, longitude);

		ss << sprintf("%02i%02u.%02u%c", (latitude_hours >= 0) ? latitude_hours : (latitude_hours * -1), latitude_minutes, latitude_seconds, latitude_north_south);
		ss << symbol_table;
		ss << sprintf("%03i%02u.%02u%c", (longitude_hours >= 0) ? longitude_hours : (longitude_hours * -1), longitude_minutes, longitude_seconds, longitude_west_east);
		ss << symbol_table_key;
	}
	// else
		; // TODO: implement compression

	ss << comment;

	return Packet_ToString(path, sender, tocall, ss.str());
}
// @throw Exception
bool        APRService::Client::Object_FromPacket(Object& object, Packet&& packet)
{
	static const std::regex regex("^;([^ *_]{3,9}) *([*_])(\\d{6}[z/h])(\\d{4}\\.\\d{2}[NS].\\d{5}\\.\\d{2}[EW].)(.*)$");
	static const std::regex regex_compressed("^;([^ *_]{3,9}) *([*_])(\\d{6}[z/h])(.[!-{]{8}.{4})(.*)$");

	tm          time = {};
	std::smatch match;
	int         flags;
	uint16_t    speed;
	uint16_t    course;
	int32_t     altitude;
	float       latitude;
	float       longitude;
	char        symbol_table;
	char        symbol_table_key;

	auto object_init = [&object, &packet](std::string&& name, std::string&& comment, const tm& time, uint16_t speed, uint16_t course, int32_t altitude, float latitude, float longitude, char symbol_table, char symbol_table_key, int flags)
	{
		object =
		{
			{ PacketTypes::Object, std::move(packet.Path), std::move(packet.IGate), std::move(packet.ToCall), std::move(packet.Sender), std::move(packet.Content), std::move(packet.QConstruct) },
			flags,
			std::move(name),
			std::move(comment),
			speed,
			course,
			altitude,
			latitude,
			longitude,
			symbol_table,
			symbol_table_key
		};
	};

	auto unpack_time = [&time](const std::string& value)
	{
		static const std::regex regex("^(\\d{2})(\\d{2})(\\d{2})([z/h])$");

		std::smatch match;

		if (std::regex_match(value, match, regex))
		{
			auto a = (uint8_t)strtoul(match[1].str().c_str(), nullptr, 10);
			auto b = (uint8_t)strtoul(match[2].str().c_str(), nullptr, 10);
			auto c = (uint8_t)strtoul(match[3].str().c_str(), nullptr, 10);

			switch (match[4].str()[0])
			{
				// DHM
				case 'z':
				case '/':
					time.tm_mday = a;
					time.tm_hour = b;
					time.tm_min  = c;
					return true;

				// HMS
				case 'h':
					time.tm_hour = a;
					time.tm_min  = b;
					time.tm_sec  = c;
					return true;
			}
		}

		return false;
	};

	try
	{
		if (std::regex_match(packet.Content, match, regex))
		{
			if (match[2].str()[0] == '*')
				flags = OBJECT_FLAG_LIVE;
			else
				flags = OBJECT_FLAG_KILLED;

			if (unpack_time(match[3].str()) && LocationAndSymbol_FromString(speed, course, altitude, latitude, longitude, symbol_table, symbol_table_key, match[4].str()))
			{
				object_init(match[1].str(), match[5].str(), time, speed, course, altitude, latitude, longitude, symbol_table, symbol_table_key, flags);

				return true;
			}
		}
		else if (std::regex_match(packet.Content, match, regex_compressed))
		{
			if (match[2].str()[0] == '*')
				flags = OBJECT_FLAG_LIVE;
			else
				flags = OBJECT_FLAG_KILLED;

			if (unpack_time(match[3].str()) && LocationAndSymbol_FromString(speed, course, altitude, latitude, longitude, symbol_table, symbol_table_key, match[4].str()))
			{
				flags |= OBJECT_FLAG_COMPRESSED;

				object_init(match[1].str(), match[5].str(), time, speed, course, altitude, latitude, longitude, symbol_table, symbol_table_key, flags);

				return true;
			}
		}
	}
	catch (const std::regex_error& error)
	{

		throw RegexException(error.what());
	}

	return false;
}

// @throw Exception
std::string APRService::Client::Message_ToString(const Path& path, const std::string& sender, const std::string& tocall, const std::string& destination, const std::string& body, const std::string& id)
{
	if (id.empty())
		return Packet_ToString(path, sender, tocall, sprintf(":%-9s:%s", destination.c_str(), body.c_str()));

	return Packet_ToString(path, sender, tocall, sprintf(":%-9s:%s{%s", destination.c_str(), body.c_str(), id.c_str()));
}
// @throw Exception
std::string APRService::Client::Message_ToString_Ack(const Path& path, const std::string& sender, const std::string& tocall, const std::string& destination, const std::string& id)
{
	return Packet_ToString(path, sender, tocall, sprintf(":%-9s:ack%s", destination.c_str(), id.c_str()));
}
// @throw Exception
std::string APRService::Client::Message_ToString_Reject(const Path& path, const std::string& sender, const std::string& tocall, const std::string& destination, const std::string& id)
{
	return Packet_ToString(path, sender, tocall, sprintf(":%-9s:rej%s", destination.c_str(), id.c_str()));
}
// @throw Exception
bool        APRService::Client::Message_FromPacket(Message& message, Packet&& packet)
{
	static const std::regex regex("^:([^ :]+):(.+?)(\\{(.+))?$");

	std::smatch match;

	try
	{
		if (!std::regex_match(packet.Content, match, regex))
			return false;
	}
	catch (const std::regex_error& error)
	{

		throw RegexException(error.what());
	}

	message =
	{
		{ PacketTypes::Message, std::move(packet.Path), std::move(packet.IGate), std::move(packet.ToCall), std::move(packet.Sender), std::move(packet.Content), std::move(packet.QConstruct) },
		std::move((match.size() < 4) ? "" : match[4].str()),
		std::move(match[2]),
		std::move(match[1])
	};

	return true;
}

// @throw Exception
std::string APRService::Client::Weather_ToString(const Path& path, const std::string& sender, const std::string& tocall, const tm& time, uint16_t wind_speed, uint16_t wind_speed_gust, uint16_t wind_direction, uint16_t rainfall_last_hour, uint16_t rainfall_last_24_hours, uint16_t rainfall_since_midnight, uint8_t humidity, int16_t temperature, uint32_t barometric_pressure, const std::string& type)
{
	if (type.length() > 4)
		throw InvalidPacketException(PacketTypes::Weather, "type", type);

	std::stringstream ss;

	ss << '_';
	ss << sprintf("%02u%02u%02u%02u", time.tm_mon, time.tm_mday, time.tm_hour, time.tm_min);
	ss << sprintf("c%03us%03ug%03u", wind_direction, wind_speed, wind_speed_gust);
	ss << sprintf("t%03i", temperature);
	ss << sprintf("r%03up%03uP%03u", rainfall_last_hour, rainfall_last_24_hours, rainfall_since_midnight);
	ss << sprintf("h%02u", humidity);
	ss << sprintf("b%04u", barometric_pressure);
	ss << type;

	return Packet_ToString(path, sender, tocall, ss.str());
}
// @throw Exception
bool        APRService::Client::Weather_FromPacket(Weather& weather, Packet&& packet)
{
	static const std::regex regex(""); // TODO: implement

	std::smatch match;

	try
	{
		if (!std::regex_match(packet.Content, match, regex))
			return false;
	}
	catch (const std::regex_error& error)
	{

		throw RegexException(error.what());
	}

	return false;
}

// @throw Exception
std::string APRService::Client::Position_ToString(const Path& path, const std::string& sender, const std::string& tocall, uint16_t speed, uint16_t course, int32_t altitude, float latitude, float longitude, const std::string& comment, char symbol_table, char symbol_table_key, int flags)
{
	std::stringstream ss;

	ss << ((flags & POSITION_FLAG_MESSAGING_ENABLED) ? '=' : '!');

	// if (!(flags & POSITION_FLAG_COMPRESSED))
	{
		char latitude_north_south = (latitude >= 0)  ? 'N' : 'S';
		char longitude_west_east  = (longitude >= 0) ? 'E' : 'W';
		auto latitude_hours       = aprservice_from_float<int16_t>(latitude, latitude);
		auto latitude_minutes     = aprservice_from_float<uint16_t>(((latitude < 0) ? (latitude * -1) : latitude) * 60, latitude);
		auto latitude_seconds     = aprservice_from_float<uint16_t>((latitude * 6000) / 60, latitude);
		auto longitude_hours      = aprservice_from_float<int16_t>(longitude, longitude);
		auto longitude_minutes    = aprservice_from_float<uint16_t>(((longitude < 0) ? (longitude * -1) : longitude) * 60, longitude);
		auto longitude_seconds    = aprservice_from_float<uint16_t>((longitude * 6000) / 60, longitude);

		ss << sprintf("%02i%02u.%02u%c", (latitude_hours >= 0) ? latitude_hours : (latitude_hours * -1), latitude_minutes, latitude_seconds, latitude_north_south);
		ss << symbol_table;
		ss << sprintf("%03i%02u.%02u%c", (longitude_hours >= 0) ? longitude_hours : (longitude_hours * -1), longitude_minutes, longitude_seconds, longitude_west_east);
		ss << symbol_table_key;

		if (altitude)
			ss << sprintf("/A=%06i", altitude);

		if (course || speed)
			ss << sprintf("%03u/%03u", course, speed);
	}
	// else
		; // TODO: implement compression

	ss << comment;

	return Packet_ToString(path, sender, tocall, ss.str());
}
// @throw Exception
bool        APRService::Client::Position_FromPacket(Position& position, Packet&& packet)
{
	bool        is_decoded             = false;
	bool        is_messaging_enabled   = false;
	bool        is_compression_enabled = false;

	uint16_t    speed     = 0;
	uint16_t    course    = 0;
	int32_t     altitude  = 0;
	float       latitude  = 0;
	float       longitude = 0;

	std::string comment;

	char        symbol_table     = '\0';
	char        symbol_table_key = '\0';

	auto match_get_int16  = [](const std::smatch& match, size_t offset)->int16_t
	{
		auto source      = match[offset].str();
		bool is_positive = *source.c_str() != '-';

		for (size_t i = 1; i < source.length(); ++i)
		{
			if (source[i] == '0')
				continue;

			if (i == 0)
				break;

			return strtol(source.substr(i).c_str(), nullptr, 10) * (is_positive ? 1 : -1);
		}

		return strtol(source.c_str(), nullptr, 10);
	};
	auto match_get_uint16 = [](const std::smatch& match, size_t offset)->uint16_t
	{
		auto source = match[offset].str();

		for (size_t i = 0; i < source.length(); ++i)
		{
			if (source[i] == '0')
				continue;

			if (i == 0)
				break;

			return strtoul(source.substr(i).c_str(), nullptr, 10);
		}

		return strtoul(source.c_str(), nullptr, 10);
	};

	// Lat/Long Position Report Format - without Timestamp
	// Lat/Long Position Report Format - with Data Extension (no Timestamp)
	if (packet.Content.starts_with('!') || (is_messaging_enabled = packet.Content.starts_with('=')))
	{
		static const std::regex regex("^[!=]((\\d{2})(\\d{2})\\.(\\d{2})([NS])(.)(\\d{3})(\\d{2})\\.(\\d{2})([EW])(.))(.*)$");
		static const std::regex regex_compressed("^[!=]((.)([!-{]{4})([!-{]{4})(.)(.{2})(.))(.*)$");

		std::smatch match;

		try
		{
			if (std::regex_match(packet.Content, match, regex))
			{
				if (LocationAndSymbol_FromString(speed, course, altitude, latitude, longitude, symbol_table, symbol_table_key, match[1].str()))
				{
					is_decoded = true;

					comment    = match[12].str();
				}
			}
			else if (std::regex_match(packet.Content, match, regex_compressed))
			{
				if (LocationAndSymbol_FromString(speed, course, altitude, latitude, longitude, symbol_table, symbol_table_key, match[1].str()))
				{
					is_decoded             = true;
					is_compression_enabled = true;

					comment                = match[8].str();
				}
			}
		}
		catch (const std::regex_error& error)
		{

			throw RegexException(error.what());
		}
	}
	// Lat/Long Position Report Format - with Timestamp
	// Lat/Long Position Report Format - with Data Extension and Timestamp
	else if (packet.Content.starts_with('/') || (is_messaging_enabled = packet.Content.starts_with('@')))
	{
		static const std::regex regex("^[/@](\\d+)[hz/]((\\d{2})(\\d{2})\\.(\\d{2})([NS])(.)(\\d{3})(\\d{2})\\.(\\d{2})([EW])(.))(.*)$");
		static const std::regex regex_compressed("^[/@]((.)([!-{]{4})([!-{]{4})(.)(.{2})(.))(.*)$");

		std::smatch match;

		try
		{
			if (std::regex_match(packet.Content, match, regex))
			{
				if (LocationAndSymbol_FromString(speed, course, altitude, latitude, longitude, symbol_table, symbol_table_key, match[2].str()))
				{
					is_decoded = true;

					comment    = match[13].str();
				}
			}
			else if (std::regex_match(packet.Content, match, regex_compressed))
			{
				if (LocationAndSymbol_FromString(speed, course, altitude, latitude, longitude, symbol_table, symbol_table_key, match[1].str()))
				{
					is_decoded             = true;
					is_compression_enabled = true;

					comment                = match[8].str();
				}
			}
		}
		catch (const std::regex_error& error)
		{

			throw RegexException(error.what());
		}
	}

	if (is_decoded)
	{
		if (!is_compression_enabled)
		{
			static const std::regex regex_altitude("(/A=(-?\\d+))");
			static const std::regex regex_speed_course("^((\\d{3})/(\\d{3}))");

			std::smatch match_altitude;
			std::smatch match_speed_course;

			try
			{
				if (std::regex_search(comment, match_altitude, regex_altitude))
				{
					altitude = match_get_int16(match_altitude, 2);

					auto match_altitude_length  = match_altitude[1].length();
					auto comment_altitude_begin = comment.find(match_altitude[1].str(), 0);

					comment.erase(comment_altitude_begin, match_altitude_length);
				}

				if (std::regex_search(comment, match_speed_course, regex_speed_course))
				{
					speed   = match_get_uint16(match_speed_course, 3);
					course  = match_get_uint16(match_speed_course, 2);
					comment = comment.substr(match_speed_course[1].length());
				}
			}
			catch (const std::regex_error& error)
			{

				throw RegexException(error.what());
			}
		}

		position =
		{
			{ PacketTypes::Position, std::move(packet.Path), std::move(packet.IGate), std::move(packet.ToCall), std::move(packet.Sender), std::move(packet.Content), std::move(packet.QConstruct) },
			0,
			speed,
			course,
			altitude,
			latitude,
			longitude,
			std::move(comment),
			symbol_table,
			symbol_table_key
		};

		if (is_messaging_enabled)
			position.Flags |= POSITION_FLAG_MESSAGING_ENABLED;

		if (is_compression_enabled)
			position.Flags |= POSITION_FLAG_COMPRESSED;
	}

	return is_decoded;
}

// @throw Exception
std::string APRService::Client::Telemetry_ToString(const Path& path, const std::string& sender, const std::string& tocall, const TelemetryAnalog& analog, TelemetryDigital digital, uint16_t sequence)
{
	std::stringstream ss;

	ss << "T#" << sprintf("%03u", sequence) << ',';
	for (auto a : analog)
		ss << a << ',';
	for (uint8_t i = 0; i < 8; ++i)
		ss << (((digital & (1 << i)) == (1 << i)) ? 1 : 0);

	return Packet_ToString(path, sender, tocall, ss.str());
}
// @throw Exception
bool        APRService::Client::Telemetry_FromPacket(Telemetry& telemetry, Packet&& packet)
{
	static const std::regex regex("^T#(\\d{3}|)(,(\\d{0,3}\\.?\\d{0,3}))?(,(\\d{0,3}\\.?\\d{0,3}))?(,(\\d{0,3}\\.?\\d{0,3}))?(,(\\d{0,3}\\.?\\d{0,3}))?(,(\\d{0,3}\\.?\\d{0,3}))?(,(\\d{8}))?.*?$");

	std::smatch match;

	try
	{
		if (!std::regex_match(packet.Content, match, regex))
			return false;
	}
	catch (const std::regex_error& error)
	{

		throw RegexException(error.what());
	}

	telemetry =
	{
		{ PacketTypes::Telemetry, std::move(packet.Path), std::move(packet.IGate), std::move(packet.ToCall), std::move(packet.Sender), std::move(packet.Content), std::move(packet.QConstruct) }
	};

	telemetry.Analog[0] = strtof(match[3].str().c_str(), nullptr);
	telemetry.Analog[1] = strtof(match[5].str().c_str(), nullptr);
	telemetry.Analog[2] = strtof(match[7].str().c_str(), nullptr);
	telemetry.Analog[3] = strtof(match[9].str().c_str(), nullptr);
	telemetry.Analog[4] = strtof(match[11].str().c_str(), nullptr);
	telemetry.Digital   = strtoul(match[13].str().c_str(), nullptr, 10);
	telemetry.Sequence  = strtoul(match[1].str().c_str(), nullptr, 10);

	return true;
}

// @throw Exception
bool APRService::Client::LocationAndSymbol_FromString(uint16_t& speed, uint16_t& course, int32_t& altitude, float& latitude, float& longitude, char& symbol_table, char& symbol_table_key, const std::string& string)
{
	static const std::regex regex("^(\\d{2})(\\d{2})\\.(\\d{2})([NS])(.)(\\d{3})(\\d{2})\\.(\\d{2})([EW])(.)$");
	static const std::regex regex_compressed("^(.)([!-{]{4})([!-{]{4})(.)(.{2})(.)$");

	std::smatch match;

	uint16_t    latitude_hours       = 0;
	uint16_t    latitude_minutes     = 0;
	uint16_t    latitude_seconds     = 0;
	char        latitude_north_south = 'N';

	uint16_t    longitude_hours      = 0;
	uint16_t    longitude_minutes    = 0;
	uint16_t    longitude_seconds    = 0;
	char        longitude_west_east  = 'E';

	auto match_get_int16  = [](const std::smatch& match, size_t offset)->int16_t
	{
		auto source      = match[offset].str();
		bool is_positive = *source.c_str() != '-';

		for (size_t i = 1; i < source.length(); ++i)
		{
			if (source[i] == '0')
				continue;

			if (i == 0)
				break;

			return strtol(source.substr(i).c_str(), nullptr, 10) * (is_positive ? 1 : -1);
		}

		return strtol(source.c_str(), nullptr, 10);
	};
	auto match_get_uint16 = [](const std::smatch& match, size_t offset)->uint16_t
	{
		auto source = match[offset].str();

		for (size_t i = 0; i < source.length(); ++i)
		{
			if (source[i] == '0')
				continue;

			if (i == 0)
				break;

			return strtoul(source.substr(i).c_str(), nullptr, 10);
		}

		return strtoul(source.c_str(), nullptr, 10);
	};

	auto match_decompress = [&symbol_table, &symbol_table_key, &latitude, &longitude, &altitude, &speed, &course](const std::smatch& match)
	{
		auto match_is_valid = [](const std::string& value, bool allow_space = false)
		{
			for (auto c : value)
			{
				if (allow_space && (c == ' '))
					continue;

				if (!allow_space && ((c < '!') || (c > '{')))
					return false;
			}

			return true;
		};

		auto match_2 = match[2].str();
		auto match_3 = match[3].str();
		auto match_5 = match[5].str();
		auto match_6 = match[6].str();

		if (!match_is_valid(match_2) || !match_is_valid(match_3) || !match_is_valid(match_5, true) || !match_is_valid(match_6, true))
			return false;

		// https://www.aprs.org/doc/APRS101.PDF#page=48

		latitude         = 90 - (((match_2[0] - 33) * 753571) + ((match_2[1] - 33) * 8281) + ((match_2[2] - 33) * 91) + (match_2[3] - 33)) / 380926.0f;
		longitude        = -180 + (((match_3[0] - 33) * 753571) + ((match_3[1] - 33) * 8281) + ((match_3[2] - 33) * 91) + (match_3[3] - 33)) / 190463.0f;
		symbol_table     = *match[2].str().c_str();
		symbol_table_key = *match[5].str().c_str();

		if (match_5[0] != ' ')
		{
			uint8_t t     = static_cast<uint8_t>(match_6[0] - 33);
			uint8_t cs[2] = { static_cast<uint8_t>(match_5[0] - 33), static_cast<uint8_t>(match_5[1] - 33) };

			if ((t & 0x10) == 0x10)
				altitude = (cs[0] * 91) + cs[1];
			else if (match_5[0] == '{')
			{
				// TODO: radio range
			}
			else if ((match_5[0] >= '!') && (match_5[0] <= 'z'))
			{
				speed  = match_5[1] - 33;
				course = match_5[0] - 33;
			}
		}

		return true;
	};

	try
	{
		if (std::regex_match(string, match, regex))
		{
			speed                = 0;
			course               = 0;
			altitude             = 0;

			latitude_hours       = match_get_uint16(match, 1);
			latitude_minutes     = match_get_uint16(match, 2);
			latitude_seconds     = match_get_uint16(match, 3);
			latitude_north_south = *match[4].str().c_str();
			latitude             = (latitude_hours  + (latitude_minutes / 60.0f)  + (latitude_seconds / 6000.0f))  * ((latitude_north_south == 'N') ? 1 : -1);

			longitude_hours      = match_get_uint16(match, 6);
			longitude_minutes    = match_get_uint16(match, 7);
			longitude_seconds    = match_get_uint16(match, 8);
			longitude_west_east  = *match[9].str().c_str();
			longitude            = (longitude_hours + (longitude_minutes / 60.0f) + (longitude_seconds / 6000.0f)) * ((longitude_west_east  == 'E') ? 1 : -1);

			symbol_table         = *match[5].str().c_str();
			symbol_table_key     = *match[10].str().c_str();

			return true;
		}
		else if (std::regex_match(string, match, regex_compressed))
		{
			if (!match_decompress(match))
				throw Exception("Invalid compression");

			return true;
		}
	}
	catch (const std::regex_error& error)
	{

		throw RegexException(error.what());
	}

	return false;
}

bool APRService::Service::Task::Cancel()
{
	// TODO: optimize
	// TODO: handle case where tasks are being executed

	for (auto it = service->tasks.begin(); it != service->tasks.end(); ++it)
	{
		for (auto jt = it->second.begin(); jt != it->second.end(); ++jt)
		{
			if (*jt == this)
			{
				it->second.erase(jt);

				if (it->second.empty())
					service->tasks.erase(it);

				delete this;

				return true;
			}
		}
	}

	return false;
}

// @throw Exception
bool APRService::Service::Object::Kill()
{
	if (!service->IsConnected())
		return false;

	service->SendObject(GetName(), GetComment(), GetLatitude(), GetLongitude(), GetSymbolTable(), GetSymbolTableKey(), false);

	service->objects.remove(this);

	delete this;

	return true;
}

// @throw Exception
bool APRService::Service::Object::Announce()
{
	if (!service->IsConnected())
		return false;

	service->SendObject(GetName(), GetComment(), GetLatitude(), GetLongitude(), GetSymbolTable(), GetSymbolTableKey());

	return true;
}

// @throw Exception
APRService::Service::Service(std::string&& station, Path&& path, char symbol_table, char symbol_table_key)
	: Client(std::move(station), std::move(path), symbol_table, symbol_table_key),
	time(::time(nullptr))
{
}

APRService::Service::~Service()
{
	for (auto it = tasks.begin(); it != tasks.end(); )
	{
		for (auto jt = it->second.begin(); jt != it->second.end(); )
		{
			delete *jt;

			it->second.erase(jt++);
		}

		tasks.erase(it++);
	}

	for (auto it = objects.begin(); it != objects.end(); )
	{
		delete *it;

		objects.erase(it++);
	}
}

APRService::IObject* APRService::Service::AddObject(std::string&& name, std::string&& comment, float latitude, float longitude, char symbol_table, char symbol_table_key)
{
	auto object = new Object(this, std::move(name), std::move(comment), latitude, longitude, symbol_table, symbol_table_key);

	objects.push_back(object);

	return object;
}

APRService::ITask* APRService::Service::ScheduleTask(uint32_t seconds, TaskHandler&& handler)
{
	return tasks[time + seconds].emplace_back(new Task(this, std::move(handler), seconds));
}

bool APRService::Service::ExecuteCommand(const std::string& name, const APRService::Command& command)
{
	for (auto& cmd : commands)
		if (!stricmp(cmd.Name.c_str(), name.c_str()))
		{
			cmd.Handler(command);

			return true;
		}

	return false;
}
void APRService::Service::RegisterCommand(std::string&& name, CommandHandler&& handler)
{
	for (auto it = commands.begin(); it != commands.end(); ++it)
	{
		if (!stricmp(it->Name.c_str(), name.c_str()))
		{
			it->Handler = std::move(handler);

			return;
		}
	}

	commands.push_back({
		.Name    = std::move(name),
		.Handler = std::move(handler)
	});
}
void APRService::Service::UnregisterCommand(const std::string& name)
{
	for (auto it = commands.begin(); it != commands.end(); ++it)
	{
		if (!stricmp(it->Name.c_str(), name.c_str()))
		{
			commands.erase(it);

			break;
		}
	}
}
void APRService::Service::EnumerateCommands(ServiceEnumCommandsCallback&& callback)
{
	for (auto& command : commands)
		callback(command.Name, command.Handler);
}

// @throw Exception
// @return false on connection closed
bool APRService::Service::Update()
{
	time = ::time(nullptr);

	for (auto it = tasks.begin(); it != tasks.end(); )
	{
		if (it->first > time)
			break;

		for (auto jt = it->second.begin(); jt != it->second.end(); )
		{
			auto task = *jt;

			try
			{
				if (task->Execute())
				{
					tasks[time + task->GetTime()].emplace_back(task);

					task = nullptr;
				}
			}
			catch (...)
			{
				delete task;

				it->second.erase(jt);

				throw;
			}

			if (task)
				delete task;

			it->second.erase(jt++);
		}

		tasks.erase(it++);
	}

	return Client::Update();
}

// @throw Exception
void APRService::Service::HandleMessage(const std::string& raw, Message& message)
{
	if (ParseCommand(command, raw, std::move(message)))
		ExecuteCommand(command.Name, command);
	else
		Client::HandleMessage(raw, message);
}

bool APRService::Service::ParseCommand(APRService::Command& command, const std::string& raw, Message&& message)
{
	static const std::regex regex("^\\.([^ ]+) ?(.+)?$");

	if (!message.Body.starts_with('.'))
		return false;

	std::smatch match;

	try
	{
		if (!std::regex_match(message.Body, match, regex))
			return false;
	}
	catch (const std::regex_error& error)
	{
		RegexException exception(error.what());

		HandleDecodeError(raw, &exception);

		return false;
	}

	command =
	{
		{
			{ PacketTypes::Message, std::move(message.Path), std::move(message.IGate), std::move(message.ToCall), std::move(message.Sender), std::move(message.Content), std::move(message.QConstruct) },
			std::move(message.ID),
			std::move(message.Body),
			std::move(message.Destination)
		},
		match[1].str(),
		(match.size() == 1) ? "" : match[2].str()
	};

	return true;
}
