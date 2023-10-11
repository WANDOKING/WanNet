#include "NetUtils.h"

#include "Session.h"
#include "../Logger/Logger.h"
#include "NetworkHeader.h"
#include "Serializer.h"
#include "NetServer.h"
#include "../Profiler/Profiler.h"

void Session::Init(const SOCKET sock, const SOCKADDR_IN address, NetServer* netServer, const uint64_t sessionID, const uint32_t sessionListKey)
{
    // ������ �÷��׸� ����Ѵ� - �ش� ������ IoCount�� �ٸ� ���ǿ��� �ǵ帱 ������ �ֱ⿡ Interlocked �ʿ�
    InterlockedAnd(reinterpret_cast<LONG*>(&IoCount), 0x7FFF'FFFF);

    ID = sessionID;
    Socket = sock;
    Address = address;
    Server = netServer;

    ::memset(&SendOverlapped, 0, sizeof(OVERLAPPED));
    ::memset(&RecvOverlapped, 0, sizeof(OVERLAPPED));

    bSendFlag = 0;
    SessionListKey = sessionListKey;
    bDisconnected = false;
    bDisconnectRegistered = false;

    RecvBuffer.ClearBuffer();

    while (!SendQueue.IsEmpty())
    {
        Serializer* packet;
        SendQueue.TryDequeue(packet);
        packet->DecrementRefCount();
    }

    for (uint32_t i = 0; i < RegisteredPacketCount; ++i)
    {
        RegisteredPackets[i]->DecrementRefCount();
    }

    RegisteredPacketCount = 0;
}

bool Session::DecrementIoCount()
{
    // Release Flag�� false�̰�, IoCount�� 1�� ����� Release Flag�� set�ϰ� IoCount�� 1���δ�
    // �̰��� ���������� �����ϱ� ���� �ڵ�

    for (;;)
    {
        if (InterlockedCompareExchange(&IoCount, 0x8000'0000, 0x0000'0001) == 0x0000'0001)
        {
            break;
        }

        if (InterlockedDecrement(&IoCount) == 0)
        {
            InterlockedIncrement(&IoCount);
            continue;
        }

        return false;
    }

    /************************** release session **************************/

    ASSERT_LIVE(TryClosesocket(), L"TryClosesocket failed");

    InterlockedIncrement(&Server->mSessionDisconnectedCount);
    InterlockedDecrement(&Server->mSessionCount);

    // OnRelease ȣ���� �ٸ� ������� ������ ��Ͷ��� ���� ������ ������ ȸ���Ѵ�
    ::PostQueuedCompletionStatus(Server->mIOCP, 0, ID, 0);

    // ���� Ű �ε��� �ݳ�
    Server->mUnusedSessionKeys.Push(SessionListKey);

    return true;
}

bool Session::PostRecv()
{
    if (bDisconnected || bDisconnectRegistered)
    {
        return false;
    }

    WSABUF wsabuf;
    DWORD flags = 0;

    wsabuf.buf = RecvBuffer.GetRearBufferPtr();
    wsabuf.len = RecvBuffer.GetDirectEnqueueSize();

    ::ZeroMemory(&RecvOverlapped, sizeof(RecvOverlapped));

    IncrementIoCount();

    PROFILE_BEGIN(L"WSARecv");
    int retWSARecv = ::WSARecv(Socket, &wsabuf, 1, NULL, &flags, &RecvOverlapped, NULL);
    PROFILE_END(L"WSARecv");

    bool ret = false;

    if (retWSARecv == SOCKET_ERROR)
    {
        int errorCode = ::WSAGetLastError();
        switch (errorCode)
        {
        case WSA_IO_PENDING: // IO PENDING
        {
            if (bDisconnectRegistered)
            {
                ::CancelIoEx((HANDLE)Socket, NULL);
            }

            InterlockedIncrement(&Server->mMonitoringVariables.RecvPendingTPS);
        }
        break;
        case WSAECONNRESET:
        case WSAECONNABORTED:
        {
            ret = DecrementIoCount();
        }
        break;
        case WSAEINTR:
        {
            LOGF(ELogLevel::System, L"WSARecv WSAEINTR Error (errorCode = %d)", errorCode);
            ret = DecrementIoCount();
        }
        break;
        default:
            LOGF(ELogLevel::Error, L"WSARecv Error (errorCode = %d)", errorCode);
            Logger::RaiseCrash();
        }
    }

    return ret;
}

bool Session::PostSend()
{
    uint32_t sendCount = SendQueue.GetCount();

    if (sendCount <= 0 || bDisconnected || bDisconnectRegistered)
    {
        return false;
    }

    if (InterlockedExchange(&bSendFlag, 1))
    {
        return false;
    }

    sendCount = SendQueue.GetCount();

    if (sendCount <= 0)
    {
        ASSERT_LIVE(InterlockedExchange(&bSendFlag, 0) == 1, L"more than 1 Send Error");

        return false;
    }

    WSABUF wsabuf[MAX_WSA_BUF_COUNT];
    int wsaBufCount;

    for (wsaBufCount = 0; wsaBufCount < MAX_WSA_BUF_COUNT; ++wsaBufCount)
    {
        if (sendCount == 0)
        {
            break;
        }

        Serializer* packet;
        bool retTryDequeue = SendQueue.TryDequeue(packet);
        ASSERT_LIVE(retTryDequeue, L"SendQueue TryDequeue Failed");

        sendCount--;

        wsabuf[wsaBufCount].buf = packet->GetFullBufferPointer();
        wsabuf[wsaBufCount].len = packet->GetFullSize();

        RegisteredPackets[RegisteredPacketCount++] = packet;
    }

    ::ZeroMemory(&SendOverlapped, sizeof(SendOverlapped));

    IncrementIoCount();

    PROFILE_BEGIN(L"WSASend");
    int retWSASend = ::WSASend(Socket, wsabuf, wsaBufCount, NULL, 0, &SendOverlapped, NULL);
    PROFILE_END(L"WSASend");

    bool ret = false;

    if (retWSASend == SOCKET_ERROR)
    {
        int errorCode = ::WSAGetLastError();
        switch (errorCode)
        {
        case WSA_IO_PENDING: // IO PENDING
        {
            if (bDisconnectRegistered)
            {
                ::CancelIoEx((HANDLE)Socket, NULL);
            }

            InterlockedIncrement(&Server->mMonitoringVariables.SendPendingTPS);
        }
        break;
        case WSAECONNRESET:
        case WSAECONNABORTED:
        {
            ret = DecrementIoCount();
        }
        break;
        case WSAEINTR:
        {
            LOGF(ELogLevel::System, L"WSASend WSAEINTR Error (errorCode = %d)", errorCode);
            ret = DecrementIoCount();
        }
        break;
        default:
            LOGF(ELogLevel::Error, L"WSASend Error (errorCode = %d)", errorCode);
            Logger::RaiseCrash();
        }
    }

    return ret;
}

bool Session::TryClosesocket()
{
    if (InterlockedExchange8((CHAR*)&bDisconnected, true))
    {
        return false;
    }

    int retClosesocket = ::closesocket(Socket);

    if (retClosesocket == SOCKET_ERROR)
    {
        LOGF(ELogLevel::Error, L"closesocket() Error : %d", WSAGetLastError());
        Logger::RaiseCrash();
    }

    return true;
}
