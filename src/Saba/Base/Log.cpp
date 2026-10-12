//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "Log.h"
#include "UnicodeUtil.h"

#include <spdlog/sinks/stdout_color_sinks.h>

#include <iostream>

#if _WIN32

#include <Windows.h>

#endif // _WIN32

namespace saba
{
	DefaultSink::DefaultSink()
	{
		m_defaultLogger = spdlog::stdout_color_mt("default");
#if _WIN32
		SetConsoleOutputCP(CP_UTF8);
#endif // _WIN32
	}

	void DefaultSink::sink_it_(const spdlog::details::log_msg & msg)
	{
		m_defaultLogger->log(msg.level, msg.payload);
	}

	void DefaultSink::flush_()
	{
		m_defaultLogger->flush();
	}
}
