#pragma once

#include <iostream>

// ������ �α�
#define LOGF(Level, FormatString, ...)	Logger::LogFormatWithTime(Level, FormatString, __VA_ARGS__)

// ����͸� �α� (��¥ ǥ�� ���� �α븸 ��)
#define LOG_MONITOR(FormatString, ...)	Logger::LogMonitor(FormatString, __VA_ARGS__)

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

typedef wchar_t WCHAR;
typedef void* HANDLE;

class Logger
{
public:

	// �ΰ��� �α� ������ �����Ѵ�
	inline static void SetLogLevel(ELogLevel logLevel) { mLogLevel = logLevel; }

	// ����͸� �α׿� ���� �ð� ���
	static void LogCurrentTime(void);

	// ����͸� �α�
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
	static HANDLE		mhLogFile;		// �α� ���� �ڵ�
	static HANDLE		mhMonitorFile;	// ����͸� ���� �ڵ�
	static ELogLevel	mLogLevel;
};