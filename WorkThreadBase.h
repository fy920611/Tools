/********************************************************************
	created:	2012/09/13	17:49
	filename: 	WorkThreadBase.h
	author:		Weiqy
	
	purpose:	工作线程基类
*********************************************************************/
#pragma once
#ifndef WorkThreadBase_h__
#define WorkThreadBase_h__
#include <LockHelper.h>

class CWorkThreadBase
{
public:
    CWorkThreadBase(DWORD dwHeartExitTime = INFINITE);
    virtual ~CWorkThreadBase(void);

private:
    CWorkThreadBase(const CWorkThreadBase&);
    CWorkThreadBase& operator=(const CWorkThreadBase&);

public:
    DWORD GetThreadId() const;
    HANDLE GetThreadHandle() const;
    BOOL SurePostMessage(UINT uMsg, WPARAM wp = 0, LPARAM lp = 0);

protected:  // 默认设置为保护权限，是否开启接口由派生类重载决定
    // 启动工作线程
    virtual void Run();
    // 结束工作线程
    virtual void Stop();
    
protected:
    // 线程入口函数
    static unsigned int __stdcall ThreadProc(LPVOID lpParam);
    // 线程执行体
    virtual void DoWork() = 0;

    // 校验运行标志
    bool IsThrdRunning();
    void SetThrdRunning(bool bRunning);

protected:
    HANDLE        m_hThread;        // 线程句柄
    unsigned int  m_dwThreadId;     // 线程ID
    volatile bool m_bThrdRunning;   // 运行标志位
    UINT          m_uHeartTime;     // 心跳时间
    HANDLE        m_hExitEvent;     // 线程退出事件
};
#endif // WorkThreadBase_h__

