require('APRService');
require('APRService.Modules.File');
require('APRService.Modules.Timer');
require('APRService.Modules.TextFile');

local aprs_is_config =
{
	['Host']              = 'noam.aprs2.net',
	['Port']              = 14580,
	['Path']              = 'WIDE1-1',
	['Callsign']          = 'N0CALL',
	['Passcode']          = 0,
	['SymbolTable']       = '/',
	['SymbolTableKey']    = 'k',
	['EnableMonitorMode'] = false
};

local database_config =
{
	['Path']           = './IGateMapper.db',
	['UpdateInterval'] = 10 * 60
};

local IGateMapper = {};

IGateMapper.DB               = {};
IGateMapper.DB.gateways      = {}; -- [callsign] = { packet_count }
IGateMapper.DB.stations      = {}; -- [callsign] = { latitude, longitude, altitude, is_located }
IGateMapper.DB.gateway_count = 0;
IGateMapper.DB.station_count = 0;
IGateMapper.DB.gateway_index = {}; -- [callsign] = true
IGateMapper.DB.station_index = {}; -- [callsign] = true

-- @return exists, latitude, longitude, altitude, is_located
function IGateMapper.DB.GetStation(callsign)
	if IGateMapper.DB.station_index[callsign] then
		local station = IGateMapper.DB.stations[callsign];

		return true, station[1], station[2], station[3], station[4];
	end

	return false;
end

-- @return exists, packet_count, latitude, longitude, altitude, is_located
function IGateMapper.DB.GetGateway(callsign)
	if IGateMapper.DB.gateway_index[callsign] then
		local gateway                                           = IGateMapper.DB.gateways[callsign];
		local exists, latitude, longitude, altitude, is_located = IGateMapper.DB.GetStation(callsign);

		return true, gateway[1], latitude, longitude, altitude, is_located;
	end

	return false;
end

function IGateMapper.DB.AddStation(callsign)
	if not IGateMapper.DB.station_index[callsign] then
		IGateMapper.DB.stations[callsign]      = { 0, 0, 0, false };
		IGateMapper.DB.station_index[callsign] = true;
		IGateMapper.DB.station_count           = IGateMapper.DB.station_count + 1;
	end
end

function IGateMapper.DB.AddGateway(callsign)
	if not IGateMapper.DB.gateway_index[callsign] then
		IGateMapper.DB.AddStation(callsign);

		IGateMapper.DB.gateways[callsign]      = { 0 };
		IGateMapper.DB.gateway_index[callsign] = true;
		IGateMapper.DB.gateway_count           = IGateMapper.DB.gateway_count + 1;
	end
end

-- @param callback(callsign, packet_count, latitude, longitude, altitude, is_located)->boolean
function IGateMapper.DB.EnumerateGateways(callback)
	for callsign, gateway in pairs(IGateMapper.DB.gateways) do
		local exists, latitude, longitude, altitude, is_located = IGateMapper.DB.GetStation(callsign);

		if not callback(callsign, gateway[1], latitude, longitude, altitude, is_located) then
			break;
		end
	end
end

-- @param callback(callsign, latitude, longitude, altitude, is_located)->boolean
function IGateMapper.DB.EnumerateStations(callback)
	for callsign, station in pairs(IGateMapper.DB.stations) do
		if not callback(callsign, station[1], station[2], station[3], station[4]) then
			break;
		end
	end
end

function IGateMapper.DB.SetStationLocation(callsign, latitude, longitude, altitude)
	if IGateMapper.DB.station_index[callsign] then
		local station                     = IGateMapper.DB.stations[callsign];
		local station_position_changed    = not station[4] or (station[1] ~= latitude) or (station[2] ~= longitude) or (station[3] ~= altitude);
		IGateMapper.DB.stations[callsign] = { latitude, longitude, altitude, station_position_changed, true };
	end
end

function IGateMapper.DB.IncrementGatewayPacketCount(callsign)
	if IGateMapper.DB.gateway_index[callsign] then
		local gateway = IGateMapper.DB.gateways[callsign];

		if gateway[1] < 0xFFFFFFFF then
			IGateMapper.DB.gateways[callsign] = { gateway[1] + 1 };
		end
	end
