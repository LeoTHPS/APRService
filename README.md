# APRService
###### An APRS client with a Lua 5.4 API

<hr />

### Lua API

```lua
APRService.FLAG_NONE
APRService.FLAG_STOP_ON_APRS_DISCONNECT

APRService.MEASUREMENT_TYPE_FEET
APRService.MEASUREMENT_TYPE_MILES
APRService.MEASUREMENT_TYPE_METERS
APRService.MEASUREMENT_TYPE_KILOMETERS

APRService.APRS_PACKET_TYPE_UNKNOWN
APRService.APRS_PACKET_TYPE_MESSAGE
APRService.APRS_PACKET_TYPE_POSITION
APRService.APRS_PACKET_TYPE_TELEMETRY

APRService.APRS_POSITION_FLAG_NONE
APRService.APRS_POSITION_FLAG_COMPRESSED
APRService.APRS_POSITION_FLAG_MESSAGING_ENABLED

APRService.APRS_CONNECTION_TYPE_NONE
APRService.APRS_CONNECTION_TYPE_APRS_IS
APRService.APRS_CONNECTION_TYPE_KISS_TCP
APRService.APRS_CONNECTION_TYPE_KISS_SERIAL

APRService.APRS_DISCONNECT_REASON_UNDEFINED
APRService.APRS_DISCONNECT_REASON_USER_REQUESTED
APRService.APRS_DISCONNECT_REASON_CONNECTION_LOST
APRService.APRS_DISCONNECT_REASON_AUTHENTICATION_FAILED

-- @return service
function APRService.Init(config)
function APRService.Deinit(service)
function APRService.IsRunning(service)
function APRService.Run(service, tick_rate, flags)
function APRService.Stop(service)

function APRService.APRS.IsConnected(service)
function APRService.APRS.IS.Connect(service, remote_host, remote_port, passcode)
function APRService.APRS.KISS.Tcp.Connect(service, remote_host, remote_port)
function APRService.APRS.KISS.Serial.Connect(service, device)
function APRService.APRS.Disconnect(service)
-- @param filter(service, station, tocall, path, igate, content)->bool
-- @param callback(service, station, tocall, path, igate, content)
function APRService.APRS.AddPacketMonitor(service, filter, callback)
-- @return encoding_failed, connection_closed
function APRService.APRS.SendMessage(service, destination, content)
-- @return encoding_failed, connection_closed
function APRService.APRS.SendPosition(service, altitude, latitude, longitude, comment)
-- @return encoding_failed, connection_closed
function APRService.APRS.SendTelemetry(service, analog_1, analog_2, analog_3, analog_4, analog_5, digital_1, digital_2, digital_3, digital_4, digital_5, digital_6, digital_7, digital_8)
-- @param callback(service)
-- @return encoding_failed, connection_closed
function APRService.APRS.BeginSendMessage(service, destination, content, callback)

-- @return config
function APRService.Config.Init()
function APRService.Config.Deinit(config)
function APRService.Config.APRS.IsMonitorModeEnabled(config)
function APRService.Config.APRS.EnableMonitorMode(config, value)
function APRService.Config.APRS.GetPath(config)
function APRService.Config.APRS.SetPath(config, value)
function APRService.Config.APRS.GetStation(config)
function APRService.Config.APRS.SetStation(config, value)
function APRService.Config.APRS.GetSymbolTable(config)
function APRService.Config.APRS.SetSymbolTable(config, value)
function APRService.Config.APRS.GetSymbolTableKey(config)
function APRService.Config.APRS.SetSymbolTableKey(config, value)
-- @param handler(service, type)
function APRService.Config.Events.SetOnConnect(config, handler)
-- @param handler(service, reason)
function APRService.Config.Events.SetOnDisconnect(config, handler)
-- @param handler(service, value)
function APRService.Config.Events.SetOnSend(config, handler)
-- @param handler(service, value)
function APRService.Config.Events.SetOnReceive(config, handler)
-- @param handler(service, station, tocall, path, igate, content)
function APRService.Config.Events.SetOnSendPacket(config, handler)
-- @param handler(service, station, tocall, path, igate, content)
function APRService.Config.Events.SetOnReceivePacket(config, handler)
-- @param handler(service, station, path, igate, destination, content)
function APRService.Config.Events.SetOnSendMessage(config, handler)
-- @param handler(service, station, path, igate, destination, content)
function APRService.Config.Events.SetOnReceiveMessage(config, handler)
-- @param handler(service, station, path, igate, altitude, latitude, longitude, symbol_table, symbol_table_key, comment, flags)
function APRService.Config.Events.SetOnSendPosition(config, handler)
-- @param handler(service, station, path, igate, altitude, latitude, longitude, symbol_table, symbol_table_key, comment, flags)
function APRService.Config.Events.SetOnReceivePosition(config, handler)
-- @param handler(service, station, tocall, path, igate, analog_1, analog_2, analog_3, analog_4, analog_5, digital_1, digital_2, digital_3, digital_4, digital_5, digital_6, digital_7, digital_8)
function APRService.Config.Events.SetOnSendTelemetry(config, handler)
-- @param handler(service, station, tocall, path, igate, analog_1, analog_2, analog_3, analog_4, analog_5, digital_1, digital_2, digital_3, digital_4, digital_5, digital_6, digital_7, digital_8)
function APRService.Config.Events.SetOnReceiveTelemetry(config, handler)
-- @param handler(service, station, tocall, path, igate, content, type)
function APRService.Config.Events.SetOnReceiveInvalidPacket(config, handler)

function APRService.Math.GetDistanceBetweenPoints(latitude1, longitude1, latitude2, longitude2, measurement_type)

function APRService.Events.GetCount(service)
function APRService.Events.Clear(service)
-- @param handler(service)
function APRService.Events.Schedule(service, seconds, handler)

function APRService.Console.SetTitle(value)
-- @return success, char
function APRService.Console.Read()
-- @return success, string
function APRService.Console.ReadLine()
function APRService.Console.Write(value)
function APRService.Console.WriteLine(value)

function APRService.Commands.Execute(service, sender, message)
-- @param handler(service, sender, command_name, command_params)
function APRService.Commands.Register(service, name, handler)

APRService.Modules.SQLite3.FLAG_NONE
APRService.Modules.SQLite3.FLAG_URI
APRService.Modules.SQLite3.FLAG_CREATE
APRService.Modules.SQLite3.FLAG_READ_ONLY
APRService.Modules.SQLite3.FLAG_READ_WRITE
APRService.Modules.SQLite3.FLAG_MEMORY
APRService.Modules.SQLite3.FLAG_NO_MUTEX
APRService.Modules.SQLite3.FLAG_FULL_MUTEX
APRService.Modules.SQLite3.FLAG_NO_FOLLOW
APRService.Modules.SQLite3.FLAG_SHARED_CACHE
APRService.Modules.SQLite3.FLAG_PRIVATE_CACHE

-- @return database
function APRService.Modules.SQLite3.Database.Open(path, flags)
function APRService.Modules.SQLite3.Database.Close(database)
-- @return query_result
function APRService.Modules.SQLite3.Database.ExecuteQuery(database, query)
function APRService.Modules.SQLite3.QueryResult.GetSize(query_result)
-- @return query_result_row
function APRService.Modules.SQLite3.QueryResult.GetRow(query_result, index)
function APRService.Modules.SQLite3.QueryResult.Release(query_result)
function APRService.Modules.SQLite3.QueryResult.Row.GetSize(query_result_row)
function APRService.Modules.SQLite3.QueryResult.Row.GetValue(query_result_row, index)
function APRService.Modules.SQLite3.QueryResult.Row.GetColumn(query_result_row, index)
```

