#include <conio.h>

#include "EchoServer.h"
#include "../NetLibrary/Logger/Logger.h"
#include "../NetLibrary/Tool/ConfigReader.h"

EchoServer g_echoServer;

int main(void)
{
#ifdef PROFILE_ON
    LOGF(ELogLevel::System, L"Profiler: PROFILE_ON");
#endif

#pragma region config 파일 읽기
    const WCHAR* CONFIG_FILE_NAME = L"EchoServer.config";

    /*************************************** Config - Logger ***************************************/

    WCHAR inputLogLevel[10];

    ASSERT_LIVE(ConfigReader::GetString(CONFIG_FILE_NAME, L"LOG_LEVEL", inputLogLevel, sizeof(inputLogLevel)), L"ERROR: config file read failed (LOG_LEVEL)");

    if (wcscmp(inputLogLevel, L"DEBUG") == 0)
    {
        Logger::SetLogLevel(ELogLevel::System);
    }
    else if (wcscmp(inputLogLevel, L"ERROR") == 0)
    {
        Logger::SetLogLevel(ELogLevel::Error);
    }
    else if (wcscmp(inputLogLevel, L"SYSTEM") == 0)
    {
        Logger::SetLogLevel(ELogLevel::System);
    }
    else
    {
        ASSERT_LIVE(false, L"ERROR: invalid LOG_LEVEL");
    }

    LOGF(ELogLevel::System, L"Logger Log Level = %s", inputLogLevel);

    /*************************************** Config - NetServer ***************************************/

    uint32_t inputPortNumber;
    uint32_t inputMaxSessionCount;
    uint32_t inputConcurrentThreadCount;
    uint32_t inputWorkerThreadCount;
    uint32_t inputSetTcpNodelay;
    uint32_t inputSetSendBufZero;

    ASSERT_LIVE(ConfigReader::GetInt(CONFIG_FILE_NAME, L"PORT", &inputPortNumber), L"ERROR: config file read failed (PORT)");
    ASSERT_LIVE(ConfigReader::GetInt(CONFIG_FILE_NAME, L"MAX_SESSION_COUNT", &inputMaxSessionCount), L"ERROR: config file read failed (MAX_SESSION_COUNT)");
    ASSERT_LIVE(ConfigReader::GetInt(CONFIG_FILE_NAME, L"CONCURRENT_THREAD_COUNT", &inputConcurrentThreadCount), L"ERROR: config file read failed (CONCURRENT_THREAD_COUNT)");
    ASSERT_LIVE(ConfigReader::GetInt(CONFIG_FILE_NAME, L"WORKER_THREAD_COUNT", &inputWorkerThreadCount), L"ERROR: config file read failed (WORKER_THREAD_COUNT)");
    ASSERT_LIVE(ConfigReader::GetInt(CONFIG_FILE_NAME, L"TCP_NODELAY", &inputSetTcpNodelay), L"ERROR: config file read failed (TCP_NODELAY)");
    ASSERT_LIVE(ConfigReader::GetInt(CONFIG_FILE_NAME, L"SND_BUF_ZERO", &inputSetSendBufZero), L"ERROR: config file read failed (SND_BUF_ZERO)");

    LOGF(ELogLevel::System, L"CONCURRENT_THREAD_COUNT = %u", inputConcurrentThreadCount);
    LOGF(ELogLevel::System, L"WORKER_THREAD_COUNT = %u", inputWorkerThreadCount);

    if (inputSetTcpNodelay != 0)
    {
        g_echoServer.SetTcpNodelay(true);
        LOGF(ELogLevel::System, L"ChatServer - SetTcpNodelay(true)");
    }

    if (inputSetSendBufZero != 0)
    {
        g_echoServer.SetSendBufferSizeToZero(true);
        LOGF(ELogLevel::System, L"ChatServer - SetSendBufferSizeToZero(true)");
    }

#pragma endregion

    // 최대 페이로드 길이 지정
    g_echoServer.SetMaxPayloadLength(1'000);

    // 서버 시작
    g_echoServer.Start(static_cast<uint16_t>(inputPortNumber), inputMaxSessionCount, inputConcurrentThreadCount, inputWorkerThreadCount);

    MonitoringVariables monitoringInfo; // NetServer의 모니터링 정보

    HANDLE mainThreadTimer = ::CreateWaitableTimer(NULL, FALSE, NULL);
    ASSERT_LIVE(mainThreadTimer != INVALID_HANDLE_VALUE, L"mainThreadTimer create failed");

    LARGE_INTEGER timerTime{};
    timerTime.QuadPart = -1 * (10'000 * static_cast<LONGLONG>(1'000));
    ::SetWaitableTimer(mainThreadTimer, &timerTime, 1'000, nullptr, nullptr, FALSE);

    for (;;)
    {
        ::WaitForSingleObject(mainThreadTimer, INFINITE);

        if (_kbhit())
        {
            int input = _getch();
            if (input == 'Q' || input == 'q')
            {
                g_echoServer.Shutdown();
                break;
            }
#ifdef PROFILE_ON
            else if (input == 'S' || input == 's')
            {
                PROFILE_SAVE(L"Profile_ChatServer.txt");
                LOGF(ELogLevel::System, L"Profile_ChatServer.txt saved");
            }
#endif
        }

        // NetServer Monitoring
        monitoringInfo = g_echoServer.GetMonitoringInfo();

        wprintf(L"\n\n");
        //LOG_CURRENT_TIME();
        wprintf(L"[ EchoServer Running (S: profile save) (Q: quit)]\n");
        wprintf(L"=================================================\n");
        wprintf(L"Session Count        = %u / %u\n", g_echoServer.GetSessionCount(), g_echoServer.GetMaxSessionCount());
        wprintf(L"Accept Total         = %llu\n", g_echoServer.GetTotalAcceptCount());
        wprintf(L"Disconnected Total   = %llu\n", g_echoServer.GetTotalDisconnectCount());
        wprintf(L"Packet Pool Size     = %u\n", Serializer::GetTotalPacketCount());
        wprintf(L"---------------------- TPS ----------------------\n");
        wprintf(L"Accept TPS           = %9u (Avg: %9u)\n", monitoringInfo.AcceptTPS, monitoringInfo.AverageAcceptTPS);
        wprintf(L"Send Message TPS     = %9u (Avg: %9u)\n", monitoringInfo.SendMessageTPS, monitoringInfo.AverageSendMessageTPS);
        wprintf(L"Recv Message TPS     = %9u (Avg: %9u)\n", monitoringInfo.RecvMessageTPS, monitoringInfo.AverageRecvMessageTPS);
        wprintf(L"Send Pending TPS     = %9u (Avg: %9u)\n", monitoringInfo.SendPendingTPS, monitoringInfo.AverageSendPendingTPS);
        wprintf(L"Recv Pending TPS     = %9u (Avg: %9u)\n", monitoringInfo.RecvPendingTPS, monitoringInfo.AverageRecvPendingTPS);
        wprintf(L"----------------------- CPU ---------------------\n");
        wprintf(L"Total  = Processor: %6.3f / Process: %6.3f\n", monitoringInfo.ProcessorTimeTotal, monitoringInfo.ProcessTimeTotal);
        wprintf(L"User   = Processor: %6.3f / Process: %6.3f\n", monitoringInfo.ProcessorTimeUser, monitoringInfo.ProcessTimeUser);
        wprintf(L"Kernel = Processor: %6.3f / Process: %6.3f\n", monitoringInfo.ProcessorTimeKernel, monitoringInfo.ProcessTimeKernel);
    }

    ::CloseHandle(mainThreadTimer);

    return 0;
}