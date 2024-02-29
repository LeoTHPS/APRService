#include "aprservice.hpp"
#include "aprservice_lua.hpp"
#include "aprservice_lua_module_socket.hpp"

#include <AL/Lua54/Lua.hpp>

#include <AL/Network/DNS.hpp>
#include <AL/Network/TcpSocket.hpp>
#include <AL/Network/UdpSocket.hpp>
#include <AL/Network/UnixSocket.hpp>

struct aprservice_lua_module_socket
{
	bool                                             is_blocking;
	bool                                             is_connected;
	bool                                             is_listening;

	AL::BitMask<AL::uint8>                           type;
	AL::uint8                                        address_family;

	AL::Network::TcpSocket*                          tcp;
	AL::Network::UdpSocket*                          udp;
	AL::Network::UnixSocket<AL::Network::TcpSocket>* unix_tcp;
	AL::Network::UnixSocket<AL::Network::UdpSocket>* unix_udp;
};

void                                                                                                       aprservice_lua_module_socket_register_globals(aprservice_lua* lua)
{
	auto lua_state = aprservice_lua_get_state(lua);

	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SOCKET_TYPE_TCP);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SOCKET_TYPE_UDP);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_TCP);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_UDP);

	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SOCKET_ADDRESS_FAMILY_IPV4);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SOCKET_ADDRESS_FAMILY_IPV6);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SOCKET_ADDRESS_FAMILY_NOT_SPECIFIED);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_socket_open_tcp);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_socket_open_udp);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_socket_open_unix_tcp);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_socket_open_unix_udp);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_socket_close);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_socket_is_blocking);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_socket_is_connected);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_socket_is_listening);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_socket_get_type);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_socket_get_address_family);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_socket_set_blocking);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_socket_bind);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_socket_listen);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_socket_connect);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_socket_accept);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_socket_send);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_socket_send_to);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_socket_receive);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_socket_receive_from);
}

aprservice_lua_module_socket*                                                                              aprservice_lua_module_socket_open_tcp(AL::uint8 address_family)
{
	auto socket = new aprservice_lua_module_socket
	{
		.is_connected   = false,
		.is_listening   = false,

		.type           = APRSERVICE_LUA_MODULE_SOCKET_TYPE_TCP,
		.address_family = address_family,

		.tcp            = new AL::Network::TcpSocket(static_cast<AL::Network::AddressFamilies>(address_family))
	};

	socket->is_blocking = socket->tcp->IsBlocking();

	try
	{
		socket->tcp->Open();
	}
	catch (const AL::Exception& exception)
	{
		delete socket->tcp;
		delete socket;

		aprservice_console_write_line("Error opening AL::Network::TcpSocket");
		aprservice_console_write_exception(exception);

		return nullptr;
	}

	return socket;
}
aprservice_lua_module_socket*                                                                              aprservice_lua_module_socket_open_udp(AL::uint8 address_family)
{
	auto socket = new aprservice_lua_module_socket
	{
		.is_connected   = false,
		.is_listening   = false,

		.type           = APRSERVICE_LUA_MODULE_SOCKET_TYPE_UDP,
		.address_family = address_family,

		.udp            = new AL::Network::UdpSocket(static_cast<AL::Network::AddressFamilies>(address_family))
	};

	socket->is_blocking = socket->udp->IsBlocking();

	try
	{
		socket->udp->Open();
	}
	catch (const AL::Exception& exception)
	{
		delete socket->udp;
		delete socket;

		aprservice_console_write_line("Error opening AL::Network::UdpSocket");
		aprservice_console_write_exception(exception);

		return nullptr;
	}

	return socket;
}
aprservice_lua_module_socket*                                                                              aprservice_lua_module_socket_open_unix_tcp(const AL::String& path, AL::uint8 address_family)
{
	auto socket = new aprservice_lua_module_socket
	{
		.is_connected   = false,
		.is_listening   = false,

		.type           = APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_TCP,
		.address_family = address_family,

		.unix_tcp       = new AL::Network::UnixSocket<AL::Network::TcpSocket>(path, static_cast<AL::Network::AddressFamilies>(address_family))
	};

	socket->is_blocking = socket->unix_tcp->IsBlocking();

	try
	{
		socket->unix_tcp->Open();
	}
	catch (const AL::Exception& exception)
	{
		delete socket->unix_tcp;
		delete socket;

		aprservice_console_write_line("Error opening AL::Network::UnixSocket<TcpSocket>");
		aprservice_console_write_exception(exception);

		return nullptr;
	}

	return socket;
}
aprservice_lua_module_socket*                                                                              aprservice_lua_module_socket_open_unix_udp(const AL::String& path, AL::uint8 address_family)
{
	auto socket = new aprservice_lua_module_socket
	{
		.is_connected   = false,
		.is_listening   = false,

		.type           = APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_UDP,
		.address_family = address_family,

		.unix_udp       = new AL::Network::UnixSocket<AL::Network::UdpSocket>(path, static_cast<AL::Network::AddressFamilies>(address_family))
	};

	socket->is_blocking = socket->unix_udp->IsBlocking();

	try
	{
		socket->unix_udp->Open();
	}
	catch (const AL::Exception& exception)
	{
		delete socket->unix_udp;
		delete socket;

		aprservice_console_write_line("Error opening AL::Network::UnixSocket<UdpSocket>");
		aprservice_console_write_exception(exception);

		return nullptr;
	}

	return socket;
}
void                                                                                                       aprservice_lua_module_socket_close(aprservice_lua_module_socket* socket)
{
	switch (aprservice_lua_module_socket_get_type(socket))
	{
		case APRSERVICE_LUA_MODULE_SOCKET_TYPE_TCP:
			socket->tcp->Close();
			delete socket->tcp;
			break;

		case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UDP:
			socket->udp->Close();
			delete socket->udp;
			break;

		case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_TCP:
			socket->unix_tcp->Close();
			delete socket->unix_tcp;
			break;

		case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_UDP:
			socket->unix_udp->Close();
			delete socket->unix_udp;
			break;
	}

	delete socket;
}

