#pragma once

// ����� Ǯ�� �ɼ�
#define USING_OBJECT_POOL_OPTION POOL_OPTION_TLS_POOl

///////////////////////////////////////////////////////////////////////////////
// [����]
// OBJECT_POOL<Object> pool;
// Object* obj = pool.Alloc();
// pool.Free(obj);
///////////////////////////////////////////////////////////////////////////////

// TLS ������ƮǮ
#define POOL_OPTION_TLS_POOl 1

// ���� ��� TLS ������Ʈ Ǯ
#define POOL_OPTION_DEBUG_TLS_POOL 2

// �� ���� ������ƮǮ
#define POOL_OPTION_LOCK_FREE_POOl 3

// ���� ��� �� ���� ������ƮǮ
#define POOL_OPTION_DEBUG_LOCK_FREE_POOL 4

// ��Ÿ�� �����÷ο� ������
#define POOL_OPTION_OVERFLOW_CHECKER 5

// �⺻ new, delete�� ���
#define POOL_OPTION_NEW_DELETE 6

#pragma region Ǯ �ɼǿ� ���� ��ó���� ����

#if USING_OBJECT_POOL_OPTION == POOL_OPTION_TLS_POOl || USING_OBJECT_POOL_OPTION == POOL_OPTION_DEBUG_TLS_POOL

#define OBJECT_POOL thread_local TlsObjectPool

#elif USING_OBJECT_POOL_OPTION == POOL_OPTION_LOCK_FREE_POOl || USING_OBJECT_POOL_OPTION == POOL_OPTION_DEBUG_LOCK_FREE_POOL

#define OBJECT_POOL LockFreeObjectPool

#elif USING_OBJECT_POOL_OPTION == POOL_OPTION_OVERFLOW_CHECKER

#define OBJECT_POOL OverflowChecker

#elif USING_OBJECT_POOL_OPTION == POOL_OPTION_NEW_DELETE

template <typename T>
class NewDeleteAllocator
{
public:
    static T* Alloc(void)
    {
        return new T;
    }

    static void Free(T* address)
    {
        delete address;
    }
};

#define OBJECT_POOL NewDeleteAllocator

#else
static_assert(false, "invalid pool option selected");
#endif

#pragma endregion