require('APRService');
require('APRService.Modules.ByteBuffer');

APRService.Modules.File = {};

APRService.Modules.File.OPEN_MODE_READ     = APRSERVICE_LUA_MODULE_FILE_OPEN_MODE_READ;
APRService.Modules.File.OPEN_MODE_WRITE    = APRSERVICE_LUA_MODULE_FILE_OPEN_MODE_WRITE;
APRService.Modules.File.OPEN_MODE_APPEND   = APRSERVICE_LUA_MODULE_FILE_OPEN_MODE_APPEND;
APRService.Modules.File.OPEN_MODE_TRUNCATE = APRSERVICE_LUA_MODULE_FILE_OPEN_MODE_TRUNCATE;

-- @return file
function APRService.Modules.File.Open(path, mode)
	return aprservice_lua_module_file_open(tostring(path), mode);
end

function APRService.Modules.File.Close(file)
	aprservice_lua_module_file_close(file);
end

function APRService.Modules.File.GetSize(path)
	return aprservice_lua_module_file_get_size(tostring(path));
end

function APRService.Modules.File.Copy(source_path, destination_path)
	return aprservice_lua_module_file_copy(tostring(source_path), tostring(destination_path));
end

function APRService.Modules.File.Move(source_path, destination_path)
	return aprservice_lua_module_file_move(tostring(source_path), tostring(destination_path));
end

function APRService.Modules.File.Delete(path)
	return aprservice_lua_module_file_delete(tostring(path));
end

function APRService.Modules.File.Exists(path)
	return aprservice_lua_module_file_exists(tostring(path));
end

-- @return success, byte_buffer, byte_buffer_size
function APRService.Modules.File.Read(file, buffer_size, byte_buffer_endian)
	return aprservice_lua_module_file_read(file, tonumber(buffer_size), byte_buffer_endian);
end

function APRService.Modules.File.Write(file, byte_buffer, buffer_size)
	return aprservice_lua_module_file_write(file, byte_buffer, tonumber(buffer_size));
end
