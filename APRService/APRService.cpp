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
#include <iostream>
#include <unordered_map>

#if defined(APRSERVICE_UNIX)
	#include <fcntl.h>
	#include <netdb.h>
	#include <unistd.h>

	#include <arpa/inet.h>

	#include <sys/ioctl.h>
	#include <sys/types.h>
	#include <sys/socket.h>

	#include <netinet/tcp.h>
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

struct aprservice_event
{
	aprservice_event_handler handler;
	void*                    handler_param;
};

struct aprservice_position
{
	APRSERVICE_POSITION_TYPES type;
	aprs_packet*              packet;
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
	int                                      mode;
	int                                      type;

#if defined(APRSERVICE_UNIX)
	int                                      serial;
	int                                      socket;
#elif defined(APRSERVICE_WIN32)
	HANDLE                                   serial;
	SOCKET                                   socket;
	WSADATA                                  winsock;
#endif

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

struct aprservice
{
	bool                                                                            is_connected;
	bool                                                                            is_monitoring;

	std::string                                                                     line;
	int64_t                                                                         time;
	std::vector<aprservice_connection*>                                             connections;

	aprs_path*                                                                      path;
	const std::string                                                               station;
	aprservice_position                                                             position;

	std::list<aprservice_item*>                                                     items;
	std::map<uint64_t, std::list<aprservice_task*>>                                 tasks;
	aprservice_event                                                                events[APRSERVICE_EVENTS_COUNT + 1];
	std::list<aprservice_object*>                                                   objects;
	std::unordered_map<std::string, aprservice_command*>                            commands;

	uint16_t                                                                        message_count;
	std::list<aprservice_message_callback_context*>                                 message_callbacks;
	std::unordered_map<std::string, std::list<aprservice_message_callback_context>> message_callbacks_index;

	uint16_t                                                                        telemetry_count;
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
	aprservice*                service;

