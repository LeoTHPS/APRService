require('APRService');

APRService.Modules.TextFile = {};

APRService.Modules.TextFile.OPEN_MODE_READ     = APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_READ;
APRService.Modules.TextFile.OPEN_MODE_WRITE    = APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_WRITE;
APRService.Modules.TextFile.OPEN_MODE_APPEND   = APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_APPEND;
APRService.Modules.TextFile.OPEN_MODE_TRUNCATE = APRSERVICE_LUA_MODULE_TEXT_FILE_OPEN_MODE_TRUNCATE;

APRService.Modules.TextFile.LINE_ENDING_LF   = APRSERVICE_LUA_MODULE_TEXT_FILE_LINE_ENDING_LF
APRService.Modules.TextFile.LINE_ENDING_CRLF = APRSERVICE_LUA_MODULE_TEXT_FILE_LINE_ENDING_CRLF
APRService.Modules.TextFile.LINE_ENDING_AUTO = APRSERVICE_LUA_MODULE_TEXT_FILE_LINE_ENDING_AUTO

-- @return text_file
function APRService.Modules.TextFile.Open(path, mode, line_ending)
	return aprservice_lua_module_text_file_open(tostring(path), mode, line_ending);
end

function APRService.Modules.TextFile.Close(text_file)
	aprservice_lua_module_text_file_close(text_file);
end

-- @return success, end_of_file, string
function APRService.Modules.TextFile.Read(text_file, length)
	return aprservice_lua_module_text_file_read(text_file, tonumber(length));
end

function APRService.Modules.TextFile.Write(text_file, value)
	return aprservice_lua_module_text_file_write(text_file, tostring(value));
end

-- @return success, end_of_file, string
function APRService.Modules.TextFile.ReadLine(text_file)
	return aprservice_lua_module_text_file_read_line(text_file);
end

function APRService.Modules.TextFile.WriteLine(text_file, value)
	return aprservice_lua_module_text_file_write_line(text_file, tostring(value));
end
