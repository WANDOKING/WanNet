#pragma comment(lib, "pathcch.lib")

#include "Logger.h"

#include <time.h>
#include <Windows.h>
#include <process.h>
#include <strsafe.h>

Logger Logger::mInstance;
HANDLE Logger::mhLogFile;
HANDLE Logger::mhMonitorFile;
ELogLevel Logger::mLogLevel = ELogLevel::Debug;
LockFreeQueue<WCHAR*> Logger::mAsyncLogQueue;
HANDLE Logger::mAsyncLogEvent;

void Logger::LogMonitor(const WCHAR * formatMessage, ...)
{
	WCHAR completedMessage[LOG_MESSAGE_MAX_LENGTH];

	va_list ap;
	va_start(ap, formatMessage);
	{
		StringCchVPrintfW(completedMessage, LOG_MESSAGE_MAX_LENGTH, formatMessage, ap);
	}
	va_end(ap);

	WCHAR log[LOG_MESSAGE_MAX_LENGTH];

	StringCchPrintfW(log, LOG_MESSAGE_MAX_LENGTH, L"%s\n", completedMessage);

	wprintf(log);
	writeToFile(mhMonitorFile, log);
}

void Logger::LogMessageWithTime(ELogLevel logLevel, const WCHAR* message)
{
	if (mLogLevel > logLevel)
	{
		return;
	}

	WCHAR log[LOG_MESSAGE_MAX_LENGTH];
	WCHAR dayInfo[DAY_INFO_BUFFER_LENGTH];
	getCurrentTimeInfo(dayInfo);

	StringCchPrintfW(log, LOG_MESSAGE_MAX_LENGTH, L"[%s] : %s\n", dayInfo, message);

	wprintf(log);
	writeToFile(mhLogFile, log);
}

void Logger::LogMessageWithTime(ELogLevel logLevel, const WCHAR* message, const WCHAR* fileName, int line)
{
	if (mLogLevel > logLevel)
	{
		return;
	}

	WCHAR log[LOG_MESSAGE_MAX_LENGTH];
	WCHAR dayInfo[DAY_INFO_BUFFER_LENGTH];
	getCurrentTimeInfo(dayInfo);

	StringCchPrintfW(log, LOG_MESSAGE_MAX_LENGTH, L"[%s][%s line:%4d] : %s\n", dayInfo, fileName, line, message);

	wprintf(log);
	writeToFile(mhLogFile, log);
}

void Logger::LogFormat(ELogLevel logLevel, const WCHAR* formatMessage, ...)
{
	if (mLogLevel > logLevel)
	{
		return;
	}

	WCHAR completedMessage[LOG_MESSAGE_MAX_LENGTH];

	va_list ap;
	va_start(ap, formatMessage);
	{
		StringCchVPrintfW(completedMessage, LOG_MESSAGE_MAX_LENGTH, formatMessage, ap);
	}
	va_end(ap);

	wprintf(L"%s\n", completedMessage);
	writeToFile(mhLogFile, completedMessage);
	writeToFile(mhLogFile, L"\n");
}

void Logger::LogCurrentTime(void)
{
	WCHAR dayInfo[DAY_INFO_BUFFER_LENGTH];
	getCurrentTimeInfo(dayInfo);

	WCHAR log[LOG_MESSAGE_MAX_LENGTH];
	StringCchPrintfW(log, LOG_MESSAGE_MAX_LENGTH, L"<%s>\n", dayInfo);

	wprintf(log);
	writeToFile(mhMonitorFile, log);
}

void Logger::LogFormatWithTime(ELogLevel logLevel, const WCHAR* formatMessage, ...)
{
	if (mLogLevel > logLevel)
	{
		return;
	}

	WCHAR completedMessage[LOG_MESSAGE_MAX_LENGTH];

	va_list ap;
	va_start(ap, formatMessage);
	{
		StringCchVPrintfW(completedMessage, LOG_MESSAGE_MAX_LENGTH, formatMessage, ap);
	}
	va_end(ap);

	Logger::LogMessageWithTime(logLevel, completedMessage);
}

