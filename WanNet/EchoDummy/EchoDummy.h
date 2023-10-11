#pragma once
#include "../NetLibrary/NetServer/NetClient.h"

class EchoDummy : public NetClient
{
public:

    EchoDummy(const std::wstring serverIP, const uint16_t serverPortNumber)
        : NetClient(serverIP, serverPortNumber)
    {
    }

    // NetClient��(��) ���� ��ӵ�
    virtual void OnReceive(Serializer* packet) override;
};