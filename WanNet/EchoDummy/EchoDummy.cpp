#include "EchoDummy.h"
#include "../NetLibrary/NetServer/Serializer.h"

#include "GlobalVariables.h"

void EchoDummy::OnReceive(Serializer* packet)
{
    // recv

    DWORD receiveTick;
    *packet >> receiveTick;

    if (mLastSendTick != receiveTick)
    {
        InterlockedIncrement(&g_echoErrorCount);
    }
    else
    {
        mLastRTT = ::timeGetTime() - mLastSendTick;
        InterlockedAdd64(reinterpret_cast<LONG64*>(&g_rttSum), mLastRTT);
        InterlockedIncrement(&g_rttCount);
    }

    Serializer::Free(packet);

    InterlockedIncrement(&g_echoRecvCount);

    // Send

    Serializer* echoPacket = Serializer::Alloc();
    mLastSendTick = ::timeGetTime();

    *echoPacket << mLastSendTick;
    SendPacket(echoPacket);

    InterlockedIncrement(&g_echoSendCount);

    Serializer::Free(echoPacket);
}

void EchoDummy::OnDisconnect(int errorCode)
{
    InterlockedDecrement(&g_totalConnectedClientCount);
    InterlockedIncrement(&g_downCount);
    wprintf(L"client Disconnected (%d)\n", errorCode);

    while (!this->Connect())
    {
        InterlockedIncrement(&g_connectFailCount);
        ::Sleep(1'000);
    }

    Serializer* echoPacket = Serializer::Alloc();
    uint32_t sendTick = ::timeGetTime();
    *echoPacket << sendTick;
    mLastSendTick = sendTick;
    SendPacket(echoPacket);
    Serializer::Free(echoPacket);

    InterlockedIncrement(&g_connectTotalCount);
    InterlockedIncrement(&g_totalConnectedClientCount);
}