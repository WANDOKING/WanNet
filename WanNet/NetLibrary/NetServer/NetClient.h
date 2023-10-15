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
    virtual void OnDisconnect(int errorCode) = 0;

private: // ������ �Լ���

    static unsigned int recvThread(void* netClientSocket);

private:
    SOCKET          mSocket = 0;
    HANDLE          mRecvThread = 0;
    bool            mbIsConnected = false;
    RingBuffer      mRecvBuffer;
    int             mSendErrorCode = 0;
    std::wstring    mServerIP;
    uint16_t        mServerPortNumber;
};