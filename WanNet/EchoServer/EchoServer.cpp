#include "EchoServer.h"

void EchoServer::OnAccept(const uint64_t sessionID)
{
}

void EchoServer::OnReceive(const uint64_t sessionID, Serializer* packet)
{
    SendPacket(sessionID, packet);
    Serializer::Free(packet);
}

void EchoServer::OnRelease(const uint64_t sessionID)
{
}
