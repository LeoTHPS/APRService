require('APRService');
require('APRService.Modules.ByteBuffer');

APRService.Modules.Socket = {};

APRService.Modules.Socket.SOCKET_TYPE_TCP      = APRSERVICE_LUA_MODULE_SOCKET_TYPE_TCP;
APRService.Modules.Socket.SOCKET_TYPE_UDP      = APRSERVICE_LUA_MODULE_SOCKET_TYPE_UDP;
APRService.Modules.Socket.SOCKET_TYPE_UNIX     = APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX;
APRService.Modules.Socket.SOCKET_TYPE_UNIX_TCP = APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_TCP;
APRService.Modules.Socket.SOCKET_TYPE_UNIX_UDP = APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_UDP;

APRService.Modules.Socket.ADDRESS_FAMILY_IPV4          = APRSERVICE_LUA_MODULE_SOCKET_ADDRESS_FAMILY_IPV4;
APRService.Modules.Socket.ADDRESS_FAMILY_IPV6          = APRSERVICE_LUA_MODULE_SOCKET_ADDRESS_FAMILY_IPV6;
APRService.Modules.Socket.ADDRESS_FAMILY_NOT_SPECIFIED = APRSERVICE_LUA_MODULE_SOCKET_ADDRESS_FAMILY_NOT_SPECIFIED;

-- @retrun socket
function APRService.Modules.Socket.OpenTcp(address_family)
	return aprservice_lua_module_socket_open_tcp(address_family);
end

-- @retrun socket
function APRService.Modules.Socket.OpenUdp(address_family)
	return aprservice_lua_module_socket_open_udp(address_family);
end

-- @retrun socket
function APRService.Modules.Socket.OpenUnixTcp(path, address_family)
	return aprservice_lua_module_socket_open_unix_tcp(tostring(path), address_family);
end

-- @retrun socket
function APRService.Modules.Socket.OpenUnixUdp(path, address_family)
	return aprservice_lua_module_socket_open_unix_udp(tostring(path), address_family);
end

function APRService.Modules.Socket.Close(socket)
	aprservice_lua_module_socket_close(socket);
end

function APRService.Modules.Socket.IsBlocking(socket)
	return aprservice_lua_module_socket_is_blocking(socket);
end

function APRService.Modules.Socket.IsConnected(socket)
	return aprservice_lua_module_socket_is_connected(socket);
end

function APRService.Modules.Socket.IsListening(socket)
	return aprservice_lua_module_socket_is_listening(socket);
end

function APRService.Modules.Socket.GetType(socket)
	return aprservice_lua_module_socket_get_type(socket);
end

function APRService.Modules.Socket.GetAddressFamily(socket)
	return aprservice_lua_module_socket_get_address_family(socket);
end

function APRService.Modules.Socket.SetBlocking(socket, value)
	return aprservice_lua_module_socket_set_blocking(socket, value and true or false);
end

function APRService.Modules.Socket.Bind(socket, local_address, local_port)
	return aprservice_lua_module_socket_bind(socket, tostring(local_address), tonumber(local_port));
end

function APRService.Modules.Socket.Listen(socket)
	return aprservice_lua_module_socket_listen(socket);
end

function APRService.Modules.Socket.Connect(socket, remote_host, remote_port)
	return aprservice_lua_module_socket_connect(socket, tostring(remote_host), tonumber(remote_port));
end

-- @return success, would_block, socket
function APRService.Modules.Socket.Accept(socket)
	return aprservice_lua_module_socket_accept(socket);
end

function APRService.Modules.Socket.Send(socket, byte_buffer, byte_buffer_size)
	return aprservice_lua_module_socket_send(socket, byte_buffer, tonumber(byte_buffer_size));
end

function APRService.Modules.Socket.SendTo(socket, byte_buffer, byte_buffer_size, remote_host, remote_port)
	return aprservice_lua_module_socket_send_to(socket, byte_buffer, tonumber(byte_buffer_size), tostring(remote_host), tonumber(remote_port));
end

-- @return connection_closed, would_block, byte_buffer, byte_buffer_size
function APRService.Modules.Socket.Receive(socket, byte_buffer_size, byte_buffer_endian)
	return aprservice_lua_module_socket_receive(socket, tonumber(byte_buffer_size), byte_buffer_endian);
end

-- @return connection_closed, would_block, byte_buffer, byte_buffer_size, remote_address, remote_port
function APRService.Modules.Socket.ReceiveFrom(socket, byte_buffer_size, byte_buffer_endian)
	return aprservice_lua_module_socket_receive_from(socket, tonumber(byte_buffer_size), byte_buffer_endian);
end