end

function IGateMapper.DB.Init()
	if not APRService.Modules.File.Exists(database_config['Path']) then
		local file    = APRService.Modules.File.Open(database_config['Path'], APRService.Modules.File.OPEN_MODE_WRITE | APRService.Modules.File.OPEN_MODE_TRUNCATE);
		local success = IGateMapper.DB.Private.WriteHeader(file, 0, 0);

		APRService.Modules.File.Close(file);

		return success;
	end

	local file = APRService.Modules.File.Open(database_config['Path'], APRService.Modules.File.OPEN_MODE_READ);

	if not file then
		return false;
	end

	local success, gateway_count, station_count = IGateMapper.DB.Private.ReadHeader(file);

	if not success then
		APRService.Modules.File.Close(file);

		return false;
	end

	for i = 1, gateway_count do
		local gateway_success, gateway_callsign, gateway_packet_count = IGateMapper.DB.Private.ReadGateway(file);

		if not gateway_success then
			APRService.Modules.File.Close(file);

			return false;
		end

		IGateMapper.DB.gateways[gateway_callsign]      = { gateway_packet_count };
		IGateMapper.DB.gateway_index[gateway_callsign] = true;
		IGateMapper.DB.gateway_count                   = IGateMapper.DB.gateway_count + 1;
	end

	for i = 1, station_count do
		local station_success, station_callsign, station_latitude, station_longitude, station_altitude, station_is_location_set = IGateMapper.DB.Private.ReadStation(file);

		if not station_success then
			APRService.Modules.File.Close(file);

			return false;
		end

		IGateMapper.DB.stations[station_callsign]      = { station_latitude, station_longitude, station_altitude, station_is_location_set };
		IGateMapper.DB.station_index[station_callsign] = true;
		IGateMapper.DB.station_count                   = IGateMapper.DB.station_count + 1;
	end

	APRService.Modules.File.Close(file);

	return true;
end

function IGateMapper.DB.Save()
	local file = APRService.Modules.File.Open(database_config['Path'], APRService.Modules.File.OPEN_MODE_WRITE | APRService.Modules.File.OPEN_MODE_TRUNCATE);

	if not file then
		return false;
	end

	if not IGateMapper.DB.Private.WriteHeader(file, IGateMapper.DB.gateway_count, IGateMapper.DB.station_count) then
		APRService.Modules.File.Close(file);

		return false;
	end

	for callsign, gateway in pairs(IGateMapper.DB.gateways) do
		if not IGateMapper.DB.Private.WriteGateway(file, callsign, gateway[1]) then
			APRService.Modules.File.Close(file);

			return false;
		end
	end

	for callsign, station in pairs(IGateMapper.DB.stations) do
		if not IGateMapper.DB.Private.WriteStation(file, callsign, station[1], station[2], station[3], station[4]) then
			APRService.Modules.File.Close(file);

			return false;
		end
	end

	APRService.Modules.File.Close(file);

	return true;
end

