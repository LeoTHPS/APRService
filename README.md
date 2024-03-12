# APRService
###### An APRS client with a Lua 5.4 API

<hr />

### API
###### Note: KISS TNC and compressed position reports are not fully implemented.

- [C++](/APRService/aprservice.hpp)
- [Lua](/Services/APRService.lua)
- [Lua.Modules.ByteBuffer](/Services/APRService/Modules/ByteBuffer.lua)
- [Lua.Modules.Environment](/Services/APRService/Modules/Environment.lua)
- [Lua.Modules.File](/Services/APRService/Modules/File.lua)
- [Lua.Modules.GPIO](/Services/APRService/Modules/GPIO.lua)
- [Lua.Modules.HTTP](/Services/APRService/Modules/HTTP.lua)
- [Lua.Modules.I2C](/Services/APRService/Modules/I2C.lua)
- [Lua.Modules.Mutex](/Services/APRService/Modules/Mutex.lua)
- [Lua.Modules.Socket](/Services/APRService/Modules/Socket.lua)
- [Lua.Modules.SPI](/Services/APRService/Modules/SPI.lua)
- [Lua.Modules.SQLite3](/Services/APRService/Modules/SQLite3.lua)
- [Lua.Modules.System](/Services/APRService/Modules/System.lua)
- [Lua.Modules.TextFile](/Services/APRService/Modules/TextFile.lua)
- [Lua.Modules.Thread](/Services/APRService/Modules/Thread.lua)
- [Lua.Modules.Timer](/Services/APRService/Modules/Timer.lua)
- [Lua.Modules.UART](/Services/APRService/Modules/UART.lua)

### Dependencies
- [Lua 5.4](//github.com/lua/lua)
- [SQLite3](//github.com/sqlite/sqlite)
- [OpenSSL](//github.com/openssl/openssl)
- [AbstractionLayer](//github.com/LeoTHPS/AbstractionLayer)

### Quick Start
###### Note: The current Lua API has issues compiling on GCC 10.2 - You will need a more recent version until this is fixed.

#### Linux - Debian
```sh
# Install dependencies

apt install git liblua5.4-dev libssl-dev libsqlite3-dev

# Clone AbstractionLayer and set temporary environment variable

git clone https://github.com/LeoTHPS/AbstractionLayer AbstractionLayer
export AL_INCLUDE=../../AbstractionLayer

# Clone and build APRService

git clone https://github.com/LeoTHPS/APRService APRService
make -C APRService/APRService -e COMPILER=GNU PLATFORM=LINUX
```
#### Linux - Other
```sh
# Install dependencies
# - You need to install git, OpenSSL, SQLite3 and Lua 5.4 c development packages for your distribution

# Clone AbstractionLayer and set temporary environment variable

git clone https://github.com/LeoTHPS/AbstractionLayer AbstractionLayer
export AL_INCLUDE=../../AbstractionLayer

# Clone and build APRService

git clone https://github.com/LeoTHPS/APRService APRService
make -C APRService/APRService -e COMPILER=GNU PLATFORM=LINUX
```
#### Windows - MinGW64
```sh
# Install dependencies

pacman -S git mingw-w64-x86_64-lua mingw-w64-x86_64-openssl mingw-w64-x86_64-sqlite3

# Clone AbstractionLayer and set temporary environment variable

git clone https://github.com/LeoTHPS/AbstractionLayer AbstractionLayer
export AL_INCLUDE=../../AbstractionLayer

# Clone and build APRService

git clone https://github.com/LeoTHPS/APRService APRService
make -C APRService/APRService -e COMPILER=GNU PLATFORM=WINDOWS
```
#### Windows - Visual Studio
Instructions coming soon.
