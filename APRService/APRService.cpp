#include "APRService.hpp"

#include <map>
#include <list>
#include <array>
#include <ctime>
#include <queue>
#include <regex>
#include <chrono>
#include <string>
#include <cstring>
#include <sstream>
#include <iostream>
#include <unordered_map>

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

enum APRSERVICE_AUTH_STATES
{
	APRSERVICE_AUTH_STATE_NONE,
	APRSERVICE_AUTH_STATE_SENT,
	APRSERVICE_AUTH_STATE_RECEIVED
};

struct aprservice_io_buffer
{
	std::string value;
	size_t      offset;
};

struct aprservice_io
{
	aprservice*                      service;

	bool                             is_connected;

#if defined(APRSERVICE_UNIX)
	int                              socket;
#elif defined(APRSERVICE_WIN32)
	SOCKET                           socket;
	WSADATA                          winsock;
#endif

	std::string                      rx_buffer;
	std::array<char, 128>            rx_buffer_tmp;

	std::queue<aprservice_io_buffer> tx_buffer_queue;
};

struct aprservice_auth
{
	APRSERVICE_AUTH_STATES state;
	std::string            message;
	bool                   success;
	bool                   verified;
};

struct aprservice_event
{
	aprservice_event_handler handler;
	void*                    handler_param;
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
	bool                                                                            is_auth_verified;

	aprservice_io*                                                                  io;
	aprservice_auth                                                                 auth;
	std::string                                                                     line;
	int64_t                                                                         time;

	aprs_path*                                                                      path;
	const std::string                                                               station;
	aprs_packet*                                                                    position;

	std::map<uint64_t, std::list<aprservice_task*>>                                 tasks;
	aprservice_event                                                                events[APRSERVICE_EVENTS_COUNT + 1];
	std::list<aprservice_object*>                                                   objects;
	std::unordered_map<std::string, aprservice_command*>                            commands;

	uint16_t                                                                        message_count;
	std::list<aprservice_message_callback_context*>                                 message_callbacks;
	std::unordered_map<std::string, std::list<aprservice_message_callback_context>> message_callbacks_index;

	uint16_t                                                                        telemetry_count;
};

struct aprservice_task
{
	aprservice*             service;

	uint32_t                seconds;
	aprservice_task_handler handler;
	void*                   handler_param;
};

struct aprservice_error
{
	int         code;
	const char* string;
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

bool                                       aprservice_io_is_connected(aprservice_io* io);
void                                       aprservice_io_disconnect(aprservice_io* io);
int                                        aprservice_io_read(aprservice_io* io, char* buffer, size_t size, size_t* number_of_bytes_received);
int                                        aprservice_io_write(aprservice_io* io, const char* buffer, size_t size, size_t* number_of_bytes_sent);

bool                                       aprservice_io_init(aprservice* service, aprservice_io** io)
{
	*io = new aprservice_io
	{
		.service      = service,

		.is_connected = false
	};

#if defined(APRSERVICE_WIN32)
	if (auto error = WSAStartup(MAKEWORD(2, 2), &(*io)->winsock))
	{
		aprservice_log_error_ex(WSAStartup, error);

		delete *io;

		return false;
	}
#endif

	return true;
}
void                                       aprservice_io_deinit(aprservice_io* io)
{
	if (aprservice_io_is_connected(io))
		aprservice_io_disconnect(io);

#if defined(APRSERVICE_WIN32)
	WSACleanup();
#endif

	delete io;
}
bool                                       aprservice_io_is_connected(aprservice_io* io)
{
	return io->is_connected;
}
bool                                       aprservice_io_connect(aprservice_io* io, const char* host, uint16_t port)
{
	if (aprservice_io_is_connected(io))
		return true;

resolve_host:
	addrinfo  dns_hint = { .ai_family = AF_UNSPEC };
	addrinfo* dns_result;

	if (auto error = getaddrinfo(host, "", &dns_hint, &dns_result))
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
			dns_result_ipv4.sin_port = htons(port);
			dns_result_address        = reinterpret_cast<sockaddr*>(&dns_result_ipv4);
			dns_result_address_length = sizeof(sockaddr_in);
			break;

		case AF_INET6:
			dns_result_ipv6           = *reinterpret_cast<const sockaddr_in6*>(dns_result->ai_addr);
			dns_result_ipv6.sin6_port = htons(port);
			dns_result_address        = reinterpret_cast<sockaddr*>(&dns_result_ipv6);
			dns_result_address_length = sizeof(sockaddr_in6);
			break;
	}