void Logger::LogMonitorAsync(const WCHAR* formatMessage, ...)
{
	WCHAR completedMessage[LOG_MESSAGE_MAX_LENGTH];

	va_list ap;
	va_start(ap, formatMessage);
	{
		StringCchVPrintfW(completedMessage, LOG_MESSAGE_MAX_LENGTH, formatMessage, ap);
	}
	va_end(ap);

	WCHAR* log = new WCHAR[LOG_MESSAGE_MAX_LENGTH];

	StringCchPrintfW(log, LOG_MESSAGE_MAX_LENGTH, L"%s\n", completedMessage);

	mAsyncLogQueue.Enqueue(log);

	::SetEvent(mAsyncLogEvent);
}

Logger::Logger()
{
	WCHAR fileName[_MAX_PATH * 2];

	// get logFileName
	WCHAR dayInfo[DAY_INFO_BUFFER_LENGTH];
	getCurrentTimeInfo(dayInfo);

	// get process path
	DWORD dwPID = GetCurrentProcessId();
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwPID);
	WCHAR processPath[MAX_PATH];
	if (GetModuleFileNameW(nullptr, processPath, MAX_PATH) == 0)
	{
		CloseHandle(hProcess);
		exit(1);
	}
	CloseHandle(hProcess);

	wsprintfW(fileName, L"%s Log", processPath);
	CreateDirectoryW(fileName, nullptr); // create directory

	wsprintfW(processPath, L"%s", fileName); // path update (log file directory)

	wsprintfW(fileName, L"%s\\Log_%s.txt", processPath, dayInfo);
	mhLogFile = ::CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (mhLogFile == INVALID_HANDLE_VALUE)
	{
		printf("Log File CreateFile() Failed %d\n", GetLastError());
		raiseCrash();
	}

	wsprintfW(fileName, L"%s\\Monitor_%s.txt", processPath, dayInfo);
	mhMonitorFile = ::CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (mhMonitorFile == INVALID_HANDLE_VALUE)
	{
		printf("Monitoring Log File CreateFile() Failed %d\n", GetLastError());
		raiseCrash();
	}

	// Write BOM
	constexpr unsigned short BOM_UTF_16_LE = 0xFEFF;

	::WriteFile(mhLogFile, &BOM_UTF_16_LE, sizeof(BOM_UTF_16_LE), nullptr, nullptr);
	::WriteFile(mhMonitorFile, &BOM_UTF_16_LE, sizeof(BOM_UTF_16_LE), nullptr, nullptr);

	mAsyncLogEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	HANDLE hAsyncLogThread = reinterpret_cast<HANDLE>(::_beginthreadex(nullptr, 0, asyncLogThread, nullptr, 0, nullptr));
}

Logger::~Logger()
{
	::CloseHandle(mhLogFile);
	::CloseHandle(mhMonitorFile);
}

void Logger::writeToFile(HANDLE hFile, const WCHAR* log)
{
	if (WriteFile(hFile, log, (DWORD)(wcslen(log) * sizeof(WCHAR)), nullptr, nullptr))
	{
		wprintf(L"[!] File Logging ERROR - GetLastError() = %d\n", GetLastError());
	}

	FlushFileBuffers(hFile);
}

void Logger::getCurrentTimeInfo(WCHAR* outBuffer)
{
	time_t startTime = time(nullptr);
	tm localTime;
	localtime_s(&localTime, &startTime);

	int YYYY = localTime.tm_year + 1900;
	int MM = localTime.tm_mon + 1;
	int DD = localTime.tm_mday;
	int hh = localTime.tm_hour;
	int mm = localTime.tm_min;
	int ss = localTime.tm_sec;

	StringCchPrintfW(outBuffer, DAY_INFO_BUFFER_LENGTH, L"%04d%02d%02d_%02d%02d%02d", YYYY, MM, DD, hh, mm, ss);
}

unsigned int Logger::asyncLogThread(void* param)
{
	for (;;)
	{
		::WaitForSingleObject(mAsyncLogEvent, INFINITE);

		WCHAR* log;

		while (mAsyncLogQueue.TryDequeue(log))
		{
			wprintf(log);
			writeToFile(mhMonitorFile, log);

			delete[] log;
		}
	}

	return 0;
}

#pragma warning(push)
#pragma warning(disable: 6011)
void Logger::raiseCrash()
{
	int* nullPointer = 0x00000000;
	*nullPointer = 0;
}
#pragma warning(pop)