#include "aprservice.hpp"
#include "aprservice_lua.hpp"
#include "aprservice_lua_module_file.hpp"

#include <AL/Lua54/Lua.hpp>

#include <AL/FileSystem/File.hpp>

struct aprservice_lua_module_file
{
};

struct aprservice_lua_module_file_instance
{
	AL::FileSystem::File file;
};

AL::uint64                                                                                     aprservice_lua_module_file_get_size(const AL::String& path)
{
	try
	{
		return AL::FileSystem::File::GetSize(path);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error ");
		aprservice_console_write_exception(exception);
	}

	return 0;
}
bool                                                                                           aprservice_lua_module_file_copy(const AL::String& source, const AL::String& destination)
{
	try
	{
		AL::FileSystem::File::Copy(source, destination);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error copying AL::FileSystem::File");
		aprservice_console_write_exception(exception);

		return false;
	}

	return true;
}
bool                                                                                           aprservice_lua_module_file_move(const AL::String& source, const AL::String& destination)
{
	try
	{
		AL::FileSystem::File::Move(source, destination);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error moving AL::FileSystem::File");
		aprservice_console_write_exception(exception);

		return false;
	}

	return true;
}
bool                                                                                           aprservice_lua_module_file_delete(const AL::String& path)
{
	try
	{
		AL::FileSystem::File::Delete(path);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error deleting AL::FileSystem::File");
		aprservice_console_write_exception(exception);

		return false;
	}

	return true;
}
bool                                                                                           aprservice_lua_module_file_exists(const AL::String& path)
{
	try
	{
		if (!AL::FileSystem::File::Exists(path))
			return false;
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error checking if AL::FileSystem::File exists");
		aprservice_console_write_exception(exception);

		return false;
	}

	return true;
}

aprservice_lua_module_file_instance*                                                           aprservice_lua_module_file_open(const AL::String& path, AL::uint8 mode)
{
	auto file = new aprservice_lua_module_file_instance
	{
		.file = AL::FileSystem::File(path)
	};

	AL::BitMask<AL::FileSystem::FileOpenModes> open_mode;

	if ((mode & APRSERVICE_LUA_MODULE_FILE_OPEN_MODE_READ)     == APRSERVICE_LUA_MODULE_FILE_OPEN_MODE_READ)     open_mode.Add(AL::FileSystem::FileOpenModes::Read);
	if ((mode & APRSERVICE_LUA_MODULE_FILE_OPEN_MODE_WRITE)    == APRSERVICE_LUA_MODULE_FILE_OPEN_MODE_WRITE)    open_mode.Add(AL::FileSystem::FileOpenModes::Write);
	if ((mode & APRSERVICE_LUA_MODULE_FILE_OPEN_MODE_APPEND)   == APRSERVICE_LUA_MODULE_FILE_OPEN_MODE_APPEND)   open_mode.Add(AL::FileSystem::FileOpenModes::Append);
	if ((mode & APRSERVICE_LUA_MODULE_FILE_OPEN_MODE_TRUNCATE) == APRSERVICE_LUA_MODULE_FILE_OPEN_MODE_TRUNCATE) open_mode.Add(AL::FileSystem::FileOpenModes::Truncate);

	try
	{
		if (!file->file.Open(open_mode.Value))
			throw AL::Exception("File not found");
	}
	catch (const AL::Exception& exception)
	{
		delete file;

		aprservice_console_write_line("Error opening AL::FileSystem::File");
		aprservice_console_write_exception(exception);

		return nullptr;
	}

	return file;
}
void                                                                                           aprservice_lua_module_file_close(aprservice_lua_module_file_instance* file)
{
	file->file.Close();

	delete file;
}

// @return success, byte_buffer, byte_buffer_size
aprservice_lua_module_file_read_value<aprservice_lua_module_byte_buffer_instance*, AL::size_t> aprservice_lua_module_file_read(aprservice_lua_module_file_instance* file, AL::size_t buffer_size, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN endian)
{
	aprservice_lua_module_file_read_value<aprservice_lua_module_byte_buffer_instance*, AL::size_t> value(false, aprservice_lua_module_byte_buffer_create(endian, buffer_size), 0);

	try
	{
		value.Set<2>(file->file.Read(const_cast<void*>(aprservice_lua_module_byte_buffer_get_buffer(value.Get<1>())), buffer_size));
		value.Set<0>(true);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_lua_module_byte_buffer_destroy(value.Get<1>());

		aprservice_console_write_line("Error reading AL::FileSystem::File");
		aprservice_console_write_exception(exception);
	}

	return value;
}
bool                                                                                           aprservice_lua_module_file_write(aprservice_lua_module_file_instance* file, aprservice_lua_module_byte_buffer_instance* byte_buffer, AL::size_t buffer_size)
{
	auto buffer = reinterpret_cast<const AL::uint8*>(aprservice_lua_module_byte_buffer_get_buffer(byte_buffer));

	try
	{
		for (AL::size_t i = 0; i < buffer_size; )
			i += file->file.Write(&buffer[i], buffer_size - i);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error writing AL::FileSystem::File");
		aprservice_console_write_exception(exception);

		return false;
	}

	return true;
}

aprservice_lua_module_file* aprservice_lua_module_file_init(aprservice_lua* lua)
{
	auto file = new aprservice_lua_module_file
	{
	};

	auto lua_state = aprservice_lua_get_state(lua);

	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_FILE_OPEN_MODE_READ);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_FILE_OPEN_MODE_WRITE);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_FILE_OPEN_MODE_APPEND);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_FILE_OPEN_MODE_TRUNCATE);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_file_get_size);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_file_copy);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_file_move);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_file_delete);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_file_exists);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_file_open);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_file_close);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_file_read);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_file_write);

	return file;
}
void                        aprservice_lua_module_file_deinit(aprservice_lua_module_file* file)
{
	delete file;
}
