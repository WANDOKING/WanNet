#include "EchoDummy.h"
#include "../NetLibrary/NetServer/Serializer.h"

#include "GlobalVariables.h"

void EchoDummy::OnReceive(Serializer* packet)
{
    // recv

    DWORD receiveTick;
    *packet >> receiveTick;
    Serializer::Free(packet);

    InterlockedIncrement(&g_echoRecvCount);

    // Send

    Serializer* echoPacket = Serializer::Alloc();
    *echoPacket << ::timeGetTime();
    SendPacket(echoPacket);

    InterlockedIncrement(&g_echoSendCount);

    Serializer::Free(echoPacket);
}