bool                                                                                                       aprservice_lua_module_socket_is_blocking(aprservice_lua_module_socket* socket)
{
	return socket->is_blocking;
}
bool                                                                                                       aprservice_lua_module_socket_is_connected(aprservice_lua_module_socket* socket)
{
	return socket->is_connected;
}
bool                                                                                                       aprservice_lua_module_socket_is_listening(aprservice_lua_module_socket* socket)
{
	return socket->is_listening;
}

AL::uint8                                                                                                  aprservice_lua_module_socket_get_type(aprservice_lua_module_socket* socket)
{
	return socket->type.Value;
}
AL::uint8                                                                                                  aprservice_lua_module_socket_get_address_family(aprservice_lua_module_socket* socket)
{
	return socket->address_family;
}

bool                                                                                                       aprservice_lua_module_socket_set_blocking(aprservice_lua_module_socket* socket, bool set)
{
	try
	{
		switch (aprservice_lua_module_socket_get_type(socket))
		{
			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_TCP:
				socket->tcp->SetBlocking(set);
				break;

			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UDP:
				socket->udp->SetBlocking(set);
				break;

			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_TCP:
				socket->unix_tcp->SetBlocking(set);
				break;

			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_UDP:
				socket->unix_udp->SetBlocking(set);
				break;
		}
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error setting blocking mode");
		aprservice_console_write_exception(exception);

		return false;
	}

	socket->is_blocking = set;

	return true;
}