	std::string                name;
	std::string                help;
	aprservice_command_handler handler;
	void*                      handler_param;
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

#define                                    aprservice_log(...)                      std::cerr << __VA_ARGS__ << std::endl
#if defined(APRSERVICE_UNIX)
	#define                                aprservice_log_error(function)           aprservice_log_error_ex(function, errno)
#elif defined(APRSERVICE_WIN32)
	#define                                aprservice_log_error(function)           aprservice_log_error_ex(function, GetLastError())
#endif
#define                                    aprservice_log_error_ex(function, error) std::cerr << #function " returned " << error << std::endl

template<typename T>
T                                          aprservice_parse_uint(const char* string)
{
	T value = 0;

	for (size_t i = 0; *string; ++i, ++string)
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
		aprservice_log_error_ex(std::regex_match, exception.what());

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

aprservice_connection*                     aprservice_connection_init(aprservice* service, int mode, int type, const char* host_or_device, uint16_t port, uint32_t speed, uint16_t passcode);
void                                       aprservice_connection_deinit(aprservice_connection* connection);
bool                                       aprservice_connection_is_open(aprservice_connection* connection);
bool                                       aprservice_connection_open(aprservice_connection* connection);
void                                       aprservice_connection_close(aprservice_connection* connection);
bool                                       aprservice_connection_poll(aprservice_connection* connection);
// @return 0 on disconnect
// @return -1 on would block
int                                        aprservice_connection_read(aprservice_connection* connection, void* buffer, size_t size, size_t* number_of_bytes_received);
// @return 0 on disconnect
// @return -1 on would block
int                                        aprservice_connection_write(aprservice_connection* connection, const void* buffer, size_t size, size_t* number_of_bytes_sent);
bool                                       aprservice_connection_read_string(aprservice_connection* connection, std::string& value);
bool                                       aprservice_connection_write_packet(aprservice_connection* connection, aprs_packet* value);
bool                                       aprservice_connection_write_aprs_is(aprservice_connection* connection, std::string&& value);

bool                                       aprservice_enum_connections(aprservice* service, bool(*callback)(aprservice* service, aprservice_connection* connection, void* param), void* param)
{
	for (auto connection : service->connections)
		if (!callback(service, connection, param))
			return false;

	return true;
}
bool                                       aprservice_enum_connections_by_mode(aprservice* service, int mode, bool(*callback)(aprservice* service, aprservice_connection* connection, void* param), void* param)
{
	for (auto connection : service->connections)
		if (!mode || (connection->mode & mode))
			if (!callback(service, connection, param))
				return false;

	return true;
}

bool                                       aprservice_poll_tasks(struct aprservice* service);
bool                                       aprservice_poll_messages(struct aprservice* service);
bool                                       aprservice_poll_connections(struct aprservice* service);

bool                                       aprservice_send_message_ack(struct aprservice* service, const char* destination, const char* id);
bool                                       aprservice_send_message_reject(struct aprservice* service, const char* destination, const char* id);

aprservice_connection*                     aprservice_connection_init(aprservice* service, int mode, int type, const char* host_or_device, uint16_t port, uint32_t speed, uint16_t passcode)
{
	auto connection = new aprservice_connection
	{
		.service        = service,

		.is_open        = false,

		.mode           = mode,
		.type           = type,

		.device_speed   = speed,
		.host_or_device = host_or_device,
		.port           = port,
		.passcode       = passcode
	};

#if defined(APRSERVICE_WIN32)
	if (auto error = WSAStartup(MAKEWORD(2, 2), &connection->winsock))
	{
		aprservice_log_error_ex(WSAStartup, error);

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
				return false;
		}

		aprservice_log_error_ex(getaddrinfo, error);

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
		aprservice_log_error(socket);

		return false;
	}
#elif defined(APRSERVICE_WIN32)
	if ((connection->socket = WSASocketW(dns_result_address->sa_family, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, 0)) == INVALID_SOCKET)
	{
		aprservice_log_error(WSASocketW);

		return false;
	}
#endif

socket_connect:
#if defined(APRSERVICE_UNIX)
	if (connect(connection->socket, socket_address, socket_address_length) == -1)
	{
		aprservice_log_error(connect);

		close(connection->socket);

		return false;
	}

	int flags;

	if ((flags = fcntl(connection->socket, F_GETFL, 0)) == -1)
	{
		aprservice_log_error(fcntl);

		close(connection->socket);

		return false;
	}

	if (fcntl(connection->socket, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		aprservice_log_error(fcntl);

		close(connection->socket);

		return false;
	}
#elif defined(APRSERVICE_WIN32)
	if (connect(connection->socket, dns_result_address, dns_result_address_length) == SOCKET_ERROR)
	{
		aprservice_log_error(connect);

		closesocket(connection->socket);

		return false;
	}

	u_long arg = 1;

	if (ioctlsocket(connection->socket, FIONBIO, &arg) == SOCKET_ERROR)
	{
		aprservice_log_error(ioctlsocket);

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
	if ((connection->serial = open(connection->host_or_device.c_str(), O_RDWR | O_NOCTTY | O_NDELAY)) == -1)
	{
		auto error = errno;

		aprservice_log_error_ex(open, error);

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
	ioctl(connection->serial, TIOCMGET, &status);

	status |= TIOCM_DTR | TIOCM_RTS;
	ioctl(connection->serial, TIOCMSET, &status);
#elif defined(APRSERVICE_WIN32)
	if ((connection->serial = CreateFileA(connection->host_or_device.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0)) == INVALID_HANDLE_VALUE)
	{
		auto error = GetLastError();

		aprservice_log_error_ex(CreateFileA, error);

		return false;
	}

	DCB dcb       = {};
	dcb.DCBlength = sizeof(DCB);

	if (!GetCommState(connection->serial, &dcb))
	{
		auto error = GetLastError();

		aprservice_log_error_ex(GetCommState, error);

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

		aprservice_log_error_ex(SetCommState, error);

		CloseHandle(connection->serial);

		return false;
	}

	COMMTIMEOUTS timeouts                = {};
	timeouts.ReadIntervalTimeout         = MAXWORD;
	timeouts.ReadTotalTimeoutConstant    = 0;
	timeouts.ReadTotalTimeoutMultiplier  = 0;
	timeouts.WriteTotalTimeoutConstant   = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;

	if (!SetCommTimeouts(connection->serial, &timeouts))
	{
		auto error = GetLastError();

		aprservice_log_error_ex(SetCommTimeouts, error);

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
				aprservice_log_error_ex(aprservice_connection_open_tcp, false);

				return false;
			}
			break;

		case APRSERVICE_CONNECTION_TYPE_KISS_TNC_SERIAL:
			if (!aprservice_connection_open_serial(connection))
			{
				aprservice_log_error_ex(aprservice_connection_open_serial, false);

				return false;
			}
			break;
	}

	aprservice_event_execute(connection->service, APRSERVICE_EVENT_CONNECT, { .connection_mode = connection->mode, .connection_type = connection->type });

	auto generate_auth_request = [](aprservice* service, uint16_t passcode)
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
				aprservice_log_error_ex(aprservice_connection_write_aprs_is, false);

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
			aprservice_event_execute(connection->service, APRSERVICE_EVENT_AUTHENTICATE, { .message = connection->auth.message.c_str(), .success = connection->auth.success, .verified = connection->auth.verified, .connection_mode = connection->mode, .connection_type = connection->type });
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

		aprservice_event_execute(connection->service, APRSERVICE_EVENT_DISCONNECT, { .connection_mode = connection->mode, .connection_type = connection->type });
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

										auto command_decode_path = [](std::string& string, const std::vector<uint8_t>& buffer, size_t offset)
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
										auto command_decode_tocall = [](std::string& string, const std::vector<uint8_t>& buffer, size_t offset)
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
										auto command_decode_station = [](std::string& string, const std::vector<uint8_t>& buffer, size_t offset)
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
											string.append((const char*)&command_buffer[offset], command_buffer.size() - offset);

											// TODO: is this specific to pinpoint or is there an error in this implementation?
											if (string.ends_with('\r'))
												string.pop_back();

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
// @return false on connection closed
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
			return aprservice_connection_poll_aprs_is(connection);

		case APRSERVICE_CONNECTION_TYPE_KISS_TNC_TCP:
		case APRSERVICE_CONNECTION_TYPE_KISS_TNC_SERIAL:
			return aprservice_connection_poll_kiss_tnc(connection);
	}

	return true;
}
// @return 0 on disconnect
// @return -1 on would block
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

			if ((bytes_received = recv(connection->socket, buffer, size, 0)) == -1)
			{
				auto error = errno;

				if ((error == EAGAIN) || (error == EWOULDBLOCK))
					return -1;

				aprservice_connection_close(connection);

				if ((error == EHOSTDOWN) || (error == ECONNRESET) || (error == EHOSTUNREACH))
					return 0;

				aprservice_log_error_ex(recv, error);

				return 0;
			}

			*number_of_bytes_received = bytes_received;
#elif defined(APRSERVICE_WIN32)
			int bytes_received;

			if ((bytes_received = recv(connection->socket, reinterpret_cast<char*>(buffer), static_cast<int>(size), 0)) == SOCKET_ERROR)
			{
				auto error = WSAGetLastError();

				switch (error)
				{
					case WSAEWOULDBLOCK:
						return -1;

					case WSAENETDOWN:
					case WSAENETRESET:
					case WSAETIMEDOUT:
					case WSAECONNRESET:
					case WSAECONNABORTED:
						aprservice_connection_close(connection);
						return 0;
				}

				return 0;
			}

			*number_of_bytes_received = bytes_received;
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

				aprservice_log_error_ex(read, error);

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

				aprservice_log_error_ex(ReadFile, error);

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

	return 1;
}
// @return 0 on disconnect
// @return -1 on would block
int                                        aprservice_connection_write(aprservice_connection* connection, const void* buffer, size_t size, size_t* number_of_bytes_sent)
{
	if (!aprservice_connection_is_open(connection))
		return false;

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

				if ((error == EAGAIN) || (error == EWOULDBLOCK))
					return -1;

				aprservice_connection_close(sn);

				if ((error == EHOSTDOWN) || (error == ECONNRESET) || (error == EHOSTUNREACH))
					return 0;

				aprservice_log_error_ex(send, error);

				return 0;
			}

			*number_of_bytes_sent = bytes_sent;
#elif defined(APRSERVICE_WIN32)
			int bytes_sent;

			if ((bytes_sent = send(connection->socket, reinterpret_cast<const char*>(buffer), static_cast<int>(size), 0)) == SOCKET_ERROR)
			{
				auto error = WSAGetLastError();

				switch (error)
				{
					case WSAEWOULDBLOCK:
						return -1;

					case WSAENETDOWN:
					case WSAENETRESET:
					case WSAETIMEDOUT:
					case WSAECONNRESET:
					case WSAECONNABORTED:
					case WSAEHOSTUNREACH:
						aprservice_connection_close(connection);
						return 0;
				}

				aprservice_log_error_ex(send, error);

				return 0;
			}

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

				aprservice_log_error_ex(write, error);

				aprservice_connection_close(connection);

				return 0;
			}

			if (bytes_sent == 0)
				return -1;

			*number_of_bytes_sent = bytes_sent;
#elif defined(APRSERVICE_WIN32)
			DWORD bytes_sent;

