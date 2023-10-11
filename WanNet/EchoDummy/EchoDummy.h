#pragma once
#include "../NetLibrary/NetServer/NetClient.h"

class EchoDummy : public NetClient
{
public:

    EchoDummy(const std::wstring serverIP, const uint16_t serverPortNumber)
        : NetClient(serverIP, serverPortNumber)
    {
    }

    // NetClient을(를) 통해 상속됨
    virtual void OnReceive(Serializer* packet) override;
};