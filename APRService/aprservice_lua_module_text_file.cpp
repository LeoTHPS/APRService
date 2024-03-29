#include "aprservice.hpp"
#include "aprservice_lua.hpp"
#include "aprservice_lua_module_text_file.hpp"

#include <AL/Lua54/Lua.hpp>

#include <AL/FileSystem/TextFile.hpp>

struct aprservice_lua_module_text_file
{
	AL::FileSystem::TextFile file;
};

void                                           aprservice_lua_module_text_file_register_globals(aprservice_lua* lua)
{
	auto lua_state = aprservice_lua_get_state(lua);

	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_READ);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_WRITE);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_APPEND);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_TRUNCATE);

	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_TEXT_FILE_LINE_ENDING_LF);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_TEXT_FILE_LINE_ENDING_CRLF);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_TEXT_FILE_LINE_ENDING_AUTO);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_text_file_open);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_text_file_close);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_text_file_read);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_text_file_write);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_text_file_read_line);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_text_file_write_line);
}

aprservice_lua_module_text_file*               aprservice_lua_module_text_file_open(const AL::String& path, AL::uint8 mode, AL::uint8 line_ending)
{
	auto text_file = new aprservice_lua_module_text_file
	{
		.file = AL::FileSystem::TextFile(path)
	};

	AL::BitMask<AL::FileSystem::FileOpenModes> file_open_mode;
	if ((mode & APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_READ)     == APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_READ)     file_open_mode.Add(AL::FileSystem::FileOpenModes::Read);
	if ((mode & APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_WRITE)    == APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_WRITE)    file_open_mode.Add(AL::FileSystem::FileOpenModes::Write);
	if ((mode & APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_APPEND)   == APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_APPEND)   file_open_mode.Add(AL::FileSystem::FileOpenModes::Append);
	if ((mode & APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_TRUNCATE) == APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_TRUNCATE) file_open_mode.Add(AL::FileSystem::FileOpenModes::Truncate);

	switch (line_ending)
	{
		case APRSERVICE_LUA_MODULE_TEXT_FILE_LINE_ENDING_LF:
			text_file->file.SetLineEnding(AL::FileSystem::TextFileLineEndings::LF);
			break;

		case APRSERVICE_LUA_MODULE_TEXT_FILE_LINE_ENDING_CRLF:
			text_file->file.SetLineEnding(AL::FileSystem::TextFileLineEndings::CRLF);
			break;
	}

	try
	{
		if (!text_file->file.Open(file_open_mode.Value))
			throw AL::Exception("File not found");
	}
	catch (const AL::Exception& exception)
	{
		delete text_file;

		aprservice_console_write_line("Error opening AL::FileSystem::TextFile");
		aprservice_console_write_exception(exception);

		return nullptr;
	}

	return text_file;
}
void                                           aprservice_lua_module_text_file_close(aprservice_lua_module_text_file* text_file)
{
	text_file->file.Close();

	delete text_file;
}

// @return success, end_of_file, string
AL::Collections::Tuple<bool, bool, AL::String> aprservice_lua_module_text_file_read(aprservice_lua_module_text_file* text_file, AL::size_t length)
{
	AL::Collections::Tuple<bool, bool, AL::String> value(false, false, "");

	try
	{
		value.Set<1>(!text_file->file.Read(value.Get<2>(), length));
		value.Set<0>(true);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error reading AL::FileSystem::TextFile");
		aprservice_console_write_exception(exception);
	}

	return value;
}
bool                                           aprservice_lua_module_text_file_write(aprservice_lua_module_text_file* text_file, const AL::String& value)
{
	try
	{
		text_file->file.Write(value);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error writing AL::FileSystem::TextFile");
		aprservice_console_write_exception(exception);

		return false;
	}

	return true;
}

// @return success, end_of_file, string
AL::Collections::Tuple<bool, bool, AL::String> aprservice_lua_module_text_file_read_line(aprservice_lua_module_text_file* text_file)
{
	AL::Collections::Tuple<bool, bool, AL::String> value(false, false, "");

	try
	{
		value.Set<1>(!text_file->file.ReadLine(value.Get<2>()));
		value.Set<0>(true);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error reading AL::FileSystem::TextFile");
		aprservice_console_write_exception(exception);
	}

	return value;
}
bool                                           aprservice_lua_module_text_file_write_line(aprservice_lua_module_text_file* text_file, const AL::String& value)
{
	try
	{
		text_file->file.WriteLine(value);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error writing AL::FileSystem::TextFile");
		aprservice_console_write_exception(exception);

		return false;
	}

	return true;
}
