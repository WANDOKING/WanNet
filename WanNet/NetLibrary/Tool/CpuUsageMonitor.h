#pragma once

#include <Windows.h>

class CpuUsageMonitor
{
public:
	CpuUsageMonitor(HANDLE hProcess)
	{
		Init(hProcess);
	}

	CpuUsageMonitor(void)
	{
		Init(GetCurrentProcess());
	}

	void Init(HANDLE hProcess)
	{
		mhProcess = hProcess;

		// ���μ����� ���� ������
		SYSTEM_INFO systemInfo;
		GetSystemInfo(&systemInfo);
		mProcessorCount = systemInfo.dwNumberOfProcessors;

		UpdateCpuTime();
	}

public:

	float GetProcessorTimeTotal() { return mProcessorTotal; }
	float GetProcessorTimeUser() { return mProcessorUser; }
	float GetProcessorTimeKernel() { return mProcessorKernel; }

	float GetProcessTimeTotal() { return mProcessTotal; }
	float GetProcessTimeUser() { return mProcessUser; }
	float GetProcessTimeKernel() { return mProcessKernel; }

	void UpdateCpuTime()
	{
		/****************************** ���μ��� ���� ���� ******************************/

		// 1. �ý��� ��� �ð��� ���´�
		ULARGE_INTEGER systemTimeKernel; // Ŀ�� ��� �ð��� Idle �ð��� ���ԵǾ� ����
		ULARGE_INTEGER systemTimeUser;
		ULARGE_INTEGER systemTimeIdle;
		if (!GetSystemTimes((PFILETIME)&systemTimeIdle, (PFILETIME)&systemTimeKernel, (PFILETIME)&systemTimeUser))
		{
			return;
		}

		// 2. ���� �ð��� ���Ѵ�
		ULONGLONG systemTimeDiffKernel = systemTimeKernel.QuadPart - mProcessorLastKernel.QuadPart;
		ULONGLONG systemTimeDiffUser = systemTimeUser.QuadPart - mProcessorLastUser.QuadPart;
		ULONGLONG systemTimeDiffIdle = systemTimeIdle.QuadPart - mProcessorLastIdle.QuadPart;

		// 3. �� CPU ��� �ð� = Ŀ�� �ð� + ���� �ð�
		ULONGLONG systemTimeTotal = systemTimeDiffKernel + systemTimeDiffUser;
		if (systemTimeTotal == 0)
		{
			mProcessorKernel = 0.0f;
			mProcessorUser = 0.0f;
			mProcessorTotal = 0.0f;
		}
		else
		{
			// Ŀ�� ��� �ð��� Idle �ð��� ���ԵǾ� �����Ƿ� ���� �����
			mProcessorKernel = (float)((double)(systemTimeDiffKernel - systemTimeDiffIdle) / systemTimeTotal * 100.0f);
			mProcessorUser = (float)((double)systemTimeDiffUser / systemTimeTotal * 100.0f);
			mProcessorTotal = (float)((double)(systemTimeTotal - systemTimeDiffIdle) / systemTimeTotal * 100.0f);
		}

		// 4. Last �ð� ����
		mProcessorLastKernel = systemTimeKernel;
		mProcessorLastUser = systemTimeUser;
		mProcessorLastIdle = systemTimeIdle;

		/****************************** ���μ��� ���� ���� ******************************/

		// 1. ���� 100ns ���� �ð��� ���Ѵ�
		ULARGE_INTEGER nowTime;
		GetSystemTimeAsFileTime((LPFILETIME)&nowTime);

		// 2. ���μ��� ��� �ð� ���ϱ�
		ULARGE_INTEGER ignore;
		ULARGE_INTEGER processUsageTimeKernel;
		ULARGE_INTEGER processUsageTimeUser;
		GetProcessTimes(mhProcess, (LPFILETIME)&ignore, (LPFILETIME)&ignore, (LPFILETIME)&processUsageTimeKernel, (LPFILETIME)&processUsageTimeUser);

		// 3. ���� �ð��� ���Ѵ�
		ULONGLONG timeDiffKernel = processUsageTimeKernel.QuadPart - mProcessLastKernel.QuadPart;
		ULONGLONG timeDiffUser = processUsageTimeUser.QuadPart - mProcessLastUser.QuadPart;
		ULONGLONG timeDiffTime = nowTime.QuadPart - mProcessLastTime.QuadPart;

		// 4. �� CPU ��� �ð� = Ŀ�� �ð� + ���� �ð�
		ULONGLONG timeTotal = timeDiffKernel + timeDiffUser;

		mProcessKernel = (float)(timeDiffKernel / (double)mProcessorCount / (double)timeDiffTime * 100.0f);
		mProcessUser = (float)(timeDiffUser / (double)mProcessorCount / (double)timeDiffTime * 100.0f);
		mProcessTotal = (float)(timeTotal / (double)mProcessorCount / (double)timeDiffTime * 100.0f);

		// 5. Last �ð� ����
		mProcessLastTime = nowTime;
		mProcessLastKernel = processUsageTimeKernel;
		mProcessLastUser = processUsageTimeUser;
	}
private:
	HANDLE mhProcess{};
	int mProcessorCount{};

	/************************ ���� �ð��� ���� % ����� ************************/

	float mProcessorKernel{};
	float mProcessorUser{};
	float mProcessorTotal{};

	float mProcessKernel{};
	float mProcessUser{};
	float mProcessTotal{};

	/************** UpdateCpuTime() ȣ�� �� ����� ���� ���� ���� **************/

	ULARGE_INTEGER mProcessorLastKernel{};
	ULARGE_INTEGER mProcessorLastUser{};
	ULARGE_INTEGER mProcessorLastIdle{};

	ULARGE_INTEGER mProcessLastKernel{};
	ULARGE_INTEGER mProcessLastUser{};
	ULARGE_INTEGER mProcessLastTime{};
};