#pragma once

#include <memory>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Swift
{

	class Log
	{
	public:
		enum class Level : uint8_t
		{
			Trace = 0, Info, Warn, Error, Fatal, None = 255
		};

		static void Init();
		static bool Initialized();

		template<typename ... Args>
		static void LogMessage(Log::Level level, Args&&... args);

		#ifndef APP_DIST
		#define APP_LOG_TRACE(...)	Swift::Log::LogMessage(Swift::Log::Level::Trace, __VA_ARGS__);
		#define APP_LOG_INFO(...)	Swift::Log::LogMessage(Swift::Log::Level::Info, __VA_ARGS__);
		#define APP_LOG_WARN(...)	Swift::Log::LogMessage(Swift::Log::Level::Warn, __VA_ARGS__);
		#define APP_LOG_ERROR(...)	Swift::Log::LogMessage(Swift::Log::Level::Error, __VA_ARGS__);
		#define APP_LOG_FATAL(...)	Swift::Log::LogMessage(Swift::Log::Level::Fatal, __VA_ARGS__);
		#else
		#define APP_LOG_TRACE(...)
		#define APP_LOG_INFO(...)
		#define APP_LOG_WARN(...)
		#define APP_LOG_ERROR(...)
		#define APP_LOG_FATAL(...)
		#endif

		static std::shared_ptr<spdlog::logger>& GetLogger();

	private:
		static std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> s_Sink;
		static std::shared_ptr<spdlog::logger> s_Logger;
	};

	template<typename ... Args>
	void Log::LogMessage(Log::Level level, Args&&... args)
	{
		switch (level)
		{
		case Level::Trace:
			spdlog::trace(fmt::format(std::forward<Args>(args)...));
			break;
		case Level::Info:
			spdlog::info(fmt::format(std::forward<Args>(args)...));
			break;
		case Level::Warn:
			spdlog::warn(fmt::format(std::forward<Args>(args)...));
			break;
		case Level::Error:
			spdlog::error(fmt::format(std::forward<Args>(args)...));
			break;
		case Level::Fatal:
			spdlog::critical(fmt::format(std::forward<Args>(args)...));
			break;
		}
	}

	#ifndef APP_DIST
	#define APP_VERIFY(value, ...) if (!value) \
	{ \
		APP_LOG_FATAL(__VA_ARGS__); \
	} 
	#else
	#define APP_VERIFY(value, ...)
	#endif

	#ifdef APP_DEBUG
	#define APP_ASSERT(value, ...) if (!value) \
	{ \
		APP_LOG_FATAL(__VA_ARGS__); \
		__debugbreak(); \
	}
	#elif defined(APP_RELEASE)
	#define APP_ASSERT(value, ...) if (!value) \
	{ \
		APP_LOG_FATAL(__VA_ARGS__); \
	}
	#else
	#define APP_ASSERT(value, ...)
	#endif

}