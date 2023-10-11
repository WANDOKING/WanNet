#pragma once

#include <cstdint>
#include <Windows.h>

#include "../CrashDump/CrashDump.h"

template <typename T>
class LockFreeObjectPool
{
public:
    LockFreeObjectPool(bool bNeedPlacementNew = false)
        : mbNeedPlacementNew(bNeedPlacementNew)
    {
        checkIdFlagBitCountIsValid();
    }

    LockFreeObjectPool(int capacity, bool bNeedPlacementNew = false)
        : mbNeedPlacementNew(bNeedPlacementNew)
    {
        checkIdFlagBitCountIsValid();

        T** allocAddresses = new T * [capacity];

        for (int i = 0; i < capacity; ++i)
        {
            allocAddresses[i] = Alloc();
        }

        for (int i = 0; i < capacity; ++i)
        {
            Free(allocAddresses[i]);
        }

        delete[] allocAddresses;
    }

    inline uint32_t	GetCapacity() { return mCapacity; }
    inline uint32_t	GetSize() { return mSize; }
    inline bool		IsCallPlacementNewWhenAlloc() { return mbNeedPlacementNew; }

    // ������Ʈ Ǯ�κ��� ������Ʈ�� �Ҵ�޴´�
    T* Alloc()
    {
        Node* retNode;			// ��ȯ�� �ּҸ� ������ ����� �ּ�

        Node* localMyTop;
        Node* localMyNext;

        do
        {
            localMyTop = mTop;

            // ���ο� ��带 �Ҵ��ؼ� ��ȯ
            if (localMyTop == nullptr)
            {
                InterlockedIncrement(&mCapacity);
                retNode = new Node;

                // ������ ȣ��
                new (&(retNode->Data)) T();

                goto END;
            }

            localMyNext = getPurePointer(localMyTop)->Next;

        } while (InterlockedCompareExchangePointer((PVOID*)&mTop, localMyNext, localMyTop) != localMyTop);

        InterlockedDecrement(&mSize);

        retNode = getPurePointer(localMyTop);

        if (mbNeedPlacementNew)
        {
            new (&(retNode->Data)) T();
        }

    END:
#if USING_OBJECT_POOL_OPTION == POOL_OPTION_DEBUG_LOCK_FREE_POOL
        retNode->SafeBlock = (Node*)this;
        retNode->Next = (Node*)this;
#endif

        return &(retNode->Data);
    }

    // ������Ʈ Ǯ�� ������Ʈ�� �ݳ��Ѵ�
    void Free(T* address)
    {
#if USING_OBJECT_POOL_OPTION == POOL_OPTION_DEBUG_LOCK_FREE_POOL
        Node* node = (Node*)((unsigned char*)address - sizeof(Node*));

        // memory pollution detect
        CrashDump::Assert(node->SafeBlock == (Node*)this);
        CrashDump::Assert(node->Next == (Node*)this);

#else
        Node* node = (Node*)address;
#endif

        if (mbNeedPlacementNew)
        {
            address->~T();
        }

        uint64_t localMyIdFlag = (uint64_t)InterlockedIncrement(&mID) << ADDRESS_BIT_COUNT;
        PVOID newTopHopeToChange = (PVOID)((uint64_t)node | localMyIdFlag);
        Node* localMyTop;

        do
        {
            localMyTop = mTop;
            node->Next = localMyTop;
        } while (InterlockedCompareExchangePointer((PVOID*)&mTop, newTopHopeToChange, localMyTop) != localMyTop);

        InterlockedIncrement(&mSize);
    }

    // ������Ʈ Ǯ�� ����
    // �� �Լ��� ������Ʈ Ǯ�� ��� ������Ʈ�� �ݳ��� ��Ȳ�� �ƴ϶�� �����Ѵ�
    void Clear(void)
    {
        CrashDump::Assert(mCapacity == mSize);

        Node* visit = getPurePointer(mTop);

        while (visit != nullptr)
        {
            Node* next = getPurePointer(visit->Next);
            delete visit;
            visit = next;
        }

        mTop = nullptr;
        mID = 0;
        mCapacity = 0;
        mSize = 0;
    }
private:
    // Node
#if USING_OBJECT_POOL_OPTION == POOL_OPTION_DEBUG_LOCK_FREE_POOL
    struct Node
    {
        Node* SafeBlock{};
        T Data{};
        Node* Next{};
    };
#else
    struct Node
    {
        T Data{};
        Node* Next{};
    };
#endif

    // ���� ���� �ּ� �ִ밪�� Ȯ���Ͽ� ID ��Ʈ�� ���� ���� ����� �� �ִ��� Ȯ��
    static void checkIdFlagBitCountIsValid()
    {
        SYSTEM_INFO systemInfo;

        ::GetSystemInfo(&systemInfo);

        CrashDump::Assert(systemInfo.lpMaximumApplicationAddress == (LPVOID)0x0000'7FFF'FFFE'FFFF);
    }

    // ������ �ּ��� ������ ���� ��� (ID ��Ʈ�� ����)
    static inline Node* getPurePointer(Node* address)
    {
        return (Node*)((uint64_t)address & PURE_POINTER_MASK);
    }

    enum : uint64_t
    {
        ID_FLAG_BIT_COUNT = 16,
        ADDRESS_BIT_COUNT = 48,
        PURE_POINTER_MASK = UINT64_MAX >> ID_FLAG_BIT_COUNT
    };
private:
    Node* mTop = nullptr;
    bool		mbNeedPlacementNew; // Alloc()/Free()ȣ�� ��, ������/�Ҹ��ڸ� ȣ�� �� �������� ���� �ɼ�
    uint32_t	mID = 0;			// Top�� ���� 16��Ʈ�� ID�� ����� ��
    uint32_t	mCapacity = 0;
    uint32_t	mSize = 0;
};