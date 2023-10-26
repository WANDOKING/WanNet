#include <conio.h>
#include <vector>

#include "EchoDummy.h"
#include "../NetLibrary/NetServer/Serializer.h"
#include "../NetLibrary/Logger/Logger.h"
#include "../NetLibrary/Tool/ConfigReader.h"

#include <Windows.h>

#include "GlobalVariables.h"

int main(void)
{
#pragma region config 파일 읽기
    const WCHAR* CONFIG_FILE_NAME = L"EchoDummy.config";

    WCHAR inputServerIP[16];
    uint32_t inputPortNumber;
    uint32_t inputClientCount;

    ASSERT_LIVE(ConfigReader::GetString(CONFIG_FILE_NAME, L"SERVER_IP", inputServerIP, sizeof(inputServerIP)), L"ERROR: config file read failed (SERVER_IP)");
    ASSERT_LIVE(ConfigReader::GetInt(CONFIG_FILE_NAME, L"PORT", &inputPortNumber), L"ERROR: config file read failed (PORT)");
    ASSERT_LIVE(ConfigReader::GetInt(CONFIG_FILE_NAME, L"CLIENT_COUNT", &inputClientCount), L"ERROR: config file read failed (CLIENT_COUNT)");

#pragma endregion

    std::vector<std::unique_ptr<EchoDummy>> echoDummys;

    echoDummys.reserve(inputClientCount);

    for (uint32_t i = 0; i < inputClientCount; ++i)
    {
        echoDummys.push_back(std::make_unique<EchoDummy>(inputServerIP, inputPortNumber));

        bool bConnectSuccess = echoDummys.back()->Connect();

        if (bConnectSuccess)
        {
            Serializer* echoPacket = Serializer::Alloc();
            uint32_t sendTick = ::timeGetTime();
            *echoPacket << sendTick;
            echoDummys.back()->SetLastSendTick(sendTick);
            echoDummys.back()->SendPacket(echoPacket);

            Serializer::Free(echoPacket);

            g_totalConnectedClientCount++;
            g_connectTotalCount++;

            wprintf(L"Connect Success\n");
        }
        else
        {
            g_connectFailCount++;
            wprintf(L"Connect Fail\n");
        }
    }

    for (;;)
    {
        ::Sleep(1'000);

        uint32_t echoNotRecvCount = 0;
        uint32_t currentTick = ::timeGetTime();
        for (const auto& dummy : echoDummys)
        {
            if (currentTick - dummy->GetLastSendTick() > 500)
            {
                echoNotRecvCount++;
            }
        }

        uint32_t echoSendCount = InterlockedExchange(&g_echoSendCount, 0);
        uint32_t echoRecvCount = InterlockedExchange(&g_echoRecvCount, 0);

        wprintf(L"\n");
        wprintf(L"[     Echo Dummy Running    ]\n");
        wprintf(L"=============================\n");
        wprintf(L"Total Client Count = %u / %u\n", g_totalConnectedClientCount, inputClientCount);
        wprintf(L"Connect Total      = %llu\n", g_connectTotalCount);
        wprintf(L"Packet Pool Size   = %u\n", Serializer::GetTotalPacketCount());
        wprintf(L"-----------------------------\n");
        wprintf(L"Connect Fail          = %llu\n", g_connectFailCount);
        wprintf(L"Down Client           = %llu\n", g_downCount);
        wprintf(L"Echo Error            = %llu\n", g_echoErrorCount);
        wprintf(L"Echo Not Recv (500ms) = %u\n", echoNotRecvCount);
        wprintf(L"-----------------------------\n");
        wprintf(L"Send TPS     = %u\n", echoSendCount);
        wprintf(L"Recv TPS     = %u\n", echoRecvCount);
        
        uint64_t averageRTT = g_rttCount == 0 ? 0 : g_rttSum / g_rttCount;
        wprintf(L"Average RTT  = %3llu ms\n", averageRTT);
    }

    return 0;
}