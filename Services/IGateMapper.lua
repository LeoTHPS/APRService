require('APRService');
require('APRService.Modules.File');
require('APRService.Modules.Timer');
require('APRService.Modules.System');
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
	['UpdateInterval'] = 2 * 60
};

local IGateMapper = {};

IGateMapper.DB                       = {};
IGateMapper.DB.Private               = {};
IGateMapper.DB.Private.stations      = {}; -- [id] = { callsign, first_seen_timestamp, last_seen_timestamp, latitude, longitude, altitude, packet_count_digi, packet_count_igate, is_location_set }
IGateMapper.DB.Private.station_count = 0;

-- @return success, station_count
function IGateMapper.DB.Private.ReadHeader(file)
	local success, byte_buffer, byte_buffer_size = APRService.Modules.File.Read(file, 8, APRService.Modules.ByteBuffer.ENDIAN_BIG);

	if not success then
		return false;
	end

	if byte_buffer_size ~= 8 then
		APRService.Modules.ByteBuffer.Destroy(byte_buffer);

		return false;
	end

	local magic_success, magic_value = APRService.Modules.ByteBuffer.ReadUInt32(byte_buffer);

	if magic_value ~= 0x12345678 then
		APRService.Modules.ByteBuffer.Destroy(byte_buffer);

		return false;
	end

	local station_success, station_count = APRService.Modules.ByteBuffer.ReadUInt32(byte_buffer);

	APRService.Modules.ByteBuffer.Destroy(byte_buffer);

	return true, station_count;
end

function IGateMapper.DB.Private.WriteHeader(file, station_count)
	local byte_buffer = APRService.Modules.ByteBuffer.Create(APRService.Modules.ByteBuffer.ENDIAN_BIG, 8);

	APRService.Modules.ByteBuffer.WriteUInt32(byte_buffer, 0x12345678);
	APRService.Modules.ByteBuffer.WriteUInt32(byte_buffer, station_count);

	if not APRService.Modules.File.Write(file, byte_buffer, 8) then
		APRService.Modules.ByteBuffer.Destroy(byte_buffer);

		return false;
	end

	APRService.Modules.ByteBuffer.Destroy(byte_buffer);

	return true;
end

-- @return success, callsign, first_seen_timestamp, last_seen_timestamp, latitude, longitude, altitude, packet_count_digi, packet_count_igate, is_location_set
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

	local station_callsign_success,             station_callsign             = APRService.Modules.ByteBuffer.ReadString(byte_buffer);
	local station_first_seen_timestamp_success, station_first_seen_timestamp = APRService.Modules.ByteBuffer.ReadUInt64(byte_buffer);
	local station_last_seen_timestamp_success,  station_last_seen_timestamp  = APRService.Modules.ByteBuffer.ReadUInt64(byte_buffer);
	local station_latitude_success,             station_latitude             = APRService.Modules.ByteBuffer.ReadFloat(byte_buffer);
	local station_longitude_success,            station_longitude            = APRService.Modules.ByteBuffer.ReadFloat(byte_buffer);
	local station_altitude_success,             station_altitude             = APRService.Modules.ByteBuffer.ReadInt32(byte_buffer);
	local station_packet_count_digi_success,    station_packet_count_digi    = APRService.Modules.ByteBuffer.ReadUInt64(byte_buffer);
	local station_packet_count_igate_success,   station_packet_count_igate   = APRService.Modules.ByteBuffer.ReadUInt64(byte_buffer);
	local station_is_location_set_success,      station_is_location_set      = APRService.Modules.ByteBuffer.ReadBoolean(byte_buffer);

	APRService.Modules.ByteBuffer.Destroy(byte_buffer);

	return true, station_callsign, station_first_seen_timestamp, station_last_seen_timestamp, station_latitude, station_longitude, station_altitude, station_packet_count_digi, station_packet_count_igate, station_is_location_set;
end

function IGateMapper.DB.Private.WriteStation(file, callsign, first_seen_timestamp, last_seen_timestamp, latitude, longitude, altitude, packet_count_digi, packet_count_igate, is_location_set)
	local byte_buffer = APRService.Modules.ByteBuffer.Create(APRService.Modules.ByteBuffer.ENDIAN_BIG, 0x50);
	APRService.Modules.ByteBuffer.WriteString(byte_buffer,  callsign);
	APRService.Modules.ByteBuffer.WriteUInt64(byte_buffer,  first_seen_timestamp);
	APRService.Modules.ByteBuffer.WriteUInt64(byte_buffer,  last_seen_timestamp);
	APRService.Modules.ByteBuffer.WriteFloat(byte_buffer,   latitude);
	APRService.Modules.ByteBuffer.WriteFloat(byte_buffer,   longitude);
	APRService.Modules.ByteBuffer.WriteInt32(byte_buffer,   altitude);
	APRService.Modules.ByteBuffer.WriteUInt64(byte_buffer,  packet_count_digi);
	APRService.Modules.ByteBuffer.WriteUInt64(byte_buffer,  packet_count_igate);
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

