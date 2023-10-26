#pragma once

#include <iostream>

// 포매팅 로그
#define LOGF(Level, FormatString, ...)	Logger::LogFormatWithTime(Level, FormatString, __VA_ARGS__)

// 모니터링 로그 (날짜 표시 없이 로깅만 함)
#define LOG_MONITOR(FormatString, ...)	Logger::LogMonitor(FormatString, __VA_ARGS__)

// Condition이 false이면 로깅 후 크래시
#define ASSERT_LIVE(Condition, Message) Logger::Assert((Condition), (Message), __FILEW__, __LINE__)

// 로그의 레벨 (현재 로그의 레벨과 같거나 보다 낮은 로그만 작동함)
enum class ELogLevel
{
	Debug,		// Debug (Log All)
	System,
	Error,
	Test,		// temporary log for test
	Assert,		// assertion
};

typedef wchar_t WCHAR;
typedef void* HANDLE;

class Logger
{
public:

	// 로거의 로깅 수준을 변경한다
	inline static void SetLogLevel(ELogLevel logLevel) { mLogLevel = logLevel; }

	// 모니터링 로그에 현재 시간 기록
	static void LogCurrentTime(void);

	// 모니터링 로그
	static void LogMonitor(const WCHAR* formatMessage, ...);

	static void LogMessageWithTime(ELogLevel logLevel, const WCHAR* message);

	static void LogMessageWithTime(ELogLevel logLevel, const WCHAR* message, const WCHAR* fileName, int line);
	
	static void LogFormatWithTime(ELogLevel logLevel, const WCHAR* formatMessage, ...);

	static void LogFormat(ELogLevel logLevel, const WCHAR* formatMessage, ...);

	inline static void Assert(bool condition, const WCHAR* message)
	{
		if (!condition)
		{
			LogMessageWithTime(ELogLevel::Assert, message);
			raiseCrash();
		}
	}

	inline static void Assert(bool condition, const WCHAR* message, const WCHAR* fileName, int line)
	{
		if (!condition)
		{
			LogMessageWithTime(ELogLevel::Assert, message, fileName, line);
			raiseCrash();
		}
	}

	enum
	{
		LOG_MESSAGE_MAX_LENGTH = 512,
		DAY_INFO_BUFFER_LENGTH = 16
	};

private:

	Logger(void);
	~Logger(void);

	static void writeToFile(HANDLE hFile, const WCHAR* log);

	static void getCurrentTimeInfo(WCHAR* outBuffer);

	// intentional crash
	static void raiseCrash();

	static Logger		mInstance;
	static HANDLE		mhLogFile;		// 로그 파일 핸들
	static HANDLE		mhMonitorFile;	// 모니터링 파일 핸들
	static ELogLevel	mLogLevel;
};