#pragma once
#include <map>
#include <list>
#include <array>
#include <queue>
#include <ctime>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <utility>
#include <exception>
#include <functional>

#if defined(APRSERVICE_WIN32)
	#include <WinSock2.h>

	#undef SendMessage
#endif

namespace APRService
{
	enum POSITION_FLAGS
	{
		POSITION_FLAG_COMPRESSED        = 0x1,
		POSITION_FLAG_MESSAGING_ENABLED = 0x2,
	};

	enum class PacketTypes
	{
		Unknown,
		Message,
		Weather,
		Position,
		Telemetry
	};

	typedef std::array<std::string, 8>  Path;

	typedef std::array<float, 5>        TelemetryAnalog;
	typedef std::uint8_t                TelemetryDigital;

	struct Packet
	{
		PacketTypes      Type;
		APRService::Path Path;
		std::string      IGate;
		std::string      ToCall;
		std::string      Sender;
		std::string      Content;
		std::string      QConstruct;
	};

	struct Message
		: public Packet
	{
		std::string ID;
		std::string Body;
		std::string Destination;
	};

	struct Weather
		: public Packet
	{
		tm            Time;

		std::uint16_t WindSpeed;
		std::uint16_t WindSpeedGust;
		std::uint16_t WindDirection;

		std::uint16_t RainfallLastHour;
		std::uint16_t RainfallLast24Hours;
		std::uint16_t RainfallSinceMidnight;

		std::uint8_t  Humidity;
		std::int16_t  Temperature;
		std::uint32_t BarometricPressure;
	};

	struct Position
		: public Packet
	{
		int            Flags;

		std::uint16_t  Speed;
		std::uint16_t  Course;
		std::int32_t   Altitude;
		float          Latitude;
		float          Longitude;

		std::string    Comment;

		char           SymbolTable;
		char           SymbolTableKey;

		float CalculateDistance(float latitude, float longitude) const;
		float CalculateDistance3D(float latitude, float longitude, float altitude) const;
	};

	struct Telemetry
		: public Packet
	{
		TelemetryAnalog  Analog;
		TelemetryDigital Digital;
		std::uint16_t    Sequence;
	};

	class Exception
		: public std::exception
	{
		std::string message;

	public:
		explicit Exception(std::string&& message)
			: message(std::move(message))
		{
		}

		virtual const char* what() const noexcept override
		{
			return message.c_str();
		}
	};

	class RegexException
		: public Exception
	{
	public:
		explicit RegexException(std::string&& what)
			: Exception(std::move(what))
		{
		}
	};

	class SystemException
		: public Exception
	{
	public:
		explicit SystemException(const std::string& function);

		SystemException(const std::string& function, int error);
	};

	class AuthFailedException
		: public Exception
	{
	public:
		explicit AuthFailedException(const std::string& message)
			: Exception("Authentication failed: " + message)
		{
		}
	};

	class InvalidPathException
		: public Exception
	{
	public:
		explicit InvalidPathException(const Path& path)
			: Exception("Invalid Path: " + Path_ToString(path))
		{
		}

	private:
		static std::string Path_ToString(const Path& path);
	};

	class InvalidFormatException
		: public Exception
	{
	public:
		explicit InvalidFormatException(const std::string& format)
			: Exception("Invalid format: " + format)
		{
		}
	};

	class InvalidPacketException
		: public Exception
	{
	public:
		InvalidPacketException(PacketTypes type, const std::string& field, const std::string& value)
			: Exception("Invalid Packet [Type: " + PacketTypesToString(type) + ", Field: " + field + ", Value: " + value + "]")
		{
		}

	private:
		static std::string PacketTypesToString(PacketTypes type);
	};

	class InvalidStationException
		: public Exception
	{
	public:
		explicit InvalidStationException(const std::string& station)
			: Exception("Invalid Station: " + station)
		{
		}
	};

	class InvalidSymbolTableException
		: public Exception
	{
	public:
		explicit InvalidSymbolTableException(char table)
			: Exception("Invalid symbol table: " + table)
		{
		}
	};

