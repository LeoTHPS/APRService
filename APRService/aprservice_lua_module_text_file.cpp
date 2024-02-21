#include "aprservice.hpp"
#include "aprservice_lua.hpp"
#include "aprservice_lua_module_text_file.hpp"

#include <AL/Lua54/Lua.hpp>

#include <AL/FileSystem/TextFile.hpp>

struct aprservice_lua_module_text_file
{
};

struct aprservice_lua_module_text_file_instance
{
	AL::FileSystem::TextFile file;
};

aprservice_lua_module_text_file_instance*                    aprservice_lua_module_text_file_open(const AL::String& path, AL::uint8 mode)
{
	auto text_file = new aprservice_lua_module_text_file_instance
	{
		.file = AL::FileSystem::TextFile(path)
	};

	AL::BitMask<AL::FileSystem::FileOpenModes> open_mode;

	if ((mode & APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_READ)     == APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_READ)     open_mode.Add(AL::FileSystem::FileOpenModes::Read);
	if ((mode & APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_WRITE)    == APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_WRITE)    open_mode.Add(AL::FileSystem::FileOpenModes::Write);
	if ((mode & APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_APPEND)   == APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_APPEND)   open_mode.Add(AL::FileSystem::FileOpenModes::Append);
	if ((mode & APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_TRUNCATE) == APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_TRUNCATE) open_mode.Add(AL::FileSystem::FileOpenModes::Truncate);

	try
	{
		if (!text_file->file.Open(open_mode.Value))
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
void                                                         aprservice_lua_module_text_file_close(aprservice_lua_module_text_file_instance* text_file)
{
	text_file->file.Close();

	delete text_file;
}

// @return success, end_of_file, string
aprservice_lua_module_text_file_read_value<bool, AL::String> aprservice_lua_module_text_file_read(aprservice_lua_module_text_file_instance* text_file, AL::size_t length)
{
	aprservice_lua_module_text_file_read_value<bool, AL::String> value(false, false, "");

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
bool                                                         aprservice_lua_module_text_file_write(aprservice_lua_module_text_file_instance* text_file, const AL::String& value)
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
aprservice_lua_module_text_file_read_value<bool, AL::String> aprservice_lua_module_text_file_read_line(aprservice_lua_module_text_file_instance* text_file)
{
	aprservice_lua_module_text_file_read_value<bool, AL::String> value(false, false, "");

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
bool                                                         aprservice_lua_module_text_file_write_line(aprservice_lua_module_text_file_instance* text_file, const AL::String& value)
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

aprservice_lua_module_text_file* aprservice_lua_module_text_file_init(aprservice_lua* lua)
{
	auto text_file = new aprservice_lua_module_text_file
	{
	};

	auto lua_state = aprservice_lua_get_state(lua);

	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_READ);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_WRITE);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_APPEND);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_TRUNCATE);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_text_file_open);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_text_file_close);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_text_file_read);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_text_file_write);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_text_file_read_line);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_text_file_write_line);

	return text_file;
}
void                             aprservice_lua_module_text_file_deinit(aprservice_lua_module_text_file* text_file)
{
	delete text_file;
}