function IGateMapper.DB.Export()
	local text_file = APRService.Modules.TextFile.Open(string.format('%s.gateways.kml', database_config['Path']), APRService.Modules.TextFile.OPEN_MODE_WRITE | APRService.Modules.TextFile.OPEN_MODE_TRUNCATE, APRService.Modules.TextFile.LINE_ENDING_LF);

	if text_file then
		APRService.Modules.TextFile.WriteLine(text_file, '<?xml version="1.0" encoding="UTF-8"?>');
		APRService.Modules.TextFile.WriteLine(text_file, '<kml xmlns="http://www.opengis.net/kml/2.2">');
		APRService.Modules.TextFile.WriteLine(text_file, '\t<Document>');

		IGateMapper.DB.EnumerateGateways(function(callsign, packet_count, latitude, longitude, altitude, is_located)
			if is_located then
				APRService.Modules.TextFile.WriteLine(text_file, '\t\t<Placemark>');
				APRService.Modules.TextFile.WriteLine(text_file, string.format('\t\t\t<name>%s</name>', callsign));
				APRService.Modules.TextFile.WriteLine(text_file, string.format('\t\t\t<description>http://aprs.fi/#!call=%s</description>', callsign));
				APRService.Modules.TextFile.WriteLine(text_file, string.format('\t\t\t<Point><coordinates>%f,%f,%i</coordinates></Point>', longitude, latitude, altitude));
				APRService.Modules.TextFile.WriteLine(text_file, '\t\t</Placemark>');
			end

			return true;
		end);

		APRService.Modules.TextFile.WriteLine(text_file, '\t</Document>');
		APRService.Modules.TextFile.WriteLine(text_file, '</kml>');
		APRService.Modules.TextFile.Close(text_file);

		text_file = APRService.Modules.TextFile.Open(string.format('%s.stations.kml', database_config['Path']), APRService.Modules.TextFile.OPEN_MODE_WRITE | APRService.Modules.TextFile.OPEN_MODE_TRUNCATE, APRService.Modules.TextFile.LINE_ENDING_LF);

		if text_file then
			APRService.Modules.TextFile.WriteLine(text_file, '<?xml version="1.0" encoding="UTF-8"?>');
			APRService.Modules.TextFile.WriteLine(text_file, '<kml xmlns="http://www.opengis.net/kml/2.2">');
			APRService.Modules.TextFile.WriteLine(text_file, '\t<Document>');

			IGateMapper.DB.EnumerateStations(function(callsign, latitude, longitude, altitude, is_located)
				if is_located then
					APRService.Modules.TextFile.WriteLine(text_file, '\t\t<Placemark>');
					APRService.Modules.TextFile.WriteLine(text_file, string.format('\t\t\t<name>%s</name>', callsign));
					APRService.Modules.TextFile.WriteLine(text_file, string.format('\t\t\t<description>http://aprs.fi/#!call=%s</description>', callsign));
					APRService.Modules.TextFile.WriteLine(text_file, string.format('\t\t\t<Point><coordinates>%f,%f,%i</coordinates></Point>', longitude, latitude, altitude));
					APRService.Modules.TextFile.WriteLine(text_file, '\t\t</Placemark>');
				end

				return true;
			end);

			APRService.Modules.TextFile.WriteLine(text_file, '\t</Document>');
			APRService.Modules.TextFile.WriteLine(text_file, '</kml>');
			APRService.Modules.TextFile.Close(text_file);

			return true;
		end
	end

	return false;
end

IGateMapper.DB.Private = {};

-- @return success, gateway_count, station_count
function IGateMapper.DB.Private.ReadHeader(file)
	local success, byte_buffer, byte_buffer_size = APRService.Modules.File.Read(file, 12, APRService.Modules.ByteBuffer.ENDIAN_BIG);

	if not success then
		return false;
	end

	if byte_buffer_size ~= 12 then
		APRService.Modules.ByteBuffer.Destroy(byte_buffer);

		return false;
	end

	local magic_success, magic_value = APRService.Modules.ByteBuffer.ReadUInt32(byte_buffer);

	if magic_value ~= 0x12345678 then
		APRService.Modules.ByteBuffer.Destroy(byte_buffer);

		return false;
	end

	local gateway_success, gateway_count = APRService.Modules.ByteBuffer.ReadUInt32(byte_buffer);
	local station_success, station_count = APRService.Modules.ByteBuffer.ReadUInt32(byte_buffer);

	APRService.Modules.ByteBuffer.Destroy(byte_buffer);

	return true, gateway_count, station_count;
end

function IGateMapper.DB.Private.WriteHeader(file, gateway_count, station_count)
	local byte_buffer = APRService.Modules.ByteBuffer.Create(APRService.Modules.ByteBuffer.ENDIAN_BIG, 12);

	APRService.Modules.ByteBuffer.WriteUInt32(byte_buffer, 0x12345678);    -- magic
	APRService.Modules.ByteBuffer.WriteUInt32(byte_buffer, gateway_count); -- gateway_count
	APRService.Modules.ByteBuffer.WriteUInt32(byte_buffer, station_count); -- station_count

	if not APRService.Modules.File.Write(file, byte_buffer, 12) then
		APRService.Modules.ByteBuffer.Destroy(byte_buffer);

		return false;
	end

	APRService.Modules.ByteBuffer.Destroy(byte_buffer);

	return true;
end

