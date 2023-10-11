#pragma once

#include <cstdint>

#define NETWORK_HEADER_TYPE_LAN 0 // LAN ���� : ���̸� ����
#define NETWORK_HEADER_TYPE_NET 1 // NET ���� : ���� �ڵ� �߰� �� ��Ŷ ��ȣȭ/��ȣȭ ����

#define NETWORK_HEADER_USE_TYPE NETWORK_HEADER_TYPE_NET

#if NETWORK_HEADER_USE_TYPE == NETWORK_HEADER_TYPE_LAN

struct NetworkHeader
{
	uint16_t Length;  // ���̷ε��� ���� (��� ����)
};

static_assert(sizeof(NetworkHeader) == 2);

#elif NETWORK_HEADER_USE_TYPE == NETWORK_HEADER_TYPE_NET

#define NETWORK_HEADER_CODE 0x77      // ������ ��Ŷ�� �ɷ����� ���� ���� ��
#define NETWORK_HEADER_FIXED_KEY 0x32 // �޽��� ��ȣȭ�� ���� ���� Ű

#pragma pack (push,1)
struct NetworkHeader
{
    uint8_t     Code;       // NETWORK_HEADER_CODE ���� ��
    uint16_t    Length;     // ���̷ε��� ���� (��� ����)
    uint8_t     RandKey;    // �޽��� ��ȣȭ�� ���� ���� Ű
    uint8_t     CheckSum;   // ��ȣȭ �� ������ �Ǵ��� ���� üũ��
};
#pragma pack (pop)

static_assert(sizeof(NetworkHeader) == 5);

#endif