#pragma once

#include <iostream>

/************************ recommend call this macros instead of Logger::functions ************************/

// �ΰ��� �α� ������ �����Ѵ�
#define SET_LOG_LEVEL(Level)			Logger::SetLogLevel(Level)

// �α׸� �����
#define LOG(Level, Log)					Logger::LogMessageWithTime(Level, Log)

// �α׿� �Բ� �ҽ� ���ϸ�� �� ��ȣ�� �����
#define LOG_WITH_LINE(Level, Log)		Logger::LogMessageWithTime((Level), (Log), __FILEW__, __LINE__)

// ������ �α�
#define LOGF(Level, FormatString, ...)	Logger::LogFormatWithTime(Level, FormatString, __VA_ARGS__)

// ����͸� �α� (��¥ ǥ�� ���� �α븸 ��)
#define LOG_MONITOR(FormatString, ...)	Logger::LogMonitor(FormatString, __VA_ARGS__)

// ����͸� �α׿� ���� �ð��� ���� �α׸� �����
#define LOG_CURRENT_TIME()				Logger::LogCurrentTime()

// Condition�� false�̸� �α� �� ũ����
#define ASSERT_LIVE(Condition, Message) Logger::Assert((Condition), (Message), __FILEW__, __LINE__)

// �α��� ���� (���� �α��� ������ ���ų� ���� ���� �α׸� �۵���)
enum class ELogLevel
{
	Debug,		// Debug (Log All)
	System,
	Error,
	Test,		// temporary log for test
	Assert,		// assertion
};

/*********************************************************************************************************/

typedef wchar_t WCHAR;
typedef void* HANDLE;

class Logger
{
public:

	// �ΰ��� �α� ������ �����Ѵ�
	inline static void SetLogLevel(ELogLevel logLevel) { mLogLevel = logLevel; }


	static void LogMonitor(const WCHAR* formatMessage, ...);

	static void LogMessage(ELogLevel logLevel, const WCHAR* message);

	static void LogMessageWithTime(ELogLevel logLevel, const WCHAR* message);
	static void LogMessageWithTime(ELogLevel logLevel, const WCHAR* message, const WCHAR* fileName, int line);
	static void LogFormatWithTime(ELogLevel logLevel, const WCHAR* formatMessage, ...);

	static void LogFormat(ELogLevel logLevel, const WCHAR* formatMessage, ...);
	 
	static void LogCurrentTime(void);

	inline static void Assert(bool condition, const WCHAR* message)
	{
		if (!condition)
		{
			LogMessageWithTime(ELogLevel::Assert, message);
			RaiseCrash();
		}
	}

	inline static void Assert(bool condition, const WCHAR* message, const WCHAR* fileName, int line)
	{
		if (!condition)
		{
			LogMessageWithTime(ELogLevel::Assert, message, fileName, line);
			RaiseCrash();
		}
	}

	// intentional crash
	static void RaiseCrash();
private:
	Logger();
	~Logger();

	static void getCurrentTimeInfo(WCHAR* outBuffer);

	static Logger mInstance;
	static HANDLE mhLogFile;
	static HANDLE mhMonitorFile;
	static ELogLevel mLogLevel;
};