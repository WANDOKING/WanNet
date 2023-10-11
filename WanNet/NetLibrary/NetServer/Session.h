#pragma once

#include <unordered_map>
#include <stack>

#include "RingBuffer.h"
#include "Serializer.h"
#include "../DataStructure/LockFreeQueue.h"

class NetServer;

class Session
{
	friend class NetServer;
private:

#pragma warning(push)
#pragma warning(disable: 26495) // ���� �ʱ�ȭ ��� ����
	Session(void) = default;
#pragma warning(pop)

	~Session() = default;

	// ���� ��ü �ʱ�ȭ
	void Init(const SOCKET sock, const SOCKADDR_IN address, NetServer* netServer, const uint64_t sessionID, const uint32_t sessionListKey);

	// IO Count�� ����(Interlocked)
	inline uint32_t IncrementIoCount(void) { return InterlockedIncrement(&IoCount); }

	// IO Count�� ����(Interlocked), 0�̶�� release
	bool DecrementIoCount(void);

	// RecvBuffer�� ������� WSARecv()�� ȣ��
	bool PostRecv(void);

	// SendBuffer�� ������� WSASend()�� ȣ��, ���������� bSendFlag�� WSASend�� 1ȸ ������
	// WSASend()�� ���� �Ϸ� ������ ���� �� Flag�� 0���� �������ְ� �ٽ� PostSend()�� ȣ���� ��
	bool PostSend(void);

	// bDisconnected�� false���, true�� �����ϰ� ������ �ݴ´�
	bool TryClosesocket(void);

private:

	enum
	{
		MAX_WSA_BUF_COUNT = 10
	};
	
	uint64_t					ID;			// [SessionList key(index) 32bit][ID++ 32bit]	
	SOCKET						Socket;
	SOCKADDR_IN					Address;
	NetServer*					Server;

	OVERLAPPED					SendOverlapped;
	OVERLAPPED					RecvOverlapped;
	
	uint32_t					IoCount;	// IoCount�� �ֻ��� ��Ʈ�� Release Flag�� ���
	uint32_t					bSendFlag;
	uint32_t					SessionListKey;
	bool						bDisconnected;
	bool						bDisconnectRegistered;

	RingBuffer					RecvBuffer;
	LockFreeQueue<Serializer*>	SendQueue;
	uint32_t					RegisteredPacketCount;
	Serializer*					RegisteredPackets[MAX_WSA_BUF_COUNT];
};