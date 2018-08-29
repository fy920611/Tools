/********************************************************************
	created:	2014/01/07	17:16
	filename: 	LockHelper.h
	author:		Weiqy
	
	purpose:	锁工具类，提供各种锁
*********************************************************************/
#pragma once
#include <Windows.h>
#include <assert.h>

struct ISmartLock
{
    virtual void Lock()     = 0;
    virtual void Unlock()   = 0;
};

class CCriticalSectionLock : public ISmartLock
{
public:
    CCriticalSectionLock()
    {
        InitializeCriticalSection(&m_CS);
    }
    
    ~CCriticalSectionLock()
    {
        DeleteCriticalSection(&m_CS);
    }

private: // 拒绝拷贝
    CCriticalSectionLock(const CCriticalSectionLock&);
    CCriticalSectionLock& operator=(const CCriticalSectionLock&);
    
public:
    void Lock()
    {
        EnterCriticalSection(&m_CS);
    }
    
    void Unlock()
    {
        LeaveCriticalSection(&m_CS);
    }

    CRITICAL_SECTION* Host()
    {
        return &m_CS;
    }
    
private:
    CRITICAL_SECTION	m_CS;
};

class CMutexLock : public ISmartLock
{
public:
    CMutexLock(LPCTSTR name)
    {
        m_hMutex = CreateMutex(NULL, FALSE, name);
        DWORD dwErrorId = GetLastError();
        m_bAlreadyExist = (dwErrorId == ERROR_ALREADY_EXISTS || dwErrorId == ERROR_ACCESS_DENIED);
    }
    ~CMutexLock()
    {
        if(m_hMutex)
        {
            CloseHandle(m_hMutex);
            m_hMutex = NULL;
        }
    }

private: // 拒绝拷贝
    CMutexLock(const CMutexLock&);
    CMutexLock& operator=(const CMutexLock&);
    
public:
    void Lock()
    {
        assert(m_hMutex != NULL);
		if(m_hMutex)
		{
			WaitForSingleObject(m_hMutex,INFINITE);
		}
    }
    
    void Unlock()
    {
        assert(m_hMutex != NULL);
		if(m_hMutex)
		{
			ReleaseMutex(m_hMutex);
		}
    }

    BOOL IsValid() const
    {
        return m_hMutex != NULL;
    }

    bool IsAlreadyExist() const
    {
        return m_bAlreadyExist;
    }
    
public:
    HANDLE	m_hMutex;
    bool    m_bAlreadyExist;
};

class LockHelper
{
public:
    LockHelper(ISmartLock* pLock):m_pLock(pLock)
    {
        if(m_pLock)
        {
            m_pLock->Lock();
        }
    }
    
    ~LockHelper()
    {
        if(m_pLock)
        {
            m_pLock->Unlock();
        }
    }

private: // 拒绝拷贝
    LockHelper(const LockHelper&);
    LockHelper& operator=(const LockHelper&);
    
private:
    ISmartLock* m_pLock;
};

#define SCOPED_SAFELOCK(cs) LockHelper lock(cs)
#define SCOPED_SAFELOCK1(cs) LockHelper lock1(cs)
#define SCOPED_SAFELOCK2(cs) LockHelper lock2(cs)
