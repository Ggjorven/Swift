#include "swpch.h"
#include "Logging.hpp"

namespace Swift
{

	std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> Log::s_Sink = nullptr;
	std::shared_ptr<spdlog::logger> Log::s_Logger = nullptr;

	void Log::Init()
	{
		s_Sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		s_Sink->set_pattern("[%H:%M:%S] [%L]: %v%$");

		s_Logger = std::make_shared<spdlog::logger>("Swift Logger", s_Sink);
		spdlog::set_default_logger(s_Logger);
		spdlog::set_level(spdlog::level::trace); 
	}

	bool Log::Initialized()
	{
		return s_Logger != nullptr;
	}

	std::shared_ptr<spdlog::logger>& Log::GetLogger()
	{
		return s_Logger;
	}

}