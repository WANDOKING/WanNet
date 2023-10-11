#pragma comment(lib, "DbgHelp.Lib")

#include <iostream>

#include "CrashDump.h"

DWORD CrashDump::s_mDumpCount;
CrashDump CrashDump::s_mInstance;

void CrashDump::Crash()
{
#pragma warning(push)
#pragma warning(disable: 6011)
	int* nullPointer = nullptr;
	*nullPointer = 0;
#pragma warning(pop)
}

CrashDump::CrashDump()
{
	s_mDumpCount = 0;

	// CRT �Լ��� nullptr ���� �־��� �� �߻��ϴ� �ڵ鷯 ��ü
	::_set_invalid_parameter_handler(myInvalidParameterHandler);

	// CRT ���� �޽��� ǥ�� �ߴ� -> �ٷ� ������ ������
	_CrtSetReportMode(_CRT_WARN, 0);
	_CrtSetReportMode(_CRT_ASSERT, 0);
	_CrtSetReportMode(_CRT_ERROR, 0);

	_CrtSetReportHook(_custom_Report_hook);

	// pure virtual function called ���� �ڵ鷯 ��ü
	::_set_purecall_handler(myPurecallHandler);

	// ó������ ���� ���� ���͸� myExceptionFilter�� ��ü
	::SetUnhandledExceptionFilter(myExceptionFilter);
}

LONG WINAPI CrashDump::myExceptionFilter(PEXCEPTION_POINTERS pExceptionPointer)
{
	DWORD dumpCount = InterlockedIncrement(&s_mDumpCount);

	SYSTEMTIME nowTime;
	::GetLocalTime(&nowTime);

	WCHAR fileName[MAX_PATH];
	wsprintf(fileName, L"Dump_%d%02d%02d_%02d%02d%02d_%d.dmp",
		nowTime.wYear, nowTime.wMonth, nowTime.wDay, nowTime.wHour, nowTime.wMinute, nowTime.wSecond, dumpCount);

	wprintf(L"\n\n\n!!! Crash Error !!!\nCrashDump Save... (%s)\n", fileName);

	// ���� ���� ����
	HANDLE hDumpFile = ::CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDumpFile == INVALID_HANDLE_VALUE) { return 0; }

	// ���� ����
	_MINIDUMP_EXCEPTION_INFORMATION miniDumpExceptionInfo;
	miniDumpExceptionInfo.ThreadId = ::GetCurrentThreadId();
	miniDumpExceptionInfo.ExceptionPointers = pExceptionPointer;
	miniDumpExceptionInfo.ClientPointers = TRUE;

	::MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpWithFullMemory, &miniDumpExceptionInfo, NULL, NULL);

	wprintf(L"CrashDump Saved\n");

	::CloseHandle(hDumpFile);

	return EXCEPTION_EXECUTE_HANDLER;
}