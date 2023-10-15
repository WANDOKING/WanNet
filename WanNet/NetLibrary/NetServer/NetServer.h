#pragma once

#include <string>

#include "Serializer.h"
#include "../DataStructure/LockFreeStack.h"

class Session;
typedef void* HANDLE;
typedef unsigned long long SOCKET;

/************************** monitoring variables **************************/
struct MonitoringVariables
{
    uint32_t AcceptTPS;                 // �ʴ� ���� ó�� Ƚ�� (OnAccept ���� Ƚ��)
    uint32_t RecvMessageTPS;            // �ʴ� �޼��� ó�� Ƚ�� (OnReceive ���� Ƚ��)
    uint32_t SendMessageTPS;            // �ʴ� �޼��� �۽� Ƚ��
    uint32_t RecvPendingTPS;
    uint32_t SendPendingTPS;
    uint32_t AverageAcceptTPS;
    uint32_t AverageRecvMessageTPS;
    uint32_t AverageSendMessageTPS;
    uint32_t AverageRecvPendingTPS;
    uint32_t AverageSendPendingTPS;
    float ProcessorTimeTotal;
    float ProcessTimeTotal;
    float ProcessorTimeUser;
    float ProcessTimeUser;
    float ProcessorTimeKernel;
    float ProcessTimeKernel;
};
/************************** monitoring variables **************************/

class NetServer
{
    friend class Session;
    
public:
#pragma warning(push)
#pragma warning(disable: 26495) // ���� �ʱ�ȭ ��� ����
    NetServer() = default;
#pragma warning(pop)

    virtual ~NetServer();

public: // ���� ���� �� ���� �Լ��� (+ TCP Options)

    // TCP_NODELAY �ɼ� ���� (Nagle�� Off�� ���ΰ�)
    inline void SetTcpNodelay(bool bToSet) { mbIsTcpNodelay = bToSet; }

    // SND_BUF = 0 �ɼ� ����
    inline void SetSendBufferSizeToZero(bool bToSet) { mbIsSendBufferSizeZero = bToSet; }

    // �޼����� �ִ� ���� (�ִ� ���̸� �Ѿ�� �޼����� �� ��� ������ ���´�)
    inline void SetMaxPayloadLength(const uint16_t length) { mMaxPayloadLength = length; }

    // ���� ����
    virtual void Start(
        const uint16_t port,
        const uint32_t maxSessionCount,
        const uint32_t iocpConcurrentThreadCount,
        const uint32_t iocpWorkerThreadCount);

    // ���� ���� (��� �����尡 ����� �� ���� �����)
    virtual void Shutdown(void);

public: // ���� ��� �Լ�

    // ��Ŷ ���� ��û - SendQueue�� Enqueue
    void SendPacket(const uint64_t sessionID, Serializer* packet);

    // ���� ���� ��û
    void Disconnect(const uint64_t sessionID);

    // ��Ŷ�� ������ ���� ���� ��û
    void SendAndDisconnect(const uint64_t sessionID, Serializer* packet);

    // ������ �ּҸ� ��´�
    bool GetSessionAddress(const uint64_t sessionID, SOCKADDR_IN* outAddress) const;

public: // Getters

    inline static std::wstring	GetServerVersion(void) { return L"6.7.0"; }

    inline bool					IsRunning(void) const { return mbIsRunning; }
    inline uint16_t				GetPortNumber(void) const { return mPort; }
    inline uint16_t				GetMaxPayloadLength(void) const { return mMaxPayloadLength; }
    inline uint64_t				GetTotalAcceptCount(void) const { return mSessionAcceptedCount; }
    inline uint64_t				GetTotalDisconnectCount(void) const { return mSessionDisconnectedCount; }
    inline MonitoringVariables	GetMonitoringInfo(void) const { return mMonitorResult; }
    inline uint32_t				GetSessionCount(void) const { return mSessionCount; }
    inline uint32_t				GetMaxSessionCount(void) const { return mMaxSessionCount; }

public: // ���� �ڵ鷯 ���� �Լ���

    // ������ ������ �� ȣ���
    // ������ ���ǿ� ���� ���� Recv�� ȣ��Ǳ� ���� �� �Լ��� ȣ��˴ϴ�.
    virtual void OnAccept(const uint64_t sessionID) = 0;

    // �ϼ��� ��Ŷ�� �����Ͽ��� �� ȣ��˴ϴ�.
    // ���޵� ��Ŷ�� ������ �������� �������־�� �մϴ�.
    virtual void OnReceive(const uint64_t sessionID, Serializer* packet) = 0;

    // ������ ������� �� ȣ���
    // �� �Լ��� ȣ��Ǹ� �� �̻� �ش� ����ID�� ��ȿ���� �ʽ��ϴ�.
    virtual void OnRelease(const uint64_t sessionID) = 0;

private: // ������ �Լ���

    static unsigned int acceptThread(void* netServerParam);     // Accept, ���� ���� ����
    static unsigned int iocpWorkerThread(void* netServerParam); // IOCP �̺�Ʈ ó��
    static unsigned int monitorThread(void* netServerParam);    // ����͸� ���� ����

private: // ���� ��ƿ �Լ�

    // ���� ID�� ���� ���� ��ü�� ���´�
    Session* findSessionOrNull(const uint64_t sessionID) const;

private:

    bool				    mbIsRunning;				// ������ ����������
    bool				    mbIsTcpNodelay;				// �ɼ� - TCP_NODELAY�� ����ϴ°�
    bool				    mbIsSendBufferSizeZero;		// �ɼ� - SND_BUF ������ 0
    HANDLE				    mIOCP;						// IOCP �ڵ�
    SOCKET				    mListenSocket;				// ���� ����
    uint16_t			    mPort;						// ��Ʈ ��ȣ
    uint16_t			    mMaxPayloadLength;			// ���̷ε��� �ִ� ���� (Header.Length)
    uint32_t			    mMaxSessionCount;			// ������ �ִ� ���� ����
    uint32_t			    mThreadCount;				// ������ ������ ����
    HANDLE* mThreads;					                // ������ ������ �������
    MonitoringVariables     mMonitoringVariables;		// ����͸� ����� ����ϱ� ���� ����
    MonitoringVariables     mMonitorResult;				// ����͸� ���� ��� (�� �ʸ��� ���ŵ�)
    uint64_t			    mSessionAcceptedCount;		// ������ ���۵� �ĺ��� ���ݱ��� ������ ������ ��
    uint64_t			    mSessionDisconnectedCount;	// ������ ���۵� �ĺ��� ���ݱ��� ���� ������ ��
    uint32_t			    mSessionCount;				// ���� ���� ���� ������ ����

    Session* mSessionList;                              // ���� ����Ʈ (Ǯ)
    LockFreeStack<uint32_t>	mUnusedSessionKeys;         // ������� ���� ���� Ű��
};