	class InvalidSymbolTableKeyException
		: public Exception
	{
	public:
		explicit InvalidSymbolTableKeyException(char key)
			: Exception("Invalid symbol table key: " + key)
		{
		}
	};

	class ConnectionFailedException
		: public Exception
	{
	public:
		explicit ConnectionFailedException(std::string&& message)
			: Exception("Connection failed: " + message)
		{
		}
	};

	template<typename F>
	using EventHandler = std::function<F>;

	template<typename F>
	class Event;
	template<typename T, typename ... TArgs>
	class Event<T(TArgs ...)>
	{
		struct Handler
		{
			EventHandler<T(TArgs ...)>        Function;
			const EventHandler<T(TArgs ...)>* FunctionPtr;
		};

		std::list<Handler> handlers;

	public:
		Event()
		{
		}

		virtual ~Event()
		{
		}

		void Clear()
		{
			handlers.clear();
		}

		auto Execute(TArgs ... args) const
		{
			if constexpr (std::is_same<T, void>::value)
				for (auto& handler : handlers)
					handler.Function(std::forward<TArgs>(args) ...);
			else
			{
				T value;

				for (auto& handler : handlers)
					if (!(value = handler.Function(std::forward<TArgs>(args) ...)))
						break;

				return value;
			}
		}
		template<typename T_RETURN = typename std::enable_if<true, T>::type>
		auto Execute(TArgs ... args, T_RETURN&& false_value = T_RETURN()) const
		{
			if constexpr (std::is_same<T, void>::value)
				for (auto& handler : handlers)
					handler.Function(std::forward<TArgs>(args) ...);
			else
			{
				T value;

				for (auto& handler : handlers)
					if ((value = handler.Function(std::forward<TArgs>(args) ...)) == false_value)
						break;

				return value;
			}
		}

		void Register(EventHandler<T(TArgs ...)>&& handler)
		{
			handlers.push_front({ .Function = std::move(handler), .FunctionPtr = nullptr });
		}
		void Register(const EventHandler<T(TArgs ...)>& handler)
		{
			handlers.push_front({ .Function = handler, .FunctionPtr = &handler });
		}
		void Unregister(const EventHandler<T(TArgs ...)>& handler)
		{
			for (auto it = handlers.begin(); it != handlers.end(); ++it)
			{
				if (it->FunctionPtr == &handler)
				{
					handlers.erase(it);

					break;
				}
			}
		}
	};
	template<typename T, typename ... TArgs>
	class Event<EventHandler<T(TArgs ...)>>
		: public Event<T(TArgs ...)>
	{
	public:
		using Event<T(TArgs ...)>::Event;
	};

	typedef EventHandler<void()>                                                   OnConnectHandler;
	typedef EventHandler<void()>                                                   OnDisconnectHandler;
	typedef EventHandler<void(const std::string& message)>                         OnAuthenticateHandler;

	typedef EventHandler<void(const std::string& raw)>                             OnSendHandler;
	typedef EventHandler<void(const std::string& raw)>                             OnReceiveHandler;

	typedef EventHandler<void(const std::string& raw, const Exception& exception)> OnDecodeErrorHandler;

	typedef EventHandler<void(const Packet& packet)>                               OnReceivePacketHandler;
	typedef EventHandler<void(const Message& message)>                             OnReceiveMessageHandler;
	typedef EventHandler<void(const Weather& weather)>                             OnReceiveWeatherHandler;
	typedef EventHandler<void(const Position& position)>                           OnReceivePositionHandler;
	typedef EventHandler<void(const Telemetry& telemetry)>                         OnReceiveTelemetryHandler;

	class Client
	{
		enum IO_RESULTS
		{
			IO_RESULT_SUCCESS,
			IO_RESULT_DISCONNECT,
			IO_RESULT_WOULD_BLOCK
		};

