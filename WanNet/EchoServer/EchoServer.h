#pragma once

#include "../NetLibrary/NetServer/NetServer.h"

class EchoServer : public NetServer
{
    // NetServer을(를) 통해 상속됨
    virtual void OnAccept(const uint64_t sessionID) override;
    virtual void OnReceive(const uint64_t sessionID, Serializer* packet) override;
    virtual void OnRelease(const uint64_t sessionID) override;
};