-- @return success, callsign, packet_count
function IGateMapper.DB.Private.ReadGateway(file)
	local success, byte_buffer, byte_buffer_size = APRService.Modules.File.Read(file, 4, APRService.Modules.ByteBuffer.ENDIAN_BIG);

	if not success then
		return false;
	end

	if byte_buffer_size ~= 4 then
		APRService.Modules.ByteBuffer.Destroy(byte_buffer);

		return false;
	end

	local gateway_success, gateway_byte_buffer_size = APRService.Modules.ByteBuffer.ReadUInt32(byte_buffer);

	APRService.Modules.ByteBuffer.Destroy(byte_buffer);

	success, byte_buffer, byte_buffer_size = APRService.Modules.File.Read(file, gateway_byte_buffer_size, APRService.Modules.ByteBuffer.ENDIAN_BIG);

	if not success then
		return false;
	end

	if byte_buffer_size ~= gateway_byte_buffer_size then
		APRService.Modules.ByteBuffer.Destroy(byte_buffer);

		return false;
	end

	local gateway_callsign_success,     gateway_callsign     = APRService.Modules.ByteBuffer.ReadString(byte_buffer);
	local gateway_packet_count_success, gateway_packet_count = APRService.Modules.ByteBuffer.ReadUInt32(byte_buffer);

	APRService.Modules.ByteBuffer.Destroy(byte_buffer);

	return true, gateway_callsign, gateway_packet_count;
end

function IGateMapper.DB.Private.WriteGateway(file, callsign, packet_count)
	local byte_buffer = APRService.Modules.ByteBuffer.Create(APRService.Modules.ByteBuffer.ENDIAN_BIG, 0x20);
	APRService.Modules.ByteBuffer.WriteString(byte_buffer, callsign);
	APRService.Modules.ByteBuffer.WriteUInt32(byte_buffer, packet_count);

	local byte_buffer2 = APRService.Modules.ByteBuffer.Create(APRService.Modules.ByteBuffer.ENDIAN_BIG, 4);
	APRService.Modules.ByteBuffer.WriteUInt32(byte_buffer2, APRService.Modules.ByteBuffer.GetSize(byte_buffer));

	if not APRService.Modules.File.Write(file, byte_buffer2, 4) then
		APRService.Modules.ByteBuffer.Destroy(byte_buffer2);
		APRService.Modules.ByteBuffer.Destroy(byte_buffer);

		return false;
	end

	APRService.Modules.ByteBuffer.Destroy(byte_buffer2);

	if not APRService.Modules.File.Write(file, byte_buffer, APRService.Modules.ByteBuffer.GetSize(byte_buffer)) then
		APRService.Modules.ByteBuffer.Destroy(byte_buffer);

		return false;
	end

	APRService.Modules.ByteBuffer.Destroy(byte_buffer);

	return true;
end

-- @return success, callsign, latitude, longitude, altitude, is_location_set
function IGateMapper.DB.Private.ReadStation(file)
	local success, byte_buffer, byte_buffer_size = APRService.Modules.File.Read(file, 4, APRService.Modules.ByteBuffer.ENDIAN_BIG);

	if not success then
		return false;
	end

	if byte_buffer_size ~= 4 then
		APRService.Modules.ByteBuffer.Destroy(byte_buffer);

		return false;
	end

	local station_success, station_byte_buffer_size = APRService.Modules.ByteBuffer.ReadUInt32(byte_buffer);

	APRService.Modules.ByteBuffer.Destroy(byte_buffer);

	success, byte_buffer, byte_buffer_size = APRService.Modules.File.Read(file, station_byte_buffer_size, APRService.Modules.ByteBuffer.ENDIAN_BIG);

	if not success then
		return false;
	end

	if byte_buffer_size ~= station_byte_buffer_size then
		APRService.Modules.ByteBuffer.Destroy(byte_buffer);

		return false;
	end

	local station_callsign_success,         station_callsign         = APRService.Modules.ByteBuffer.ReadString(byte_buffer);
	local station_latitude_success,         station_latitude         = APRService.Modules.ByteBuffer.ReadFloat(byte_buffer);
	local station_longitude_success,        station_longitude        = APRService.Modules.ByteBuffer.ReadFloat(byte_buffer);
	local station_altitude_success,         station_altitude         = APRService.Modules.ByteBuffer.ReadInt32(byte_buffer);
	local station_is_location_set_success,  station_is_location_set  = APRService.Modules.ByteBuffer.ReadBoolean(byte_buffer);

	APRService.Modules.ByteBuffer.Destroy(byte_buffer);

	return true, station_callsign, station_latitude, station_longitude, station_altitude, station_is_location_set;
