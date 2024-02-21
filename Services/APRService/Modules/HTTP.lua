require('APRService');
require('APRService.Modules.ByteBuffer');

APRService.Modules.HTTP = {};

APRService.Modules.HTTP.HTTP_VERSION_1_0 = APRSERVICE_LUA_MODULE_HTTP_VERSION_1_0;
APRService.Modules.HTTP.HTTP_VERSION_1_1 = APRSERVICE_LUA_MODULE_HTTP_VERSION_1_1;

APRService.Modules.HTTP.REQUEST_METHOD_GET     = APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_GET;
APRService.Modules.HTTP.REQUEST_METHOD_HEAD    = APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_HEAD;
APRService.Modules.HTTP.REQUEST_METHOD_POST    = APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_POST;
APRService.Modules.HTTP.REQUEST_METHOD_PUT     = APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_PUT;
APRService.Modules.HTTP.REQUEST_METHOD_DELETE  = APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_DELETE;
APRService.Modules.HTTP.REQUEST_METHOD_CONNECT = APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_CONNECT;
APRService.Modules.HTTP.REQUEST_METHOD_OPTIONS = APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_OPTIONS;
APRService.Modules.HTTP.REQUEST_METHOD_TRACE   = APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_TRACE;
APRService.Modules.HTTP.REQUEST_METHOD_PATCH   = APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_PATCH;

-- @return http_request
function APRService.Modules.HTTP.CreateRequest(version, method)
	return aprservice_lua_module_http_request_create(version, method);
end

function APRService.Modules.HTTP.DestroyRequest(http_request)
	aprservice_lua_module_http_request_destroy(http_request);
end

APRService.Modules.HTTP.Request = {};

-- @return exists, string
function APRService.Modules.HTTP.Request.GetHeader(http_request, key)
	return aprservice_lua_module_http_request_get_header(http_request, tostring(key));
end

function APRService.Modules.HTTP.Request.SetHeader(http_request, key, value)
	aprservice_lua_module_http_request_set_header(http_request, tostring(key), tostring(value));
end

function APRService.Modules.HTTP.Request.GetMethod(http_request)
	return aprservice_lua_module_http_request_get_method(http_request);
end

function APRService.Modules.HTTP.Request.GetVersion(http_request)
	return aprservice_lua_module_http_request_get_version(http_request);
end

-- @return success, status_code, string
function APRService.Modules.HTTP.Request.DownloadString(http_request, url)
	return aprservice_lua_module_http_request_download_string(http_request, tostring(url));
end

-- @return success, status_code, byte_buffer
function APRService.Modules.HTTP.Request.DownloadByteBuffer(http_request, url)
	return aprservice_lua_module_http_request_download_byte_buffer(http_request, tostring(url));
end
