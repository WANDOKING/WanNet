///////////////////////////////////////////////////////////////////////////////
// TLS ������Ʈ Ǯ
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// ������ƮǮ�� ���� ���
// ���� ��� ��� �� ������Ʈ �� �ڷ� ������Ʈ Ǯ �Ŵ����� this �����͸� ���δ�.
// Free()�� ���޹��� �������� �� �ڸ� �����Ͽ� �� ���� �����Ǿ����� Ȯ���Ѵ�.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <Windows.h>

#include "ObjectPool.h"
#include "../CrashDump/CrashDump.h"

template <typename T>
class TlsObjectPool
{
public:
    inline static bool      IsCallPlacementNewWhenAlloc(void) { return mbNeedPlacementNew; }
    inline static uint32_t  GetObjectPerChunkCount(void) { return OBJECT_COUNT_PER_CHUNK; }
    inline static uint32_t  GetTotalChunkCount(void) { return mPoolManager.mChunkTotalCount; }
    inline static uint32_t  GetTotalCreatedObjectCount(void) { return mPoolManager.mChunkTotalCount * OBJECT_COUNT_PER_CHUNK; }

    // �̸� ûũ�� ����� ���´�
    static void PreCreateChunk(uint32_t chunkCount)
    {
        for (uint32_t i = 0; i < chunkCount; ++i)
        {
            mPoolManager.CreateChunk();
        }
    }

public:
    TlsObjectPool(bool bNeedPlacementNew = false) { mbNeedPlacementNew = bNeedPlacementNew; }

    inline uint32_t GetSize(void) const { return mSize; }

    // ������Ʈ Ǯ�κ��� ������Ʈ�� �Ҵ�޴´�
    T* Alloc(void)
    {
        // ����ִٸ� ������Ʈ Ǯ �Ŵ����κ��� ������Ʈ ����� �����´�
        if (mTop == nullptr)
        {
            mTop = mPoolManager.AllocChunk();
            mSize = OBJECT_COUNT_PER_CHUNK;
        }

        Node* retNode = mTop;
        mTop = mTop->Next;
        --mSize;

        if (mbNeedPlacementNew)
        {
            new (&(retNode->Data)) T();
        }

#if USING_OBJECT_POOL_OPTION == POOL_OPTION_DEBUG_POOL
        retNode->SafeBlock = (Node*)(&mPoolManager);
        retNode->Next = (Node*)(&mPoolManager);
#endif

        return &(retNode->Data);
    }

    // ������Ʈ Ǯ�� ������Ʈ�� �ݳ��Ѵ�
    void Free(T* address)
    {
#if USING_OBJECT_POOL_OPTION == POOL_OPTION_DEBUG_POOL
        Node* node = (Node*)((unsigned char*)address - sizeof(Node*));

        // �޸� ���� üũ
        CrashDump::Assert(node->SafeBlock == (Node*)(&mPoolManager));
        CrashDump::Assert(node->Next == (Node*)(&mPoolManager));
#else
        Node* node = (Node*)address;
#endif

        if (mbNeedPlacementNew)
        {
            address->~T();
        }

        node->Next = mTop;
        mTop = node;
        ++mSize;

        if (mSize == MAX_OBJECT_COUNT_PER_THREAD)
        {
            Node* newTop = mHalfTop->Next;
            mHalfTop->Next = nullptr;
            mPoolManager.FreeChunk(mTop);
            mTop = newTop;
            mSize -= OBJECT_COUNT_PER_CHUNK;
        }
        else if (mSize == OBJECT_COUNT_PER_CHUNK + 1)
        {
            mHalfTop = mTop;
        }
    }

private:

#if USING_OBJECT_POOL_OPTION == POOL_OPTION_DEBUG_POOL
    struct Node
    {
        Node* SafeBlock;
        T Data;
        Node* Next;
    };
#else
    struct Node
    {
        T Data;
        Node* Next;
    };
#endif

    enum
    {
        MAX_CHUNK_COUNT = 100'000,
        OBJECT_COUNT_PER_CHUNK = 500,
        MAX_OBJECT_COUNT_PER_THREAD = OBJECT_COUNT_PER_CHUNK * 2,
    };

private:

    // Ǯ ���� �ϳ��� ������ �߾� Ǯ ������
    // Ǯ �Ŵ����κ��� ûũ�� �Ҵ�޴´�
    class ObjectPoolManager
    {
    public:
        ObjectPoolManager(void) { ::InitializeSRWLock(&mLock); }

        // ûũ�� �Ҵ�޴´�
        Node* AllocChunk(void)
        {
            Node* ret = nullptr;
            bool bIsEmpty;

            {
                ::AcquireSRWLockExclusive(&mLock);

                bIsEmpty = mChunkInManagerCount == 0;

                if (!bIsEmpty)
                {
                    --mChunkInManagerCount;
                    ret = mChunks[mChunkInManagerCount];
                }

                ::ReleaseSRWLockExclusive(&mLock);
            }

            if (bIsEmpty)
            {
                // ���Ӱ� ûũ�� ����� ��ȯ�Ѵ�

                Node* prevNode = nullptr;

                for (int i = 0; i < OBJECT_COUNT_PER_CHUNK; ++i)
                {
                    Node* newNode = new Node;
                    newNode->Next = prevNode;
                    prevNode = newNode;
                }

                ret = prevNode;

                CrashDump::Assert(InterlockedIncrement(&mChunkTotalCount) <= MAX_CHUNK_COUNT);
            }

            return ret;
        }

        // ûũ ��ȯ
        void FreeChunk(Node* chunkTop)
        {
            ::AcquireSRWLockExclusive(&mLock);

            mChunks[mChunkInManagerCount] = chunkTop;
            ++mChunkInManagerCount;

            ::ReleaseSRWLockExclusive(&mLock);
        }

        // ûũ�� ���� Ǯ �Ŵ����� ������ ���´�
        void CreateChunk(void)
        {
            Node* prevNode = nullptr;

            for (int i = 0; i < OBJECT_COUNT_PER_CHUNK; ++i)
            {
                Node* newNode = new Node;
                newNode->Next = prevNode;
                prevNode = newNode;
            }

            CrashDump::Assert(InterlockedIncrement(&mChunkTotalCount) <= MAX_CHUNK_COUNT);

            ::AcquireSRWLockExclusive(&mLock);

            mChunks[mChunkInManagerCount] = prevNode;
            ++mChunkInManagerCount;

            ::ReleaseSRWLockExclusive(&mLock);
        }

    public:
        SRWLOCK mLock;
        Node* mChunks[MAX_CHUNK_COUNT]{};
        uint32_t mChunkInManagerCount = 0;
        uint32_t mChunkTotalCount = 0;
    };

private:
    Node* mTop = nullptr;
    Node* mHalfTop = nullptr; // OBJECT_COUNT_PER_CHUNK + 1 ��° ��带 ����Ų��
    uint32_t mSize = 0;

    inline static bool mbNeedPlacementNew;  // Alloc()/Free()ȣ�� ��, ������/�Ҹ��ڸ� ȣ�� �� �������� ���� �ɼ�
    inline static ObjectPoolManager mPoolManager;
};