			if (!WriteFile(connection->serial, buffer, size, &bytes_sent, nullptr))
			{
				auto error = GetLastError();

				aprservice_log_error_ex(WriteFile, error);

				aprservice_connection_close(connection);

				return 0;
			}

			if (bytes_sent == 0)
				return -1;

			*number_of_bytes_sent = bytes_sent;
#endif
		}
		break;
	}

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
				// string isn't used but aprs_packet_to_string populates packet content
				if (auto string = aprs_packet_to_string(value))
				{
					auto packet_to_ax25   = [](aprs_packet* value)
					{
						auto        path         = aprs_packet_get_path(value);
						auto        path_size    = aprs_path_get_length(path);
						std::string content      = aprs_packet_get_content(value);
						auto        content_size = content.length();

						std::vector<uint8_t> buffer(14 + (path_size * 7) + 2 + content_size, 0);
						auto                 buffer_encode_path    = [](std::vector<uint8_t>& buffer, size_t offset, aprs_path* value)
						{
							auto size = aprs_path_get_length(value);

							for (size_t i = 0; i < size; )
							{
								auto path = aprs_path_get(value, i);

								for (size_t j = 0; j < 6; ++j, ++path)
									switch (*path)
									{
										case '-':
										case '\0':
											while (j++ < 6)
												buffer[offset++] = 0x40;
											break;

										case '*':
											buffer[offset - j] |= 0x80;
											while (j++ < 6)
												buffer[offset++] = 0x40;
											break;

										default:
											buffer[offset++] = *path << 1;
											break;
									}

								buffer[offset++] = (aprservice_parse_uint<uint8_t>(path) & 0x0F) << 1;

								if (++i == size)
									buffer[offset - 1] |= 0x01;
							}

							return offset;
						};
						auto                 buffer_encode_tocall  = [](std::vector<uint8_t>& buffer, size_t offset, const char* value)
						{
							for (size_t i = 0; i < 6; ++i, ++value)
								switch (*value)
								{
									case '-':
									case '\0':
										while (i++ < 6)
											buffer[offset++] = 0x40;
										break;

									default:
										buffer[offset++] = *value << 1;
										break;
								}

							buffer[offset] = (aprservice_parse_uint<uint8_t>(value) & 0x0F) << 1;
						};
						auto                 buffer_encode_station = [](std::vector<uint8_t>& buffer, size_t offset, const char* value)
						{
							for (size_t i = 0; i < 6; ++i, ++value)
								switch (*value)
								{
									case '-':
									case '\0':
										while (i++ < 6)
											buffer[offset++] = 0x40;
										break;

									default:
										buffer[offset++] = *value << 1;
										break;
								}

							buffer[offset] = (aprservice_parse_uint<uint8_t>(value) & 0x0F) << 1;
						};

						buffer_encode_tocall(buffer, 0, aprs_packet_get_tocall(value));
						buffer_encode_station(buffer, 7, aprs_packet_get_sender(value));
						auto offset      = buffer_encode_path(buffer, 14, path);
						buffer[offset++] = 0x03;
						buffer[offset++] = 0xF0;
						memcpy(&buffer[offset], content.c_str(), content_size);

						return buffer;
					};
					auto ax25_to_kiss_tnc = [](const uint8_t* value, size_t count)
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

struct aprservice*         APRSERVICE_CALL aprservice_init(const char* station, struct aprs_path* path, char symbol_table, char symbol_table_key)
{
	if (!station || !path)
		return nullptr;

	auto service = new aprservice
	{
		.is_connected     = false,
		.is_monitoring    = false,

		.time             = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count(),

		.path             = path,
		.station          = station,
		.position         = { .type = APRSERVICE_POSITION_TYPE_POSITION },

		.events           = {},

		.message_count    = 0,

		.telemetry_count  = 0
	};

	if (!(service->position.packet = aprs_packet_position_init(station, APRSERVICE_TOCALL, path, 0, 0, 0, 0, 0, "", symbol_table, symbol_table_key)))
	{
		aprservice_log_error_ex(aprs_packet_position_init, nullptr);

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

	service->items.remove_if([](aprservice_item* item) {
		aprservice_item_destroy(item);

		return true;
	});

	service->objects.remove_if([](aprservice_object* object) {
		aprservice_object_destroy(object);

		return true;
	});

	for (auto it = service->commands.begin(); it != service->commands.end(); )
		aprservice_command_unregister((it++)->second);

	aprs_packet_deinit(service->position.packet);

	aprs_path_deinit(service->path);

	delete service;
}
bool                       APRSERVICE_CALL aprservice_is_read_only(struct aprservice* service)
{
	return aprservice_enum_connections_by_mode(service, APRSERVICE_CONNECTION_MODE_OUTPUT, [](aprservice* service, aprservice_connection* connection, void* param) {
		if (connection->auth.state == APRSERVICE_AUTH_STATE_RECEIVED)
			if (connection->auth.success && connection->auth.verified)
				return false;

		return true;
	}, nullptr);
}
bool                       APRSERVICE_CALL aprservice_is_connected(struct aprservice* service)
{
	return service->is_connected;
}
bool                       APRSERVICE_CALL aprservice_is_authenticated(struct aprservice* service)
{
	return aprservice_enum_connections(service, [](aprservice* service, aprservice_connection* connection, void* param) {
		if (connection->auth.state != APRSERVICE_AUTH_STATE_RECEIVED)
			return false;

		if (!connection->auth.success || !connection->auth.verified)
			return false;

		return true;
	}, nullptr);
}
bool                       APRSERVICE_CALL aprservice_is_authenticating(struct aprservice* service)
{
	return !aprservice_enum_connections(service, [](aprservice* service, aprservice_connection* connection, void* param) {
		if (connection->auth.state == APRSERVICE_AUTH_STATE_SENT)
			return false;

		return true;
	}, nullptr);
}
bool                       APRSERVICE_CALL aprservice_is_monitoring_enabled(struct aprservice* service)
{
	return service->is_monitoring;
}
bool                       APRSERVICE_CALL aprservice_is_compression_enabled(struct aprservice* service)
{
	switch (service->position.type)
	{
		case APRSERVICE_POSITION_TYPE_MIC_E:
		case APRSERVICE_POSITION_TYPE_POSITION_COMPRESSED:
			return true;
	}

	return false;
}
struct aprs_path*          APRSERVICE_CALL aprservice_get_path(struct aprservice* service)
{
	return service->path;
}
uint32_t                   APRSERVICE_CALL aprservice_get_time(struct aprservice* service)
{
	return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count() - service->time;
}
const char*                APRSERVICE_CALL aprservice_get_comment(struct aprservice* service)
{
	return aprs_packet_position_get_comment(service->position.packet);
}
const char*                APRSERVICE_CALL aprservice_get_station(struct aprservice* service)
{
	return service->station.c_str();
}
char                       APRSERVICE_CALL aprservice_get_symbol_table(struct aprservice* service)
{
	return aprs_packet_position_get_symbol_table(service->position.packet);
}
char                       APRSERVICE_CALL aprservice_get_symbol_table_key(struct aprservice* service)
{
	return aprs_packet_position_get_symbol_table_key(service->position.packet);
}
int                        APRSERVICE_CALL aprservice_get_position_type(struct aprservice* service)
{
	return service->position.type;
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
bool                       APRSERVICE_CALL aprservice_set_symbol(struct aprservice* service, char table, char key)
{
	if (!aprs_packet_position_set_symbol(service->position.packet, table, key))
	{
		aprservice_log_error_ex(aprs_packet_position_set_symbol, false);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_set_comment(struct aprservice* service, const char* value)
{
	if (!aprs_packet_position_set_comment(service->position.packet, value))
	{
		aprservice_log_error_ex(aprs_packet_position_set_comment, false);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_set_position(struct aprservice* service, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course)
{
	if (!aprs_packet_position_set_speed(service->position.packet, speed))
	{
		aprservice_log_error_ex(aprs_packet_position_set_speed, false);

		return false;
	}

	if (!aprs_packet_position_set_course(service->position.packet, course))
	{
		aprservice_log_error_ex(aprs_packet_position_set_course, false);

		return false;
	}

	if (!aprs_packet_position_set_altitude(service->position.packet, altitude))
	{
		aprservice_log_error_ex(aprs_packet_position_set_altitude, false);

		return false;
	}

	if (!aprs_packet_position_set_latitude(service->position.packet, latitude))
	{
		aprservice_log_error_ex(aprs_packet_position_set_latitude, false);

		return false;
	}

	if (!aprs_packet_position_set_longitude(service->position.packet, longitude))
	{
		aprservice_log_error_ex(aprs_packet_position_set_longitude, false);

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
			if (auto packet = aprs_packet_position_init_mic_e(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), aprs_packet_position_get_latitude(service->position.packet), aprs_packet_position_get_longitude(service->position.packet), aprs_packet_position_get_altitude(service->position.packet), aprs_packet_position_get_speed(service->position.packet), aprs_packet_position_get_course(service->position.packet), aprs_packet_position_get_comment(service->position.packet), aprs_packet_position_get_symbol_table(service->position.packet), aprs_packet_position_get_symbol_table_key(service->position.packet)))
			{
				aprs_packet_deinit(service->position.packet);

				service->position.type   = value;
				service->position.packet = packet;

				return true;
			}
			aprservice_log_error_ex(aprs_packet_position_init_mic_e, nullptr);
			return false;

		case APRSERVICE_POSITION_TYPE_POSITION:
			if (auto packet = aprs_packet_position_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), aprs_packet_position_get_latitude(service->position.packet), aprs_packet_position_get_longitude(service->position.packet), aprs_packet_position_get_altitude(service->position.packet), aprs_packet_position_get_speed(service->position.packet), aprs_packet_position_get_course(service->position.packet), aprs_packet_position_get_comment(service->position.packet), aprs_packet_position_get_symbol_table(service->position.packet), aprs_packet_position_get_symbol_table_key(service->position.packet)))
			{
				aprs_packet_deinit(service->position.packet);

				service->position.type   = value;
				service->position.packet = packet;

				return true;
			}
			aprservice_log_error_ex(aprs_packet_position_init, nullptr);
			return false;

		case APRSERVICE_POSITION_TYPE_POSITION_COMPRESSED:
			if (auto packet = aprs_packet_position_init_compressed(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), aprs_packet_position_get_latitude(service->position.packet), aprs_packet_position_get_longitude(service->position.packet), aprs_packet_position_get_altitude(service->position.packet), aprs_packet_position_get_speed(service->position.packet), aprs_packet_position_get_course(service->position.packet), aprs_packet_position_get_comment(service->position.packet), aprs_packet_position_get_symbol_table(service->position.packet), aprs_packet_position_get_symbol_table_key(service->position.packet)))
			{
				aprs_packet_deinit(service->position.packet);

				service->position.type   = value;
				service->position.packet = packet;

				return true;
			}
			aprservice_log_error_ex(aprs_packet_position_init_compressed, nullptr);
			return false;
	}

	return false;
}
void                       APRSERVICE_CALL aprservice_set_event_handler(struct aprservice* service, enum APRSERVICE_EVENTS event, aprservice_event_handler handler, void* param)
{
	if (event < APRSERVICE_EVENTS_COUNT)
		service->events[event] = { .handler = handler, .handler_param = param };
}
void                       APRSERVICE_CALL aprservice_set_default_event_handler(struct aprservice* service, aprservice_event_handler handler, void* param)
{
	service->events[APRSERVICE_EVENTS_COUNT] = { .handler = handler, .handler_param = param };
}
void                       APRSERVICE_CALL aprservice_enable_monitoring(struct aprservice* service, bool value)
{
	service->is_monitoring = value;
}
bool                       APRSERVICE_CALL aprservice_poll(struct aprservice* service)
{
	if (!aprservice_poll_tasks(service))
	{
		aprservice_log_error_ex(aprservice_poll_tasks, false);

		return false;
	}

	if (!aprservice_poll_messages(service))
	{
		aprservice_log_error_ex(aprservice_poll_messages, false);

		return false;
	}

	if (!aprservice_poll_connections(service) && aprservice_is_connected(service))
	{
		aprservice_log_error_ex(aprservice_poll_connections, false);

		return false;
	}

	return true;
}
bool                                       aprservice_poll_tasks(struct aprservice* service)
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

	return true;
}
bool                                       aprservice_poll_messages(struct aprservice* service)
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

	return true;
}
bool                                       aprservice_poll_connections(struct aprservice* service)
{
	auto callback = [](aprservice* service, aprservice_connection* connection, void* param)
	{
		auto on_receive_packet = [](aprservice* service, aprs_packet* packet, aprservice_connection* connection)
		{
			auto packet_type   = aprs_packet_get_type(packet);
			auto packet_sender = aprs_packet_get_sender(packet);

			aprservice_event_execute(service, APRSERVICE_EVENT_RECEIVE_PACKET, { .packet = packet, .connection_mode = connection->mode, .connection_type = connection->type });

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
								aprservice_event_execute(service, APRSERVICE_EVENT_RECEIVE_MESSAGE, { .packet = packet, .sender = packet_sender, .content = packet_message_content, .destination = packet_message_destination, .connection_mode = connection->mode, .connection_type = connection->type });
						}
						else
						{
							if (packet_message_id)
								aprservice_send_message_ack(service, packet_sender, packet_message_id);

							aprservice_event_execute(service, APRSERVICE_EVENT_RECEIVE_MESSAGE, { .packet = packet, .sender = packet_sender, .content = packet_message_content, .destination = packet_message_destination, .connection_mode = connection->mode, .connection_type = connection->type });
						}
						break;
				}
			}
		};

		if (!aprservice_connection_poll(connection))
		{
			aprservice_connection_close(connection);

			return false;
		}

		if (connection->mode & APRSERVICE_CONNECTION_MODE_INPUT)
			switch (connection->type)
			{
				case APRSERVICE_CONNECTION_TYPE_APRS_IS:
					if (aprservice_connection_read_string(connection, service->line))
						if (service->line.starts_with("# "))
						{
							if ((connection->auth.state == APRSERVICE_AUTH_STATE_SENT) && aprservice_connection_auth_from_string(&connection->auth, &service->line[2], connection->passcode != 0))
								aprservice_event_execute(service, APRSERVICE_EVENT_AUTHENTICATE, { .message = connection->auth.message.c_str(), .success = connection->auth.success, .verified = connection->auth.verified, .connection_mode = connection->mode, .connection_type = connection->type });
							else
								aprservice_event_execute(service, APRSERVICE_EVENT_RECEIVE_SERVER_MESSAGE, { .message = &service->line[2], .connection_mode = connection->mode, .connection_type = connection->type });
						}
						else if (auto packet = aprs_packet_init_from_string(service->line.c_str()))
						{
							on_receive_packet(service, packet, connection);

							aprs_packet_deinit(packet);
						}
					break;

				case APRSERVICE_CONNECTION_TYPE_KISS_TNC_TCP:
				case APRSERVICE_CONNECTION_TYPE_KISS_TNC_SERIAL:
					if (aprservice_connection_read_string(connection, service->line))
						if (auto packet = aprs_packet_init_from_string(service->line.c_str()))
						{
							on_receive_packet(service, packet, connection);

							aprs_packet_deinit(packet);
						}
					break;
			}

		return true;
	};

	if (!aprservice_enum_connections(service, callback, nullptr))
		aprservice_disconnect(service);

	return true;
}
bool                                       aprservice_send(struct aprservice* service, aprs_packet* packet)
{
	auto callback = [](aprservice* service, aprservice_connection* connection, void* param)
	{
		if (!aprservice_connection_write_packet(connection, (aprs_packet*)param))
		{
			aprservice_log_error_ex(aprservice_connection_write_packet, false);

			return false;
		}

		return true;
	};

	return aprservice_enum_connections_by_mode(service, APRSERVICE_CONNECTION_MODE_OUTPUT, callback, packet);
}
bool                       APRSERVICE_CALL aprservice_send_raw(struct aprservice* service, const char* content)
{
	if (!aprservice_is_connected(service))
		return false;

	if (aprservice_is_read_only(service))
		return false;

	if (auto packet = aprs_packet_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service)))
	{
		if (!aprservice_send(service, packet))
		{
			aprservice_log_error_ex(aprservice_send, false);

			aprs_packet_deinit(packet);

			return false;
		}

		aprs_packet_deinit(packet);

		return true;
	}

	return false;
}
bool                       APRSERVICE_CALL aprservice_send_item(struct aprservice* service, const char* name, const char* comment, char symbol_table, char symbol_table_key, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course, bool live)
{
	if (!aprservice_is_connected(service))
		return false;

	if (aprservice_is_read_only(service))
		return false;

	if (auto packet = aprs_packet_item_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), name, symbol_table, symbol_table_key))
	{
		if (!aprs_packet_item_set_alive(packet, live))
		{
			aprservice_log_error_ex(aprs_packet_item_set_alive, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprs_packet_item_set_speed(packet, speed))
		{
			aprservice_log_error_ex(aprs_packet_item_set_speed, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprs_packet_item_set_course(packet, course))
		{
			aprservice_log_error_ex(aprs_packet_item_set_course, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprs_packet_item_set_comment(packet, comment))
		{
			aprservice_log_error_ex(aprs_packet_item_set_comment, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprs_packet_item_set_altitude(packet, altitude))
		{
			aprservice_log_error_ex(aprs_packet_item_set_altitude, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprs_packet_item_set_latitude(packet, latitude))
		{
			aprservice_log_error_ex(aprs_packet_item_set_latitude, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprs_packet_item_set_longitude(packet, longitude))
		{
			aprservice_log_error_ex(aprs_packet_item_set_longitude, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprs_packet_item_set_compressed(packet, aprservice_is_compression_enabled(service)))
		{
			aprservice_log_error_ex(aprs_packet_item_set_compressed, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprservice_send(service, packet))
		{
			aprservice_log_error_ex(aprservice_send, false);

			aprs_packet_deinit(packet);

			return false;
		}

		aprs_packet_deinit(packet);

		return true;
	}

	return false;
}
bool                       APRSERVICE_CALL aprservice_send_object(struct aprservice* service, const char* name, const char* comment, char symbol_table, char symbol_table_key, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course, bool live)
{
	if (!aprservice_is_connected(service))
		return false;

	if (aprservice_is_read_only(service))
		return false;

	if (auto packet = aprs_packet_object_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), name, symbol_table, symbol_table_key))
	{
		if (!aprs_packet_object_set_time(packet, aprs_time_now()))
		{
			aprservice_log_error_ex(aprs_packet_object_set_time, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprs_packet_object_set_alive(packet, live))
		{
			aprservice_log_error_ex(aprs_packet_object_set_alive, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprs_packet_object_set_speed(packet, speed))
		{
			aprservice_log_error_ex(aprs_packet_object_set_speed, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprs_packet_object_set_course(packet, course))
		{
			aprservice_log_error_ex(aprs_packet_object_set_course, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprs_packet_object_set_comment(packet, comment))
		{
			aprservice_log_error_ex(aprs_packet_object_set_comment, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprs_packet_object_set_altitude(packet, altitude))
		{
			aprservice_log_error_ex(aprs_packet_object_set_altitude, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprs_packet_object_set_latitude(packet, latitude))
		{
			aprservice_log_error_ex(aprs_packet_object_set_latitude, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprs_packet_object_set_longitude(packet, longitude))
		{
			aprservice_log_error_ex(aprs_packet_object_set_longitude, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprs_packet_object_set_compressed(packet, aprservice_is_compression_enabled(service)))
		{
			aprservice_log_error_ex(aprs_packet_object_set_compressed, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprservice_send(service, packet))
		{
			aprservice_log_error_ex(aprservice_send, false);

			aprs_packet_deinit(packet);

			return false;
		}

		aprs_packet_deinit(packet);

		return true;
	}

	return false;
}
bool                       APRSERVICE_CALL aprservice_send_status(struct aprservice* service, const char* message)
{
	if (!aprservice_is_connected(service))
		return false;

	if (aprservice_is_read_only(service))
		return false;

	if (auto packet = aprs_packet_status_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), message))
	{
		if (!aprs_packet_status_set_time(packet, aprs_time_now()))
		{
			aprservice_log_error_ex(aprs_packet_status_set_time, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprservice_send(service, packet))
		{
			aprservice_log_error_ex(aprservice_send, false);

			aprs_packet_deinit(packet);

			return false;
		}

		aprs_packet_deinit(packet);

		return true;
	}

	return false;
}
bool                       APRSERVICE_CALL aprservice_send_message(struct aprservice* service, const char* destination, const char* content, uint32_t timeout, aprservice_message_callback callback, void* param)
{
	if (service->message_count++ == 0xFFFFF)
		service->message_count = 0;

	char id[6] = {};
	snprintf(id, sizeof(id), "%05X", service->message_count);

	return aprservice_send_message_ex(service, destination, content, id, timeout, callback, param);
}
bool                       APRSERVICE_CALL aprservice_send_message_ex(struct aprservice* service, const char* destination, const char* content, const char* id, uint32_t timeout, aprservice_message_callback callback, void* param)
{
	if (!aprservice_is_connected(service))
		return false;

	if (aprservice_is_read_only(service))
		return false;

	if (auto packet = aprs_packet_message_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), destination, content))
	{
		if (id && !aprs_packet_message_set_id(packet, id))
		{
			aprservice_log_error_ex(aprs_packet_message_set_id, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprservice_send(service, packet))
		{
			aprservice_log_error_ex(aprservice_send, false);

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

	return false;
}
bool                       APRSERVICE_CALL aprservice_send_message_ack(struct aprservice* service, const char* destination, const char* id)
{
	if (!aprservice_is_connected(service))
		return false;

	if (aprservice_is_read_only(service))
		return false;

	if (auto packet = aprs_packet_message_init_ack(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), destination, id))
	{
		if (!aprservice_send(service, packet))
		{
			aprservice_log_error_ex(aprservice_send, false);

			aprs_packet_deinit(packet);

			return false;
		}

		aprs_packet_deinit(packet);

		return true;
	}

	return false;
}
bool                       APRSERVICE_CALL aprservice_send_message_reject(struct aprservice* service, const char* destination, const char* id)
{
	if (!aprservice_is_connected(service))
		return false;

	if (aprservice_is_read_only(service))
		return false;

	if (auto packet = aprs_packet_message_init_reject(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), destination, id))
	{
		if (!aprservice_send(service, packet))
		{
			aprservice_log_error_ex(aprservice_send, false);

			aprs_packet_deinit(packet);

			return false;
		}

		aprs_packet_deinit(packet);

		return true;
	}

	return false;
}
bool                       APRSERVICE_CALL aprservice_send_weather(struct aprservice* service, uint16_t wind_speed, uint16_t wind_speed_gust, uint16_t wind_direction, uint16_t rainfall_last_hour, uint16_t rainfall_last_24_hours, uint16_t rainfall_since_midnight, uint8_t humidity, int16_t temperature, uint32_t barometric_pressure, const char* type)
{
	if (!aprservice_is_connected(service))
		return false;

	if (aprservice_is_read_only(service))
		return false;

	if (auto packet = aprs_packet_weather_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), type))
	{
		if (!aprs_packet_weather_set_time(packet, aprs_time_now()))
		{
			aprservice_log_error_ex(aprs_packet_weather_set_time, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprs_packet_weather_set_wind_speed(packet, wind_speed))
		{
			aprservice_log_error_ex(aprs_packet_weather_set_wind_speed, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprs_packet_weather_set_wind_speed_gust(packet, wind_speed_gust))
		{
			aprservice_log_error_ex(aprs_packet_weather_set_wind_speed_gust, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprs_packet_weather_set_wind_direction(packet, wind_direction))
		{
			aprservice_log_error_ex(aprs_packet_weather_set_wind_direction, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprs_packet_weather_set_rainfall_last_hour(packet, rainfall_last_hour))
		{
			aprservice_log_error_ex(aprs_packet_weather_set_rainfall_last_hour, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprs_packet_weather_set_rainfall_last_24_hours(packet, rainfall_last_24_hours))
		{
			aprservice_log_error_ex(aprs_packet_weather_set_rainfall_last_24_hours, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprs_packet_weather_set_rainfall_since_midnight(packet, rainfall_since_midnight))
		{
			aprservice_log_error_ex(aprs_packet_weather_set_rainfall_since_midnight, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprs_packet_weather_set_humidity(packet, humidity))
		{
			aprservice_log_error_ex(aprs_packet_weather_set_humidity, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprs_packet_weather_set_temperature(packet, temperature))
		{
			aprservice_log_error_ex(aprs_packet_weather_set_temperature, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprs_packet_weather_set_barometric_pressure(packet, barometric_pressure))
		{
			aprservice_log_error_ex(aprs_packet_weather_set_barometric_pressure, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprservice_send(service, packet))
		{
			aprservice_log_error_ex(aprservice_send, false);

			aprs_packet_deinit(packet);

			return false;
		}

		aprs_packet_deinit(packet);

		return true;
	}

	return false;
}
bool                       APRSERVICE_CALL aprservice_send_position(struct aprservice* service)
{
	if (!aprservice_is_connected(service))
		return false;

	if (aprservice_is_read_only(service))
		return false;

	if (!aprservice_send(service, service->position.packet))
	{
		aprservice_log_error_ex(aprservice_send, false);

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

	if (auto packet = aprs_packet_position_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), latitude, longitude, altitude, speed, course, comment, aprservice_get_symbol_table(service), aprservice_get_symbol_table_key(service)))
	{
		aprs_packet_position_enable_messaging(packet, true);
		aprs_packet_position_enable_compression(packet, aprs_packet_position_is_compressed(service->position.packet));

		if (!aprservice_send(service, packet))
		{
			aprservice_log_error_ex(aprservice_send, false);

			aprs_packet_deinit(packet);

			return false;
		}

		aprs_packet_deinit(packet);

		return true;
	}

	return false;
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

	if (auto packet = aprs_packet_telemetry_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), a1, a2, a3, a4, a5, digital, sequence))
	{
		if (comment && !aprs_packet_telemetry_set_comment(packet, comment))
		{
			aprservice_log_error_ex(aprs_packet_telemetry_set_comment, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprservice_send(service, packet))
		{
			aprservice_log_error_ex(aprservice_send, false);

			aprs_packet_deinit(packet);

			return false;
		}

		aprs_packet_deinit(packet);

		return true;
	}

	return false;
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

	if (auto packet = aprs_packet_telemetry_init_float(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), a1, a2, a3, a4, a5, digital, sequence))
	{
		if (comment && !aprs_packet_telemetry_set_comment(packet, comment))
		{
			aprservice_log_error_ex(aprs_packet_telemetry_set_comment, false);

			aprs_packet_deinit(packet);

			return false;
		}

		if (!aprservice_send(service, packet))
		{
			aprservice_log_error_ex(aprservice_send, false);

			aprs_packet_deinit(packet);

			return false;
		}

		aprs_packet_deinit(packet);

		return true;
	}

	return false;
}
bool                       APRSERVICE_CALL aprservice_send_user_defined(struct aprservice* service, char id, char type, const char* data)
{
	if (!aprservice_is_connected(service))
		return false;

	if (aprservice_is_read_only(service))
		return false;

	if (auto packet = aprs_packet_user_defined_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), id, type, data))
	{
		if (!aprservice_send(service, packet))
		{
			aprservice_log_error_ex(aprservice_send, false);

			aprs_packet_deinit(packet);

			return false;
		}

		aprs_packet_deinit(packet);

		return true;
	}

	return false;
}
bool                       APRSERVICE_CALL aprservice_connect(struct aprservice* service, struct aprservice_connection_information* connections, size_t count)
{
	if (aprservice_is_connected(service))
		return false;

	service->connections.resize(count);

	for (size_t i = 0; i < count; ++i)
	{
		switch (connections[i].type)
		{
			case APRSERVICE_CONNECTION_TYPE_APRS_IS:
				service->connections[i] = aprservice_connection_init(service, connections[i].mode, connections[i].type, connections[i].aprs_is.host, connections[i].aprs_is.port, 0, connections[i].aprs_is.passcode);
				break;

			case APRSERVICE_CONNECTION_TYPE_KISS_TNC_TCP:
				service->connections[i] = aprservice_connection_init(service, connections[i].mode, connections[i].type, connections[i].kiss_tnc_tcp.host, connections[i].kiss_tnc_tcp.port, 0, 0);
				break;

			case APRSERVICE_CONNECTION_TYPE_KISS_TNC_SERIAL:
				service->connections[i] = aprservice_connection_init(service, connections[i].mode, connections[i].type, connections[i].kiss_tnc_serial.device, 0, connections[i].kiss_tnc_serial.speed, 0);
				break;
		}

		if (!service->connections[i])
		{
			aprservice_log_error_ex(aprservice_connection_init, nullptr);

			while (i--)
				aprservice_connection_deinit(service->connections[i]);

			service->connections.clear();

			return false;
		}

		if (!aprservice_connection_open(service->connections[i]))
		{
			aprservice_log_error_ex(aprservice_connection_open, false);

			do
				aprservice_connection_deinit(service->connections[i]);
			while (i--);

			service->connections.clear();

			return false;
		}
	}

	service->is_connected = true;

	return true;
}
void                       APRSERVICE_CALL aprservice_disconnect(struct aprservice* service)
{
	if (aprservice_is_connected(service))
	{
		for (auto connection : service->connections)
			aprservice_connection_deinit(connection);

		service->connections.clear();

		service->message_callbacks.remove_if([service](aprservice_message_callback_context* context) {
			context->callback(service, APRSERVICE_MESSAGE_ERROR_DISCONNECTED, context->callback_param);

			return true;
		});

		service->message_callbacks_index.clear();

		service->is_connected = false;
	}
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
struct aprservice*         APRSERVICE_CALL aprservice_task_get_service(struct aprservice_task* task)
{
	return task->service;
}

struct aprservice_item*    APRSERVICE_CALL aprservice_item_create(struct aprservice* service, const char* name, const char* comment, char symbol_table, char symbol_table_key, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course)
{
	auto item = new aprservice_item
	{
		.service = service
	};

	if (!(item->packet = aprs_packet_object_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), name, symbol_table, symbol_table_key)))
	{
		delete item;

		return nullptr;
	}

	if (!aprs_packet_item_set_alive(item->packet, true))
	{
		aprservice_log_error_ex(aprs_packet_item_set_alive, false);

		aprs_packet_deinit(item->packet);

		delete item;

		return nullptr;
	}

	if (!aprs_packet_item_set_speed(item->packet, speed))
	{
		aprservice_log_error_ex(aprs_packet_item_set_speed, false);

		aprs_packet_deinit(item->packet);

		delete item;

		return nullptr;
	}

	if (!aprs_packet_item_set_course(item->packet, course))
	{
		aprservice_log_error_ex(aprs_packet_item_set_course, false);

		aprs_packet_deinit(item->packet);

		delete item;

		return nullptr;
	}

	if (!aprs_packet_item_set_comment(item->packet, comment))
	{
		aprservice_log_error_ex(aprs_packet_item_set_comment, false);

		aprs_packet_deinit(item->packet);

		delete item;

		return nullptr;
	}

	if (!aprs_packet_item_set_altitude(item->packet, altitude))
	{
		aprservice_log_error_ex(aprs_packet_item_set_altitude, false);

		aprs_packet_deinit(item->packet);

		delete item;

		return nullptr;
	}

	if (!aprs_packet_item_set_latitude(item->packet, latitude))
	{
		aprservice_log_error_ex(aprs_packet_item_set_latitude, false);

		aprs_packet_deinit(item->packet);

		delete item;

		return nullptr;
	}

	if (!aprs_packet_item_set_longitude(item->packet, longitude))
	{
		aprservice_log_error_ex(aprs_packet_item_set_longitude, false);

		aprs_packet_deinit(item->packet);

		delete item;

		return nullptr;
	}

	if (!aprs_packet_item_set_compressed(item->packet, aprservice_is_compression_enabled(service)))
	{
		aprservice_log_error_ex(aprs_packet_item_set_compressed, false);

		aprs_packet_deinit(item->packet);

		delete item;

		return nullptr;
	};

	service->items.push_back(item);

	return item;
}
void                       APRSERVICE_CALL aprservice_item_destroy(struct aprservice_item* item)
{
	if (auto service = aprservice_item_get_service(item))
	{
		service->items.remove(item);

		aprs_packet_deinit(item->packet);

		delete item;
	}
}
bool                       APRSERVICE_CALL aprservice_item_is_alive(struct aprservice_item* item)
{
	return aprs_packet_item_is_alive(item->packet);
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
		aprservice_log_error_ex(aprs_packet_item_set_symbol, false);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_item_set_comment(struct aprservice_item* item, const char* value)
{
	if (!aprs_packet_item_set_comment(item->packet, value))
	{
		aprservice_log_error_ex(aprs_packet_item_set_comment, false);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_item_set_position(struct aprservice_item* item, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course)
{
	if (!aprs_packet_item_set_speed(item->packet, speed))
	{
		aprservice_log_error_ex(aprs_packet_item_set_speed, false);

		return false;
	}

	if (!aprs_packet_item_set_course(item->packet, course))
	{
		aprservice_log_error_ex(aprs_packet_item_set_course, false);

		return false;
	}

	if (!aprs_packet_item_set_altitude(item->packet, altitude))
	{
		aprservice_log_error_ex(aprs_packet_item_set_altitude, false);

		return false;
	}

	if (!aprs_packet_item_set_latitude(item->packet, latitude))
	{
		aprservice_log_error_ex(aprs_packet_item_set_latitude, false);

		return false;
	}

	if (!aprs_packet_item_set_longitude(item->packet, longitude))
	{
		aprservice_log_error_ex(aprs_packet_item_set_longitude, false);

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
		aprservice_log_error_ex(aprs_packet_item_set_alive, false);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_item_announce(struct aprservice_item* item)
{
	if (!aprservice_send(item->service, item->packet))
	{
		aprservice_log_error_ex(aprservice_send, false);

		return false;
	}

	return true;
}

struct aprservice_object*  APRSERVICE_CALL aprservice_object_create(struct aprservice* service, const char* name, const char* comment, char symbol_table, char symbol_table_key, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course)
{
	auto object = new aprservice_object
	{
		.service = service
	};

	if (!(object->packet = aprs_packet_object_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), name, symbol_table, symbol_table_key)))
	{
		delete object;

		return nullptr;
	}

	if (!aprs_packet_object_set_alive(object->packet, true))
	{
		aprservice_log_error_ex(aprs_packet_object_set_alive, false);

		aprs_packet_deinit(object->packet);

		delete object;

		return nullptr;
	}

	if (!aprs_packet_object_set_speed(object->packet, speed))
	{
		aprservice_log_error_ex(aprs_packet_object_set_speed, false);

		aprs_packet_deinit(object->packet);

		delete object;

		return nullptr;
	}

	if (!aprs_packet_object_set_course(object->packet, course))
	{
		aprservice_log_error_ex(aprs_packet_object_set_course, false);

		aprs_packet_deinit(object->packet);

		delete object;

		return nullptr;
	}

	if (!aprs_packet_object_set_comment(object->packet, comment))
	{
		aprservice_log_error_ex(aprs_packet_object_set_comment, false);

		aprs_packet_deinit(object->packet);

		delete object;

		return nullptr;
	}

	if (!aprs_packet_object_set_altitude(object->packet, altitude))
	{
		aprservice_log_error_ex(aprs_packet_object_set_altitude, false);

		aprs_packet_deinit(object->packet);

		delete object;

		return nullptr;
	}

	if (!aprs_packet_object_set_latitude(object->packet, latitude))
	{
		aprservice_log_error_ex(aprs_packet_object_set_latitude, false);

		aprs_packet_deinit(object->packet);

		delete object;

		return nullptr;
	}

	if (!aprs_packet_object_set_longitude(object->packet, longitude))
	{
		aprservice_log_error_ex(aprs_packet_object_set_longitude, false);

		aprs_packet_deinit(object->packet);

		delete object;

		return nullptr;
	}

	if (!aprs_packet_object_set_compressed(object->packet, aprservice_is_compression_enabled(service)))
	{
		aprservice_log_error_ex(aprs_packet_object_set_compressed, false);

		aprs_packet_deinit(object->packet);

		delete object;

		return nullptr;
	};

	service->objects.push_back(object);

	return object;
}
void                       APRSERVICE_CALL aprservice_object_destroy(struct aprservice_object* object)
{
	if (auto service = aprservice_object_get_service(object))
	{
		service->objects.remove(object);

		aprs_packet_deinit(object->packet);

		delete object;
	}
}
bool                       APRSERVICE_CALL aprservice_object_is_alive(struct aprservice_object* object)
{
	return aprs_packet_object_is_alive(object->packet);
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
		aprservice_log_error_ex(aprs_packet_object_set_symbol, false);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_object_set_comment(struct aprservice_object* object, const char* value)
{
	if (!aprs_packet_object_set_comment(object->packet, value))
	{
		aprservice_log_error_ex(aprs_packet_object_set_comment, false);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_object_set_position(struct aprservice_object* object, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course)
{
	if (!aprs_packet_object_set_speed(object->packet, speed))
	{
		aprservice_log_error_ex(aprs_packet_object_set_speed, false);

		return false;
	}

	if (!aprs_packet_object_set_course(object->packet, course))
	{
		aprservice_log_error_ex(aprs_packet_object_set_course, false);

		return false;
	}

	if (!aprs_packet_object_set_altitude(object->packet, altitude))
	{
		aprservice_log_error_ex(aprs_packet_object_set_altitude, false);

		return false;
	}

	if (!aprs_packet_object_set_latitude(object->packet, latitude))
	{
		aprservice_log_error_ex(aprs_packet_object_set_latitude, false);

		return false;
	}

	if (!aprs_packet_object_set_longitude(object->packet, longitude))
	{
		aprservice_log_error_ex(aprs_packet_object_set_longitude, false);

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
		aprservice_log_error_ex(aprs_packet_object_set_alive, false);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_object_announce(struct aprservice_object* object)
{
	if (!aprs_packet_object_set_time(object->packet, aprs_time_now()))
	{
		aprservice_log_error_ex(aprs_packet_object_set_time, false);

		return false;
	}

	if (!aprservice_send(object->service, object->packet))
	{
		aprservice_log_error_ex(aprservice_send, false);

		return false;
	}

	return true;
}

struct aprservice_command* APRSERVICE_CALL aprservice_command_register(struct aprservice* service, const char* name, const char* help, aprservice_command_handler handler, void* param)
{
	if (auto it = service->commands.find(name); it != service->commands.end())
	{
		auto command = it->second;

		command->name          = name;
		command->help          = help;
		command->handler       = handler;
		command->handler_param = param;

		return command;
	}

	auto command = new aprservice_command
	{
		.service       = service,

		.name          = name,
		.help          = help,
		.handler       = handler,
		.handler_param = param
	};

	service->commands.emplace(name, command);

	return command;
}
void                       APRSERVICE_CALL aprservice_command_unregister(struct aprservice_command* command)
{
	if (auto service = aprservice_command_get_service(command))
	{
		service->commands.erase(command->name);

		delete command;
	}
}
struct aprservice*         APRSERVICE_CALL aprservice_command_get_service(struct aprservice_command* command)
{
	return command->service;
}
