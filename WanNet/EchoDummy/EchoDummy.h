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

    // NetClient��(��) ���� ��ӵ�
    virtual void OnDisconnect(int errorCode) override;

public:

    inline uint32_t GetLastSendTick(void) { return mLastSendTick; }
    inline uint32_t GetLastRTT(void) { return mLastRTT; }

    inline void SetLastSendTick(uint32_t sendTick) { mLastSendTick = sendTick; }

private:
    uint32_t mLastSendTick = 0;
    uint32_t mLastRTT = 0;
};