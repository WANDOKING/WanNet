#pragma once

#include <cstring>

class RingBuffer
{
public:
    enum { DEFAULT_SIZE = 4096 * 2 };

    RingBuffer() : mCapacity(DEFAULT_SIZE) { mBuffer = new char[DEFAULT_SIZE]; }
    RingBuffer(int bufferSize) : mCapacity(bufferSize) { mBuffer = new char[bufferSize]; }
    ~RingBuffer() { delete mBuffer; }

    inline void  ClearBuffer(void) { mFront = mRear = 0; }

    inline int   GetCapacity(void) const { return mCapacity; }
    inline int   GetUseSize(void) const { return (mRear - mFront + mCapacity) % mCapacity; }
    inline int   GetFreeSize(void) const { return mCapacity - GetUseSize() - 1; }
    inline char* GetFrontBufferPtr(void) const { return mBuffer + mFront; }
    inline char* GetRearBufferPtr(void) const { return mBuffer + mRear; }

    // ���� �����ͷ� �ܺο��� �ѹ濡 �� �� �ִ� ����
    inline int GetDirectEnqueueSize(void) const
    {
        if (mRear > GetUseSize())
        {
            return mCapacity - mRear;
        }

        return mCapacity - GetUseSize();
    }

    // ���� �����ͷ� �ܺο��� �ѹ濡 ���� �� �ִ� ����
    inline int GetDirectDequeueSize(void) const
    {
        if (mCapacity - mFront < GetUseSize())
        {
            return mCapacity - mFront;
        }

        return GetUseSize();
    }

    inline bool Enqueue(const char* data, int dataSize)
    {
        if (GetFreeSize() < dataSize)
        {
            return false;
        }

        int directEnqueueSize = GetDirectEnqueueSize();
        if (directEnqueueSize >= dataSize)
        {
            memcpy(mBuffer + mRear, data, dataSize);
        }
        else
        {
            int firstEnqueueSize = directEnqueueSize;
            int secondEnqueueSize = dataSize - directEnqueueSize;
            memcpy(mBuffer + mRear, data, firstEnqueueSize);
            memcpy(mBuffer, data + firstEnqueueSize, secondEnqueueSize);
        }

        mRear = (mRear + dataSize) % mCapacity;

        return true;
    }

    inline bool Dequeue(char* data, int dataSize)
    {
        if (GetUseSize() < dataSize)
        {
            return false;
        }

        int directDequeueSize = GetDirectDequeueSize();
        if (directDequeueSize >= dataSize)
        {
            memcpy(data, mBuffer + mFront, dataSize);
        }
        else
        {
            memcpy(data, mBuffer + mFront, directDequeueSize);
            memcpy(data + directDequeueSize, mBuffer, (size_t)dataSize - directDequeueSize);
        }

        mFront = (mFront + dataSize) % mCapacity;

        return true;
    }

    inline bool Peek(char* data, int dataSize) const
    {
        if (GetUseSize() < dataSize)
        {
            return false;
        }

        int directDequeueSize = GetDirectDequeueSize();
        if (directDequeueSize >= dataSize)
        {
            memcpy(data, mBuffer + mFront, dataSize);
        }
        else
        {
            memcpy(data, mBuffer + mFront, directDequeueSize);
            memcpy(data + directDequeueSize, mBuffer, (size_t)dataSize - directDequeueSize);
        }

        return true;
    }

    // Front ���� �̵�(����), �̵� ���� ���θ� ��ȯ
    inline bool MoveFront(int size)
    {
        if (GetUseSize() < size)
        {
            return false;
        }

        mFront = (mFront + size) % mCapacity;

        return true;
    }

    // Rear ���� �̵�(���ǹ��� ��� ����), �̵� ���� ���θ� ��ȯ
    inline bool MoveRear(int size)
    {
        if (mCapacity - GetUseSize() < size)
        {
            return false;
        }

        mRear = (mRear + size) % mCapacity;

        return true;
    }
private:
    char* mBuffer;
    int	mCapacity;
    int	mFront = 0;
    int	mRear = 0;
};