function IGateMapper.DB.Private.FindStation(callsign)
	for station_id, station in pairs(IGateMapper.DB.Private.stations) do
		if station[1] == callsign then
			return station;
		end
	end

	return nil
end

function IGateMapper.DB.Init()
	if not APRService.Modules.File.Exists(database_config['Path']) then
		local file    = APRService.Modules.File.Open(database_config['Path'], APRService.Modules.File.OPEN_MODE_WRITE | APRService.Modules.File.OPEN_MODE_TRUNCATE);
		local success = IGateMapper.DB.Private.WriteHeader(file, 0);

		APRService.Modules.File.Close(file);

		return success;
	end

	local file = APRService.Modules.File.Open(database_config['Path'], APRService.Modules.File.OPEN_MODE_READ);

	if not file then
		return false;
	end

	local success, station_count = IGateMapper.DB.Private.ReadHeader(file);

	if not success then
		APRService.Modules.File.Close(file);

		return false;
	end

	IGateMapper.DB.Private.stations      = {};
	IGateMapper.DB.Private.station_count = 0;

	for i = 1, station_count do
		local station_success, station_callsign, station_first_seen_timestamp, station_last_seen_timestamp, station_latitude, station_longitude, station_altitude, station_packet_count_digi, station_packet_count_igate, station_is_location_set = IGateMapper.DB.Private.ReadStation(file);

		if not station_success then
			APRService.Modules.File.Close(file);

			return false;
		end

		IGateMapper.DB.Private.stations[IGateMapper.DB.Private.station_count] = { station_callsign, station_first_seen_timestamp, station_last_seen_timestamp, station_latitude, station_longitude, station_altitude, station_packet_count_digi, station_packet_count_igate, station_is_location_set };
		IGateMapper.DB.Private.station_count                                  = IGateMapper.DB.Private.station_count + 1;
	end

	APRService.Modules.File.Close(file);

	return true;
end

function IGateMapper.DB.Save()
	local file = APRService.Modules.File.Open(database_config['Path'], APRService.Modules.File.OPEN_MODE_WRITE | APRService.Modules.File.OPEN_MODE_TRUNCATE);

	if not file then
		return false;
	end

	if not IGateMapper.DB.Private.WriteHeader(file, IGateMapper.DB.station_count) then
		APRService.Modules.File.Close(file);

		return false;
	end

	IGateMapper.DB.EnumerateStations(function(callsign, first_seen_timestamp, last_seen_timestamp, latitude, longitude, altitude, packet_count_digi, packet_count_igate, is_location_set)
		return IGateMapper.DB.Private.WriteStation(file, callsign, first_seen_timestamp, last_seen_timestamp, latitude, longitude, altitude, packet_count_digi, packet_count_igate, is_location_set);
	end);

	APRService.Modules.File.Close(file);

	return true;
end

