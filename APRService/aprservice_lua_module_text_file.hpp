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

enum APRSERVICE_LUA_MODULE_TEXT_FILE_LINE_ENDINGS : AL::uint8
{
	APRSERVICE_LUA_MODULE_TEXT_FILE_LINE_ENDING_LF,
	APRSERVICE_LUA_MODULE_TEXT_FILE_LINE_ENDING_CRLF,
	APRSERVICE_LUA_MODULE_TEXT_FILE_LINE_ENDING_AUTO
};

struct aprservice_lua_module_text_file;

void                                           aprservice_lua_module_text_file_register_globals(aprservice_lua* lua);

aprservice_lua_module_text_file*               aprservice_lua_module_text_file_open(const AL::String& path, AL::uint8 mode, AL::uint8 line_ending);
void                                           aprservice_lua_module_text_file_close(aprservice_lua_module_text_file* text_file);

// @return success, end_of_file, string
AL::Collections::Tuple<bool, bool, AL::String> aprservice_lua_module_text_file_read(aprservice_lua_module_text_file* text_file, AL::size_t length);
bool                                           aprservice_lua_module_text_file_write(aprservice_lua_module_text_file* text_file, const AL::String& value);

// @return success, end_of_file, string
AL::Collections::Tuple<bool, bool, AL::String> aprservice_lua_module_text_file_read_line(aprservice_lua_module_text_file* text_file);
bool                                           aprservice_lua_module_text_file_write_line(aprservice_lua_module_text_file* text_file, const AL::String& value);
