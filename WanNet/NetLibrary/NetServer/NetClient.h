#pragma once

#include <string>
#include <cstdint>

#include "RingBuffer.h"

typedef unsigned long long SOCKET;
typedef void* HANDLE;

class RingBuffer;
class Serializer;

class NetClient
{
public:
    NetClient(const std::wstring serverIP, const uint16_t serverPortNumber);

    virtual ~NetClient(void) = default;

    // ������ ����
    bool Connect(void);

    // ���� ����
    void Disconnect(void);

    // ��Ŷ ������
    bool SendPacket(Serializer* packet);

public: // Getters

    inline bool IsConnected(void) { return mbIsConnected; }
    inline int  GetSendErrorCode(void) { return mSendErrorCode; }

public: // Ŭ���̾�Ʈ �ڵ鷯 ���� �Լ�

    virtual void OnReceive(Serializer* packet) = 0;

private: // ������ �Լ���

    static unsigned int recvThread(void* netClientSocket);

private:
    SOCKET mSocket = 0;
    bool mbIsConnected = false;
    std::wstring mServerIP;
    uint16_t mServerPortNumber;

    RingBuffer mRecvBuffer;

    int mSendErrorCode = 0;

    HANDLE mRecvThread = 0;
};