# APRService
###### An APRS client with a Lua 5.4 API

<hr />

### API

- [C++](/API.cpp.md)
- [Lua](/API.lua.md)

### Dependencies
- [Lua 5.4](//github.com/lua/lua)
- [SQLite3](//github.com/sqlite/sqlite)
- [OpenSSL](//github.com/openssl/openssl)
- [AbstractionLayer](//github.com/LeoTHPS/AbstractionLayer)

### Quick Start
#### Linux - Debian
```sh
# Install dependencies

apt install git liblua5.4-dev libssl-dev libsqlite3-dev

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
# - You need to install git, OpenSSL, SQLite3 and Lua 5.4 c development packages for your distribution

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

pacman -S git mingw-w64-x86_64-lua mingw-w64-x86_64-openssl mingw-w64-x86_64-sqlite3

# Clone AbstractionLayer and set temporary environment variable

git clone https://github.com/LeoTHPS/AbstractionLayer AbstractionLayer
export AL_INCLUDE=../../../../AbstractionLayer

# Clone and build APRService

git clone https://github.com/LeoTHPS/APRService APRService
make -C APRService/APRService -e COMPILER=GNU PLATFORM=WINDOWS
```
#### Windows - Visual Studio
Instructions coming soon.
