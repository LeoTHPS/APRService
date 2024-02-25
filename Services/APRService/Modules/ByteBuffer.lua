require('APRService');

APRService.Modules.ByteBuffer = {};

APRService.Modules.ByteBuffer.ENDIAN_BIG     = APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG;
APRService.Modules.ByteBuffer.ENDIAN_LITTLE  = APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE;
APRService.Modules.ByteBuffer.ENDIAN_MACHINE = APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_MACHINE;

-- @return byte_buffer
function APRService.Modules.ByteBuffer.Create(endian, capacity)
	return aprservice_lua_module_byte_buffer_create(endian, tonumber(capacity));
end

function APRService.Modules.ByteBuffer.Destroy(byte_buffer)
	aprservice_lua_module_byte_buffer_destroy(byte_buffer);
end

function APRService.Modules.ByteBuffer.GetSize(byte_buffer)
	return aprservice_lua_module_byte_buffer_get_size(byte_buffer);
end

function APRService.Modules.ByteBuffer.GetBuffer(byte_buffer)
	return aprservice_lua_module_byte_buffer_get_buffer(byte_buffer);
end

function APRService.Modules.ByteBuffer.GetEndian(byte_buffer)
	return aprservice_lua_module_byte_buffer_get_endian(byte_buffer);
end

function APRService.Modules.ByteBuffer.GetCapacity(byte_buffer)
	return aprservice_lua_module_byte_buffer_get_capacity(byte_buffer);
end

function APRService.Modules.ByteBuffer.GetReadPosition(byte_buffer)
	return aprservice_lua_module_byte_buffer_get_read_position(byte_buffer);
end

function APRService.Modules.ByteBuffer.SetReadPosition(byte_buffer, value)
	aprservice_lua_module_byte_buffer_set_read_position(byte_buffer, tonumber(value));
end

function APRService.Modules.ByteBuffer.GetWritePosition(byte_buffer)
	return aprservice_lua_module_byte_buffer_get_write_position(byte_buffer);
end

function APRService.Modules.ByteBuffer.SetWritePosition(byte_buffer, value)
	aprservice_lua_module_byte_buffer_set_write_position(byte_buffer, tonumber(value));
end

function APRService.Modules.ByteBuffer.Clear(byte_buffer)
	aprservice_lua_module_byte_buffer_clear(byte_buffer);
end

-- @return success, byte_buffer
function APRService.Modules.ByteBuffer.Read(byte_buffer, size)
	return aprservice_lua_module_byte_buffer_read(byte_buffer, tonumber(size));
end

-- @return success, value
function APRService.Modules.ByteBuffer.ReadInt8(byte_buffer)
	return aprservice_lua_module_byte_buffer_read_int8(byte_buffer);
end

-- @return success, value
function APRService.Modules.ByteBuffer.ReadInt16(byte_buffer)
	return aprservice_lua_module_byte_buffer_read_int16(byte_buffer);
end

-- @return success, value
function APRService.Modules.ByteBuffer.ReadInt32(byte_buffer)
	return aprservice_lua_module_byte_buffer_read_int32(byte_buffer);
end

-- @return success, value
function APRService.Modules.ByteBuffer.ReadInt64(byte_buffer)
	return aprservice_lua_module_byte_buffer_read_int64(byte_buffer);
end

-- @return success, value
function APRService.Modules.ByteBuffer.ReadUInt8(byte_buffer)
	return aprservice_lua_module_byte_buffer_read_uint8(byte_buffer);
end

-- @return success, value
function APRService.Modules.ByteBuffer.ReadUInt16(byte_buffer)
	return aprservice_lua_module_byte_buffer_read_uint16(byte_buffer);
end

-- @return success, value
function APRService.Modules.ByteBuffer.ReadUInt32(byte_buffer)
	return aprservice_lua_module_byte_buffer_read_uint32(byte_buffer);
end

-- @return success, value
function APRService.Modules.ByteBuffer.ReadUInt64(byte_buffer)
	return aprservice_lua_module_byte_buffer_read_uint64(byte_buffer);
end

-- @return success, value
function APRService.Modules.ByteBuffer.ReadFloat(byte_buffer)
	return aprservice_lua_module_byte_buffer_read_float(byte_buffer);
end

-- @return success, value
function APRService.Modules.ByteBuffer.ReadDouble(byte_buffer)
	return aprservice_lua_module_byte_buffer_read_double(byte_buffer);
end

-- @return success, value
function APRService.Modules.ByteBuffer.ReadString(byte_buffer)
	return aprservice_lua_module_byte_buffer_read_string(byte_buffer);
end

-- @return success, value
function APRService.Modules.ByteBuffer.ReadBoolean(byte_buffer)
	return aprservice_lua_module_byte_buffer_read_boolean(byte_buffer);
end

function APRService.Modules.ByteBuffer.Write(byte_buffer, buffer, size)
	return aprservice_lua_module_byte_buffer_write(byte_buffer, buffer, size);
end

function APRService.Modules.ByteBuffer.WriteInt8(byte_buffer, value)
	return aprservice_lua_module_byte_buffer_write_int8(byte_buffer, tonumber(value));
end

function APRService.Modules.ByteBuffer.WriteInt16(byte_buffer, value)
	return aprservice_lua_module_byte_buffer_write_int16(byte_buffer, tonumber(value));
end

function APRService.Modules.ByteBuffer.WriteInt32(byte_buffer, value)
	return aprservice_lua_module_byte_buffer_write_int32(byte_buffer, tonumber(value));
end

function APRService.Modules.ByteBuffer.WriteInt64(byte_buffer, value)
	return aprservice_lua_module_byte_buffer_write_int64(byte_buffer, tonumber(value));
end

function APRService.Modules.ByteBuffer.WriteUInt8(byte_buffer, value)
	return aprservice_lua_module_byte_buffer_write_uint8(byte_buffer, tonumber(value));
end

function APRService.Modules.ByteBuffer.WriteUInt16(byte_buffer, value)
	return aprservice_lua_module_byte_buffer_write_uint16(byte_buffer, tonumber(value));
end

function APRService.Modules.ByteBuffer.WriteUInt32(byte_buffer, value)
	return aprservice_lua_module_byte_buffer_write_uint32(byte_buffer, tonumber(value));
end

function APRService.Modules.ByteBuffer.WriteUInt64(byte_buffer, value)
	return aprservice_lua_module_byte_buffer_write_uint64(byte_buffer, tonumber(value));
end

function APRService.Modules.ByteBuffer.WriteFloat(byte_buffer, value)
	return aprservice_lua_module_byte_buffer_write_float(byte_buffer, tonumber(value));
end

function APRService.Modules.ByteBuffer.WriteDouble(byte_buffer, value)
	return aprservice_lua_module_byte_buffer_write_double(byte_buffer, tonumber(value));
end

function APRService.Modules.ByteBuffer.WriteString(byte_buffer, value)
	return aprservice_lua_module_byte_buffer_write_string(byte_buffer, tostring(value));
end

function APRService.Modules.ByteBuffer.WriteBoolean(byte_buffer, value)
	return aprservice_lua_module_byte_buffer_write_boolean(byte_buffer, value and true or false);
end
