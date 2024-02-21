#pragma once
#include <AL/Common.hpp>

#include <AL/Collections/Tuple.hpp>

enum APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODES : AL::uint8
{
	APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_READ     = 0x01,
	APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_WRITE    = 0x02,
	APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_APPEND   = 0x04,
	APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_TRUNCATE = 0x08
};

struct aprservice_lua_module_text_file_instance;

template<typename ... T>
using aprservice_lua_module_text_file_read_value = AL::Collections::Tuple<bool, T ...>;

aprservice_lua_module_text_file_instance*                    aprservice_lua_module_text_file_open(const AL::String& path, AL::uint8 mode);
void                                                         aprservice_lua_module_text_file_close(aprservice_lua_module_text_file_instance* text_file);

// @return success, end_of_file, string
aprservice_lua_module_text_file_read_value<bool, AL::String> aprservice_lua_module_text_file_read(aprservice_lua_module_text_file_instance* text_file, AL::size_t length);
bool                                                         aprservice_lua_module_text_file_write(aprservice_lua_module_text_file_instance* text_file, const AL::String& value);

// @return success, end_of_file, string
aprservice_lua_module_text_file_read_value<bool, AL::String> aprservice_lua_module_text_file_read_line(aprservice_lua_module_text_file_instance* text_file);
bool                                                         aprservice_lua_module_text_file_write_line(aprservice_lua_module_text_file_instance* text_file, const AL::String& value);