	freeaddrinfo(dns_result);

	if (!dns_result_address_length)
		return false;

socket_open:
#if defined(APRSERVICE_UNIX)
	if ((io->socket = socket(dns_result_address->sa_family, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		aprservice_log_error(socket);

		return false;
	}
#elif defined(APRSERVICE_WIN32)
	if ((io->socket = WSASocketW(dns_result_address->sa_family, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, 0)) == INVALID_SOCKET)
	{
		aprservice_log_error(WSASocketW);

		return false;
	}
#endif

socket_connect:
#if defined(APRSERVICE_UNIX)
	if (connect(io->socket, socket_address, socket_address_length) == -1)
	{
		aprservice_log_error(connect);

		close(io->socket);

		return false;
	}

	int flags;

	if ((flags = fcntl(io->socket, F_GETFL, 0)) == -1)
	{
		aprservice_log_error(fcntl);

		close(io->socket);

		return false;
	}

	if (fcntl(io->socket, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		aprservice_log_error(fcntl);

		close(io->socket);

		return false;
	}
#elif defined(APRSERVICE_WIN32)
	if (connect(io->socket, dns_result_address, dns_result_address_length) == SOCKET_ERROR)
	{
		aprservice_log_error(connect);

		closesocket(io->socket);

		return false;
	}

	u_long arg = 1;

	if (ioctlsocket(io->socket, FIONBIO, &arg) == SOCKET_ERROR)
	{
		aprservice_log_error(ioctlsocket);

		closesocket(io->socket);

		return false;
	}
#endif

	io->is_connected = true;

	return true;
}
void                                       aprservice_io_disconnect(aprservice_io* io)
{
	if (aprservice_io_is_connected(io))
	{
#if defined(APRSERVICE_UNIX)
		close(io->socket);
#elif defined(APRSERVICE_WIN32)
		closesocket(io->socket);
#endif

		io->rx_buffer.clear();

		while (!io->tx_buffer_queue.empty())
			io->tx_buffer_queue.pop();

		io->is_connected = false;
	}
}
bool                                       aprservice_io_flush(aprservice_io* io)
{
	size_t number_of_bytes_sent;

	while (auto buffer = io->tx_buffer_queue.empty() ? nullptr : &io->tx_buffer_queue.front())
	{
		if (buffer->offset < buffer->value.length())
		{
			switch (aprservice_io_write(io, &buffer->value[buffer->offset], buffer->value.length() - buffer->offset, &number_of_bytes_sent))
			{
				case 0:  return false;
				case -1: return true;
			}

			buffer->offset += number_of_bytes_sent;
		}

		if (buffer->offset >= buffer->value.length())
		{
			switch (aprservice_io_write(io, &"\r\n"[buffer->offset - buffer->value.length()], (buffer->value.length() + 2) - buffer->offset, &number_of_bytes_sent))
			{
				case 0:  return false;
				case -1: return true;
			}

			if ((buffer->offset += number_of_bytes_sent) == (buffer->value.length() + 2))
				io->tx_buffer_queue.pop();
		}
	}

	return true;
}
// @return 0 on disconnect
// @return -1 on would block
int                                        aprservice_io_read(aprservice_io* io, char* buffer, size_t size, size_t* number_of_bytes_received)
{
	if (!aprservice_io_is_connected(io))
		return 0;

#if defined(APRSERVICE_UNIX)
	ssize_t bytes_received;

	if ((bytes_received = recv(io->socket, buffer, size, 0)) == -1)
	{
		auto error = errno;

		if ((error == EAGAIN) || (error == EWOULDBLOCK))
			return -1;

		aprservice_io_disconnect(io);

		if ((error == EHOSTDOWN) || (error == ECONNRESET) || (error == EHOSTUNREACH))
			return 0;

		aprservice_log_error_ex(recv, error);

		return 0;
	}

	*number_of_bytes_received = bytes_received;
#elif defined(APRSERVICE_WIN32)
	int bytes_received;

	if ((bytes_received = recv(io->socket, reinterpret_cast<char*>(buffer), static_cast<int>(size), 0)) == SOCKET_ERROR)
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
				aprservice_io_disconnect(io);
				return 0;
		}

		return 0;
	}

	*number_of_bytes_received = bytes_received;
#endif

	return 1;
}
// @return 0 on disconnect
// @return -1 on would block
int                                        aprservice_io_write(aprservice_io* io, const char* buffer, size_t size, size_t* number_of_bytes_sent)
{
	if (!aprservice_io_is_connected(io))
		return false;

#if defined(APRSERVICE_UNIX)
	ssize_t bytes_sent;

	if ((bytes_sent = send(io->socket, buffer, size, 0)) == -1)
	{
		auto error = errno;

		if ((error == EAGAIN) || (error == EWOULDBLOCK))
			return -1;

		aprservice_io_disconnect(sn);

		if ((error == EHOSTDOWN) || (error == ECONNRESET) || (error == EHOSTUNREACH))
			return 0;

		aprservice_log_error_ex(send, error);

		return 0;
	}

	*number_of_bytes_sent = bytes_sent;
#elif defined(APRSERVICE_WIN32)
	int bytes_sent;

	if ((bytes_sent = send(io->socket, reinterpret_cast<const char*>(buffer), static_cast<int>(size), 0)) == SOCKET_ERROR)
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
				aprservice_io_disconnect(io);
				return 0;
		}

