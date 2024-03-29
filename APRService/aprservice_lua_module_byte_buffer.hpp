#pragma once
#include <AL/Common.hpp>

#include <AL/Collections/Tuple.hpp>

typedef typename AL::Get_Enum_Or_Integer_Base<AL::Endians>::Type APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN;

enum APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIANS : APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN
{
	APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_BIG     = static_cast<APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN>(AL::Endians::Big),
	APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_LITTLE  = static_cast<APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN>(AL::Endians::Little),
	APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_MACHINE = static_cast<APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN>(AL::Endians::Machine)
};

struct aprservice_lua;
struct aprservice_lua_module_byte_buffer;

void                                                                             aprservice_lua_module_byte_buffer_register_globals(aprservice_lua* lua);

aprservice_lua_module_byte_buffer*                                               aprservice_lua_module_byte_buffer_create(APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN endian, AL::size_t capacity);
void                                                                             aprservice_lua_module_byte_buffer_destroy(aprservice_lua_module_byte_buffer* byte_buffer);
AL::size_t                                                                       aprservice_lua_module_byte_buffer_get_size(aprservice_lua_module_byte_buffer* byte_buffer);
const void*                                                                      aprservice_lua_module_byte_buffer_get_buffer(aprservice_lua_module_byte_buffer* byte_buffer);
AL::size_t                                                                       aprservice_lua_module_byte_buffer_get_endian(aprservice_lua_module_byte_buffer* byte_buffer);
AL::size_t                                                                       aprservice_lua_module_byte_buffer_get_capacity(aprservice_lua_module_byte_buffer* byte_buffer);

AL::size_t                                                                       aprservice_lua_module_byte_buffer_get_read_position(aprservice_lua_module_byte_buffer* byte_buffer);
void                                                                             aprservice_lua_module_byte_buffer_set_read_position(aprservice_lua_module_byte_buffer* byte_buffer, AL::size_t value);

AL::size_t                                                                       aprservice_lua_module_byte_buffer_get_write_position(aprservice_lua_module_byte_buffer* byte_buffer);
void                                                                             aprservice_lua_module_byte_buffer_set_write_position(aprservice_lua_module_byte_buffer* byte_buffer, AL::size_t value);

void                                                                             aprservice_lua_module_byte_buffer_clear(aprservice_lua_module_byte_buffer* byte_buffer);

// @return success, byte_buffer
AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer*>                 aprservice_lua_module_byte_buffer_read(aprservice_lua_module_byte_buffer* byte_buffer, AL::size_t size);
// @return success, value
AL::Collections::Tuple<bool, AL::int8>                                           aprservice_lua_module_byte_buffer_read_int8(aprservice_lua_module_byte_buffer* byte_buffer);
// @return success, value
AL::Collections::Tuple<bool, AL::int16>                                          aprservice_lua_module_byte_buffer_read_int16(aprservice_lua_module_byte_buffer* byte_buffer);
// @return success, value
AL::Collections::Tuple<bool, AL::int32>                                          aprservice_lua_module_byte_buffer_read_int32(aprservice_lua_module_byte_buffer* byte_buffer);
// @return success, value
AL::Collections::Tuple<bool, AL::int64>                                          aprservice_lua_module_byte_buffer_read_int64(aprservice_lua_module_byte_buffer* byte_buffer);
// @return success, value
AL::Collections::Tuple<bool, AL::uint8>                                          aprservice_lua_module_byte_buffer_read_uint8(aprservice_lua_module_byte_buffer* byte_buffer);
// @return success, value
AL::Collections::Tuple<bool, AL::uint16>                                         aprservice_lua_module_byte_buffer_read_uint16(aprservice_lua_module_byte_buffer* byte_buffer);
// @return success, value
AL::Collections::Tuple<bool, AL::uint32>                                         aprservice_lua_module_byte_buffer_read_uint32(aprservice_lua_module_byte_buffer* byte_buffer);
// @return success, value
AL::Collections::Tuple<bool, AL::uint64>                                         aprservice_lua_module_byte_buffer_read_uint64(aprservice_lua_module_byte_buffer* byte_buffer);
// @return success, value
AL::Collections::Tuple<bool, AL::Float>                                          aprservice_lua_module_byte_buffer_read_float(aprservice_lua_module_byte_buffer* byte_buffer);
// @return success, value
AL::Collections::Tuple<bool, AL::Double>                                         aprservice_lua_module_byte_buffer_read_double(aprservice_lua_module_byte_buffer* byte_buffer);
// @return success, value
AL::Collections::Tuple<bool, AL::String>                                         aprservice_lua_module_byte_buffer_read_string(aprservice_lua_module_byte_buffer* byte_buffer);
// @return success, value
AL::Collections::Tuple<bool, bool>                                               aprservice_lua_module_byte_buffer_read_boolean(aprservice_lua_module_byte_buffer* byte_buffer);

bool                                                                             aprservice_lua_module_byte_buffer_write(aprservice_lua_module_byte_buffer* byte_buffer, const void* buffer, AL::size_t size);
bool                                                                             aprservice_lua_module_byte_buffer_write_int8(aprservice_lua_module_byte_buffer* byte_buffer, AL::int8 value);
bool                                                                             aprservice_lua_module_byte_buffer_write_int16(aprservice_lua_module_byte_buffer* byte_buffer, AL::int16 value);
bool                                                                             aprservice_lua_module_byte_buffer_write_int32(aprservice_lua_module_byte_buffer* byte_buffer, AL::int32 value);
bool                                                                             aprservice_lua_module_byte_buffer_write_int64(aprservice_lua_module_byte_buffer* byte_buffer, AL::int64 value);
bool                                                                             aprservice_lua_module_byte_buffer_write_uint8(aprservice_lua_module_byte_buffer* byte_buffer, AL::uint16 value);
bool                                                                             aprservice_lua_module_byte_buffer_write_uint16(aprservice_lua_module_byte_buffer* byte_buffer, AL::uint16 value);
bool                                                                             aprservice_lua_module_byte_buffer_write_uint32(aprservice_lua_module_byte_buffer* byte_buffer, AL::uint32 value);
bool                                                                             aprservice_lua_module_byte_buffer_write_uint64(aprservice_lua_module_byte_buffer* byte_buffer, AL::uint64 value);
bool                                                                             aprservice_lua_module_byte_buffer_write_float(aprservice_lua_module_byte_buffer* byte_buffer, AL::Float value);
bool                                                                             aprservice_lua_module_byte_buffer_write_double(aprservice_lua_module_byte_buffer* byte_buffer, AL::Double value);
bool                                                                             aprservice_lua_module_byte_buffer_write_string(aprservice_lua_module_byte_buffer* byte_buffer, const AL::String& value);
bool                                                                             aprservice_lua_module_byte_buffer_write_boolean(aprservice_lua_module_byte_buffer* byte_buffer, bool value);
