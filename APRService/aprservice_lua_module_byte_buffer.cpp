#include "aprservice.hpp"
#include "aprservice_lua.hpp"
#include "aprservice_lua_module_byte_buffer.hpp"

#include <AL/Lua54/Lua.hpp>

#include <AL/Collections/ByteBuffer.hpp>

typedef AL::Collections::ByteBuffer<AL::Endians::Big>    aprservice_lua_module_byte_buffer_big;
typedef AL::Collections::ByteBuffer<AL::Endians::Little> aprservice_lua_module_byte_buffer_little;

struct aprservice_lua_module_byte_buffer
{
	APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN  endian;

	aprservice_lua_module_byte_buffer_big*    big;
	aprservice_lua_module_byte_buffer_little* little;
};

void                                                                             aprservice_lua_module_byte_buffer_register_globals(aprservice_lua* lua)
{
	auto lua_state = aprservice_lua_get_state(lua);

	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_MACHINE);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_create);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_destroy);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_get_size);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_get_buffer);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_get_endian);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_get_capacity);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_get_read_position);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_set_read_position);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_get_write_position);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_set_write_position);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_clear);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_read);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_read_int8);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_read_int16);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_read_int32);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_read_int64);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_read_uint8);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_read_uint16);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_read_uint32);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_read_uint64);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_read_float);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_read_double);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_read_string);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_read_boolean);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_write);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_write_int8);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_write_int16);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_write_int32);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_write_int64);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_write_uint8);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_write_uint16);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_write_uint32);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_write_uint64);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_write_float);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_write_double);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_write_string);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_byte_buffer_write_boolean);
}

aprservice_lua_module_byte_buffer*                                               aprservice_lua_module_byte_buffer_create(APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN endian, AL::size_t capacity)
{
	auto byte_buffer = new aprservice_lua_module_byte_buffer
	{
		.endian = endian
	};

	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			byte_buffer->big = new aprservice_lua_module_byte_buffer_big(capacity);
			break;

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			byte_buffer->little = new aprservice_lua_module_byte_buffer_little(capacity);
			break;
	}

	return byte_buffer;
}
void                                                                             aprservice_lua_module_byte_buffer_destroy(aprservice_lua_module_byte_buffer* byte_buffer)
{
	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			delete byte_buffer->big;
			break;

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			delete byte_buffer->little;
			break;
	}

	delete byte_buffer;
}
AL::size_t                                                                       aprservice_lua_module_byte_buffer_get_size(aprservice_lua_module_byte_buffer* byte_buffer)
{
	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			return byte_buffer->big->GetWritePosition() + 1;

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			return byte_buffer->little->GetWritePosition() + 1;
	}

	return 0;
}
const void*                                                                      aprservice_lua_module_byte_buffer_get_buffer(aprservice_lua_module_byte_buffer* byte_buffer)
{
	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			return byte_buffer->big->GetBuffer();

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			return byte_buffer->little->GetBuffer();
	}

	return nullptr;
}
AL::size_t                                                                       aprservice_lua_module_byte_buffer_get_endian(aprservice_lua_module_byte_buffer* byte_buffer)
{
	return byte_buffer->endian;
}
AL::size_t                                                                       aprservice_lua_module_byte_buffer_get_capacity(aprservice_lua_module_byte_buffer* byte_buffer)
{
	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			return byte_buffer->big->GetCapacity();

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			return byte_buffer->little->GetCapacity();
	}

	return 0;
}

AL::size_t                                                                       aprservice_lua_module_byte_buffer_get_read_position(aprservice_lua_module_byte_buffer* byte_buffer)
{
	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			return byte_buffer->big->GetReadPosition() + 1;

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			return byte_buffer->little->GetReadPosition() + 1;
	}

	return 0;
}
void                                                                             aprservice_lua_module_byte_buffer_set_read_position(aprservice_lua_module_byte_buffer* byte_buffer, AL::size_t value)
{
	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			byte_buffer->big->SetReadPosition(value - 1);

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			byte_buffer->little->SetReadPosition(value - 1);
	}
}