function IGateMapper.DB.Export()
	local text_file = APRService.Modules.TextFile.Open(string.format('%s.stations.kml', database_config['Path']), APRService.Modules.TextFile.OPEN_MODE_WRITE | APRService.Modules.TextFile.OPEN_MODE_TRUNCATE, APRService.Modules.TextFile.LINE_ENDING_LF);

	if text_file then
		APRService.Modules.TextFile.WriteLine(text_file, '<?xml version="1.0" encoding="UTF-8"?>');
		APRService.Modules.TextFile.WriteLine(text_file, '<kml xmlns="http://www.opengis.net/kml/2.2">');
		APRService.Modules.TextFile.WriteLine(text_file, '\t<Document>');

		IGateMapper.DB.EnumerateStations(function(callsign, first_seen_timestamp, last_seen_timestamp, latitude, longitude, altitude, packet_count_digi, packet_count_igate, is_location_set)
			if is_location_set then
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

		text_file = APRService.Modules.TextFile.Open(string.format('%s.digis.kml', database_config['Path']), APRService.Modules.TextFile.OPEN_MODE_WRITE | APRService.Modules.TextFile.OPEN_MODE_TRUNCATE, APRService.Modules.TextFile.LINE_ENDING_LF);

		if text_file then
			APRService.Modules.TextFile.WriteLine(text_file, '<?xml version="1.0" encoding="UTF-8"?>');
			APRService.Modules.TextFile.WriteLine(text_file, '<kml xmlns="http://www.opengis.net/kml/2.2">');
			APRService.Modules.TextFile.WriteLine(text_file, '\t<Document>');

			IGateMapper.DB.EnumerateStations(function(callsign, first_seen_timestamp, last_seen_timestamp, latitude, longitude, altitude, packet_count_digi, packet_count_igate, is_location_set)
				if is_location_set and (packet_count_digi ~= 0) then
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

			text_file = APRService.Modules.TextFile.Open(string.format('%s.igates.kml', database_config['Path']), APRService.Modules.TextFile.OPEN_MODE_WRITE | APRService.Modules.TextFile.OPEN_MODE_TRUNCATE, APRService.Modules.TextFile.LINE_ENDING_LF);

			if text_file then
				APRService.Modules.TextFile.WriteLine(text_file, '<?xml version="1.0" encoding="UTF-8"?>');
				APRService.Modules.TextFile.WriteLine(text_file, '<kml xmlns="http://www.opengis.net/kml/2.2">');
				APRService.Modules.TextFile.WriteLine(text_file, '\t<Document>');

				IGateMapper.DB.EnumerateStations(function(callsign, first_seen_timestamp, last_seen_timestamp, latitude, longitude, altitude, packet_count_digi, packet_count_igate, is_location_set)
					if is_location_set and (packet_count_igate ~= 0) then
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
	end

	return false;
end

-- @return exists, first_seen_timestamp, last_seen_timestamp, latitude, longitude, altitude, packet_count_digi, packet_count_igate, is_location_set
function IGateMapper.DB.GetStation(callsign)
	local station = IGateMapper.DB.Private.FindStation(callsign);

	if not station then
		return false;
	end

	return true, station[2], station[3], station[4], station[5], station[6], station[7], station[8], station[9];
end

function IGateMapper.DB.AddStation(callsign)
	IGateMapper.DB.Private.stations[IGateMapper.DB.Private.station_count] = { callsign, APRService.Modules.System.GetTimestamp(), APRService.Modules.System.GetTimestamp(), 0, 0, 0, 0, 0, false };
	IGateMapper.DB.Private.station_count                                  = IGateMapper.DB.Private.station_count + 1;
end

-- @param callback(callsign, first_seen_timestamp, last_seen_timestamp, latitude, longitude, altitude, packet_count_digi, packet_count_igate, is_location_set)->boolean
function IGateMapper.DB.EnumerateStations(callback)
	for station_id, station in pairs(IGateMapper.DB.Private.stations) do
		if not callback(station[1], station[2], station[3], station[4], station[5], station[6], station[7], station[8], station[9]) then
			break;
		end
	end
end

function IGateMapper.DB.SetStationLocation(callsign, latitude, longitude, altitude)
	local station = IGateMapper.DB.Private.FindStation(callsign);

	if not station then
		return false;
	end

	station[3] = APRService.Modules.System.GetTimestamp();
	station[4] = latitude;
	station[5] = longitude;
	station[6] = altitude;
	station[9] = true;

	return true;
end

function IGateMapper.DB.IncrementStationDigiPacketCount(callsign)
	local station = IGateMapper.DB.Private.FindStation(callsign);

	if not station then
		return false;
	end

	station[3] = APRService.Modules.System.GetTimestamp();
	station[7] = station[7] + 1;

	return true;
end

function IGateMapper.DB.IncrementStationIGatePacketCount(callsign)
	local station = IGateMapper.DB.Private.FindStation(callsign);

	if not station then
		return false;
	end

	station[3] = APRService.Modules.System.GetTimestamp();
	station[8] = station[8] + 1;

	return true;
end

IGateMapper.APRS = {};

function IGateMapper.APRS.Connect()
	return APRService.APRS.IS.Connect(IGateMapper.Service, aprs_is_config['Host'], aprs_is_config['Port'], aprs_is_config['Passcode']);
end

function IGateMapper.APRS.Disconnect()
	APRService.APRS.Disconnect(IGateMapper.Service);
end

IGateMapper.Private = {};

function IGateMapper.Private.AutoConnect()
	if not IGateMapper.APRS.Connect() then
		APRService.Console.WriteLine('Connection failed. Trying again in 3 seconds.');
		APRService.Events.Schedule(IGateMapper.Service, 3, function(service) IGateMapper.Private.AutoConnect(); end);
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
			APRService.Events.Schedule(service, 0, function(service) IGateMapper.Private.AutoConnect(); end);
		end
	end);

	APRService.Config.Events.SetOnReceivePacket(IGateMapper.ServiceConfig, function(service, station, tocall, path, igate, content)
		local station_exists, _ = IGateMapper.DB.GetStation(station);

		if not station_exists then
			IGateMapper.DB.AddStation(station);
		end

		if string.len(igate) ~= 0 then
			IGateMapper.DB.IncrementStationIGatePacketCount(igate);
		end
	end);

	APRService.Config.Events.SetOnReceivePosition(IGateMapper.ServiceConfig, function(service, station, path, igate, altitude, latitude, longitude, symbol_table, symbol_table_key, comment, flags)
		local station_exists, station_first_seen_timestamp, station_last_seen_timestamp, station_latitude, station_longitude, station_altitude, station_packet_count_digi, station_packet_count_igate, station_is_location_set = IGateMapper.DB.GetStation(station);

		IGateMapper.DB.SetStationLocation(station, latitude, longitude, altitude);

		if not station_exists or not station_is_location_set then
			APRService.Console.WriteLine(string.format('Discovered station %s at %.6f, %.6f', station, latitude, longitude));
		elseif station_is_location_set and ((station_latitude ~= latitude) or (station_longitude ~= longitude)) then
			APRService.Console.WriteLine(string.format('Updated position of station %s to %.6f, %.6f', station, latitude, longitude));
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

	IGateMapper.Private.AutoConnect();

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
