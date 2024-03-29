#pragma once
#include <AL/Common.hpp>

#include <AL/Collections/Tuple.hpp>

#include "aprservice_lua_module_byte_buffer.hpp"

enum APRSERVICE_LUA_MODULE_FILE_OPEN_MODES : AL::uint8
{
	APRSERVICE_LUA_MODULE_FILE_OPEN_MODE_READ     = 0x01,
	APRSERVICE_LUA_MODULE_FILE_OPEN_MODE_WRITE    = 0x02,
	APRSERVICE_LUA_MODULE_FILE_OPEN_MODE_APPEND   = 0x04,
	APRSERVICE_LUA_MODULE_FILE_OPEN_MODE_TRUNCATE = 0x08
};

struct aprservice_lua;
struct aprservice_lua_module_file;

void                                                                         aprservice_lua_module_file_register_globals(aprservice_lua* lua);

AL::uint64                                                                   aprservice_lua_module_file_get_size(const AL::String& path);
bool                                                                         aprservice_lua_module_file_copy(const AL::String& source, const AL::String& destination);
bool                                                                         aprservice_lua_module_file_move(const AL::String& source, const AL::String& destination);
bool                                                                         aprservice_lua_module_file_delete(const AL::String& path);
bool                                                                         aprservice_lua_module_file_exists(const AL::String& path);

aprservice_lua_module_file*                                                  aprservice_lua_module_file_open(const AL::String& path, AL::uint8 mode);
void                                                                         aprservice_lua_module_file_close(aprservice_lua_module_file* file);

// @return success, byte_buffer, byte_buffer_size
AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer*, AL::size_t> aprservice_lua_module_file_read(aprservice_lua_module_file* file, AL::size_t byte_buffer_size, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN byte_buffer_endian);
bool                                                                         aprservice_lua_module_file_write(aprservice_lua_module_file* file, aprservice_lua_module_byte_buffer* byte_buffer, AL::size_t byte_buffer_size);