		enum AUTH_STATES
		{
			AUTH_STATE_NONE,
			AUTH_STATE_SENT,
			AUTH_STATE_RECEIVED
		};

		struct Auth
		{
			bool        Success;
			bool        Verified;

			std::string Message;
		};

		struct SendQueueEntry
		{
			std::string Buffer;
			std::size_t Offset;
			std::size_t OffsetEOL;
		};

		class DNS
		{
			DNS() = delete;

		public:
			struct Entry;

			// @throw Exception
			// @return false on not found
			static bool Resolve(const std::string& host, Entry& entry);
		};

#if defined(APRSERVICE_WIN32)
		class WinSock
		{
			WinSock() = delete;

		public:
			// @throw Exception
			static void Load();
			static void Unload();
		};
#endif

		class TcpSocket
		{
			bool   is_connected = false;

#if defined(APRSERVICE_UNIX)
			int    handle;
#elif defined(APRSERVICE_WIN32)
			SOCKET handle;
#endif

			TcpSocket(TcpSocket&&) = delete;
			TcpSocket(const TcpSocket&) = delete;

		public:
			TcpSocket()
			{
			}

			~TcpSocket()
			{
				if (IsConnected())
					Disconnect();
			}

			bool IsConnected() const
			{
				return is_connected;
			}

			// @throw Exception
			void Connect(const std::string& host, std::uint16_t port);
			void Disconnect();

			// @throw Exception
			// @return false on connection closed
			bool Send(const void* buffer, std::size_t size, std::size_t& number_of_bytes_sent);

			// @throw Exception
			// @return false on connection closed
			bool Receive(void* buffer, std::size_t size, std::size_t& number_of_bytes_received);
		};

		static constexpr const char  EOL[]    = { '\r', '\n' };
		static constexpr std::size_t EOL_SIZE = sizeof(EOL);

		bool                       is_read_only            = false;
		bool                       is_connected            = false;
		bool                       is_auto_ack_enabled     = true;
		bool                       is_messaging_enabled    = true;
		bool                       is_compression_enabled  = false;
		bool                       is_monitor_mode_enabled = false;

		Path                       path;
		TcpSocket*                 socket;
		const std::string          station;
		char                       symbol_table;
		char                       symbol_table_key;

		Auth                       auth;
		AUTH_STATES                auth_state;

		std::queue<SendQueueEntry> send_queue;

		std::vector<char>          receive_buffer;
		std::size_t                receive_buffer_offset = 0;
		std::string                receive_buffer_string;
		Packet                     receive_buffer_packet;

		std::uint16_t              message_ack_counter = 0;
		std::uint16_t              telemetry_sequence_counter = 0;

		Client(Client&&) = delete;
		Client(const Client&) = delete;

	public:
		Event<OnConnectHandler>           OnConnect;
		Event<OnDisconnectHandler>        OnDisconnect;
		Event<OnAuthenticateHandler>      OnAuthenticate;

		Event<OnDecodeErrorHandler>       OnDecodeError;

		Event<OnSendHandler>              OnSend;
		Event<OnReceiveHandler>           OnReceive;

		Event<OnReceivePacketHandler>     OnReceivePacket;
		Event<OnReceiveMessageHandler>    OnReceiveMessage;
		Event<OnReceiveWeatherHandler>    OnReceiveWeather;
		Event<OnReceivePositionHandler>   OnReceivePosition;
		Event<OnReceiveTelemetryHandler>  OnReceiveTelemetry;

		// @throw Exception
		Client(std::string&& station, Path&& path, char symbol_table, char symbol_table_key);

		virtual ~Client()
		{
			if (IsConnected())
				Disconnect();
		}

		bool IsReadOnly() const
		{
			return is_read_only;
		}

		bool IsConnected() const
		{
			return is_connected;
		}

		bool IsAuthenticated() const
		{
			return (auth_state == AUTH_STATE_RECEIVED) && auth.Success;
		}

		bool IsAuthenticating() const
		{
			return auth_state == AUTH_STATE_SENT;
		}

