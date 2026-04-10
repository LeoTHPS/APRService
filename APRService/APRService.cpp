#include "APRService.hpp"

#include <map>
#include <list>
#include <array>
#include <ctime>
#include <queue>
#include <regex>
#include <chrono>
#include <string>
#include <vector>
#include <cassert>
#include <cstring>
#include <sstream>
#include <utility>
#include <iostream>
#include <unordered_map>

#if defined(APRSERVICE_UNIX)
	#include <poll.h>
	#include <fcntl.h>
	#include <netdb.h>
	#include <unistd.h>
	#include <termios.h>

	#include <arpa/inet.h>

	#include <sys/ioctl.h>
	#include <sys/types.h>
	#include <sys/socket.h>

	#include <netinet/tcp.h>

	#include <strings.h>

	#define stricmp strcasecmp
#elif defined(APRSERVICE_WIN32)
	#include <WS2tcpip.h>
	#include <MSWSock.h>
#endif

enum KISS_TNC_COMMANDS : uint8_t
{
	// The following bytes should be transmitted by the TNC.
	// The maximum number of bytes, thus the size of the encapsulated packet, is determined by the amount of memory in the TNC.
	KISS_TNC_COMMAND_DATA             = 0x00,

	// The next byte is the time to hold up the TX after the FCS has been sent, in 10 ms units.
	// This command is obsolete, and is included here only for compatibility with some existing implementations.
	KISS_TNC_COMMAND_SET_TX_TAIL      = 0x04,
	// The next byte is the transmitter keyup delay in 10 ms units.
	// The default start-up value is 50 (i.e., 500 ms).
	KISS_TNC_COMMAND_SET_TX_DELAY     = 0x01,
	// The next byte is the slot interval in 10 ms units.
	// The default is 10 (i.e., 100ms).
	KISS_TNC_COMMAND_SET_SLOT_TIME    = 0x03,
	// The next byte is 0 for half duplex, nonzero for full duplex.
	// The default is 0 (i.e., half duplex).
	KISS_TNC_COMMAND_SET_FULL_DUPLEX  = 0x05,
	// Specific for each TNC.
	// In the TNC-1, this command sets the modem speed.
	// Other implementations may use this function for other hardware-specific functions.
	KISS_TNC_COMMAND_SET_HARDWARE     = 0x06,
	// The next byte is the persistence parameter, p, scaled to the range 0 - 255 with the following formula:
	// P = p * 256 - 1
	// The default value is P = 63 (i.e., p = 0.25).
	KISS_TNC_COMMAND_SET_PERSISTENCE  = 0x02,

	// Exit KISS and return control to a higher-level program.
	// This is useful only when KISS is incorporated into the TNC along with other applications.
	KISS_TNC_COMMAND_RETURN           = 0xFF
};

enum KISS_TNC_SPECIAL_CHARACTERS : uint8_t
{
	KISS_TNC_SPECIAL_CHARACTER_FRAME_END               = 0xC0,
	KISS_TNC_SPECIAL_CHARACTER_FRAME_ESCAPE            = 0xDB,
	KISS_TNC_SPECIAL_CHARACTER_TRANSPOSED_FRAME_END    = 0xDC,
	KISS_TNC_SPECIAL_CHARACTER_TRANSPOSED_FRAME_ESCAPE = 0xDD
};

enum APRSERVICE_AUTH_STATES
{
	APRSERVICE_AUTH_STATE_NONE,
	APRSERVICE_AUTH_STATE_SENT,
	APRSERVICE_AUTH_STATE_RECEIVED
};

enum APRSERVICE_CONNECTION_TYPES
{
	APRSERVICE_CONNECTION_TYPE_APRS_IS,
	APRSERVICE_CONNECTION_TYPE_KISS_TNC_TCP,
	APRSERVICE_CONNECTION_TYPE_KISS_TNC_SERIAL
};

struct aprservice_event
{
	aprservice_event_handler handler;
	void*                    handler_param;
};

struct aprservice_connection_auth
{
	APRSERVICE_AUTH_STATES state;
	std::string            message;
	bool                   success;
	bool                   verified;
};
struct aprservice_connection_buffer
{
	std::string value;
	size_t      offset;
};
struct aprservice_connection
{
	aprservice*                              service;

	bool                                     is_open;

	aprservice_connection_auth               auth;
	int                                      type;

#if defined(APRSERVICE_UNIX)
	int                                      serial;
	int                                      socket;
#elif defined(APRSERVICE_WIN32)
	HANDLE                                   serial;
	SOCKET                                   socket;
	WSADATA                                  winsock;
#endif

	uint32_t                                 io_time;

	std::queue<std::string>                  rx_queue;
	std::string                              rx_buffer;
	std::array<char, 128>                    rx_buffer_tmp;

	std::queue<aprservice_connection_buffer> tx_queue;

	uint32_t                                 device_speed;
	std::string                              host_or_device;
	std::uint16_t                            port, passcode;
};

struct aprservice_message_callback_context
{
	std::string                 id;
	std::string                 station;
	uint32_t                    timeout;
	aprservice_message_callback callback;
	void*                       callback_param;
};

struct aprservice_item
{
	aprservice*  service;

	aprs_packet* packet;
};

struct aprservice_task
{
	aprservice*             service;

	uint32_t                seconds;
	aprservice_task_handler handler;
	void*                   handler_param;
};

struct aprservice_object
{
	aprservice*  service;

	aprs_packet* packet;
};

struct aprservice_command
{
	aprservice*                       service;

	std::string                       name;
	std::string                       help;

	aprservice_command_filter_handler filter;
	void*                             filter_param;

	aprservice_command_handler        handler;
	void*                             handler_param;
};

struct aprservice
{
	bool                                                                            is_monitoring;

	std::string                                                                     line;
	int64_t                                                                         time;
	int                                                                             time_type;

	aprs_path*                                                                      path;
	const std::string                                                               station;
	aprs_packet*                                                                    position;
	aprservice_connection*                                                          connection;
	uint32_t                                                                        connection_timeout;

	std::list<aprservice_item>                                                      items;
	std::map<uint64_t, std::list<aprservice_task*>>                                 tasks;
	aprservice_event                                                                events[APRSERVICE_EVENTS_COUNT + 1];
	std::list<aprservice_object>                                                    objects;
	std::list<aprservice_command>                                                   commands;

	std::string                                                                     command_prefix;

	uint32_t                                                                        message_count;
	std::list<aprservice_message_callback_context*>                                 message_callbacks;
	std::unordered_map<std::string, std::list<aprservice_message_callback_context>> message_callbacks_index;

	uint16_t                                                                        telemetry_count;
};

template<APRSERVICE_EVENTS EVENT>
struct aprservice_get_event_information;
template<>
struct aprservice_get_event_information<APRSERVICE_EVENT_CONNECT>
{
	typedef aprservice_event_information_connect type;
};
template<>
struct aprservice_get_event_information<APRSERVICE_EVENT_DISCONNECT>
{
	typedef aprservice_event_information_disconnect type;
};
template<>
struct aprservice_get_event_information<APRSERVICE_EVENT_AUTHENTICATE>
{
	typedef aprservice_event_information_authenticate type;
};
template<>
struct aprservice_get_event_information<APRSERVICE_EVENT_RECEIVE_PACKET>
{
	typedef aprservice_event_information_receive_packet type;
};
template<>
struct aprservice_get_event_information<APRSERVICE_EVENT_RECEIVE_MESSAGE>
{
	typedef aprservice_event_information_receive_message type;
};
template<>
struct aprservice_get_event_information<APRSERVICE_EVENT_RECEIVE_SERVER_MESSAGE>
{
	typedef aprservice_event_information_receive_server_message type;
};

#define                                    aprservice_log_error(function, error) std::cerr << #function " returned " << error << std::endl

template<typename T>
T                                          aprservice_parse_uint(const char* string)
{
	T value = 0;

	for (size_t i = 0; *string; ++i, ++string)
		value = 10 * value + (*string - '0');

	return value;
}
template<typename T>
T                                          aprservice_parse_uint(const char* string, size_t length)
{
	T value = 0;

	for (size_t i = 0; *string && (i < length); ++i, ++string)
		value = 10 * value + (*string - '0');

	return value;
}

template<APRSERVICE_EVENTS EVENT>
constexpr bool                             aprservice_event_execute(aprservice* service, typename aprservice_get_event_information<EVENT>::type&& event)
{
	if (EVENT >= APRSERVICE_EVENTS_COUNT)
		return false;

	event.type = EVENT;

	if (auto e = &service->events[EVENT]; auto event_handler = e->handler)
		event_handler(service, (aprservice_event_information*)&event, e->handler_param);
	else if (auto e = &service->events[APRSERVICE_EVENTS_COUNT]; auto event_handler = e->handler)
		event_handler(service, (aprservice_event_information*)&event, e->handler_param);

	return true;
}
#define                                    aprservice_event_execute(service, event, ...) aprservice_event_execute<event>(service, __VA_ARGS__)

bool                                       aprservice_regex_match(std::cmatch& match, const std::regex& regex, const char* string)
{
	try
	{
		if (!std::regex_match(string, match, regex))
			return false;
	}
	catch (const std::regex_error& exception)
	{
		aprservice_log_error(std::regex_match, exception.what());

		return false;
	}

	return true;
}

bool                                       aprservice_connection_auth_from_string(aprservice_connection_auth* auth, const char* string, bool is_verified)
{
	static const std::regex regex("^logresp ([^ ]+) ([^ ,]+)[^ ]* ?(.*)$");

	std::cmatch match;

	if (!aprservice_regex_match(match, regex, string))
		return false;

	auto status = match[2].str();

	auth->state    = APRSERVICE_AUTH_STATE_RECEIVED;
	auth->success  = (!is_verified && !status.compare("unverified")) || !status.compare("verified");
	auth->verified = !status.compare("verified");
	auth->message  = match[3].str();

	return true;
}

aprservice_connection*                     aprservice_connection_init(aprservice* service, int type, const char* host_or_device, uint16_t port, uint32_t speed, uint16_t passcode);
void                                       aprservice_connection_deinit(aprservice_connection* connection);
bool                                       aprservice_connection_is_open(aprservice_connection* connection);
bool                                       aprservice_connection_open(aprservice_connection* connection);
void                                       aprservice_connection_close(aprservice_connection* connection);
// @return false on connection closed
bool                                       aprservice_connection_poll(aprservice_connection* connection);
// @return 0 on disconnect
// @return -1 on would block
int                                        aprservice_connection_read(aprservice_connection* connection, void* buffer, size_t size, size_t* number_of_bytes_received);
// @return 0 on disconnect
// @return -1 on would block
int                                        aprservice_connection_write(aprservice_connection* connection, const void* buffer, size_t size, size_t* number_of_bytes_sent);
bool                                       aprservice_connection_read_string(aprservice_connection* connection, std::string& value);
// @return false on connection closed
bool                                       aprservice_connection_write_packet(aprservice_connection* connection, aprs_packet* value);
// @return false on connection closed
bool                                       aprservice_connection_write_aprs_is(aprservice_connection* connection, std::string&& value);