bool                                                                                                       aprservice_lua_module_socket_bind(aprservice_lua_module_socket* socket, const AL::String& local_address, AL::uint16 local_port)
{
	AL::Network::IPEndPoint local_ep =
	{
		.Port = local_port
	};

	try
	{
		if (!AL::Network::DNS::Resolve(local_ep.Host, local_address))
			throw AL::Exception("Address not found");
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error resolving address");
		aprservice_console_write_exception(exception);

		return false;
	}

	try
	{
		switch (aprservice_lua_module_socket_get_type(socket))
		{
			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_TCP:
				socket->tcp->Bind(local_ep);
				break;

			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UDP:
				socket->udp->Bind(local_ep);
				break;

			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_TCP:
			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_UDP:
				throw AL::Exception("Invalid socket type");
		}
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error binding socket");
		aprservice_console_write_exception(exception);

		return false;
	}

	return true;
}
bool                                                                                                       aprservice_lua_module_socket_listen(aprservice_lua_module_socket* socket, AL::size_t backlog)
{
	try
	{
		switch (aprservice_lua_module_socket_get_type(socket))
		{
			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_TCP:
				socket->tcp->Listen(backlog);
				break;

			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UDP:
			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_TCP:
			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_UDP:
				throw AL::Exception("Invalid socket type");
		}

		socket->is_listening = true;
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error listening to socket");
		aprservice_console_write_exception(exception);

		return false;
	}

	return true;
}
bool                                                                                                       aprservice_lua_module_socket_connect(aprservice_lua_module_socket* socket, const AL::String& remote_host, AL::uint16 remote_port)
{
	AL::Network::IPEndPoint remote_ep =
	{
		.Port = remote_port
	};

	try
	{
		if (!AL::Network::DNS::Resolve(remote_ep.Host, remote_host))
			throw AL::Exception("Host not found");
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error resolving remote address");
		aprservice_console_write_exception(exception);

		return false;
	}

	try
	{
		switch (aprservice_lua_module_socket_get_type(socket))
		{
			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_TCP:
				return socket->is_connected = socket->tcp->Connect(remote_ep);

			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UDP:
			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_TCP:
			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_UDP:
				throw AL::Exception("Invalid socket type");
		}
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error connecting socket");
		aprservice_console_write_exception(exception);

		return false;
	}

	return true;
}

// @return success, would_block, socket
AL::Collections::Tuple<bool, bool, aprservice_lua_module_socket*>                                          aprservice_lua_module_socket_accept(aprservice_lua_module_socket* socket)
{
	AL::Collections::Tuple<bool, bool, aprservice_lua_module_socket*> value(false, false, nullptr);

	AL::Network::TcpSocket tcp_socket(socket->tcp->GetAddressFamily());

	try
	{
		switch (aprservice_lua_module_socket_get_type(socket))
		{
			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_TCP:
				value.Set<1>(!socket->tcp->Accept(tcp_socket));
				break;

			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UDP:
			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_TCP:
			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_UDP:
				throw AL::Exception("Invalid socket type");
		}

		value.Set<0>(true);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error accepting socket");
		aprservice_console_write_exception(exception);
	}

	if (value.Get<0>() && !value.Get<2>())
		value.Set<2>(new aprservice_lua_module_socket { .is_connected = true, .is_listening = false, .type = socket->type, .address_family = socket->address_family, .tcp = new AL::Network::TcpSocket(AL::Move(tcp_socket)) });

	return value;
}

bool                                                                                                       aprservice_lua_module_socket_send(aprservice_lua_module_socket* socket, aprservice_lua_module_byte_buffer* byte_buffer, AL::size_t byte_buffer_size)
{
	auto buffer = reinterpret_cast<const AL::uint8*>(aprservice_lua_module_byte_buffer_get_buffer(byte_buffer));

	try
	{
		for (AL::size_t total_bytes_sent = 0, number_of_bytes_sent; total_bytes_sent < byte_buffer_size; total_bytes_sent += number_of_bytes_sent)
		{
			switch (aprservice_lua_module_socket_get_type(socket))
			{
				case APRSERVICE_LUA_MODULE_SOCKET_TYPE_TCP:
					return socket->tcp->Send(&buffer[total_bytes_sent], byte_buffer_size - total_bytes_sent, number_of_bytes_sent);

				case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UDP:
					throw AL::Exception("Invalid socket type");

				case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_TCP:
					return socket->unix_tcp->Send(&buffer[total_bytes_sent], byte_buffer_size - total_bytes_sent, number_of_bytes_sent);

				case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_UDP:
					return socket->unix_udp->Send(&buffer[total_bytes_sent], byte_buffer_size - total_bytes_sent, number_of_bytes_sent);
			}
		}
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error writing socket");
		aprservice_console_write_exception(exception);

		return false;
	}

	return true;
}
// @return number of bytes sent
AL::size_t                                                                                                 aprservice_lua_module_socket_send_to(aprservice_lua_module_socket* socket, aprservice_lua_module_byte_buffer* byte_buffer, AL::size_t byte_buffer_size, const AL::String& remote_host, AL::uint16 remote_port)
{
	AL::Network::IPEndPoint remote_ep =
	{
		.Port = remote_port
	};

	try
	{
		if (!AL::Network::DNS::Resolve(remote_ep.Host, remote_host))
			throw AL::Exception("Remote host not found");
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error resolving remote address");
		aprservice_console_write_exception(exception);

		return 0;
	}

	auto buffer = reinterpret_cast<const AL::uint8*>(aprservice_lua_module_byte_buffer_get_buffer(byte_buffer));

	try
	{
		for (AL::size_t total_bytes_sent = 0, number_of_bytes_sent; total_bytes_sent < byte_buffer_size; total_bytes_sent += number_of_bytes_sent)
		{
			switch (aprservice_lua_module_socket_get_type(socket))
			{
				case APRSERVICE_LUA_MODULE_SOCKET_TYPE_TCP:
				case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_TCP:
				case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_UDP:
					throw AL::Exception("Invalid socket type");

				case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UDP:
					return socket->udp->Send(&buffer[total_bytes_sent], byte_buffer_size - total_bytes_sent, remote_ep);
			}
		}
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error writing socket");
		aprservice_console_write_exception(exception);
	}

	return 0;
}