		bool IsAutoAckEnabled() const
		{
			return is_auto_ack_enabled;
		}

		bool IsMessagingEnabled() const
		{
			return is_messaging_enabled;
		}

		bool IsMonitorModeEnabled() const
		{
			return is_monitor_mode_enabled;
		}

		bool IsCompressionEnabled() const
		{
			return is_compression_enabled;
		}

		auto& GetPath() const
		{
			return path;
		}

		auto& GetStation() const
		{
			return station;
		}

		auto GetSymbolTable() const
		{
			return symbol_table;
		}

		auto GetSymbolTableKey() const
		{
			return symbol_table_key;
		}

		void SetPath(Path&& path)
		{
			if (!Path_IsValid(path))
				throw InvalidPathException(path);

			this->path = std::move(path);
		}

		void SetSymbol(char table, char key)
		{
			if (!SymbolTable_IsValid(table))
				throw InvalidSymbolTableException(table);

			if (!SymbolTableKey_IsValid(table, key))
				throw InvalidSymbolTableKeyException(key);

			symbol_table     = table;
			symbol_table_key = key;
		}

		void EnableAutoAck(bool set = true)
		{
			is_auto_ack_enabled = set;
		}

		void EnableMessaging(bool set = true)
		{
			is_messaging_enabled = set;
		}

		void EnableMonitorMode(bool set = true)
		{
			is_monitor_mode_enabled = set;
		}

		void EnableCompression(bool set = true)
		{
			is_compression_enabled = set;
		}

		// @throw Exception
		void Connect(const std::string& host, std::uint16_t port, std::int32_t passcode);
		void Disconnect();

		void Send(std::string&& raw);
		void Send(const std::string& raw)
		{
			Send(std::string(raw));
		}

		// @throw Exception
		void SendPacket(const std::string& content);

		// @throw Exception
		void SendMessage(const std::string& destination, const std::string& message);
		// @throw Exception
		void SendMessage(const std::string& destination, const std::string& message, const std::string& id);

		// @throw Exception
		void SendWeather(std::uint16_t wind_speed, std::uint16_t wind_speed_gust, std::uint16_t wind_direction, std::uint16_t rainfall_last_hour, std::uint16_t rainfall_last_24_hours, std::uint16_t rainfall_since_midnight, std::uint8_t humidity, std::int16_t temperature, std::uint32_t barometric_pressure, const std::string& type);

		// @throw Exception
		void SendPosition(std::uint16_t speed, std::uint16_t course, std::int32_t altitude, float latitude, float longitude, const std::string& comment = "");

		// @throw Exception
		void SendTelemetry(const TelemetryAnalog& analog, TelemetryDigital digital);
		// @throw Exception
		void SendTelemetry(const TelemetryAnalog& analog, TelemetryDigital digital, std::uint16_t sequence);

		// @throw Exception
		// @return false on connection closed
		virtual bool Update();

	protected:
		// @throw Exception
		virtual void HandlePacket(const std::string& raw, Packet& packet);
		// @throw Exception
		virtual void HandleMessage(const std::string& raw, Message& message);
		// @throw Exception
		virtual void HandleWeather(const std::string& raw, Weather& weather);
		// @throw Exception
		virtual void HandlePosition(const std::string& raw, Position& position);
		// @throw Exception
		virtual void HandleTelemetry(const std::string& raw, Telemetry& telemetry);

		// @throw Exception
		virtual void HandleDecodeError(const std::string& raw, Exception& exception);

	private:
		// @throw Exception
		IO_RESULTS SendOnce();

		// @throw Exception
		IO_RESULTS ReceiveOnce();

		// @throw Exception
		static bool Auth_FromString(Auth& auth, const std::string& string);

		// @throw Exception
		static bool Path_IsValid(const Path& path);

		// @throw Exception
		static bool Station_IsValid(const std::string& station);

		// @throw Exception
		static bool SymbolTable_IsValid(char table);