### Dependencies

- [Lua 5.4](//github.com/lua/lua)
- [SQLite3](//github.com/sqlite/sqlite)
- [AbstractionLayer](//github.com/LeoTHPS/AbstractionLayer)

### Quick Start

#### Linux - Debian

```sh
# Install dependencies

apt install git liblua5.4-dev

# Clone AbstractionLayer and set temporary environment variable

git clone https://github.com/LeoTHPS/AbstractionLayer AbstractionLayer
export AL_INCLUDE=../../../../AbstractionLayer

# Clone and build APRService

git clone https://github.com/LeoTHPS/APRService APRService
make -C APRService/APRService -e COMPILER=GNU PLATFORM=LINUX
```

#### Linux - Other

```sh
# Install dependencies
# - You need to install git and the Lua 5.4 c development packages for your distribution

# Clone AbstractionLayer and set temporary environment variable

git clone https://github.com/LeoTHPS/AbstractionLayer AbstractionLayer
export AL_INCLUDE=../../../../AbstractionLayer

# Clone and build APRService

git clone https://github.com/LeoTHPS/APRService APRService
make -C APRService/APRService -e COMPILER=GNU PLATFORM=LINUX
```

#### Windows - MinGW64

```sh
# Install dependencies

pacman -S git mingw-w64-x86_64-lua

# Clone AbstractionLayer and set temporary environment variable

git clone https://github.com/LeoTHPS/AbstractionLayer AbstractionLayer
export AL_INCLUDE=../../../../AbstractionLayer

# Clone and build APRService

git clone https://github.com/LeoTHPS/APRService APRService
make -C APRService/APRService -e COMPILER=GNU PLATFORM=WINDOWS
```

#### Windows - Visual Studio

1. Create a solution with the files in this repository.
2. Download Lua 5.4 and add all src files to the solution except lua.c and luac.c.
3. Download AbstractionLayer and add its contents to the solution's include path.
4. Build.
