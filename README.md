# APRService
###### An APRS client with a Lua 5.4 API

<hr />

### API
#### Core
```lua
APRService.FLAG_NONE
APRService.FLAG_STOP_ON_APRS_DISCONNECT

-- @return service
function APRService.Init(config)
function APRService.Deinit(service)
function APRService.IsRunning(service)
function APRService.Run(service, tick_rate, flags)
function APRService.Stop(service)
```
##### APRS
```lua
APRService.APRS.PACKET_TYPE_UNKNOWN
APRService.APRS.PACKET_TYPE_MESSAGE
APRService.APRS.PACKET_TYPE_POSITION
APRService.APRS.PACKET_TYPE_TELEMETRY

APRService.APRS.POSITION_FLAG_NONE
APRService.APRS.POSITION_FLAG_COMPRESSED
APRService.APRS.POSITION_FLAG_MESSAGING_ENABLED

APRService.APRS.CONNECTION_TYPE_NONE
APRService.APRS.CONNECTION_TYPE_APRS_IS
APRService.APRS.CONNECTION_TYPE_KISS_TCP
APRService.APRS.CONNECTION_TYPE_KISS_SERIAL

APRService.APRS.DISCONNECT_REASON_UNDEFINED
APRService.APRS.DISCONNECT_REASON_USER_REQUESTED
APRService.APRS.DISCONNECT_REASON_CONNECTION_LOST
APRService.APRS.DISCONNECT_REASON_AUTHENTICATION_FAILED

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
```
##### Config
```lua
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
```
##### Math
```lua
APRService.Math.MEASUREMENT_TYPE_FEET
APRService.Math.MEASUREMENT_TYPE_MILES
APRService.Math.MEASUREMENT_TYPE_METERS
APRService.Math.MEASUREMENT_TYPE_KILOMETERS

function APRService.Math.GetDistanceBetweenPoints(latitude1, longitude1, latitude2, longitude2, measurement_type)
```
##### Events
```lua
function APRService.Events.GetCount(service)
function APRService.Events.Clear(service)
-- @param handler(service)
function APRService.Events.Schedule(service, seconds, handler)
```
##### Console
```lua
function APRService.Console.SetTitle(value)
-- @return success, char
function APRService.Console.Read()
-- @return success, string
function APRService.Console.ReadLine()
function APRService.Console.Write(value)
function APRService.Console.WriteLine(value)
```
##### Commands
```lua
function APRService.Commands.Execute(service, sender, message)
-- @param handler(service, sender, command_name, command_params)
function APRService.Commands.Register(service, name, handler)
```
#### Modules
##### Byte Buffer
```lua
APRService.Modules.ByteBuffer.ENDIAN_BIG
APRService.Modules.ByteBuffer.ENDIAN_LITTLE
APRService.Modules.ByteBuffer.ENDIAN_MACHINE

-- @return byte_buffer
function APRService.Modules.ByteBuffer.Create(endian, capacity)
function APRService.Modules.ByteBuffer.Destroy(byte_buffer)

function APRService.Modules.ByteBuffer.GetSize(byte_buffer)
function APRService.Modules.ByteBuffer.GetBuffer(byte_buffer)
function APRService.Modules.ByteBuffer.GetCapacity(byte_buffer)

function APRService.Modules.ByteBuffer.Clear(byte_buffer)

-- @return success, byte_buffer
function APRService.Modules.ByteBuffer.Read(byte_buffer, size)
-- @return success, value
function APRService.Modules.ByteBuffer.ReadInt8(byte_buffer)
-- @return success, value
function APRService.Modules.ByteBuffer.ReadInt16(byte_buffer)
-- @return success, value
function APRService.Modules.ByteBuffer.ReadInt32(byte_buffer)
-- @return success, value
function APRService.Modules.ByteBuffer.ReadInt64(byte_buffer)
-- @return success, value
function APRService.Modules.ByteBuffer.ReadUInt8(byte_buffer)
-- @return success, value
function APRService.Modules.ByteBuffer.ReadUInt16(byte_buffer)
-- @return success, value
function APRService.Modules.ByteBuffer.ReadUInt32(byte_buffer)
-- @return success, value
function APRService.Modules.ByteBuffer.ReadUInt64(byte_buffer)
-- @return success, value
function APRService.Modules.ByteBuffer.ReadFloat(byte_buffer)
-- @return success, value
function APRService.Modules.ByteBuffer.ReadDouble(byte_buffer)
-- @return success, value
function APRService.Modules.ByteBuffer.ReadString(byte_buffer)
-- @return success, value
function APRService.Modules.ByteBuffer.ReadWString(byte_buffer)
-- @return success, value
function APRService.Modules.ByteBuffer.ReadBoolean(byte_buffer)

function APRService.Modules.ByteBuffer.Write(byte_buffer, buffer, size)
function APRService.Modules.ByteBuffer.WriteInt8(byte_buffer, value)
function APRService.Modules.ByteBuffer.WriteInt16(byte_buffer, value)
function APRService.Modules.ByteBuffer.WriteInt32(byte_buffer, value)
function APRService.Modules.ByteBuffer.WriteInt64(byte_buffer, value)
function APRService.Modules.ByteBuffer.WriteUInt8(byte_buffer, value)
function APRService.Modules.ByteBuffer.WriteUInt16(byte_buffer, value)
function APRService.Modules.ByteBuffer.WriteUInt32(byte_buffer, value)
function APRService.Modules.ByteBuffer.WriteUInt64(byte_buffer, value)
function APRService.Modules.ByteBuffer.WriteFloat(byte_buffer, value)
function APRService.Modules.ByteBuffer.WriteDouble(byte_buffer, value)
function APRService.Modules.ByteBuffer.WriteString(byte_buffer, value)
function APRService.Modules.ByteBuffer.WriteWString(byte_buffer, value)
function APRService.Modules.ByteBuffer.WriteBoolean(byte_buffer, value)
```
##### CSV
```lua
-- Coming soon
```
##### Environment
```lua
-- @return exists, value
function APRService.Modules.Environment.Get(name)
function APRService.Modules.Environment.Set(name, value)
function APRService.Modules.Environment.Delete(name)
-- @param callback(name, value)->bool
function APRService.Modules.Environment.Enumerate(callback)
```
##### File
```lua
-- Coming soon
```
##### GPIO
```lua

APRService.Modules.GPIO.PIN_EDGE_BOTH
APRService.Modules.GPIO.PIN_EDGE_RISING
APRService.Modules.GPIO.PIN_EDGE_FALLING

APRService.Modules.GPIO.PIN_VALUE_LOW
APRService.Modules.GPIO.PIN_VALUE_HIGH

APRService.Modules.GPIO.PIN_DIRECTION_IN
APRService.Modules.GPIO.PIN_DIRECTION_OUT

-- @return pin
function APRService.Modules.GPIO.OpenPin(bus, pin, direction, value)
function APRService.Modules.GPIO.ClosePin(pin)

-- @return success, value
function APRService.Modules.GPIO.Pin.Read(pin)
function APRService.Modules.GPIO.Pin.Write(pin, value)
-- @return success, timeout
function APRService.Modules.GPIO.Pin.WaitForEdge(pin, edge, max_wait_time_ms)
function APRService.Modules.GPIO.Pin.SetPullUp(pin)
function APRService.Modules.GPIO.Pin.SetPullDown(pin)
function APRService.Modules.GPIO.Pin.GetDirection(pin)
function APRService.Modules.GPIO.Pin.SetDirection(pin, direction, value)
```
##### HTTP
```lua
-- Coming soon
```
##### I2C
```lua
-- Coming soon
```
##### INI
```lua
-- Coming soon
```
##### IRC
```lua
-- Coming soon
```
##### JSON
```lua
-- Coming soon
```
##### Mutex
```lua
-- @return mutex
function APRService.Modules.Mutex.Create()
function APRService.Modules.Mutex.Destroy(mutex)
function APRService.Modules.Mutex.Lock(mutex)
function APRService.Modules.Mutex.Unlock(mutex)
```
##### Process
```lua
-- Coming soon
```
##### Socket
```lua
-- Coming soon
```
##### SPI
```lua
-- Coming soon
```
##### SQLite3
```lua
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
function APRService.Modules.SQLite3.Database.ExecuteNonQuery(database, query)
function APRService.Modules.SQLite3.QueryResult.GetSize(query_result)
-- @return query_result_row
function APRService.Modules.SQLite3.QueryResult.GetRow(query_result, index)
function APRService.Modules.SQLite3.QueryResult.Release(query_result)
function APRService.Modules.SQLite3.QueryResult.Row.GetSize(query_result_row)
function APRService.Modules.SQLite3.QueryResult.Row.GetValue(query_result_row, index)
function APRService.Modules.SQLite3.QueryResult.Row.GetColumn(query_result_row, index)
```
##### SSH
```lua
-- Coming soon
```
##### System
```lua
APRService.Modules.System.PLATFORM_DEBUG
APRService.Modules.System.PLATFORM_GNU
APRService.Modules.System.PLATFORM_MSVC
APRService.Modules.System.PLATFORM_CLANG
APRService.Modules.System.PLATFORM_ARM
APRService.Modules.System.PLATFORM_ARM64
APRService.Modules.System.PLATFORM_X86
APRService.Modules.System.PLATFORM_X86_64
APRService.Modules.System.PLATFORM_PICO
APRService.Modules.System.PLATFORM_PICO_W
APRService.Modules.System.PLATFORM_RP2040
APRService.Modules.System.PLATFORM_LINUX
APRService.Modules.System.PLATFORM_MINGW
APRService.Modules.System.PLATFORM_WINDOWS
APRService.Modules.System.PLATFORM_MACHINE

function APRService.Modules.System.GetPlatform()
```
##### Thread
```lua
-- Coming soon
```
##### Timer
```lua
-- @return timer
function APRService.Modules.Timer.Create()
function APRService.Modules.Timer.Destroy(timer)
function APRService.Modules.Timer.Reset(timer)
function APRService.Modules.Timer.GetElapsedMS(timer)
function APRService.Modules.Timer.GetElapsedUS(timer)
```
##### UART
```lua
APRService.Modules.UART.DEVICE_FLAG_NONE
APRService.Modules.UART.DEVICE_FLAG_PARITY
APRService.Modules.UART.DEVICE_FLAG_PARITY_ODD
APRService.Modules.UART.DEVICE_FLAG_PARITY_EVEN
APRService.Modules.UART.DEVICE_FLAG_USE_2_STOP_BITS

-- @return device
function APRService.Modules.UART.OpenDevice(path, speed, flags)
function APRService.Modules.UART.CloseDevice(device)

-- @return success, byte_buffer
function APRService.Modules.UART.Device.Read(device, buffer_size)
function APRService.Modules.UART.Device.Write(device, byte_buffer)

-- @return success, would_block, byte_buffer
function APRService.Modules.UART.Device.TryRead(device, buffer_size)
```

### Dependencies
- [Lua 5.4](//github.com/lua/lua)
- [SQLite3](//github.com/sqlite/sqlite)
- [AbstractionLayer](//github.com/LeoTHPS/AbstractionLayer)

### Quick Start
#### Linux - Debian
```sh
# Install dependencies

apt install git liblua5.4-dev libsqlite3-dev

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
# - You need to install git, SQLite3 and Lua 5.4 c development packages for your distribution

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

pacman -S git mingw-w64-x86_64-lua mingw-w64-x86_64-sqlite3

# Clone AbstractionLayer and set temporary environment variable

git clone https://github.com/LeoTHPS/AbstractionLayer AbstractionLayer
export AL_INCLUDE=../../../../AbstractionLayer

# Clone and build APRService

git clone https://github.com/LeoTHPS/APRService APRService
make -C APRService/APRService -e COMPILER=GNU PLATFORM=WINDOWS
```
#### Windows - Visual Studio
Instructions coming soon.
