#include "StdAfx.h"
#include "WorkThreadBase.h"
#include <process.h>
#include <cassert>

#ifndef NO_XLOGGING
#include "Logging.h"
#include "wPerformLog.h"
#endif

#ifndef verify
#ifdef _DEBUG
#define verify(x) assert(x)
#else
#define verify(x) ((void)(x))
#endif
#endif

CWorkThreadBase::CWorkThreadBase(DWORD dwHeartExitTime /* = INFINITE */)
    : m_hThread(NULL)
    , m_bThrdRunning(false)
    , m_uHeartTime(dwHeartExitTime)
    , m_dwThreadId((UINT)-1)
    , m_hExitEvent(NULL)
{
    m_hExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

CWorkThreadBase::~CWorkThreadBase(void)
{
    Stop();

    // 析构时，先析构子类，再析构父类
    // 有些时候需要先结束线程，再做其他的资源释放工作，因此设定线程的结束必须有父类完成
    assert(m_hThread == NULL);
    assert(m_bThrdRunning == false);

    if(m_hExitEvent)
    {
        CloseHandle(m_hExitEvent);
        m_hExitEvent = NULL;
    }
}

void CWorkThreadBase::Run()
{
    if(m_hThread)
    {
        DWORD dwExitCode = 0;
        verify(GetExitCodeThread(m_hThread, &dwExitCode));
        if(dwExitCode != STILL_ACTIVE)
        {
            CloseHandle(m_hThread);
            m_hThread = NULL;
        }
    }

    ResetEvent(m_hExitEvent);

    if(NULL == m_hThread)
    {
        m_hThread = (HANDLE)_beginthreadex(NULL, 0, ThreadProc, this, CREATE_SUSPENDED, &m_dwThreadId);
        if(-1 == (int)m_hThread)
		{
			int iErrNo = 0;
			_get_errno(&iErrNo);
#ifndef NO_XLOGGING
            LOG_ERROR << "Failed to create a new thread. errno = " << iErrNo;
#endif		
			m_hThread = NULL;
		}
		else if(NULL == m_hThread)
		{
			int iErrNo = 0;
			unsigned long ulDosErrNo = 0;
			_get_errno(&iErrNo);
			_get_doserrno(&ulDosErrNo);
#ifndef NO_XLOGGING
            LOG_ERROR << "Failed to create a new thread. errno = " << iErrNo 
                << " doserrno = " << ulDosErrNo;
#endif
		}
		else 
		{
#ifndef NO_XLOGGING
            LOG_INFO << "启动工作线程：id = " << m_dwThreadId;
#endif        
            m_bThrdRunning = true;
			ResumeThread(m_hThread);
		} 
    }
}

void CWorkThreadBase::Stop()
{
    if(m_hThread)
    {
#ifndef NO_XLOGGING
        char chBuf[128] = {0};
        sprintf_s(chBuf, 128, "工作线程（id = %u）退出", m_dwThreadId);
        wPerformLogSys::CPerformLog oLog(chBuf);
#endif
      
        // 三种退出标识
        SetThrdRunning(false);
        SetEvent(m_hExitEvent);
        SurePostMessage(WM_QUIT, NULL, NULL);

        DWORD dwCode = WaitForSingleObject(m_hThread, m_uHeartTime);
        DWORD dwExitCode = 0;
        if(dwCode != WAIT_OBJECT_0 && GetExitCodeThread(m_hThread, &dwExitCode) && dwExitCode == STILL_ACTIVE)
        {
#ifndef NO_XLOGGING
            LOG_WARN << "Thread: " << m_dwThreadId << " stopped not security.";
#endif
            TerminateThread(m_hThread, (DWORD)-1);
        }

        CloseHandle(m_hThread);
        m_hThread = NULL; 
    }

    ResetEvent(m_hExitEvent);
    assert(IsThrdRunning() == false);
}

unsigned int __stdcall CWorkThreadBase::ThreadProc( LPVOID lpParam )
{
    assert(lpParam);
    CWorkThreadBase* pThis = (CWorkThreadBase*)lpParam;

    pThis->DoWork();
    pThis->SetThrdRunning(false);

    _endthreadex(0);
    return 0;
}

bool CWorkThreadBase::IsThrdRunning()
{
    return m_bThrdRunning;
}

void CWorkThreadBase::SetThrdRunning( bool bRunning )
{
    m_bThrdRunning = bRunning;
}

DWORD CWorkThreadBase::GetThreadId() const
{
    return m_dwThreadId;
}

HANDLE CWorkThreadBase::GetThreadHandle() const
{
    return m_hThread;
}

BOOL CWorkThreadBase::SurePostMessage(UINT uMsg, WPARAM wp /* = 0 */, LPARAM lp /* = 0 */)
{
    BOOL rlt = FALSE;
    int times = 0;
    do 
    {
        rlt = PostThreadMessage(m_dwThreadId, uMsg, wp, lp);
        if(!rlt)
        {
            Sleep(100);
            ++times;
        }
    }while(!rlt && times < 5);	

    return rlt;
}
