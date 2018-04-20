#pragma once
#include <string>
#include <assert.h>

class CSingleAppDetecter
{
public:
    CSingleAppDetecter(const std::string& sInstanceName)
        : m_hMutex(NULL)
        , m_sMutexName(sInstanceName)
    {
    }

    ~CSingleAppDetecter()
    {
        Close();
    }

    // 一个实例只能调用一次
    BOOL AnotherInstanceExists()
    {
        assert(NULL == m_hMutex);
        m_hMutex = CreateMutex(NULL, FALSE, m_sMutexName.c_str());
        if (GetLastError() == ERROR_ALREADY_EXISTS) 
        {
            ::CloseHandle(m_hMutex);
            m_hMutex = NULL;
            return TRUE;
        }

        return FALSE;
    }

    void Close()
    {
        if(m_hMutex)
        {
            CloseHandle(m_hMutex);
            m_hMutex = NULL;
        }
    }

private:
    HANDLE m_hMutex;
    std::string m_sMutexName;
};