		// @throw Exception
		static bool SymbolTableKey_IsValid(char table, char key);

		// @throw Exception
		static std::string Packet_ToString(const APRService::Path& path, const std::string& sender, const std::string& tocall, const std::string& content);
		// @throw Exception
		static bool        Packet_FromString(Packet& packet, const std::string& string);

		// @throw Exception
		static std::string Message_ToString(const APRService::Path& path, const std::string& sender, const std::string& tocall, const std::string& destination, const std::string& body, const std::string& id = "");
		// @throw Exception
		static std::string Message_ToString_Ack(const APRService::Path& path, const std::string& sender, const std::string& tocall, const std::string& destination, const std::string& id);
		// @throw Exception
		static std::string Message_ToString_Reject(const APRService::Path& path, const std::string& sender, const std::string& tocall, const std::string& destination, const std::string& id);
		// @throw Exception
		static bool        Message_FromPacket(Message& message, Packet&& packet);

		// @throw Exception
		static std::string Weather_ToString(const APRService::Path& path, const std::string& sender, const std::string& tocall, tm time, std::uint16_t wind_speed, std::uint16_t wind_speed_gust, std::uint16_t wind_direction, std::uint16_t rainfall_last_hour, std::uint16_t rainfall_last_24_hours, std::uint16_t rainfall_since_midnight, std::uint8_t humidity, std::int16_t temperature, std::uint32_t barometric_pressure, const std::string& type);
		// @throw Exception
		static bool        Weather_FromPacket(Weather& weather, Packet&& packet);

		// @throw Exception
		static std::string Position_ToString(const APRService::Path& path, const std::string& sender, const std::string& tocall, std::uint16_t speed, std::uint16_t course, std::int32_t altitude, float latitude, float longitude, const std::string& comment, char symbol_table, char symbol_table_key, int flags);
		// @throw Exception
		static bool        Position_FromPacket(Position& position, Packet&& packet);

		// @throw Exception
		static std::string Telemetry_ToString(const APRService::Path& path, const std::string& sender, const std::string& tocall, const TelemetryAnalog& analog, TelemetryDigital digital, std::uint16_t sequence);
		// @throw Exception
		static bool        Telemetry_FromPacket(Telemetry& telemetry, Packet&& packet);

		// @throw Exception
		template<std::size_t S, typename ... TArgs>
		static inline std::string sprintf(const char(&format)[S], TArgs ... args)
		{
			auto string_length = snprintf(nullptr, 0, format, args ...);

			if (string_length < 0)
				throw InvalidFormatException(format);

			std::string string(string_length, '\0');
			snprintf(&string[0], string_length + 1, format, args ...);

			return string;
		}
	};

	class Service;

	struct Command
		: public Message
	{
		std::string Name;
		std::string Params;
	};

	// @return true to reschedule
	typedef std::function<bool(std::uint32_t& seconds)> TaskHandler;
	typedef std::function<void(const Command& command)> CommandHandler;

	class Service
		: public Client
	{
		struct Task
		{
			std::uint32_t Seconds;
			TaskHandler   Handler;
		};

		struct Command
		{
			std::string    Name;
			CommandHandler Handler;
		};

		time_t                            time;
		std::map<time_t, std::list<Task>> tasks;
		APRService::Command               command;
		std::list<Command>                commands;

	public:
		// @throw Exception
		Service(std::string&& station, Path&& path, char symbol_table, char symbol_table_key);

		void ScheduleTask(std::uint32_t seconds, TaskHandler&& handler);

		bool ExecuteCommand(const std::string& name, const APRService::Command& command);
		void RegisterCommand(std::string&& name, CommandHandler&& handler);
		void UnregisterCommand(const std::string& name);

		// @throw Exception
		// @return false on connection closed
		virtual bool Update();

	protected:
		// @throw Exception
		virtual void HandleMessage(const std::string& raw, Message& message) override;

	private:
		// @throw Exception
		static bool Command_FromMessage(APRService::Command& command, Message&& message);
	};
}