AL::size_t                                                                       aprservice_lua_module_byte_buffer_get_write_position(aprservice_lua_module_byte_buffer* byte_buffer)
{
	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			return byte_buffer->big->GetWritePosition() + 1;

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			return byte_buffer->little->GetWritePosition() + 1;
	}

	return 0;
}
void                                                                             aprservice_lua_module_byte_buffer_set_write_position(aprservice_lua_module_byte_buffer* byte_buffer, AL::size_t value)
{
	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			byte_buffer->big->SetWritePosition(value - 1);
			break;

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			byte_buffer->little->SetWritePosition(value - 1);
			break;
	}
}

void                                                                             aprservice_lua_module_byte_buffer_clear(aprservice_lua_module_byte_buffer* byte_buffer)
{
	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			return byte_buffer->big->Clear();

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			return byte_buffer->little->Clear();
	}
}

// @return success, byte_buffer
AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer*>                 aprservice_lua_module_byte_buffer_read(aprservice_lua_module_byte_buffer* byte_buffer, AL::size_t size)
{
	AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer*> value(false, aprservice_lua_module_byte_buffer_create(byte_buffer->endian, size));

	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			value.Set<0>(byte_buffer->big->Read(const_cast<void*>(value.Get<1>()->big->GetBuffer()), size));
			break;

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			value.Set<0>(byte_buffer->little->Read(const_cast<void*>(value.Get<1>()->little->GetBuffer()), size));
			break;
	}

	if (!value.Get<0>())
		delete value.Get<1>();

	return value;
}
// @return success, value
AL::Collections::Tuple<bool, AL::int8>                                           aprservice_lua_module_byte_buffer_read_int8(aprservice_lua_module_byte_buffer* byte_buffer)
{
	AL::Collections::Tuple<bool, AL::int8> value(false, 0);

	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			value.Set<0>(byte_buffer->big->ReadInt8(value.Get<1>()));
			break;

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			value.Set<0>(byte_buffer->little->ReadInt8(value.Get<1>()));
			break;
	}

	return value;
}
// @return success, value
AL::Collections::Tuple<bool, AL::int16>                                          aprservice_lua_module_byte_buffer_read_int16(aprservice_lua_module_byte_buffer* byte_buffer)
{
	AL::Collections::Tuple<bool, AL::int16> value(false, 0);

	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			value.Set<0>(byte_buffer->big->ReadInt16(value.Get<1>()));
			break;

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			value.Set<0>(byte_buffer->little->ReadInt16(value.Get<1>()));
			break;
	}

	return value;
}
// @return success, value
AL::Collections::Tuple<bool, AL::int32>                                          aprservice_lua_module_byte_buffer_read_int32(aprservice_lua_module_byte_buffer* byte_buffer)
{
	AL::Collections::Tuple<bool, AL::int32> value(false, 0);

	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			value.Set<0>(byte_buffer->big->ReadInt32(value.Get<1>()));
			break;

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			value.Set<0>(byte_buffer->little->ReadInt32(value.Get<1>()));
			break;
	}

	return value;
}
// @return success, value
AL::Collections::Tuple<bool, AL::int64>                                          aprservice_lua_module_byte_buffer_read_int64(aprservice_lua_module_byte_buffer* byte_buffer)
{
	AL::Collections::Tuple<bool, AL::int64> value(false, 0);

	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			value.Set<0>(byte_buffer->big->ReadInt64(value.Get<1>()));
			break;

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			value.Set<0>(byte_buffer->little->ReadInt64(value.Get<1>()));
			break;
	}

	return value;
}
// @return success, value
AL::Collections::Tuple<bool, AL::uint8>                                          aprservice_lua_module_byte_buffer_read_uint8(aprservice_lua_module_byte_buffer* byte_buffer)
{
	AL::Collections::Tuple<bool, AL::uint8> value(false, 0);

	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			value.Set<0>(byte_buffer->big->ReadUInt8(value.Get<1>()));
			break;

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			value.Set<0>(byte_buffer->little->ReadUInt8(value.Get<1>()));
			break;
	}

	return value;
}
// @return success, value
AL::Collections::Tuple<bool, AL::uint16>                                         aprservice_lua_module_byte_buffer_read_uint16(aprservice_lua_module_byte_buffer* byte_buffer)
{
	AL::Collections::Tuple<bool, AL::uint16> value(false, 0);

	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			value.Set<0>(byte_buffer->big->ReadUInt16(value.Get<1>()));
			break;

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			value.Set<0>(byte_buffer->little->ReadUInt16(value.Get<1>()));
			break;
	}

	return value;
}
// @return success, value
AL::Collections::Tuple<bool, AL::uint32>                                         aprservice_lua_module_byte_buffer_read_uint32(aprservice_lua_module_byte_buffer* byte_buffer)
{
	AL::Collections::Tuple<bool, AL::uint32> value(false, 0);

	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			value.Set<0>(byte_buffer->big->ReadUInt32(value.Get<1>()));
			break;

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			value.Set<0>(byte_buffer->little->ReadUInt32(value.Get<1>()));
			break;
	}

	return value;
}
// @return success, value
AL::Collections::Tuple<bool, AL::uint64>                                         aprservice_lua_module_byte_buffer_read_uint64(aprservice_lua_module_byte_buffer* byte_buffer)
{
	AL::Collections::Tuple<bool, AL::uint64> value(false, 0);

	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			value.Set<0>(byte_buffer->big->ReadUInt64(value.Get<1>()));
			break;

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			value.Set<0>(byte_buffer->little->ReadUInt64(value.Get<1>()));
			break;
	}

	return value;
}
// @return success, value
AL::Collections::Tuple<bool, AL::Float>                                          aprservice_lua_module_byte_buffer_read_float(aprservice_lua_module_byte_buffer* byte_buffer)
{
	AL::Collections::Tuple<bool, AL::Float> value(false, 0);

	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			value.Set<0>(byte_buffer->big->ReadFloat(value.Get<1>()));
			break;

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			value.Set<0>(byte_buffer->little->ReadFloat(value.Get<1>()));
			break;
	}

	return value;
}
// @return success, value
AL::Collections::Tuple<bool, AL::Double>                                         aprservice_lua_module_byte_buffer_read_double(aprservice_lua_module_byte_buffer* byte_buffer)
{
	AL::Collections::Tuple<bool, AL::Double> value(false, 0);

	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			value.Set<0>(byte_buffer->big->ReadDouble(value.Get<1>()));
			break;

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			value.Set<0>(byte_buffer->little->ReadDouble(value.Get<1>()));
			break;
	}

	return value;
}
// @return success, value
AL::Collections::Tuple<bool, AL::String>                                         aprservice_lua_module_byte_buffer_read_string(aprservice_lua_module_byte_buffer* byte_buffer)
{
	AL::Collections::Tuple<bool, AL::String> value(false, "");

	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			value.Set<0>(byte_buffer->big->ReadString(value.Get<1>()));
			break;

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			value.Set<0>(byte_buffer->little->ReadString(value.Get<1>()));
			break;
	}

	return value;
}
// @return success, value
AL::Collections::Tuple<bool, bool>                                               aprservice_lua_module_byte_buffer_read_boolean(aprservice_lua_module_byte_buffer* byte_buffer)
{
	AL::Collections::Tuple<bool, bool> value(false, false);

	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			value.Set<0>(byte_buffer->big->ReadBool(value.Get<1>()));
			break;

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			value.Set<0>(byte_buffer->little->ReadBool(value.Get<1>()));
			break;
	}

	return value;
}