aprservice_connection*                     aprservice_connection_init(aprservice* service, int type, const char* host_or_device, uint16_t port, uint32_t speed, uint16_t passcode)
{
	auto connection = new aprservice_connection
	{
		.service        = service,

		.is_open        = false,

		.type           = type,

		.io_time        = aprservice_get_time(service),

		.device_speed   = speed,
		.host_or_device = host_or_device,
		.port           = port,
		.passcode       = passcode
	};

#if defined(APRSERVICE_WIN32)
	if (auto error = WSAStartup(MAKEWORD(2, 2), &connection->winsock))
	{
		aprservice_log_error(WSAStartup, error);

		delete connection;

		return nullptr;
	}
#endif

	return connection;
}
void                                       aprservice_connection_deinit(aprservice_connection* connection)
{
	if (aprservice_connection_is_open(connection))
		aprservice_connection_close(connection);

#if defined(APRSERVICE_WIN32)
	WSACleanup();
#endif

	delete connection;
}
bool                                       aprservice_connection_is_open(aprservice_connection* connection)
{
	return connection->is_open;
}
bool                                       aprservice_connection_open_tcp(aprservice_connection* connection)
{
resolve_host:
	addrinfo  dns_hint = { .ai_family = AF_UNSPEC };
	addrinfo* dns_result;

	if (auto error = getaddrinfo(connection->host_or_device.c_str(), "", &dns_hint, &dns_result))
	{
		switch (error)
		{
			case EAI_FAIL:
			case EAI_AGAIN:
			case EAI_NONAME:
				break;

			default:
				aprservice_log_error(getaddrinfo, error);
				break;
		}

		return false;
	}

	union
	{
		sockaddr_in  dns_result_ipv4;
		sockaddr_in6 dns_result_ipv6;
	};

	sockaddr* dns_result_address;
	int       dns_result_address_length = 0;

	switch (dns_result->ai_family)
	{
		case AF_INET:
			dns_result_ipv4          = *reinterpret_cast<const sockaddr_in*>(dns_result->ai_addr);
			dns_result_ipv4.sin_port = htons(connection->port);
			dns_result_address        = reinterpret_cast<sockaddr*>(&dns_result_ipv4);
			dns_result_address_length = sizeof(sockaddr_in);
			break;

		case AF_INET6:
			dns_result_ipv6           = *reinterpret_cast<const sockaddr_in6*>(dns_result->ai_addr);
			dns_result_ipv6.sin6_port = htons(connection->port);
			dns_result_address        = reinterpret_cast<sockaddr*>(&dns_result_ipv6);
			dns_result_address_length = sizeof(sockaddr_in6);
			break;
	}

	freeaddrinfo(dns_result);

	if (!dns_result_address_length)
		return false;

socket_open:
#if defined(APRSERVICE_UNIX)
	if ((connection->socket = socket(dns_result_address->sa_family, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		auto error = errno;

		aprservice_log_error(socket, error);

		return false;
	}
#elif defined(APRSERVICE_WIN32)
	if ((connection->socket = WSASocketW(dns_result_address->sa_family, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, 0)) == INVALID_SOCKET)
	{
		auto error = WSAGetLastError();

		aprservice_log_error(WSASocketW, error);

		return false;
	}
#endif

socket_connect:
#if defined(APRSERVICE_UNIX)
	if (connect(connection->socket, dns_result_address, dns_result_address_length) == -1)
	{
		auto error = errno;

		aprservice_log_error(connect, error);

		close(connection->socket);

		return false;
	}

	int flags;

	if ((flags = fcntl(connection->socket, F_GETFL, 0)) == -1)
	{
		auto error = errno;

		aprservice_log_error(fcntl, error);

		close(connection->socket);

		return false;
	}

	flags |= O_NONBLOCK;

	if (fcntl(connection->socket, F_SETFL, flags) == -1)
	{
		auto error = errno;

		aprservice_log_error(fcntl, error);

		close(connection->socket);

		return false;
	}
#elif defined(APRSERVICE_WIN32)
	if (connect(connection->socket, dns_result_address, dns_result_address_length) == SOCKET_ERROR)
	{
		auto error = WSAGetLastError();

		aprservice_log_error(connect, error);

		closesocket(connection->socket);

		return false;
	}

	u_long arg = 1;

	if (ioctlsocket(connection->socket, FIONBIO, &arg))
	{
		auto error = WSAGetLastError();

		aprservice_log_error(ioctlsocket, error);

		closesocket(connection->socket);

		return false;
	}
#endif

	connection->is_open = true;

	return true;
}
bool                                       aprservice_connection_open_serial(aprservice_connection* connection)
{
#if defined(APRSERVICE_UNIX)
	if ((connection->serial = open(connection->host_or_device.c_str(), O_RDWR | O_NOCTTY)) == -1)
	{
		auto error = errno;

		aprservice_log_error(open, error);

		return false;
	}

	fcntl(connection->serial, F_SETFL, O_RDWR);

	termios options;
	tcgetattr(connection->serial, &options);
	cfmakeraw(&options);
	cfsetspeed(&options, connection->device_speed);

	options.c_cc[VMIN]  = 1;
	options.c_cc[VTIME] = 0;
	options.c_cflag    |= CS8;
	options.c_cflag    |= CLOCAL | CREAD;
	options.c_cflag    &= ~(PARENB | CSTOPB | CSIZE);
	options.c_cflag    &= ~(ICANON | ECHO | ECHOE | ISIG | OPOST);

	tcsetattr(connection->serial, TCSANOW, &options);

	int status;

	if (ioctl(connection->serial, TIOCMGET, &status) == -1)
	{
		auto error = errno;

		aprservice_log_error(ioctl, error);

		close(connection->serial);

		return false;
	}

	status |= TIOCM_DTR | TIOCM_RTS;

	if (ioctl(connection->serial, TIOCMSET, &status) == -1)
	{
		auto error = errno;

		aprservice_log_error(ioctl, error);

		close(connection->serial);

		return false;
	}

	int flags;

	if ((flags = fcntl(connection->serial, F_GETFL, 0)) == -1)
	{
		auto error = errno;

		aprservice_log_error(fcntl, error);

		close(connection->serial);

		return false;
	}

	flags |= O_NONBLOCK;

	if (fcntl(connection->serial, F_SETFL, flags) == -1)
	{
		auto error = errno;

		aprservice_log_error(fcntl, error);

		close(connection->serial);

		return false;
	}
#elif defined(APRSERVICE_WIN32)
	if ((connection->serial = CreateFileA(connection->host_or_device.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0)) == INVALID_HANDLE_VALUE)
	{
		auto error = GetLastError();

		aprservice_log_error(CreateFileA, error);

		return false;
	}

	DCB dcb       = {};
	dcb.DCBlength = sizeof(DCB);

	if (!GetCommState(connection->serial, &dcb))
	{
		auto error = GetLastError();

		aprservice_log_error(GetCommState, error);

		CloseHandle(connection->serial);

		return false;
	}

	dcb.Parity   = NOPARITY;
	dcb.fBinary  = TRUE;
	dcb.BaudRate = connection->device_speed;
	dcb.ByteSize = 8;
	dcb.StopBits = ONESTOPBIT;

	if (!SetCommState(connection->serial, &dcb))
	{
		auto error = GetLastError();

		aprservice_log_error(SetCommState, error);

		CloseHandle(connection->serial);

		return false;
	}

	COMMTIMEOUTS timeouts = {};

	if (!SetCommTimeouts(connection->serial, &timeouts))
	{
		auto error = GetLastError();

		aprservice_log_error(SetCommTimeouts, error);

		CloseHandle(connection->serial);

		return false;
	}
#endif

	connection->is_open = true;

	return true;
}
bool                                       aprservice_connection_open(aprservice_connection* connection)
{
	if (aprservice_connection_is_open(connection))
		return false;

	switch (connection->type)
	{
		case APRSERVICE_CONNECTION_TYPE_APRS_IS:
		case APRSERVICE_CONNECTION_TYPE_KISS_TNC_TCP:
			if (!aprservice_connection_open_tcp(connection))
			{
				// aprservice_log_error(aprservice_connection_open_tcp, false);

				return false;
			}
			break;

		case APRSERVICE_CONNECTION_TYPE_KISS_TNC_SERIAL:
			if (!aprservice_connection_open_serial(connection))
			{
				// aprservice_log_error(aprservice_connection_open_serial, false);

				return false;
			}
			break;
	}

	aprservice_event_execute(connection->service, APRSERVICE_EVENT_CONNECT, { });

	static auto generate_auth_request = [](aprservice* service, uint16_t passcode)
	{
		std::stringstream ss;
		ss << "user " << service->station << " pass " << (passcode ? passcode : -1);
		ss << " vers " << APRSERVICE_SOFTWARE_NAME << ' ' << APRSERVICE_SOFTWARE_VERSION;
		ss << " filter t/poimqstunw";

		return ss.str();
	};

	switch (connection->type)
	{
		case APRSERVICE_CONNECTION_TYPE_APRS_IS:
			if (auto auth_request = generate_auth_request(connection->service, connection->passcode); !aprservice_connection_write_aprs_is(connection, std::move(auth_request)))
			{
				aprservice_log_error(aprservice_connection_write_aprs_is, false);

				aprservice_connection_close(connection);

				return false;
			}
			connection->auth.state    = APRSERVICE_AUTH_STATE_SENT;
			connection->auth.success  = false;
			connection->auth.verified = false;
			break;

		case APRSERVICE_CONNECTION_TYPE_KISS_TNC_TCP:
		case APRSERVICE_CONNECTION_TYPE_KISS_TNC_SERIAL:
			connection->auth.state    = APRSERVICE_AUTH_STATE_RECEIVED;
			connection->auth.success  = true;
			connection->auth.verified = true;
			connection->auth.message.clear();
			aprservice_event_execute(connection->service, APRSERVICE_EVENT_AUTHENTICATE, { .message = connection->auth.message.c_str(), .success = connection->auth.success, .verified = connection->auth.verified });
			break;
	}

	return true;
}
void                                       aprservice_connection_close(aprservice_connection* connection)
{
	if (aprservice_connection_is_open(connection))
	{
		switch (connection->type)
		{
			case APRSERVICE_CONNECTION_TYPE_APRS_IS:
			case APRSERVICE_CONNECTION_TYPE_KISS_TNC_TCP:
#if defined(APRSERVICE_UNIX)
				close(connection->socket);
#elif defined(APRSERVICE_WIN32)
				closesocket(connection->socket);
#endif
				break;

			case APRSERVICE_CONNECTION_TYPE_KISS_TNC_SERIAL:
#if defined(APRSERVICE_UNIX)
				close(connection->serial);
#elif defined(APRSERVICE_WIN32)
				CloseHandle(connection->serial);
#endif
				break;
		}

		connection->rx_buffer.clear();

		while (!connection->rx_queue.empty())
			connection->rx_queue.pop();

		while (!connection->tx_queue.empty())
			connection->tx_queue.pop();

		connection->is_open = false;

		connection->auth.state = APRSERVICE_AUTH_STATE_NONE;

		aprservice_event_execute(connection->service, APRSERVICE_EVENT_DISCONNECT, { });
	}
}
// @return false on connection closed
bool                                       aprservice_connection_poll_aprs_is(aprservice_connection* connection)
{
	size_t number_of_bytes_received;

read_once:
	switch (aprservice_connection_read(connection, &connection->rx_buffer_tmp[0], connection->rx_buffer_tmp.max_size(), &number_of_bytes_received))
	{
		case 0:
			return false;

		case -1:
			break;

		default:
			connection->rx_buffer.append(&connection->rx_buffer_tmp[0], number_of_bytes_received);

			for (auto i = connection->rx_buffer.find("\r\n"); i != std::string::npos; i = connection->rx_buffer.find("\r\n"))
			{
				connection->rx_queue.push(connection->rx_buffer.substr(0, i));
				connection->rx_buffer = connection->rx_buffer.erase(0, i + 2);
			}
			goto read_once;
	}

	return true;
}
// @return false on connection closed
bool                                       aprservice_connection_poll_kiss_tnc(aprservice_connection* connection)
{
	size_t number_of_bytes_received;

read_once:
	switch (aprservice_connection_read(connection, &connection->rx_buffer_tmp[0], connection->rx_buffer_tmp.max_size(), &number_of_bytes_received))
	{
		case 0:
			return false;

		case -1:
			break;

		default:
			connection->rx_buffer.append(&connection->rx_buffer_tmp[0], number_of_bytes_received);

		parse_once:
			uint8_t              command;
			std::vector<uint8_t> command_buffer;

			command_buffer.reserve(connection->rx_buffer.length());

			for (size_t i = 0, e = 0; i < connection->rx_buffer.length(); ++i)
				if (connection->rx_buffer[i] == (char)KISS_TNC_SPECIAL_CHARACTER_FRAME_END)
					for (size_t j = i + 2; j < connection->rx_buffer.length(); ++j)
						switch (uint8_t byte = connection->rx_buffer[j])
						{
							case KISS_TNC_SPECIAL_CHARACTER_FRAME_END:
								if (e)
									command_buffer.push_back(KISS_TNC_SPECIAL_CHARACTER_FRAME_END);
								else
								{
									command               = connection->rx_buffer[i + 1] & 0x0F;
									connection->rx_buffer = connection->rx_buffer.erase(0, j + 1);

									if ((command & 0x0F) == KISS_TNC_COMMAND_DATA)
									{
										std::string string;

										string.reserve(command_buffer.size());

										static auto command_decode_path    = [](std::string& string, const std::vector<uint8_t>& buffer, size_t offset)
										{
											bool is_last;

											do
											{
												string.append(1, ',');

												is_last = buffer[offset + 6] & 0x01;

												for (size_t i = 0; i < 6; ++i, ++offset)
													if (buffer[offset] != 0x40)
														string.append(1, (char)(buffer[offset] >> 1));

												bool is_repeated = buffer[offset] & 0x80;

												if (uint8_t ssid = ((buffer[offset++] & 0x0E) >> 1))
												{
													string.append(1, '-');

													do
														string.append(1, '0' + ssid % 10);
													while (ssid /= 10);
												}

												if (is_repeated)
													string.append(1, '*');
											} while (!is_last);

											return offset;
										};
										static auto command_decode_tocall  = [](std::string& string, const std::vector<uint8_t>& buffer, size_t offset)
										{
											for (size_t i = 0; i < 6; ++i, ++offset)
												if (buffer[offset] != 0x40)
													string.append(1, (char)(buffer[offset] >> 1));

											if (uint8_t ssid = ((buffer[offset++] & 0x1E) >> 1))
											{
												string.append(1, '-');

												do
													string.append(1, '0' + ssid % 10);
												while (ssid /= 10);
											}
										};
										static auto command_decode_station = [](std::string& string, const std::vector<uint8_t>& buffer, size_t offset)
										{
											for (size_t i = 0; i < 6; ++i, ++offset)
												if (buffer[offset] != 0x40)
													string.append(1, (char)(buffer[offset] >> 1));

											if (uint8_t ssid = ((buffer[offset++] & 0x1E) >> 1))
											{
												string.append(1, '-');

												do
													string.append(1, '0' + ssid % 10);
												while (ssid /= 10);
											}

											string.append(1, '>');
										};

										command_decode_station(string, command_buffer, 7);
										command_decode_tocall(string, command_buffer, 0);
										auto offset = command_decode_path(string, command_buffer, 14);

										if ((command_buffer[offset++] == 0x03) && (command_buffer[offset++] == 0xF0))
										{
											string.append(1, ':');

											if (command_buffer.back() != '\r')
												string.append((const char*)&command_buffer[offset], command_buffer.size() - offset);
											else
												string.append((const char*)&command_buffer[offset], (command_buffer.size() - offset) - 1);

											connection->rx_queue.push(std::move(string));
										}
									}

									goto parse_once;
								}
								break;

							case KISS_TNC_SPECIAL_CHARACTER_FRAME_ESCAPE:
								e = e ? 0 : 1;
								break;

							case KISS_TNC_SPECIAL_CHARACTER_TRANSPOSED_FRAME_END:
								if (e) command_buffer.push_back(KISS_TNC_SPECIAL_CHARACTER_FRAME_END);
								break;

							case KISS_TNC_SPECIAL_CHARACTER_TRANSPOSED_FRAME_ESCAPE:
								if (e) command_buffer.push_back(KISS_TNC_SPECIAL_CHARACTER_FRAME_ESCAPE);
								break;

							default:
								command_buffer.push_back((char)byte);
								break;
						}
			goto read_once;
	}

	return true;
}
bool                                       aprservice_connection_poll(aprservice_connection* connection)
{
	if (!aprservice_connection_is_open(connection))
		return false;

	size_t number_of_bytes_transferred;

	while (auto buffer = connection->tx_queue.empty() ? nullptr : &connection->tx_queue.front())
	{
		auto buffer_size = buffer->value.length();

		switch (aprservice_connection_write(connection, &buffer->value[buffer->offset], buffer_size - buffer->offset, &number_of_bytes_transferred))
		{
			case 0:  return false;
			case -1: return true;
		}

		if ((buffer->offset += number_of_bytes_transferred) == buffer_size)
			connection->tx_queue.pop();
	}

	switch (connection->type)
	{
		case APRSERVICE_CONNECTION_TYPE_APRS_IS:
			if (!aprservice_connection_poll_aprs_is(connection))
				return false;
			break;

		case APRSERVICE_CONNECTION_TYPE_KISS_TNC_TCP:
		case APRSERVICE_CONNECTION_TYPE_KISS_TNC_SERIAL:
			if (!aprservice_connection_poll_kiss_tnc(connection))
				return false;
			break;
	}

	if ((aprservice_get_time(connection->service) - connection->io_time) >= connection->service->connection_timeout)
	{
		aprservice_connection_close(connection);

		return false;
	}

	return true;
}
int                                        aprservice_connection_read(aprservice_connection* connection, void* buffer, size_t size, size_t* number_of_bytes_received)
{
	if (!aprservice_connection_is_open(connection))
		return 0;

	switch (connection->type)
	{
		case APRSERVICE_CONNECTION_TYPE_APRS_IS:
		case APRSERVICE_CONNECTION_TYPE_KISS_TNC_TCP:
		{
#if defined(APRSERVICE_UNIX)
			ssize_t bytes_received;

			switch (bytes_received = recv(connection->socket, buffer, size, 0))
			{
				case 0:
					aprservice_connection_close(connection);
					return 0;

				case -1:
				{
					auto error = errno;

					switch (error)
					{
						case EAGAIN:
	#if EAGAIN != EWOULDBLOCK
						case EWOULDBLOCK:
	#endif
							return -1;

						case ECONNRESET:
							aprservice_connection_close(connection);
							return 0;
					}

					aprservice_log_error(recv, error);
					aprservice_connection_close(connection);
				}
				return 0;

				default:
					*number_of_bytes_received = bytes_received;
					break;
			}
#elif defined(APRSERVICE_WIN32)
			int bytes_received;

			switch (bytes_received = recv(connection->socket, reinterpret_cast<char*>(buffer), static_cast<int>(size), 0))
			{
				case 0:
					aprservice_connection_close(connection);
					return 0;

				case SOCKET_ERROR:
				{
					auto error = WSAGetLastError();

					switch (error)
					{
						case WSAEINPROGRESS:
						case WSAEWOULDBLOCK:
							return -1;

						case WSAECONNRESET:
							aprservice_connection_close(connection);
							return 0;
					}

					aprservice_log_error(recv, error);
					aprservice_connection_close(connection);
				}
				return 0;

				default:
					*number_of_bytes_received = bytes_received;
					break;
			}
#endif
		}
		break;

		case APRSERVICE_CONNECTION_TYPE_KISS_TNC_SERIAL:
		{
#if defined(APRSERVICE_UNIX)
			int bytes_received;

			if ((bytes_received = read(connection->serial, buffer, size)) == -1)
			{
				auto error = errno;

				aprservice_log_error(read, error);

				aprservice_connection_close(connection);

				return 0;
			}

			if (bytes_received == 0)
				return -1;

			*number_of_bytes_received = bytes_received;
#elif defined(APRSERVICE_WIN32)
			DWORD bytes_received;

			if (!ReadFile(connection->serial, &buffer, size, &bytes_received, nullptr))
			{
				auto error = GetLastError();

				aprservice_log_error(ReadFile, error);

				aprservice_connection_close(connection);

				return 0;
			}

			if (bytes_received == 0)
				return -1;

			*number_of_bytes_received = bytes_received;
#endif
		}
		break;
	}

	connection->io_time = aprservice_get_time(connection->service);

	return 1;
}
int                                        aprservice_connection_write(aprservice_connection* connection, const void* buffer, size_t size, size_t* number_of_bytes_sent)
{
	if (!aprservice_connection_is_open(connection))
		return 0;

	switch (connection->type)
	{
		case APRSERVICE_CONNECTION_TYPE_APRS_IS:
		case APRSERVICE_CONNECTION_TYPE_KISS_TNC_TCP:
		{
#if defined(APRSERVICE_UNIX)
			ssize_t bytes_sent;

			if ((bytes_sent = send(connection->socket, buffer, size, 0)) == -1)
			{
				auto error = errno;

				switch (error)
				{
					case EAGAIN:
	#if EAGAIN != EWOULDBLOCK
					case EWOULDBLOCK:
	#endif
						return -1;

					case ECONNRESET:
						aprservice_connection_close(connection);
						return 0;
				}

				aprservice_log_error(send, error);
				aprservice_connection_close(connection);

				return 0;
			}

			if (number_of_bytes_sent)
				*number_of_bytes_sent = bytes_sent;
#elif defined(APRSERVICE_WIN32)
			int bytes_sent;

			if ((bytes_sent = send(connection->socket, reinterpret_cast<const char*>(buffer), static_cast<int>(size), 0)) == SOCKET_ERROR)
			{
				auto error = WSAGetLastError();

				switch (error)
				{
					case WSAENOBUFS:
					case WSAEINPROGRESS:
					case WSAEWOULDBLOCK:
						return -1;

					case WSAECONNRESET:
						aprservice_connection_close(connection);
						return 0;
				}

				aprservice_log_error(send, error);
				aprservice_connection_close(connection);

				return 0;
			}

			if (number_of_bytes_sent)
				*number_of_bytes_sent = bytes_sent;
#endif
		}
		break;

		case APRSERVICE_CONNECTION_TYPE_KISS_TNC_SERIAL:
		{
#if defined(APRSERVICE_UNIX)
			int bytes_sent;

			if ((bytes_sent = write(connection->serial, buffer, size)) == -1)
			{
				auto error = errno;

				aprservice_log_error(write, error);

				aprservice_connection_close(connection);

				return 0;
			}

			if (bytes_sent == 0)
				return -1;

			if (number_of_bytes_sent)
				*number_of_bytes_sent = bytes_sent;
#elif defined(APRSERVICE_WIN32)
			DWORD bytes_sent;

			if (!WriteFile(connection->serial, buffer, size, &bytes_sent, nullptr))
			{
				auto error = GetLastError();

				aprservice_log_error(WriteFile, error);

				aprservice_connection_close(connection);

				return 0;
			}

			if (bytes_sent == 0)
				return -1;

			if (number_of_bytes_sent)
				*number_of_bytes_sent = bytes_sent;
#endif
		}
		break;
	}

	connection->io_time = aprservice_get_time(connection->service);

	return 1;
}
bool                                       aprservice_connection_read_string(aprservice_connection* connection, std::string& value)
{
	if (!aprservice_connection_is_open(connection))
		return false;

	if (connection->rx_queue.empty())
		return false;

	value = std::move(connection->rx_queue.front());

	connection->rx_queue.pop();

	return true;
}
bool                                       aprservice_connection_write_packet(aprservice_connection* connection, aprs_packet* value)
{
	if (aprservice_connection_is_open(connection))
		switch (connection->type)
		{
			case APRSERVICE_CONNECTION_TYPE_APRS_IS:
				if (auto string = aprs_packet_to_string(value))
					return aprservice_connection_write_aprs_is(connection, string);
				break;

			case APRSERVICE_CONNECTION_TYPE_KISS_TNC_TCP:
			case APRSERVICE_CONNECTION_TYPE_KISS_TNC_SERIAL:
				// aprs_packet_to_string populates packet content
				if (aprs_packet_to_string(value))
				{
					static auto packet_to_ax25   = [](aprs_packet* value)
					{
						auto        path         = aprs_packet_get_path(value);
						auto        path_size    = aprs_path_get_length(path);
						std::string content      = aprs_packet_get_content(value);
						auto        content_size = content.length();

						std::vector<uint8_t> buffer(14 + (path_size * 7) + 2 + content_size, 0);
						static auto          buffer_encode_station = [](std::vector<uint8_t>& buffer, size_t offset, const char* station, bool repeated)
						{
							size_t i = 0;
							auto   b = &buffer[offset];

						encode_call:
							for (; i < 6; ++i)
								switch (*station)
								{
									case '-':
									case '*':
									case '\0':
										goto encode_ssid;

									default:
										*(b++) = *(station++) << 1;
										break;
								}

						encode_ssid:
							for (; i < 6; ++i)
								*(b++) = 0x40;

							switch (*station)
							{
								case '-':
									++station;
									for (i = 0; i < 2; ++i)
										if (!station[i] || repeated)
											break;
									*b = i ? ((aprservice_parse_uint<uint8_t>(station, i) & 0x0F) << 1) : 0;
									if (repeated)
										*b |= 0x80;
									break;

								case '*':
									*b |= 0x80;
									break;

								case '\0':
									*b  = 0;
									break;
							}
						};
						static auto          buffer_encode_path    = [](std::vector<uint8_t>& buffer, size_t offset, aprs_path* value)
						{
							auto path_node   = aprs_path_get(value);
							auto path_length = aprs_path_get_length(value);

							for (size_t i = 1; i <= path_length; ++i, ++path_node, offset += 7)
							{
								buffer_encode_station(buffer, offset, path_node->station, path_node->repeated);

								if (i == path_length)
									buffer[offset + 6] |= 0x01;
							}

							return offset;
						};

						buffer_encode_station(buffer, 0, aprs_packet_get_tocall(value), false);
						buffer_encode_station(buffer, 7, aprs_packet_get_sender(value), false);
						auto offset      = buffer_encode_path(buffer, 14, path);
						buffer[offset++] = 0x03;
						buffer[offset++] = 0xF0;
						memcpy(&buffer[offset], content.c_str(), content_size);

						return buffer;
					};
					static auto ax25_to_kiss_tnc = [](const uint8_t* value, size_t count)
					{
						std::string buffer;

						buffer.reserve(1 + 1 + (count * 2) + 1);
						buffer.append(1, (char)KISS_TNC_SPECIAL_CHARACTER_FRAME_END);
						buffer.append(1, (char)KISS_TNC_COMMAND_DATA);
						for (size_t i = 0; i < count; ++i, ++value)
						{
							switch (*value)
							{
								case KISS_TNC_SPECIAL_CHARACTER_FRAME_END:
									buffer.append(1, (char)KISS_TNC_SPECIAL_CHARACTER_FRAME_ESCAPE);
									buffer.append(1, (char)KISS_TNC_SPECIAL_CHARACTER_TRANSPOSED_FRAME_END);
									break;

								case KISS_TNC_SPECIAL_CHARACTER_FRAME_ESCAPE:
									buffer.append(1, (char)KISS_TNC_SPECIAL_CHARACTER_FRAME_ESCAPE);
									buffer.append(1, (char)KISS_TNC_SPECIAL_CHARACTER_TRANSPOSED_FRAME_ESCAPE);
									break;

								default:
									buffer.append(1, *value);
									break;
							}
						}
						buffer.append(1, (char)KISS_TNC_SPECIAL_CHARACTER_FRAME_END);

						return buffer;
					};

					auto ax25 = packet_to_ax25(value);
					auto kiss = ax25_to_kiss_tnc(ax25.data(), ax25.size());

					connection->tx_queue.push({ .value = std::move(kiss), .offset = 0 });

					return true;
				}
				break;
		}

	return false;
}
bool                                       aprservice_connection_write_aprs_is(aprservice_connection* connection, std::string&& value)
{
	if (!aprservice_connection_is_open(connection))
		return false;

	connection->tx_queue.push({ .value = std::move(value), .offset = 0 });
	connection->tx_queue.push({ .value = "\r\n",           .offset = 0 });

	return true;
}
// @return 0 on error
// @return -1 on timeout
// @return -2 on connection closed
int                                        aprservice_connection_wait_for_io(aprservice_connection* connection, uint32_t timeout)
{
	if (!aprservice_connection_is_open(connection))
		return -2;

	if (!connection->tx_queue.empty())
		return 1;

	bool would_block = false;

	switch (connection->type)
	{
		case APRSERVICE_CONNECTION_TYPE_APRS_IS:
		case APRSERVICE_CONNECTION_TYPE_KISS_TNC_TCP:
		{
#if defined(APRSERVICE_UNIX)
			pollfd fd = { .fd = connection->socket, .events = POLLRDNORM };

			switch (poll(&fd, 1, (int)(timeout * 1000)))
			{
				case 0:
					would_block = true;
					break;

				case -1:
				{
					auto error = errno;

					aprservice_log_error(poll, error);

					aprservice_connection_close(connection);
				}
				return -2;
			}
#elif defined(APRSERVICE_WIN32)
			WSAPOLLFD fd = { .fd = connection->socket, .events = POLLRDNORM };

			switch (WSAPoll(&fd, 1, (INT)(timeout * 1000)))
			{
				case 0:
					would_block = true;
					break;

				case SOCKET_ERROR:
				{
					auto error = WSAGetLastError();

					aprservice_log_error(WSAPoll, error);

					aprservice_connection_close(connection);
				}
				return -2;
			}
#endif

			if (!would_block)
				connection->io_time = aprservice_get_time(connection->service);
			else if ((aprservice_get_time(connection->service) - connection->io_time) >= connection->service->connection_timeout)
			{
				aprservice_connection_close(connection);

				return -2;
			}
		}
		break;

		case APRSERVICE_CONNECTION_TYPE_KISS_TNC_SERIAL:
		{
			char buffer;
			static_assert(sizeof(buffer) == 1);

#if defined(APRSERVICE_UNIX)
			pollfd fd = { .fd = connection->serial, .events = POLLRDNORM };

			switch (poll(&fd, 1, (int)(timeout * 1000)))
			{
				case 0:
					would_block = true;
					break;

				case -1:
				{
					auto error = errno;

					aprservice_log_error(poll, error);

					aprservice_connection_close(connection);
				}
				return -2;
			}
#elif defined(APRSERVICE_WIN32)
			COMMTIMEOUTS timeouts[2] = {};

			if (timeout)
			{
				timeouts[1].ReadIntervalTimeout        = MAXDWORD;
				timeouts[1].ReadTotalTimeoutConstant   = timeout * 1000;
				timeouts[1].ReadTotalTimeoutMultiplier = 0;

				if (!GetCommTimeouts(connection->serial, &timeouts[0]))
				{
					auto error = GetLastError();

					aprservice_log_error(GetCommTimeouts, error);

					return 0;
				}

				if (!SetCommTimeouts(connection->serial, &timeouts[1]))
				{
					auto error = GetLastError();

					aprservice_log_error(SetCommTimeouts, error);

					aprservice_connection_close(connection);

					return 0;
				}
			}

			size_t number_of_bytes_received;

			switch (aprservice_connection_read(connection, &buffer, sizeof(buffer), &number_of_bytes_received))
			{
				case 0:
					return -2;

				case -1:
					would_block = true;
					break;

				default:
					if (number_of_bytes_received == sizeof(buffer))
						connection->rx_buffer.push_back(buffer);
					break;
			}

			if (timeout && !SetCommTimeouts(connection->serial, &timeouts[0]))
			{
				auto error = GetLastError();

				aprservice_log_error(SetCommTimeouts, error);

				return 0;
			}
#endif
		}
		break;
	}

	return would_block ? -1 : 1;
}

void                                       aprservice_poll_tasks(struct aprservice* service);
void                                       aprservice_poll_messages(struct aprservice* service);
bool                                       aprservice_poll_connection(struct aprservice* service);
bool                                       aprservice_send_message_ack(struct aprservice* service, const char* destination, const char* id);
bool                                       aprservice_send_message_reject(struct aprservice* service, const char* destination, const char* id);
bool                                       aprservice_execute_command(struct aprservice* service, struct aprs_packet* packet, const char* sender, std::string_view name, const char* args);

struct aprservice*         APRSERVICE_CALL aprservice_init(const char* station, struct aprs_path* path, char symbol_table, char symbol_table_key)
{
	if (!station || !path)
		return nullptr;

	auto service = new aprservice
	{
		.is_monitoring      = false,

		.time               = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count(),
		.time_type          = APRS_TIME_ZULU_HMS,

		.path               = path,
		.station            = station,
		.connection_timeout = 2 * 60,

		.command_prefix     = "."
	};

	if (!(service->position = aprs_packet_position_init(station, APRSERVICE_TOCALL, path, 0, 0, 0, 0, 0, "", symbol_table, symbol_table_key, aprservice_get_time_type(service))))
	{
		aprservice_log_error(aprs_packet_position_init, nullptr);

		delete service;

		return nullptr;
	}

	aprs_path_add_reference(path);

	return service;
}
void                       APRSERVICE_CALL aprservice_deinit(struct aprservice* service)
{
	if (aprservice_is_connected(service))
		aprservice_disconnect(service);

	for (auto it = service->tasks.begin(); it != service->tasks.end(); )
	{
		for (auto jt = it->second.begin(); jt != it->second.end(); )
		{
			aprservice_task_information task =
			{
				.is_canceled = true,

				.seconds     = (*jt)->seconds,
				.reschedule  = false
			};

			(*jt)->handler(service, &task, (*jt)->handler_param);

			delete *jt;

			it->second.erase(jt++);
		}

		service->tasks.erase(it++);
	}

	service->items.remove_if([](aprservice_item& item) {
		aprs_packet_deinit(item.packet);

		return true;
	});

	service->objects.remove_if([](aprservice_object& object) {
		aprs_packet_deinit(object.packet);

		return true;
	});

	for (auto it = service->commands.begin(); it != service->commands.end(); )
		aprservice_command_unregister(&*it++);

	aprs_packet_deinit(service->position);
	aprs_path_deinit(service->path);

	delete service;
}
bool                       APRSERVICE_CALL aprservice_is_read_only(struct aprservice* service)
{
	if (aprservice_is_connected(service))
		if (auto auth = &service->connection->auth; auth->state == APRSERVICE_AUTH_STATE_RECEIVED)
			return !auth->success || !auth->verified;

	return true;
}
bool                       APRSERVICE_CALL aprservice_is_connected(struct aprservice* service)
{
	return service->connection != nullptr;
}
bool                       APRSERVICE_CALL aprservice_is_authenticated(struct aprservice* service)
{
	if (aprservice_is_connected(service))
		if (auto auth = &service->connection->auth; auth->state == APRSERVICE_AUTH_STATE_RECEIVED)
			return auth->success && auth->verified;

	return false;
}
bool                       APRSERVICE_CALL aprservice_is_authenticating(struct aprservice* service)
{
	if (aprservice_is_connected(service))
		return service->connection->auth.state == APRSERVICE_AUTH_STATE_SENT;

	return false;
}
bool                       APRSERVICE_CALL aprservice_is_monitoring_enabled(struct aprservice* service)
{
	return service->is_monitoring;
}
bool                       APRSERVICE_CALL aprservice_is_compression_enabled(struct aprservice* service)
{
	return aprs_packet_position_is_mic_e(service->position) || aprs_packet_position_is_compressed(service->position);
}
struct aprs_path*          APRSERVICE_CALL aprservice_get_path(struct aprservice* service)
{
	return service->path;
}
uint32_t                   APRSERVICE_CALL aprservice_get_time(struct aprservice* service)
{
	return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count() - service->time;
}
int                        APRSERVICE_CALL aprservice_get_time_type(struct aprservice* service)
{
	return service->time_type;
}
const char*                APRSERVICE_CALL aprservice_get_comment(struct aprservice* service)
{
	return aprs_packet_position_get_comment(service->position);
}
const char*                APRSERVICE_CALL aprservice_get_station(struct aprservice* service)
{
	return service->station.c_str();
}
char                       APRSERVICE_CALL aprservice_get_symbol_table(struct aprservice* service)
{
	return aprs_packet_position_get_symbol_table(service->position);
}
char                       APRSERVICE_CALL aprservice_get_symbol_table_key(struct aprservice* service)
{
	return aprs_packet_position_get_symbol_table_key(service->position);
}
void                       APRSERVICE_CALL aprservice_get_position(struct aprservice* service, float* latitude, float* longitude, int32_t* altitude, uint16_t* speed, uint16_t* course)
{
	*speed     = aprs_packet_position_get_speed(service->position);
	*course    = aprs_packet_position_get_course(service->position);
	*altitude  = aprs_packet_position_get_altitude(service->position);
	*latitude  = aprs_packet_position_get_latitude(service->position);
	*longitude = aprs_packet_position_get_longitude(service->position);
}
int                        APRSERVICE_CALL aprservice_get_position_type(struct aprservice* service)
{
	if (aprs_packet_position_is_mic_e(service->position))
		return APRSERVICE_POSITION_TYPE_MIC_E;

	if (aprs_packet_position_is_compressed(service->position))
		return APRSERVICE_POSITION_TYPE_POSITION_COMPRESSED;

	return APRSERVICE_POSITION_TYPE_POSITION;
}
const char*                APRSERVICE_CALL aprservice_get_command_prefix(struct aprservice* service)
{
	return service->command_prefix.c_str();
}
uint32_t                   APRSERVICE_CALL aprservice_get_connection_timeout(struct aprservice* service)
{
	return service->connection_timeout;
}
bool                       APRSERVICE_CALL aprservice_get_event_handler(struct aprservice* service, enum APRSERVICE_EVENTS event, aprservice_event_handler* handler, void** param)
{
	if (event >= APRSERVICE_EVENTS_COUNT)
		return false;

	auto context = &service->events[event];

	*handler = context->handler;
	*param   = context->handler_param;

	return true;
}
void                       APRSERVICE_CALL aprservice_get_default_event_handler(struct aprservice* service, aprservice_event_handler* handler, void** param)
{
	auto context = &service->events[APRSERVICE_EVENTS_COUNT];

	*handler = context->handler;
	*param   = context->handler_param;
}
bool                       APRSERVICE_CALL aprservice_set_path(struct aprservice* service, struct aprs_path* value)
{
	if (!value)
		return false;

	aprs_path_deinit(service->path);

	service->path = value;

	aprs_path_add_reference(value);

	return true;
}
bool                       APRSERVICE_CALL aprservice_set_time_type(struct aprservice* service, int value)
{
	if (!aprs_time_type_is_valid(value))
		return false;

	service->time_type = value;
	service->objects;
	service->position;

	return true;
}
bool                       APRSERVICE_CALL aprservice_set_symbol(struct aprservice* service, char table, char key)
{
	if (!aprs_packet_position_set_symbol(service->position, table, key))
	{
		aprservice_log_error(aprs_packet_position_set_symbol, false);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_set_comment(struct aprservice* service, const char* value)
{
	if (!aprs_packet_position_set_comment(service->position, value))
	{
		aprservice_log_error(aprs_packet_position_set_comment, false);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_set_position(struct aprservice* service, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course)
{
	if (!aprs_packet_position_set_speed(service->position, speed))
	{
		aprservice_log_error(aprs_packet_position_set_speed, false);

		return false;
	}

	if (!aprs_packet_position_set_course(service->position, course))
	{
		aprservice_log_error(aprs_packet_position_set_course, false);

		return false;
	}

	if (!aprs_packet_position_set_altitude(service->position, altitude))
	{
		aprservice_log_error(aprs_packet_position_set_altitude, false);

		return false;
	}

	if (!aprs_packet_position_set_latitude(service->position, latitude))
	{
		aprservice_log_error(aprs_packet_position_set_latitude, false);

		return false;
	}

	if (!aprs_packet_position_set_longitude(service->position, longitude))
	{
		aprservice_log_error(aprs_packet_position_set_longitude, false);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_set_position_type(struct aprservice* service, enum APRSERVICE_POSITION_TYPES value)
{
	if (value >= APRSERVICE_POSITION_TYPES_COUNT)
		return false;

	switch (value)
	{
		case APRSERVICE_POSITION_TYPE_MIC_E:
			if (auto packet = aprs_packet_position_init_mic_e(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), aprs_packet_position_get_latitude(service->position), aprs_packet_position_get_longitude(service->position), aprs_packet_position_get_altitude(service->position), aprs_packet_position_get_speed(service->position), aprs_packet_position_get_course(service->position), aprs_packet_position_get_comment(service->position), aprs_packet_position_get_symbol_table(service->position), aprs_packet_position_get_symbol_table_key(service->position), APRS_MIC_E_MESSAGE_OFF_DUTY))
			{
				aprs_packet_deinit(service->position);

				service->position = packet;

				return true;
			}
			aprservice_log_error(aprs_packet_position_init_mic_e, nullptr);
			return false;

		case APRSERVICE_POSITION_TYPE_POSITION:
			if (auto packet = aprs_packet_position_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), aprs_packet_position_get_latitude(service->position), aprs_packet_position_get_longitude(service->position), aprs_packet_position_get_altitude(service->position), aprs_packet_position_get_speed(service->position), aprs_packet_position_get_course(service->position), aprs_packet_position_get_comment(service->position), aprs_packet_position_get_symbol_table(service->position), aprs_packet_position_get_symbol_table_key(service->position), aprservice_get_time_type(service)))
			{
				aprs_packet_deinit(service->position);

				service->position = packet;

				return true;
			}
			aprservice_log_error(aprs_packet_position_init, nullptr);
			return false;

		case APRSERVICE_POSITION_TYPE_POSITION_COMPRESSED:
			if (auto packet = aprs_packet_position_init_compressed(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), aprs_packet_position_get_latitude(service->position), aprs_packet_position_get_longitude(service->position), aprs_packet_position_get_altitude(service->position), aprs_packet_position_get_speed(service->position), aprs_packet_position_get_course(service->position), aprs_packet_position_get_comment(service->position), aprs_packet_position_get_symbol_table(service->position), aprs_packet_position_get_symbol_table_key(service->position), aprservice_get_time_type(service)))
			{
				aprs_packet_deinit(service->position);

				service->position = packet;

				return true;
			}
			aprservice_log_error(aprs_packet_position_init_compressed, nullptr);
			return false;
	}

	return false;
}
bool                       APRSERVICE_CALL aprservice_set_event_handler(struct aprservice* service, enum APRSERVICE_EVENTS event, aprservice_event_handler handler, void* param)
{
	if (event >= APRSERVICE_EVENTS_COUNT)
		return false;

	service->events[event] = { .handler = handler, .handler_param = param };

	return true;
}
void                       APRSERVICE_CALL aprservice_set_default_event_handler(struct aprservice* service, aprservice_event_handler handler, void* param)
{
	service->events[APRSERVICE_EVENTS_COUNT] = { .handler = handler, .handler_param = param };
}
void                       APRSERVICE_CALL aprservice_set_command_prefix(struct aprservice* service, const char* value)
{
	if (!value)
		service->command_prefix.clear();
	else
		service->command_prefix = value;
}
void                       APRSERVICE_CALL aprservice_set_connection_timeout(struct aprservice* service, uint32_t seconds)
{
	service->connection_timeout = seconds;
}
void                       APRSERVICE_CALL aprservice_enable_monitoring(struct aprservice* service, bool value)
{
	service->is_monitoring = value;
}
bool                       APRSERVICE_CALL aprservice_poll(struct aprservice* service)
{
	aprservice_poll_tasks(service);
	aprservice_poll_messages(service);

	if (aprservice_is_connected(service) && !aprservice_poll_connection(service))
		;

	return true;
}
void                                       aprservice_poll_tasks(struct aprservice* service)
{
	for (auto it = service->tasks.begin(); it != service->tasks.end(); )
	{
		if (it->first > aprservice_get_time(service))
			break;

		for (auto jt = it->second.begin(); jt != it->second.end(); )
		{
			auto                        task             = *jt;
			aprservice_task_information task_information =
			{
				.is_canceled = false,

				.seconds     = task->seconds,
				.reschedule  = false
			};

			task->handler(service, &task_information, task->handler_param);

			it->second.erase(jt++);

			if (!task_information.reschedule)
				delete task;
			else
				service->tasks[aprservice_get_time(service) + task_information.seconds].push_back(task);
		}

		service->tasks.erase(it++);
	}
}
void                                       aprservice_poll_messages(struct aprservice* service)
{
	service->message_callbacks.remove_if([service](aprservice_message_callback_context* context) {
		if (context->timeout < aprservice_get_time(service))
			return false;

		context->callback(service, APRSERVICE_MESSAGE_ERROR_TIMEOUT, context->callback_param);

		if (auto it = service->message_callbacks_index.find(context->station); it != service->message_callbacks_index.end())
		{
			for (auto jt = it->second.begin(); jt != it->second.end(); ++jt)
			{
				if (&*jt == context)
				{
					it->second.erase(jt);

					break;
				}
			}

			if (it->second.empty())
				service->message_callbacks_index.erase(it);
		}

		return true;
	});
}
bool                                       aprservice_poll_connection(struct aprservice* service)
{
	static auto on_receive_packet = [](aprservice* service, aprs_packet* packet, aprservice_connection* connection)
	{
		auto packet_type   = aprs_packet_get_type(packet);
		auto packet_sender = aprs_packet_get_sender(packet);

		aprservice_event_execute(service, APRSERVICE_EVENT_RECEIVE_PACKET, { .packet = packet });

		if (packet_type == APRS_PACKET_TYPE_MESSAGE)
		{
			auto packet_message_id          = aprs_packet_message_get_id(packet);
			auto packet_message_type        = aprs_packet_message_get_type(packet);
			auto packet_message_content     = aprs_packet_message_get_content(packet);
			auto packet_message_destination = aprs_packet_message_get_destination(packet);

			switch (packet_message_type)
			{
				case APRS_MESSAGE_TYPE_ACK:
				case APRS_MESSAGE_TYPE_REJECT:
					if (packet_message_id && !stricmp(aprservice_get_station(service), packet_message_destination))
						if (auto it = service->message_callbacks_index.find(packet_sender); it != service->message_callbacks_index.end())
							for (auto jt = it->second.begin(); jt != it->second.end(); ++jt)
								if (!jt->id.compare(packet_message_id))
								{
									jt->callback(service, (packet_message_type == APRS_MESSAGE_TYPE_ACK) ? APRSERVICE_MESSAGE_ERROR_SUCCESS : APRSERVICE_MESSAGE_ERROR_REJECTED, jt->callback_param);

									for (auto lt = service->message_callbacks.begin(); lt != service->message_callbacks.end(); ++lt)
										if (*lt == &*jt)
										{
											service->message_callbacks.erase(lt);

											break;
										}

									it->second.erase(jt);

									if (it->second.empty())
										service->message_callbacks_index.erase(it);

									break;
								}
					break;

				case APRS_MESSAGE_TYPE_MESSAGE:
					if (stricmp(aprservice_get_station(service), packet_message_destination))
					{
						if (aprservice_is_monitoring_enabled(service))
							aprservice_event_execute(service, APRSERVICE_EVENT_RECEIVE_MESSAGE, { .packet = packet, .id = packet_message_id, .sender = packet_sender, .content = packet_message_content, .destination = packet_message_destination });
					}
					else
					{
						if (packet_message_id)
							aprservice_send_message_ack(service, packet_sender, packet_message_id);

						if (std::string_view packet_message_content_view(packet_message_content); packet_message_content_view.starts_with(service->command_prefix))
						{
							std::string_view command_name = packet_message_content_view.substr(service->command_prefix.length());
							std::string_view command_args;

							if (auto i = packet_message_content_view.find_first_of(' '); i != packet_message_content_view.npos)
							{
								command_name = command_name.substr(0, i);

								if ((i = packet_message_content_view.find_first_not_of(' ', i)) != packet_message_content_view.npos)
									command_args = packet_message_content_view.substr(i);
							}

							if (!command_name.empty() || !aprservice_execute_command(service, packet, packet_sender, command_name.data(), command_args.data()))
								aprservice_event_execute(service, APRSERVICE_EVENT_RECEIVE_MESSAGE, { .packet = packet, .id = packet_message_id, .sender = packet_sender, .content = packet_message_content, .destination = packet_message_destination });
						}
					}
					break;
			}
		}
	};

	if (!aprservice_connection_poll(service->connection))
	{
		aprservice_disconnect(service);

		return false;
	}

	switch (service->connection->type)
	{
		case APRSERVICE_CONNECTION_TYPE_APRS_IS:
			while (aprservice_connection_read_string(service->connection, service->line))
				if (service->line.starts_with("# "))
				{
					if ((service->connection->auth.state == APRSERVICE_AUTH_STATE_SENT) && aprservice_connection_auth_from_string(&service->connection->auth, &service->line[2], service->connection->passcode != 0))
						aprservice_event_execute(service, APRSERVICE_EVENT_AUTHENTICATE, { .message = service->connection->auth.message.c_str(), .success = service->connection->auth.success, .verified = service->connection->auth.verified });
					else
						aprservice_event_execute(service, APRSERVICE_EVENT_RECEIVE_SERVER_MESSAGE, { .content = &service->line[2] });
				}
				else if (auto packet = aprs_packet_init_from_string(service->line.c_str()))
				{
					on_receive_packet(service, packet, service->connection);

					aprs_packet_deinit(packet);
				}
			break;

		case APRSERVICE_CONNECTION_TYPE_KISS_TNC_TCP:
		case APRSERVICE_CONNECTION_TYPE_KISS_TNC_SERIAL:
			while (aprservice_connection_read_string(service->connection, service->line))
				if (auto packet = aprs_packet_init_from_string(service->line.c_str()))
				{
					on_receive_packet(service, packet, service->connection);

					aprs_packet_deinit(packet);
				}
			break;
	}

	if (!aprservice_connection_is_open(service->connection))
	{
		aprservice_disconnect(service);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_send(struct aprservice* service, struct aprs_packet* packet)
{
	if (!aprservice_connection_write_packet(service->connection, packet))
	{
		aprservice_log_error(aprservice_connection_write_packet, false);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_send_raw(struct aprservice* service, const char* content)
{
	if (!aprservice_is_connected(service))
		return false;

	if (aprservice_is_read_only(service))
		return false;

	aprs_packet* packet;

	if (!(packet = aprs_packet_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service))))
	{
		aprservice_log_error(aprs_packet_init, nullptr);

		return false;
	}

	if (!aprservice_send(service, packet))
	{
		aprservice_log_error(aprservice_send, false);

		aprs_packet_deinit(packet);

		return false;
	}

	aprs_packet_deinit(packet);

	return true;
}
bool                       APRSERVICE_CALL aprservice_send_item(struct aprservice* service, const char* name, const char* comment, char symbol_table, char symbol_table_key, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course, bool live)
{
	if (!aprservice_is_connected(service))
		return false;

	if (aprservice_is_read_only(service))
		return false;

	aprs_packet* packet;

	if (!(packet = aprs_packet_item_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), name, symbol_table, symbol_table_key)))
	{
		aprservice_log_error(aprs_packet_item_init, nullptr);

		return false;
	}

	if (!aprs_packet_item_set_alive(packet, live))
	{
		aprservice_log_error(aprs_packet_item_set_alive, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprs_packet_item_set_speed(packet, speed))
	{
		aprservice_log_error(aprs_packet_item_set_speed, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprs_packet_item_set_course(packet, course))
	{
		aprservice_log_error(aprs_packet_item_set_course, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprs_packet_item_set_comment(packet, comment))
	{
		aprservice_log_error(aprs_packet_item_set_comment, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprs_packet_item_set_altitude(packet, altitude))
	{
		aprservice_log_error(aprs_packet_item_set_altitude, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprs_packet_item_set_latitude(packet, latitude))
	{
		aprservice_log_error(aprs_packet_item_set_latitude, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprs_packet_item_set_longitude(packet, longitude))
	{
		aprservice_log_error(aprs_packet_item_set_longitude, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprs_packet_item_set_compressed(packet, aprservice_is_compression_enabled(service)))
	{
		aprservice_log_error(aprs_packet_item_set_compressed, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprservice_send(service, packet))
	{
		aprservice_log_error(aprservice_send, false);

		aprs_packet_deinit(packet);

		return false;
	}

	aprs_packet_deinit(packet);

	return true;
}
bool                       APRSERVICE_CALL aprservice_send_object(struct aprservice* service, const char* name, const char* comment, char symbol_table, char symbol_table_key, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course, bool live)
{
	if (!aprservice_is_connected(service))
		return false;

	if (aprservice_is_read_only(service))
		return false;

	aprs_packet* packet;

	if (!(packet = aprs_packet_object_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), name, symbol_table, symbol_table_key, aprservice_get_time_type(service))))
	{
		aprservice_log_error(aprs_packet_object_init, nullptr);

		return false;
	}

	if (!aprs_packet_object_set_alive(packet, live))
	{
		aprservice_log_error(aprs_packet_object_set_alive, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprs_packet_object_set_speed(packet, speed))
	{
		aprservice_log_error(aprs_packet_object_set_speed, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprs_packet_object_set_course(packet, course))
	{
		aprservice_log_error(aprs_packet_object_set_course, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprs_packet_object_set_comment(packet, comment))
	{
		aprservice_log_error(aprs_packet_object_set_comment, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprs_packet_object_set_altitude(packet, altitude))
	{
		aprservice_log_error(aprs_packet_object_set_altitude, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprs_packet_object_set_latitude(packet, latitude))
	{
		aprservice_log_error(aprs_packet_object_set_latitude, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprs_packet_object_set_longitude(packet, longitude))
	{
		aprservice_log_error(aprs_packet_object_set_longitude, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprs_packet_object_set_compressed(packet, aprservice_is_compression_enabled(service)))
	{
		aprservice_log_error(aprs_packet_object_set_compressed, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprservice_send(service, packet))
	{
		aprservice_log_error(aprservice_send, false);

		aprs_packet_deinit(packet);

		return false;
	}

	aprs_packet_deinit(packet);

	return true;
}
bool                       APRSERVICE_CALL aprservice_send_status(struct aprservice* service, const char* message)
{
	if (!aprservice_is_connected(service))
		return false;

	if (aprservice_is_read_only(service))
		return false;

	aprs_packet* packet;

	if (!(packet = aprs_packet_status_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), message)))
	{
		aprservice_log_error(aprs_packet_status_init, nullptr);

		return false;
	}

	aprs_time time;

	if (!aprs_time_now(&time, aprservice_get_time_type(service)))
	{
		aprservice_log_error(aprs_time_now, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprs_packet_status_set_time(packet, &time))
	{
		aprservice_log_error(aprs_packet_status_set_time, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprservice_send(service, packet))
	{
		aprservice_log_error(aprservice_send, false);

		aprs_packet_deinit(packet);

		return false;
	}

	aprs_packet_deinit(packet);

	return true;
}
bool                       APRSERVICE_CALL aprservice_send_message(struct aprservice* service, const char* destination, const char* content, uint32_t timeout, aprservice_message_callback callback, void* param)
{
	char id[6] = {};

	if (service->message_count == 0)
		id[0] = '0';
	else
	{
		static constexpr char BASE62[] =
		{
			'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
			'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
			'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
		};

		size_t i = 0;

		for (size_t count = service->message_count; count; ++i, count /= 62)
			id[i] = BASE62[count % 62];

		for (size_t j = 0, l = i - 1; j < l; ++j, --l)
			std::swap(id[j], id[l]);
	}

	if (!aprservice_send_message_ex(service, destination, content, id, timeout, callback, param))
		return false;

	if (++service->message_count == 0x369B13E0)
		service->message_count = 0;

	return true;
}
bool                       APRSERVICE_CALL aprservice_send_message_ex(struct aprservice* service, const char* destination, const char* content, const char* id, uint32_t timeout, aprservice_message_callback callback, void* param)
{
	if (!aprservice_is_connected(service))
		return false;

	if (aprservice_is_read_only(service))
		return false;

	aprs_packet* packet;

	if (!(packet = aprs_packet_message_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), destination, content)))
	{
		aprservice_log_error(aprs_packet_message_init, nullptr);

		return false;
	}

	if (id && !aprs_packet_message_set_id(packet, id))
	{
		aprservice_log_error(aprs_packet_message_set_id, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprservice_send(service, packet))
	{
		aprservice_log_error(aprservice_send, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (id && callback && (aprs_packet_message_get_type(packet) == APRS_MESSAGE_TYPE_MESSAGE))
		service->message_callbacks.push_back(&service->message_callbacks_index[destination].emplace_back(aprservice_message_callback_context {
			.id             = id,
			.station        = destination,
			.timeout        = aprservice_get_time(service) + timeout,
			.callback       = callback,
			.callback_param = param
		}));

	aprs_packet_deinit(packet);

	return true;
}
bool                       APRSERVICE_CALL aprservice_send_message_ack(struct aprservice* service, const char* destination, const char* id)
{
	if (!aprservice_is_connected(service))
		return false;

	if (aprservice_is_read_only(service))
		return false;

	aprs_packet* packet;

	if (!(packet = aprs_packet_message_init_ack(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), destination, id)))
	{
		aprservice_log_error(aprs_packet_message_init_ack, nullptr);

		return false;
	}

	if (!aprservice_send(service, packet))
	{
		aprservice_log_error(aprservice_send, false);

		aprs_packet_deinit(packet);

		return false;
	}

	aprs_packet_deinit(packet);

	return true;
}
bool                       APRSERVICE_CALL aprservice_send_message_reject(struct aprservice* service, const char* destination, const char* id)
{
	if (!aprservice_is_connected(service))
		return false;

	if (aprservice_is_read_only(service))
		return false;

	aprs_packet* packet;

	if (!(packet = aprs_packet_message_init_reject(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), destination, id)))
	{
		aprservice_log_error(aprs_packet_message_init_reject, nullptr);

		return false;
	}

	if (!aprservice_send(service, packet))
	{
		aprservice_log_error(aprservice_send, false);

		aprs_packet_deinit(packet);

		return false;
	}

	aprs_packet_deinit(packet);

	return true;
}
bool                       APRSERVICE_CALL aprservice_send_weather(struct aprservice* service, uint16_t wind_speed, uint16_t wind_speed_gust, uint16_t wind_direction, uint16_t rainfall_last_hour, uint16_t rainfall_last_24_hours, uint16_t rainfall_since_midnight, uint8_t humidity, int16_t temperature, uint32_t barometric_pressure, const char* type, char software)
{
	if (!aprservice_is_connected(service))
		return false;

	if (aprservice_is_read_only(service))
		return false;

	aprs_packet* packet;

	if (!(packet = aprs_packet_weather_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), type, software)))
	{
		aprservice_log_error(aprs_packet_weather_init, nullptr);

		return false;
	}

	if (!aprs_packet_weather_set_wind_speed(packet, wind_speed))
	{
		aprservice_log_error(aprs_packet_weather_set_wind_speed, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprs_packet_weather_set_wind_speed_gust(packet, wind_speed_gust))
	{
		aprservice_log_error(aprs_packet_weather_set_wind_speed_gust, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprs_packet_weather_set_wind_direction(packet, wind_direction))
	{
		aprservice_log_error(aprs_packet_weather_set_wind_direction, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprs_packet_weather_set_rainfall_last_hour(packet, rainfall_last_hour))
	{
		aprservice_log_error(aprs_packet_weather_set_rainfall_last_hour, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprs_packet_weather_set_rainfall_last_24_hours(packet, rainfall_last_24_hours))
	{
		aprservice_log_error(aprs_packet_weather_set_rainfall_last_24_hours, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprs_packet_weather_set_rainfall_since_midnight(packet, rainfall_since_midnight))
	{
		aprservice_log_error(aprs_packet_weather_set_rainfall_since_midnight, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprs_packet_weather_set_humidity(packet, humidity))
	{
		aprservice_log_error(aprs_packet_weather_set_humidity, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprs_packet_weather_set_temperature(packet, temperature))
	{
		aprservice_log_error(aprs_packet_weather_set_temperature, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprs_packet_weather_set_barometric_pressure(packet, barometric_pressure))
	{
		aprservice_log_error(aprs_packet_weather_set_barometric_pressure, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprservice_send(service, packet))
	{
		aprservice_log_error(aprservice_send, false);

		aprs_packet_deinit(packet);

		return false;
	}

	aprs_packet_deinit(packet);

	return true;
}
bool                       APRSERVICE_CALL aprservice_send_position(struct aprservice* service)
{
	if (!aprservice_is_connected(service))
		return false;

	if (aprservice_is_read_only(service))
		return false;

	if (!aprservice_send(service, service->position))
	{
		aprservice_log_error(aprservice_send, false);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_send_position_ex(struct aprservice* service, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course, const char* comment)
{
	if (!aprservice_is_connected(service))
		return false;

	if (aprservice_is_read_only(service))
		return false;

	aprs_packet* packet;

	if (!(packet = aprs_packet_position_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), latitude, longitude, altitude, speed, course, comment, aprservice_get_symbol_table(service), aprservice_get_symbol_table_key(service), aprservice_get_time_type(service))))
	{
		aprservice_log_error(aprs_packet_position_init, nullptr);

		return false;
	}

	aprs_packet_position_enable_messaging(packet, true);
	aprs_packet_position_enable_compression(packet, aprs_packet_position_is_compressed(service->position));

	if (!aprservice_send(service, packet))
	{
		aprservice_log_error(aprservice_send, false);

		aprs_packet_deinit(packet);

		return false;
	}

	aprs_packet_deinit(packet);

	return true;
}
bool                       APRSERVICE_CALL aprservice_send_telemetry(struct aprservice* service, uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t digital)
{
	if (service->telemetry_count++ == 999)
		service->telemetry_count = 0;

	return aprservice_send_telemetry_ex(service, a1, a2, a3, a4, a5, digital, aprservice_get_comment(service), service->telemetry_count);
}
bool                       APRSERVICE_CALL aprservice_send_telemetry_ex(struct aprservice* service, uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t digital, const char* comment, uint16_t sequence)
{
	if (!aprservice_is_connected(service))
		return false;

	if (aprservice_is_read_only(service))
		return false;

	aprs_packet* packet;

	if (!(packet = aprs_packet_telemetry_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), a1, a2, a3, a4, a5, digital, sequence)))
	{
		aprservice_log_error(aprs_packet_telemetry_init, nullptr);

		return false;
	}

	if (comment && !aprs_packet_telemetry_set_comment(packet, comment))
	{
		aprservice_log_error(aprs_packet_telemetry_set_comment, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprservice_send(service, packet))
	{
		aprservice_log_error(aprservice_send, false);

		aprs_packet_deinit(packet);

		return false;
	}

	aprs_packet_deinit(packet);

	return true;
}
bool                       APRSERVICE_CALL aprservice_send_telemetry_float(struct aprservice* service, float a1, float a2, float a3, float a4, float a5, uint8_t digital)
{
	if (service->telemetry_count++ == 999)
		service->telemetry_count = 0;

	return aprservice_send_telemetry_float_ex(service, a1, a2, a3, a4, a5, digital, aprservice_get_comment(service), service->telemetry_count);
}
bool                       APRSERVICE_CALL aprservice_send_telemetry_float_ex(struct aprservice* service, float a1, float a2, float a3, float a4, float a5, uint8_t digital, const char* comment, uint16_t sequence)
{
	if (!aprservice_is_connected(service))
		return false;

	if (aprservice_is_read_only(service))
		return false;

	aprs_packet* packet;

	if (!(packet = aprs_packet_telemetry_init_float(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), a1, a2, a3, a4, a5, digital, sequence)))
	{
		aprservice_log_error(aprs_packet_telemetry_init_float, nullptr);

		return false;
	}

	if (comment && !aprs_packet_telemetry_set_comment(packet, comment))
	{
		aprservice_log_error(aprs_packet_telemetry_set_comment, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprservice_send(service, packet))
	{
		aprservice_log_error(aprservice_send, false);

		aprs_packet_deinit(packet);

		return false;
	}

	aprs_packet_deinit(packet);

	return true;
}
bool                       APRSERVICE_CALL aprservice_send_user_defined(struct aprservice* service, char id, char type, const char* data)
{
	if (!aprservice_is_connected(service))
		return false;

	if (aprservice_is_read_only(service))
		return false;

	aprs_packet* packet;

	if (!(packet = aprs_packet_user_defined_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), id, type, data)))
	{
		aprservice_log_error(aprs_packet_user_defined_init, nullptr);

		return false;
	}

	if (!aprservice_send(service, packet))
	{
		aprservice_log_error(aprservice_send, false);

		aprs_packet_deinit(packet);

		return false;
	}

	aprs_packet_deinit(packet);

	return true;
}
bool                       APRSERVICE_CALL aprservice_send_third_party(struct aprservice* service, const char* content)
{
	if (!aprservice_is_connected(service))
		return false;

	if (aprservice_is_read_only(service))
		return false;

	aprs_packet* packet;

	if (!(packet = aprs_packet_third_party_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service))))
	{
		aprservice_log_error(aprs_packet_third_party_init, nullptr);

		return false;
	}

	if (!aprs_packet_third_party_set_content(packet, content))
	{
		aprservice_log_error(aprs_packet_third_party_set_content, false);

		aprs_packet_deinit(packet);

		return false;
	}

	if (!aprservice_send(service, packet))
	{
		aprservice_log_error(aprservice_send, false);

		aprs_packet_deinit(packet);

		return false;
	}

	aprs_packet_deinit(packet);

	return true;
}
bool                       APRSERVICE_CALL aprservice_connect_aprs_is(struct aprservice* service, const char* hostname, uint16_t port, uint16_t passcode)
{
	if (aprservice_is_connected(service))
		return false;

	if (!(service->connection = aprservice_connection_init(service, APRSERVICE_CONNECTION_TYPE_APRS_IS, hostname, port, 0, passcode)))
	{
		// aprservice_log_error(aprservice_connection_init, nullptr);

		return false;
	}

	if (!aprservice_connection_open(service->connection))
	{
		// aprservice_log_error(aprservice_connection_open, false);

		aprservice_connection_deinit(service->connection);

		service->connection = nullptr;

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_connect_kiss_tnc_tcp(struct aprservice* service, const char* hostname, uint16_t port)
{
	if (aprservice_is_connected(service))
		return false;

	if (!(service->connection = aprservice_connection_init(service, APRSERVICE_CONNECTION_TYPE_KISS_TNC_TCP, hostname, port, 0, 0)))
	{
		// aprservice_log_error(aprservice_connection_init, nullptr);

		return false;
	}

	if (!aprservice_connection_open(service->connection))
	{
		// aprservice_log_error(aprservice_connection_open, false);

		aprservice_connection_deinit(service->connection);

		service->connection = nullptr;

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_connect_kiss_tnc_serial(struct aprservice* service, const char* device, uint32_t speed)
{
	if (aprservice_is_connected(service))
		return false;

	if (!(service->connection = aprservice_connection_init(service, APRSERVICE_CONNECTION_TYPE_KISS_TNC_SERIAL, device, 0, speed, 0)))
	{
		// aprservice_log_error(aprservice_connection_init, nullptr);

		return false;
	}

	if (!aprservice_connection_open(service->connection))
	{
		// aprservice_log_error(aprservice_connection_open, false);

		aprservice_connection_deinit(service->connection);

		service->connection = nullptr;

		return false;
	}

	return true;
}
void                       APRSERVICE_CALL aprservice_disconnect(struct aprservice* service)
{
	if (aprservice_is_connected(service))
	{
		aprservice_connection_deinit(service->connection);
		service->connection = nullptr;

		service->message_callbacks.remove_if([service](aprservice_message_callback_context* context) {
			context->callback(service, APRSERVICE_MESSAGE_ERROR_DISCONNECTED, context->callback_param);

			return true;
		});

		service->message_callbacks_index.clear();
	}
}

int                        APRSERVICE_CALL aprservice_wait_for_io(struct aprservice* service, uint32_t timeout)
{
	if (!aprservice_is_connected(service))
		return 0;

	switch (aprservice_connection_wait_for_io(service->connection, timeout))
	{
		case 0:
			aprservice_disconnect(service);
			return 0;

		case -1:
			return -1;

		case -2:
			aprservice_disconnect(service);
			return -2;
	}

	return 1;
}

bool                                       aprservice_execute_command(struct aprservice* service, struct aprs_packet* packet, const char* sender, std::string_view name, const char* args)
{
	for (auto& command : service->commands)
		if (!command.name.compare(name))
		{
			if (!command.filter || command.filter(service, &command, packet, sender, command.name.c_str(), args, command.filter_param))
				return command.handler(service, &command, packet, sender, command.name.c_str(), args, command.handler_param), true;

			break;
		}

	return false;
}

struct aprservice_task*    APRSERVICE_CALL aprservice_task_schedule(struct aprservice* service, uint32_t seconds, aprservice_task_handler handler, void* param)
{
	auto task = new aprservice_task
	{
		.service       = service,

		.seconds       = seconds,
		.handler       = handler,
		.handler_param = param
	};

	service->tasks[aprservice_get_time(service) + seconds].push_back(task);

	return task;
}
void                       APRSERVICE_CALL aprservice_task_cancel(struct aprservice_task* task)
{
	if (auto service = aprservice_task_get_service(task))
	{
		for (auto it = service->tasks.begin(); it != service->tasks.end(); ++it)
		{
			for (auto jt = it->second.begin(); jt != it->second.end(); ++jt)
			{
				if (*jt == task)
				{
					it->second.erase(jt);

					delete task;

					if (it->second.empty())
						service->tasks.erase(it);

					break;
				}
			}
		}
	}
}
void                       APRSERVICE_CALL aprservice_task_get_handler(struct aprservice_task* task, aprservice_task_handler* handler, void** param)
{
	*param   = task->handler_param;
	*handler = task->handler;
}
struct aprservice*         APRSERVICE_CALL aprservice_task_get_service(struct aprservice_task* task)
{
	return task->service;
}

struct aprservice_item*    APRSERVICE_CALL aprservice_item_create(struct aprservice* service, const char* name, const char* comment, char symbol_table, char symbol_table_key, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course)
{
	aprservice_item item =
	{
		.service = service
	};

	if (!(item.packet = aprs_packet_item_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), name, symbol_table, symbol_table_key)))
	{
		aprservice_log_error(aprs_packet_item_init, nullptr);

		return nullptr;
	}

	if (!aprs_packet_item_set_alive(item.packet, true))
	{
		aprservice_log_error(aprs_packet_item_set_alive, false);

		aprs_packet_deinit(item.packet);

		return nullptr;
	}

	if (!aprs_packet_item_set_speed(item.packet, speed))
	{
		aprservice_log_error(aprs_packet_item_set_speed, false);

		aprs_packet_deinit(item.packet);

		return nullptr;
	}

	if (!aprs_packet_item_set_course(item.packet, course))
	{
		aprservice_log_error(aprs_packet_item_set_course, false);

		aprs_packet_deinit(item.packet);

		return nullptr;
	}

	if (!aprs_packet_item_set_comment(item.packet, comment))
	{
		aprservice_log_error(aprs_packet_item_set_comment, false);

		aprs_packet_deinit(item.packet);

		return nullptr;
	}

	if (!aprs_packet_item_set_altitude(item.packet, altitude))
	{
		aprservice_log_error(aprs_packet_item_set_altitude, false);

		aprs_packet_deinit(item.packet);

		return nullptr;
	}

	if (!aprs_packet_item_set_latitude(item.packet, latitude))
	{
		aprservice_log_error(aprs_packet_item_set_latitude, false);

		aprs_packet_deinit(item.packet);

		return nullptr;
	}

	if (!aprs_packet_item_set_longitude(item.packet, longitude))
	{
		aprservice_log_error(aprs_packet_item_set_longitude, false);

		aprs_packet_deinit(item.packet);

		return nullptr;
	}

	if (!aprs_packet_item_set_compressed(item.packet, aprservice_is_compression_enabled(service)))
	{
		aprservice_log_error(aprs_packet_item_set_compressed, false);

		aprs_packet_deinit(item.packet);

		return nullptr;
	};

	return &service->items.emplace_back(std::move(item));
}
void                       APRSERVICE_CALL aprservice_item_destroy(struct aprservice_item* item)
{
	if (auto service = aprservice_item_get_service(item))
		for (auto it = service->items.begin(); it != service->items.end(); ++it)
			if (&*it == item)
			{
				aprs_packet_deinit(item->packet);

				service->items.erase(it);

				break;
			}
}
bool                       APRSERVICE_CALL aprservice_item_is_alive(struct aprservice_item* item)
{
	return aprs_packet_item_is_alive(item->packet);
}
bool                       APRSERVICE_CALL aprservice_item_is_compressed(struct aprservice_item* item)
{
	return aprs_packet_item_is_compressed(item->packet);
}
struct aprservice*         APRSERVICE_CALL aprservice_item_get_service(struct aprservice_item* item)
{
	return item->service;
}
const char*                APRSERVICE_CALL aprservice_item_get_name(struct aprservice_item* item)
{
	return aprs_packet_item_get_name(item->packet);
}
const char*                APRSERVICE_CALL aprservice_item_get_comment(struct aprservice_item* item)
{
	return aprs_packet_item_get_comment(item->packet);
}
uint16_t                   APRSERVICE_CALL aprservice_item_get_speed(struct aprservice_item* item)
{
	return aprs_packet_item_get_speed(item->packet);
}
uint16_t                   APRSERVICE_CALL aprservice_item_get_course(struct aprservice_item* item)
{
	return aprs_packet_item_get_course(item->packet);
}
int32_t                    APRSERVICE_CALL aprservice_item_get_altitude(struct aprservice_item* item)
{
	return aprs_packet_item_get_altitude(item->packet);
}
float                      APRSERVICE_CALL aprservice_item_get_latitude(struct aprservice_item* item)
{
	return aprs_packet_item_get_latitude(item->packet);
}
float                      APRSERVICE_CALL aprservice_item_get_longitude(struct aprservice_item* item)
{
	return aprs_packet_item_get_longitude(item->packet);
}
char                       APRSERVICE_CALL aprservice_item_get_symbol_table(struct aprservice_item* item)
{
	return aprs_packet_item_get_symbol_table(item->packet);
}
char                       APRSERVICE_CALL aprservice_item_get_symbol_table_key(struct aprservice_item* item)
{
	return aprs_packet_item_get_symbol_table_key(item->packet);
}
bool                       APRSERVICE_CALL aprservice_item_set_symbol(struct aprservice_item* item, char table, char key)
{
	if (!aprs_packet_item_set_symbol(item->packet, table, key))
	{
		aprservice_log_error(aprs_packet_item_set_symbol, false);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_item_set_comment(struct aprservice_item* item, const char* value)
{
	if (!aprs_packet_item_set_comment(item->packet, value))
	{
		aprservice_log_error(aprs_packet_item_set_comment, false);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_item_set_position(struct aprservice_item* item, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course)
{
	if (!aprs_packet_item_set_speed(item->packet, speed))
	{
		aprservice_log_error(aprs_packet_item_set_speed, false);

		return false;
	}

	if (!aprs_packet_item_set_course(item->packet, course))
	{
		aprservice_log_error(aprs_packet_item_set_course, false);

		return false;
	}

	if (!aprs_packet_item_set_altitude(item->packet, altitude))
	{
		aprservice_log_error(aprs_packet_item_set_altitude, false);

		return false;
	}

	if (!aprs_packet_item_set_latitude(item->packet, latitude))
	{
		aprservice_log_error(aprs_packet_item_set_latitude, false);

		return false;
	}

	if (!aprs_packet_item_set_longitude(item->packet, longitude))
	{
		aprservice_log_error(aprs_packet_item_set_longitude, false);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_item_set_compressed(struct aprservice_item* item, bool value)
{
	if (!aprs_packet_item_set_compressed(item->packet, value))
	{
		aprservice_log_error(aprs_packet_item_set_compressed, false);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_item_kill(struct aprservice_item* item)
{
	if (!aprs_packet_item_is_alive(item->packet))
		return true;

	if (!aprs_packet_item_set_alive(item->packet, false))
	{
		aprservice_log_error(aprs_packet_item_set_alive, false);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_item_announce(struct aprservice_item* item)
{
	if (!aprservice_send(item->service, item->packet))
	{
		aprservice_log_error(aprservice_send, false);

		return false;
	}

	return true;
}

struct aprservice_object*  APRSERVICE_CALL aprservice_object_create(struct aprservice* service, const char* name, const char* comment, char symbol_table, char symbol_table_key, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course)
{
	aprservice_object object =
	{
		.service = service
	};

	if (!(object.packet = aprs_packet_object_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), name, symbol_table, symbol_table_key, aprservice_get_time_type(service))))
	{
		aprservice_log_error(aprs_packet_object_init, nullptr);

		return nullptr;
	}

	if (!aprs_packet_object_set_alive(object.packet, true))
	{
		aprservice_log_error(aprs_packet_object_set_alive, false);

		aprs_packet_deinit(object.packet);

		return nullptr;
	}

	if (!aprs_packet_object_set_speed(object.packet, speed))
	{
		aprservice_log_error(aprs_packet_object_set_speed, false);

		aprs_packet_deinit(object.packet);

		return nullptr;
	}

	if (!aprs_packet_object_set_course(object.packet, course))
	{
		aprservice_log_error(aprs_packet_object_set_course, false);

		aprs_packet_deinit(object.packet);

		return nullptr;
	}

	if (!aprs_packet_object_set_comment(object.packet, comment))
	{
		aprservice_log_error(aprs_packet_object_set_comment, false);

		aprs_packet_deinit(object.packet);

		return nullptr;
	}

	if (!aprs_packet_object_set_altitude(object.packet, altitude))
	{
		aprservice_log_error(aprs_packet_object_set_altitude, false);

		aprs_packet_deinit(object.packet);

		return nullptr;
	}

	if (!aprs_packet_object_set_latitude(object.packet, latitude))
	{
		aprservice_log_error(aprs_packet_object_set_latitude, false);

		aprs_packet_deinit(object.packet);

		return nullptr;
	}

	if (!aprs_packet_object_set_longitude(object.packet, longitude))
	{
		aprservice_log_error(aprs_packet_object_set_longitude, false);

		aprs_packet_deinit(object.packet);

		return nullptr;
	}

	if (!aprs_packet_object_set_compressed(object.packet, aprservice_is_compression_enabled(service)))
	{
		aprservice_log_error(aprs_packet_object_set_compressed, false);

		aprs_packet_deinit(object.packet);

		return nullptr;
	};

	return &service->objects.emplace_back(std::move(object));
}
void                       APRSERVICE_CALL aprservice_object_destroy(struct aprservice_object* object)
{
	if (auto service = aprservice_object_get_service(object))
		for (auto it = service->objects.begin(); it != service->objects.end(); ++it)
			if (&*it == object)
			{
				aprs_packet_deinit(object->packet);

				service->objects.erase(it);

				break;
			}
}
bool                       APRSERVICE_CALL aprservice_object_is_alive(struct aprservice_object* object)
{
	return aprs_packet_object_is_alive(object->packet);
}
bool                       APRSERVICE_CALL aprservice_object_is_compressed(struct aprservice_object* object)
{
	return aprs_packet_object_is_compressed(object->packet);
}
struct aprservice*         APRSERVICE_CALL aprservice_object_get_service(struct aprservice_object* object)
{
	return object->service;
}
const char*                APRSERVICE_CALL aprservice_object_get_name(struct aprservice_object* object)
{
	return aprs_packet_object_get_name(object->packet);
}
const char*                APRSERVICE_CALL aprservice_object_get_comment(struct aprservice_object* object)
{
	return aprs_packet_object_get_comment(object->packet);
}
uint16_t                   APRSERVICE_CALL aprservice_object_get_speed(struct aprservice_object* object)
{
	return aprs_packet_object_get_speed(object->packet);
}
uint16_t                   APRSERVICE_CALL aprservice_object_get_course(struct aprservice_object* object)
{
	return aprs_packet_object_get_course(object->packet);
}
int32_t                    APRSERVICE_CALL aprservice_object_get_altitude(struct aprservice_object* object)
{
	return aprs_packet_object_get_altitude(object->packet);
}
float                      APRSERVICE_CALL aprservice_object_get_latitude(struct aprservice_object* object)
{
	return aprs_packet_object_get_latitude(object->packet);
}
float                      APRSERVICE_CALL aprservice_object_get_longitude(struct aprservice_object* object)
{
	return aprs_packet_object_get_longitude(object->packet);
}
char                       APRSERVICE_CALL aprservice_object_get_symbol_table(struct aprservice_object* object)
{
	return aprs_packet_object_get_symbol_table(object->packet);
}
char                       APRSERVICE_CALL aprservice_object_get_symbol_table_key(struct aprservice_object* object)
{
	return aprs_packet_object_get_symbol_table_key(object->packet);
}
bool                       APRSERVICE_CALL aprservice_object_set_symbol(struct aprservice_object* object, char table, char key)
{
	if (!aprs_packet_object_set_symbol(object->packet, table, key))
	{
		aprservice_log_error(aprs_packet_object_set_symbol, false);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_object_set_comment(struct aprservice_object* object, const char* value)
{
	if (!aprs_packet_object_set_comment(object->packet, value))
	{
		aprservice_log_error(aprs_packet_object_set_comment, false);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_object_set_position(struct aprservice_object* object, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course)
{
	if (!aprs_packet_object_set_speed(object->packet, speed))
	{
		aprservice_log_error(aprs_packet_object_set_speed, false);

		return false;
	}

	if (!aprs_packet_object_set_course(object->packet, course))
	{
		aprservice_log_error(aprs_packet_object_set_course, false);

		return false;
	}

	if (!aprs_packet_object_set_altitude(object->packet, altitude))
	{
		aprservice_log_error(aprs_packet_object_set_altitude, false);

		return false;
	}

	if (!aprs_packet_object_set_latitude(object->packet, latitude))
	{
		aprservice_log_error(aprs_packet_object_set_latitude, false);

		return false;
	}

	if (!aprs_packet_object_set_longitude(object->packet, longitude))
	{
		aprservice_log_error(aprs_packet_object_set_longitude, false);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_object_set_compressed(struct aprservice_object* object, bool value)
{
	if (!aprs_packet_object_set_compressed(object->packet, value))
	{
		aprservice_log_error(aprs_packet_object_set_compressed, false);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_object_kill(struct aprservice_object* object)
{
	if (!aprs_packet_object_is_alive(object->packet))
		return true;

	if (!aprs_packet_object_set_alive(object->packet, false))
	{
		aprservice_log_error(aprs_packet_object_set_alive, false);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_object_announce(struct aprservice_object* object)
{
	aprs_time time;

	if (!aprs_time_now(&time, aprservice_get_time_type(object->service)))
	{
		aprservice_log_error(aprs_time_now, false);

		return false;
	}

	if (!aprs_packet_object_set_time(object->packet, &time))
	{
		aprservice_log_error(aprs_packet_object_set_time, false);

		return false;
	}

	if (!aprservice_send(object->service, object->packet))
	{
		aprservice_log_error(aprservice_send, false);

		return false;
	}

	return true;
}

struct aprservice_command* APRSERVICE_CALL aprservice_command_register(struct aprservice* service, const char* name, const char* help, aprservice_command_handler handler, void* param)
{
	if (!name || !handler)
		return nullptr;

	for (auto& command : service->commands)
		if (!command.name.compare(name))
		{
			command.help          = help ? help : "";
			command.filter        = nullptr;
			command.handler       = handler;
			command.handler_param = param;

			return &command;
		}

	return &service->commands.emplace_back(aprservice_command {
		.service       = service,

		.name          = name,
		.help          = help ? help : "",

		.handler       = handler,
		.handler_param = param
	});
}
void                       APRSERVICE_CALL aprservice_command_unregister(struct aprservice_command* command)
{
	if (auto service = aprservice_command_get_service(command))
		for (auto it = service->commands.begin(); it != service->commands.end(); ++it)
			if (&*it == command)
			{
				service->commands.erase(it);

				break;
			}
}
const char*                APRSERVICE_CALL aprservice_command_get_help(struct aprservice_command* command)
{
	return command->help.c_str();
}
void                       APRSERVICE_CALL aprservice_command_get_filter(struct aprservice_command* command, aprservice_command_filter_handler* handler, void** param)
{
	*param   = command->filter_param;
	*handler = command->filter;
}
void                       APRSERVICE_CALL aprservice_command_get_handler(struct aprservice_command* command, aprservice_command_handler* handler, void** param)
{
	*param   = command->handler_param;
	*handler = command->handler;
}
struct aprservice*         APRSERVICE_CALL aprservice_command_get_service(struct aprservice_command* command)
{
	return command->service;
}
void                       APRSERVICE_CALL aprservice_command_set_help(struct aprservice_command* command, const char* value)
{
	if (!value)
		command->help.clear();
	else
		command->help = value;
}
void                       APRSERVICE_CALL aprservice_command_set_filter(struct aprservice_command* command, aprservice_command_filter_handler handler, void* param)
{
	command->filter       = handler;
	command->filter_param = param;
}