end

function IGateMapper.DB.Private.WriteStation(file, callsign, latitude, longitude, altitude, is_location_set)
	local byte_buffer = APRService.Modules.ByteBuffer.Create(APRService.Modules.ByteBuffer.ENDIAN_BIG, 0x50);
	APRService.Modules.ByteBuffer.WriteString(byte_buffer,  callsign);
	APRService.Modules.ByteBuffer.WriteFloat(byte_buffer,   latitude);
	APRService.Modules.ByteBuffer.WriteFloat(byte_buffer,   longitude);
	APRService.Modules.ByteBuffer.WriteInt32(byte_buffer,   altitude);
	APRService.Modules.ByteBuffer.WriteBoolean(byte_buffer, is_location_set);

	local byte_buffer2 = APRService.Modules.ByteBuffer.Create(APRService.Modules.ByteBuffer.ENDIAN_BIG, 4);
	APRService.Modules.ByteBuffer.WriteUInt32(byte_buffer2, APRService.Modules.ByteBuffer.GetSize(byte_buffer));

	if not APRService.Modules.File.Write(file, byte_buffer2, 4) then
		APRService.Modules.ByteBuffer.Destroy(byte_buffer2);
		APRService.Modules.ByteBuffer.Destroy(byte_buffer);

		return false;
	end

	APRService.Modules.ByteBuffer.Destroy(byte_buffer2);

	if not APRService.Modules.File.Write(file, byte_buffer, APRService.Modules.ByteBuffer.GetSize(byte_buffer)) then
		APRService.Modules.ByteBuffer.Destroy(byte_buffer);

		return false;
	end

	APRService.Modules.ByteBuffer.Destroy(byte_buffer);

	return true;
end

IGateMapper.APRS = {};

function IGateMapper.APRS.Connect()
	return APRService.APRS.IS.Connect(IGateMapper.Service, aprs_is_config['Host'], aprs_is_config['Port'], aprs_is_config['Passcode']);
end

IGateMapper.Private = {};

function IGateMapper.Private.Connect()
	if not IGateMapper.APRS.Connect() then
		APRService.Console.WriteLine('Connection failed. Trying again in 3 seconds.');
		APRService.Events.Schedule(IGateMapper.Service, 3, function(service) IGateMapper.Private.Connect(); end);
	end
end

function IGateMapper.Private.UpdateDB()
	APRService.Events.Schedule(IGateMapper.Service, database_config['UpdateInterval'], function(service) IGateMapper.Private.UpdateDB(); end);

	local timer = APRService.Modules.Timer.Create();

	APRService.Console.WriteLine('Saving database');
	APRService.Modules.Timer.Reset(timer);

	if IGateMapper.DB.Save() then
		APRService.Console.WriteLine(string.format('Saved in %ums', APRService.Modules.Timer.GetElapsedMS(timer)));
	end

	APRService.Console.WriteLine('Exporting database');
	APRService.Modules.Timer.Reset(timer);

	if IGateMapper.DB.Export() then
		APRService.Console.WriteLine(string.format('Exported in %ums', APRService.Modules.Timer.GetElapsedMS(timer)));
	end

	APRService.Modules.Timer.Destroy(timer);
end