// @return connection_closed, would_block, byte_buffer, byte_buffer_size
AL::Collections::Tuple<bool, bool, aprservice_lua_module_byte_buffer*, AL::size_t>                         aprservice_lua_module_socket_receive(aprservice_lua_module_socket* socket, AL::size_t byte_buffer_size, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN byte_buffer_endian)
{
	AL::Collections::Tuple<bool, bool, aprservice_lua_module_byte_buffer*, AL::size_t> value(false, false, aprservice_lua_module_byte_buffer_create(byte_buffer_endian, byte_buffer_size), 0);

	try
	{
		switch (aprservice_lua_module_socket_get_type(socket))
		{
			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_TCP:
				value.Set<0>(socket->tcp->Receive(const_cast<void*>(aprservice_lua_module_byte_buffer_get_buffer(value.Get<2>())), byte_buffer_size, value.Get<3>()));
				break;

			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UDP:
				throw AL::Exception("Invalid socket type");

			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_TCP:
				value.Set<0>(socket->unix_tcp->Receive(const_cast<void*>(aprservice_lua_module_byte_buffer_get_buffer(value.Get<2>())), byte_buffer_size, value.Get<3>()));
				break;

			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_UDP:
				value.Set<0>(socket->unix_udp->Receive(const_cast<void*>(aprservice_lua_module_byte_buffer_get_buffer(value.Get<2>())), byte_buffer_size, value.Get<3>()));
				break;
		}

		value.Set<1>(value.Get<3>() == 0);
	}
	catch (const AL::Exception& exception)
	{
		value.Set<0>(true);

		aprservice_lua_module_byte_buffer_destroy(value.Get<2>());

		aprservice_console_write_line("Error reading socket");
		aprservice_console_write_exception(exception);
	}

	return value;
}
// @return connection_closed, would_block, byte_buffer, byte_buffer_size, remote_address, remote_port
AL::Collections::Tuple<bool, bool, aprservice_lua_module_byte_buffer*, AL::size_t, AL::String, AL::uint16> aprservice_lua_module_socket_receive_from(aprservice_lua_module_socket* socket, AL::size_t byte_buffer_size, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN byte_buffer_endian)
{
	AL::Collections::Tuple<bool, bool, aprservice_lua_module_byte_buffer*, AL::size_t, AL::String, AL::uint16> value(false, false, aprservice_lua_module_byte_buffer_create(byte_buffer_endian, byte_buffer_size), 0, "", 0);

	AL::Network::IPEndPoint remote_ep;

	try
	{
		switch (aprservice_lua_module_socket_get_type(socket))
		{
			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_TCP:
			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_TCP:
			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_UDP:
				throw AL::Exception("Invalid socket type");

			case APRSERVICE_LUA_MODULE_SOCKET_TYPE_UDP:
				value.Set<3>(socket->udp->Receive(const_cast<void*>(aprservice_lua_module_byte_buffer_get_buffer(value.Get<2>())), byte_buffer_size, remote_ep));
		}

		value.Set<1>(value.Get<3>() == 0);
		value.Set<4>(remote_ep.Host.ToString());
		value.Set<5>(remote_ep.Port);
		value.Set<0>(false);
	}
	catch (const AL::Exception& exception)
	{
		value.Set<0>(true);

		aprservice_lua_module_byte_buffer_destroy(value.Get<2>());

		aprservice_console_write_line("Error reading socket");
		aprservice_console_write_exception(exception);
	}

	return value;
}
