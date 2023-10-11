#pragma once

#include "../Memory/LockFreeObjectPool.h"

template <typename T>
class LockFreeStack
{
public:
    LockFreeStack(void) = default;
    ~LockFreeStack(void) { Clear(); mNodePool.Clear(); }

    LockFreeStack(const LockFreeStack& other) = delete;
    LockFreeStack& operator=(const LockFreeStack& other) = delete;

    inline uint32_t GetCount(void) const { return mCount; }
    inline bool     IsEmpty(void) const { return mCount == 0; }

    void Push(T data)
    {
        Node* newNode = mNodePool.Alloc();
        newNode->Data = data;

        uint64_t localMyIdFlag = ((uint64_t)InterlockedIncrement(&mID) << ADDRESS_BIT_COUNT);
        PVOID newTopHopeToChange = (PVOID)((uint64_t)newNode | localMyIdFlag);

        Node* localMyTop; // ���ÿ��� �ٶ� Top

        do
        {
            // a) Top �����͸� ���ÿ� �����Ѵ�.
            // b) ������ ����� Next�� ���� �ٶ� Top �����ͷ� �����Ѵ�
            // c) ���� Top(mTop)�� ���� �ٶ� Top ������(localMyTop)�� �������� ���ϰ�, 
            //    ���ٸ� mTop�� ���� ���� �� ���(newNode)�� ����

            localMyTop = mTop;
            newNode->Next = localMyTop;

        } while (InterlockedCompareExchangePointer((PVOID*)&mTop, newTopHopeToChange, localMyTop) != localMyTop);

        InterlockedIncrement(&mCount);
    }

    bool TryPop(T& outData)
    {
        Node* localMyTop;		// ���ÿ��� �ٶ� Top
        Node* localMyPureTop;	// ���ÿ��� �ٶ� Top���� ���� 16��Ʈ ID �÷��׸� ������ ��
        Node* localMyNext;		// ���ÿ��� �ٶ� Next

        do
        {
            // a) Top �����͸� ���ÿ� �����Ѵ�.
            //        - ���� �ٶ� Top �����Ͱ� nullptr�̶�� Pop ����
            // b) ���� �ٶ� Top �������� Next�� ���ÿ� �����Ѵ�
            // c) ���� Top(mTop)�� ���� �ٶ� Top ������(localMyTop)�� �������� ���ϰ�, 
            //    ���ٸ� mTop�� ���� �ٶ� Top�� Next(localMyNext)�� ����

            localMyTop = mTop;

            if (localMyTop == nullptr)
            {
                return false;
            }

            localMyPureTop = getPurePointer(localMyTop);
            localMyNext = localMyPureTop->Next;

        } while (InterlockedCompareExchangePointer((PVOID*)&mTop, localMyNext, localMyTop) != localMyTop);

        InterlockedDecrement(&mCount);

        outData = localMyPureTop->Data;

        mNodePool.Free(localMyPureTop);

        return true;
    }

    // ���� ����
    // ���ÿ��� Pop()�� ���������� ������ Ƚ���� ��ȯ
    uint32_t Clear(void)
    {
        uint32_t popCount = 0;
        T ignore;

        while (TryPop(ignore))
        {
            popCount++;
        }

        return popCount;
    }

private:
    struct Node
    {
        T Data;
        Node* Next;
    };

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
    Node*                       mTop = nullptr;
    uint32_t                    mID = 0;
    uint32_t                    mCount = 0;
    LockFreeObjectPool<Node>    mNodePool;
};