		aprservice_log_error_ex(send, error);

		return 0;
	}

	*number_of_bytes_sent = bytes_sent;
#endif

	return 1;
}
// @return 0 on disconnect
// @return -1 on would block
int                                        aprservice_io_read_line(aprservice_io* io, std::string& value)
{
	size_t number_of_bytes_received;

	switch (aprservice_io_read(io, &io->rx_buffer_tmp[0], io->rx_buffer_tmp.max_size(), &number_of_bytes_received))
	{
		case 0:  return 0;
		case -1: return -1;
	}

	io->rx_buffer.append(&io->rx_buffer_tmp[0], number_of_bytes_received);

	if (auto i = io->rx_buffer.find("\r\n"); i != std::string::npos)
	{
		value         = io->rx_buffer.substr(0, i);
		io->rx_buffer = io->rx_buffer.substr(i + 2);

		return 1;
	}

	return -1;
}
bool                                       aprservice_io_write_line(aprservice_io* io, std::string&& value)
{
	size_t number_of_bytes_sent[2];

	switch (aprservice_io_write(io, value.c_str(), value.length(), &number_of_bytes_sent[0]))
	{
		case 0:
			return false;

		case -1:
			number_of_bytes_sent[0] = 0;
			break;
	}

	if (number_of_bytes_sent[0] != value.length())
	{
		io->tx_buffer_queue.push({
			.value  = std::move(value),
			.offset = number_of_bytes_sent[0]
		});

		return true;
	}

	switch (aprservice_io_write(io, "\r\n", 2, &number_of_bytes_sent[1]))
	{
		case 0:
			return false;

		case -1:
			number_of_bytes_sent[1] = 0;
			break;
	}

	if (number_of_bytes_sent[1] != 2)
		io->tx_buffer_queue.push({
			.value  = std::move(value),
			.offset = number_of_bytes_sent[0] + number_of_bytes_sent[1]
		});

	return true;
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

bool                                       aprservice_authentication_from_string(aprservice_auth* auth, const char* string, bool is_verified)
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

bool                                       aprservice_poll_io(struct aprservice* service);
bool                                       aprservice_poll_tasks(struct aprservice* service);
bool                                       aprservice_poll_messages(struct aprservice* service);

bool                                       aprservice_send_message_ack(struct aprservice* service, const char* destination, const char* id);
bool                                       aprservice_send_message_reject(struct aprservice* service, const char* destination, const char* id);

struct aprservice*         APRSERVICE_CALL aprservice_init(const char* station, struct aprs_path* path, char symbol_table, char symbol_table_key)
{
	if (!station || !path)
		return nullptr;

	auto service = new aprservice
	{
		.is_connected     = false,
		.is_monitoring    = false,
		.is_auth_verified = false,

		.io               = nullptr,
		.auth             = { .state = APRSERVICE_AUTH_STATE_NONE },
		.time             = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count(),

		.path             = path,
		.station          = station,
		.position         = nullptr,

		.events           = {},

		.message_count    = 0,

		.telemetry_count  = 0
	};

	if (!aprservice_io_init(service, &service->io))
	{
		aprservice_log_error_ex(aprservice_io_init, false);

		delete service;

		return nullptr;
	}

	if (!(service->position = aprs_packet_position_init(station, APRSERVICE_TOCALL, path, 0, 0, 0, 0, 0, "", symbol_table, symbol_table_key)))
	{
		aprservice_log_error_ex(aprs_packet_position_init, nullptr);

		aprservice_io_deinit(service->io);

		delete service;

		return nullptr;
	}

	aprs_packet_position_enable_messaging(service->position, true);

	aprs_path_add_reference(path);

	return service;
}
void                       APRSERVICE_CALL aprservice_deinit(struct aprservice* service)
{
	if (aprservice_is_connected(service))
		aprservice_disconnect(service);

	aprs_packet_deinit(service->position);

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

	service->objects.remove_if([](aprservice_object* object) {
		aprservice_object_destroy(object);

		return true;
	});

	for (auto it = service->commands.begin(); it != service->commands.end(); )
		aprservice_command_unregister((it++)->second);

	aprservice_io_deinit(service->io);

	aprs_path_deinit(service->path);

	delete service;
}
bool                       APRSERVICE_CALL aprservice_is_read_only(struct aprservice* service)
{
	if (service->auth.state == APRSERVICE_AUTH_STATE_RECEIVED)
		return !service->auth.success || !service->auth.verified;

	return false;
}
bool                       APRSERVICE_CALL aprservice_is_connected(struct aprservice* service)
{
	return service->is_connected;
}
bool                       APRSERVICE_CALL aprservice_is_authenticated(struct aprservice* service)
{
	if (service->auth.state == APRSERVICE_AUTH_STATE_RECEIVED)
		return service->auth.success && service->auth.verified;

	return false;
}
bool                       APRSERVICE_CALL aprservice_is_authenticating(struct aprservice* service)
{
	return service->auth.state == APRSERVICE_AUTH_STATE_SENT;
}
bool                       APRSERVICE_CALL aprservice_is_monitoring_enabled(struct aprservice* service)
{
	return service->is_monitoring;
}
bool                       APRSERVICE_CALL aprservice_is_compression_enabled(struct aprservice* service)
{
	return aprs_packet_position_is_compressed(service->position);
}
struct aprs_path*          APRSERVICE_CALL aprservice_get_path(struct aprservice* service)
{
	return service->path;
}
uint32_t                   APRSERVICE_CALL aprservice_get_time(struct aprservice* service)
{
	return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count() - service->time;
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
	if (!aprs_packet_position_set_symbol(service->position, table, key))
	{
		aprservice_log_error_ex(aprs_packet_position_set_symbol, false);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_set_comment(struct aprservice* service, const char* value)
{
	if (!aprs_packet_position_set_comment(service->position, value))
	{
		aprservice_log_error_ex(aprs_packet_position_set_comment, false);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_set_position(struct aprservice* service, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course)
{
	if (!aprs_packet_position_set_speed(service->position, speed))
	{
		aprservice_log_error_ex(aprs_packet_position_set_speed, false);

		return false;
	}

	if (!aprs_packet_position_set_course(service->position, course))
	{
		aprservice_log_error_ex(aprs_packet_position_set_course, false);

		return false;
	}

	if (!aprs_packet_position_set_altitude(service->position, altitude))
	{
		aprservice_log_error_ex(aprs_packet_position_set_altitude, false);

		return false;
	}

	if (!aprs_packet_position_set_latitude(service->position, latitude))
	{
		aprservice_log_error_ex(aprs_packet_position_set_latitude, false);

		return false;
	}

	if (!aprs_packet_position_set_longitude(service->position, longitude))
	{
		aprservice_log_error_ex(aprs_packet_position_set_longitude, false);

		return false;
	}

	return true;
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
void                       APRSERVICE_CALL aprservice_enable_compression(struct aprservice* service, bool value)
{
	aprs_packet_position_enable_compression(service->position, value);
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

	if (!aprservice_poll_io(service) && aprservice_is_connected(service))
	{
		aprservice_log_error_ex(aprservice_poll_io, false);

		return false;
	}

	return true;
}
bool                                       aprservice_poll_io(struct aprservice* service)
{
	do
	{
		if (!aprservice_io_flush(service->io))
		{
			aprservice_disconnect(service);

			return false;
		}

		switch (aprservice_io_read_line(service->io, service->line))
		{
			case 0:
				aprservice_disconnect(service);
				return false;

			case -1:
				return true;
		}

		if (service->line.starts_with("# "))
		{
			if (aprservice_is_authenticating(service) && aprservice_authentication_from_string(&service->auth, &service->line[2], service->is_auth_verified))
				aprservice_event_execute(service, APRSERVICE_EVENT_AUTHENTICATE, { .message = service->auth.message.c_str(), .success = service->auth.success, .verified = service->auth.verified });
			else
				aprservice_event_execute(service, APRSERVICE_EVENT_RECEIVE_SERVER_MESSAGE, { .message = &service->line[2] });
		}
		else if (auto packet = aprs_packet_init_from_string(service->line.c_str()))
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
								aprservice_event_execute(service, APRSERVICE_EVENT_RECEIVE_MESSAGE, { .packet = packet, .sender = packet_sender, .content = packet_message_content, .destination = packet_message_destination });
						}
						else
						{
							if (packet_message_id)
								aprservice_send_message_ack(service, packet_sender, packet_message_id);

							aprservice_event_execute(service, APRSERVICE_EVENT_RECEIVE_MESSAGE, { .packet = packet, .sender = packet_sender, .content = packet_message_content, .destination = packet_message_destination });
						}
						break;
				}
			}

			aprs_packet_deinit(packet);
		}
	} while (aprservice_is_connected(service));

	return false;
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
bool                       APRSERVICE_CALL aprservice_send(struct aprservice* service, const char* raw)
{
	if (!aprservice_is_connected(service))
		return false;

	if ((service->auth.state != APRSERVICE_AUTH_STATE_NONE) && aprservice_is_read_only(service))
		return false;

	if (!aprservice_io_write_line(service->io, raw))
	{
		aprservice_log_error_ex(aprservice_io_write_line, false);

		aprservice_disconnect(service);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_send(struct aprservice* service, std::string&& raw)
{
	if (!aprservice_is_connected(service))
		return false;

	if ((service->auth.state != APRSERVICE_AUTH_STATE_NONE) && aprservice_is_read_only(service))
		return false;

	if (!aprservice_io_write_line(service->io, std::move(raw)))
	{
		aprservice_log_error_ex(aprservice_io_write_line, false);

		aprservice_disconnect(service);

		return false;
	}

	return true;
}
bool                       APRSERVICE_CALL aprservice_send_packet(struct aprservice* service, const char* content)
{
	if (!aprservice_is_connected(service))
		return false;

	if (auto packet = aprs_packet_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service)))
	{
		std::string string = aprs_packet_to_string(packet);

		aprs_packet_deinit(packet);

		if (!aprservice_send(service, std::move(string)))
		{
			aprservice_log_error_ex(aprservice_send, false);

			return false;
		}

		return true;
	}

	return false;
}
bool                       APRSERVICE_CALL aprservice_send_object(struct aprservice* service, const char* name, const char* comment, char symbol_table, char symbol_table_key, float latitude, float longitude, int32_t altitude, uint16_t speed, uint16_t course, bool live)
{
	if (!aprservice_is_connected(service))
		return false;

	if (auto packet = aprs_packet_object_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), name, symbol_table, symbol_table_key))
	{
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

		std::string string = aprs_packet_to_string(packet);

		aprs_packet_deinit(packet);

		if (!aprservice_send(service, std::move(string)))
		{
			aprservice_log_error_ex(aprservice_send, false);

			return false;
		}

		return true;
	}

	return false;
}
bool                       APRSERVICE_CALL aprservice_send_status(struct aprservice* service, const char* message)
{
	if (!aprservice_is_connected(service))
		return false;

	if (auto packet = aprs_packet_status_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), message))
	{
		if (!aprs_packet_status_set_time(packet, aprs_time_now()))
		{
			aprservice_log_error_ex(aprs_packet_status_set_time, false);

			aprs_packet_deinit(packet);

			return false;
		}

		std::string string = aprs_packet_to_string(packet);

		aprs_packet_deinit(packet);

		if (!aprservice_send(service, std::move(string)))
		{
			aprservice_log_error_ex(aprservice_send, false);

			return false;
		}

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

	if (auto packet = aprs_packet_message_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), destination, content))
	{
		if (id && !aprs_packet_message_set_id(packet, id))
		{
			aprservice_log_error_ex(aprs_packet_message_set_id, false);

			aprs_packet_deinit(packet);

			return false;
		}

		std::string string = aprs_packet_to_string(packet);

		aprs_packet_deinit(packet);

		if (!aprservice_send(service, std::move(string)))
		{
			aprservice_log_error_ex(aprservice_send, false);

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

		return true;
	}

	return false;
}
bool                       APRSERVICE_CALL aprservice_send_message_ack(struct aprservice* service, const char* destination, const char* id)
{
	if (!aprservice_is_connected(service))
		return false;

	if (auto packet = aprs_packet_message_init_ack(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), destination, id))
	{
		std::string string = aprs_packet_to_string(packet);

		aprs_packet_deinit(packet);

		if (!aprservice_send(service, std::move(string)))
		{
			aprservice_log_error_ex(aprservice_send, false);

			return false;
		}

		return true;
	}

	return false;
}
bool                       APRSERVICE_CALL aprservice_send_message_reject(struct aprservice* service, const char* destination, const char* id)
{
	if (!aprservice_is_connected(service))
		return false;

	if (auto packet = aprs_packet_message_init_reject(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), destination, id))
	{
		std::string string = aprs_packet_to_string(packet);

		aprs_packet_deinit(packet);

		if (!aprservice_send(service, std::move(string)))
		{
			aprservice_log_error_ex(aprservice_send, false);

			return false;
		}

		return true;
	}

	return false;
}
bool                       APRSERVICE_CALL aprservice_send_weather(struct aprservice* service, uint16_t wind_speed, uint16_t wind_speed_gust, uint16_t wind_direction, uint16_t rainfall_last_hour, uint16_t rainfall_last_24_hours, uint16_t rainfall_since_midnight, uint8_t humidity, int16_t temperature, uint32_t barometric_pressure, const char* type)
{
	if (!aprservice_is_connected(service))
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

		std::string string = aprs_packet_to_string(packet);

		aprs_packet_deinit(packet);

		if (!aprservice_send(service, std::move(string)))
		{
			aprservice_log_error_ex(aprservice_send, false);

			return false;
		}

		return true;
	}

	return false;
}
bool                       APRSERVICE_CALL aprservice_send_position(struct aprservice* service)
{
	if (!aprservice_is_connected(service))
		return false;

	std::string string = aprs_packet_to_string(service->position);

	if (!aprservice_send(service, std::move(string)))
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

	if (auto packet = aprs_packet_position_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), latitude, longitude, altitude, speed, course, comment, aprservice_get_symbol_table(service), aprservice_get_symbol_table_key(service)))
	{
		aprs_packet_position_enable_messaging(packet, true);
		aprs_packet_position_enable_compression(packet, aprservice_is_compression_enabled(service));

		std::string string = aprs_packet_to_string(packet);

		aprs_packet_deinit(packet);

		if (!aprservice_send(service, std::move(string)))
		{
			aprservice_log_error_ex(aprservice_send, false);

			return false;
		}

		return true;
	}

	return false;
}
bool                       APRSERVICE_CALL aprservice_send_telemetry(struct aprservice* service, uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t digital)
{
	if (service->telemetry_count++ == 999)
		service->telemetry_count = 0;

	return aprservice_send_telemetry_ex(service, a1, a2, a3, a4, a5, digital, aprs_packet_position_get_comment(service->position), service->telemetry_count);
}
bool                       APRSERVICE_CALL aprservice_send_telemetry_ex(struct aprservice* service, uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t digital, const char* comment, uint16_t sequence)
{
	if (!aprservice_is_connected(service))
		return false;

	if (auto packet = aprs_packet_telemetry_init(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), a1, a2, a3, a4, a5, digital, sequence))
	{
		if (comment && !aprs_packet_telemetry_set_comment(packet, comment))
		{
			aprservice_log_error_ex(aprs_packet_telemetry_set_comment, false);

			aprs_packet_deinit(packet);

			return false;
		}

		std::string string = aprs_packet_to_string(packet);

		aprs_packet_deinit(packet);

		if (!aprservice_send(service, std::move(string)))
		{
			aprservice_log_error_ex(aprservice_send, false);

			return false;
		}

		return true;
	}

	return false;
}
bool                       APRSERVICE_CALL aprservice_send_telemetry_float(struct aprservice* service, float a1, float a2, float a3, float a4, float a5, uint8_t digital)
{
	if (service->telemetry_count++ == 999)
		service->telemetry_count = 0;

	return aprservice_send_telemetry_float_ex(service, a1, a2, a3, a4, a5, digital, aprs_packet_position_get_comment(service->position), service->telemetry_count);
}
bool                       APRSERVICE_CALL aprservice_send_telemetry_float_ex(struct aprservice* service, float a1, float a2, float a3, float a4, float a5, uint8_t digital, const char* comment, uint16_t sequence)
{
	if (!aprservice_is_connected(service))
		return false;

	if (auto packet = aprs_packet_telemetry_init_float(aprservice_get_station(service), APRSERVICE_TOCALL, aprservice_get_path(service), a1, a2, a3, a4, a5, digital, sequence))
	{
		if (comment && !aprs_packet_telemetry_set_comment(packet, comment))
		{
			aprservice_log_error_ex(aprs_packet_telemetry_set_comment, false);

			aprs_packet_deinit(packet);

			return false;
		}

		std::string string = aprs_packet_to_string(packet);

		aprs_packet_deinit(packet);

		if (!aprservice_send(service, std::move(string)))
		{
			aprservice_log_error_ex(aprservice_send, false);

			return false;
		}

		return true;
	}

	return false;
}
bool                       APRSERVICE_CALL aprservice_connect(struct aprservice* service, const char* host, uint16_t port, uint16_t passwd)
{
	if (aprservice_is_connected(service))
		return true;

	if (!aprservice_io_connect(service->io, host, port))
	{
		aprservice_log_error_ex(aprservice_io_connect, false);

		return false;
	}

	std::stringstream ss;
	ss << "user " << service->station << " pass " << (passwd ? passwd : -1);
	ss << " vers " << APRSERVICE_SOFTWARE_NAME << ' ' << APRSERVICE_SOFTWARE_VERSION;
	ss << " filter t/poimqstunw";

	service->is_connected     = true;
	service->is_auth_verified = passwd != 0;

	service->auth.state = APRSERVICE_AUTH_STATE_NONE;

	if (!aprservice_send(service, ss.str()))
	{
		aprservice_log_error_ex(aprservice_send, false);

		aprservice_io_disconnect(service->io);

		service->is_connected = false;

		return false;
	}

	service->auth.state = APRSERVICE_AUTH_STATE_SENT;

	aprservice_event_execute(service, APRSERVICE_EVENT_CONNECT, {});

	return true;
}
void                       APRSERVICE_CALL aprservice_disconnect(struct aprservice* service)
{
	if (aprservice_is_connected(service))
	{
		aprservice_io_disconnect(service->io);

		service->message_callbacks.remove_if([service](aprservice_message_callback_context* context) {
			context->callback(service, APRSERVICE_MESSAGE_ERROR_DISCONNECTED, context->callback_param);

			return true;
		});

		service->message_callbacks_index.clear();

		service->auth.state = APRSERVICE_AUTH_STATE_NONE;

		service->is_connected     = false;
		service->is_auth_verified = false;

		aprservice_event_execute(service, APRSERVICE_EVENT_DISCONNECT, {});
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

	aprs_packet_object_set_alive(object->packet, true);
	aprs_packet_object_set_speed(object->packet, speed);
	aprs_packet_object_set_course(object->packet, course);
	aprs_packet_object_set_comment(object->packet, comment);
	aprs_packet_object_set_altitude(object->packet, altitude);
	aprs_packet_object_set_latitude(object->packet, latitude);
	aprs_packet_object_set_longitude(object->packet, longitude);
	aprs_packet_object_set_compressed(object->packet, aprservice_is_compression_enabled(service));

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
	if (!aprservice_send(object->service, aprs_packet_to_string(object->packet)))
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