function IGateMapper.Init()
	IGateMapper.ServiceConfig = APRService.Config.Init();

	if not IGateMapper.ServiceConfig then
		return false;
	end

	APRService.Config.APRS.SetPath(IGateMapper.ServiceConfig, aprs_is_config['Path']);
	APRService.Config.APRS.SetStation(IGateMapper.ServiceConfig, aprs_is_config['Callsign']);
	APRService.Config.APRS.SetSymbolTable(IGateMapper.ServiceConfig, aprs_is_config['SymbolTable']);
	APRService.Config.APRS.SetSymbolTableKey(IGateMapper.ServiceConfig, aprs_is_config['SymbolTableKey']);
	APRService.Config.APRS.EnableMonitorMode(IGateMapper.ServiceConfig, aprs_is_config['EnableMonitorMode']);

	APRService.Config.Events.SetOnDisconnect(IGateMapper.ServiceConfig, function(service, reason)
		if reason == APRService.APRS.DISCONNECT_REASON_USER_REQUESTED then
			APRService.Stop(service);
		elseif reason == APRService.APRS.DISCONNECT_REASON_AUTHENTICATION_FAILED then
			APRService.Stop(service);
		else
			APRService.Events.Schedule(service, 0, function(service) IGateMapper.Private.Connect(); end);
		end
	end);

	APRService.Config.Events.SetOnReceivePacket(IGateMapper.ServiceConfig, function(service, station, tocall, path, igate, content)
		local station_exists, station_latitude, station_longitude, station_altitude, station_is_located = IGateMapper.DB.GetStation(station);

		if not station_exists then
			IGateMapper.DB.AddStation(station);

			APRService.Console.WriteLine(string.format('Identified station %s', station));
		end

		if string.len(igate) ~= 0 then
			local gateway_exists, _ = IGateMapper.DB.GetGateway(igate);
			      station_exists, _ = IGateMapper.DB.GetStation(igate);

			if not station_exists and not gateway_exists then
				IGateMapper.DB.AddGateway(igate);

				APRService.Console.WriteLine(string.format('Identified gateway %s', station));
			elseif station_exists and not gateway_exists then
				IGateMapper.DB.AddGateway(igate);

				APRService.Console.WriteLine(string.format('Identified station %s as gateway', station));
			end

			IGateMapper.DB.IncrementGatewayPacketCount(igate);
		end
	end);

	APRService.Config.Events.SetOnReceivePosition(IGateMapper.ServiceConfig, function(service, station, path, igate, altitude, latitude, longitude, symbol_table, symbol_table_key, comment, flags)
		local gateway_exists, gateway_packet_count, gateway_latitude, gateway_longitude, gateway_altitude, gateway_is_located = IGateMapper.DB.GetGateway(station);
		local station_exists,                       station_latitude, station_longitude, station_altitude, station_is_located = IGateMapper.DB.GetStation(station);

		IGateMapper.DB.SetStationLocation(station, latitude, longitude, altitude);

		if gateway_exists then
			if not gateway_is_located then
				APRService.Console.WriteLine(string.format('Discovered gateway %s at %.6f, %.6f', station, latitude, longitude));
			elseif (gateway_latitude ~= latitude) or (gateway_longitude ~= longitude) then
				APRService.Console.WriteLine(string.format('Updated gateway %s position to %.6f, %.6f', station, latitude, longitude));
			end
		elseif not station_is_located then
			APRService.Console.WriteLine(string.format('Discovered station %s at %.6f, %.6f', station, latitude, longitude));
		elseif (station_latitude ~= latitude) or (station_longitude ~= longitude) then
			APRService.Console.WriteLine(string.format('Updated station %s position to %.6f, %.6f', station, latitude, longitude));
		end
	end);

	IGateMapper.Service = APRService.Init(IGateMapper.ServiceConfig);

	if not IGateMapper.Service then
		APRService.Config.Deinit(IGateMapper.ServiceConfig);

		return false;
	end

	if not IGateMapper.DB.Init() then
		APRService.Deinit(IGateMapper.Service);
		APRService.Config.Deinit(IGateMapper.ServiceConfig);

		return false;
	end

	IGateMapper.Private.Connect();

	return true;
end

function IGateMapper.Deinit()
	APRService.Deinit(IGateMapper.Service);
	APRService.Config.Deinit(IGateMapper.ServiceConfig);
end

function IGateMapper.Run(tick_rate)
	APRService.Events.Schedule(IGateMapper.Service, database_config['UpdateInterval'], function(service) IGateMapper.Private.UpdateDB(); end);
	APRService.Run(IGateMapper.Service, tick_rate, APRService.FLAG_NONE);
end

if IGateMapper.Init() then
	IGateMapper.Run(10);
	IGateMapper.Deinit();
end