bool                                                                             aprservice_lua_module_byte_buffer_write(aprservice_lua_module_byte_buffer* byte_buffer, const void* buffer, AL::size_t size)
{
	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			return byte_buffer->big->Write(buffer, size);

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			return byte_buffer->little->Write(buffer, size);
	}

	return false;
}
bool                                                                             aprservice_lua_module_byte_buffer_write_int8(aprservice_lua_module_byte_buffer* byte_buffer, AL::int8 value)
{
	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			return byte_buffer->big->WriteInt8(value);

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			return byte_buffer->little->WriteInt8(value);
	}

	return false;
}
bool                                                                             aprservice_lua_module_byte_buffer_write_int16(aprservice_lua_module_byte_buffer* byte_buffer, AL::int16 value)
{
	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			return byte_buffer->big->WriteInt16(value);

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			return byte_buffer->little->WriteInt16(value);
	}

	return false;
}
bool                                                                             aprservice_lua_module_byte_buffer_write_int32(aprservice_lua_module_byte_buffer* byte_buffer, AL::int32 value)
{
	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			return byte_buffer->big->WriteInt32(value);

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			return byte_buffer->little->WriteInt32(value);
	}

	return false;
}
bool                                                                             aprservice_lua_module_byte_buffer_write_int64(aprservice_lua_module_byte_buffer* byte_buffer, AL::int64 value)
{
	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			return byte_buffer->big->WriteInt64(value);

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			return byte_buffer->little->WriteInt64(value);
	}

	return false;
}
bool                                                                             aprservice_lua_module_byte_buffer_write_uint8(aprservice_lua_module_byte_buffer* byte_buffer, AL::uint16 value)
{
	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			return byte_buffer->big->WriteUInt8(value);

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			return byte_buffer->little->WriteUInt8(value);
	}

	return false;
}
bool                                                                             aprservice_lua_module_byte_buffer_write_uint16(aprservice_lua_module_byte_buffer* byte_buffer, AL::uint16 value)
{
	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			return byte_buffer->big->WriteUInt16(value);

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			return byte_buffer->little->WriteUInt16(value);
	}

	return false;
}
bool                                                                             aprservice_lua_module_byte_buffer_write_uint32(aprservice_lua_module_byte_buffer* byte_buffer, AL::uint32 value)
{
	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			return byte_buffer->big->WriteUInt32(value);

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			return byte_buffer->little->WriteUInt32(value);
	}

	return false;
}
bool                                                                             aprservice_lua_module_byte_buffer_write_uint64(aprservice_lua_module_byte_buffer* byte_buffer, AL::uint64 value)
{
	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			return byte_buffer->big->WriteUInt64(value);

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			return byte_buffer->little->WriteUInt64(value);
	}

	return false;
}
bool                                                                             aprservice_lua_module_byte_buffer_write_float(aprservice_lua_module_byte_buffer* byte_buffer, AL::Float value)
{
	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			return byte_buffer->big->WriteFloat(value);

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			return byte_buffer->little->WriteFloat(value);
	}

	return false;
}
bool                                                                             aprservice_lua_module_byte_buffer_write_double(aprservice_lua_module_byte_buffer* byte_buffer, AL::Double value)
{
	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			return byte_buffer->big->WriteDouble(value);

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			return byte_buffer->little->WriteDouble(value);
	}

	return false;
}
bool                                                                             aprservice_lua_module_byte_buffer_write_string(aprservice_lua_module_byte_buffer* byte_buffer, const AL::String& value)
{
	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			return byte_buffer->big->WriteString(value);

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			return byte_buffer->little->WriteString(value);
	}

	return false;
}
bool                                                                             aprservice_lua_module_byte_buffer_write_boolean(aprservice_lua_module_byte_buffer* byte_buffer, bool value)
{
	switch (byte_buffer->endian)
	{
		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG:
			return byte_buffer->big->WriteBool(value);

		case APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE:
			return byte_buffer->little->WriteBool(value);
	}

	return false;
}
