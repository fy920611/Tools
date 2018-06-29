// CollectDaemon.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "GuardExe.h"
#include "MonitorCfg.h"
#include "UpdateExe.h"
#include "LockHelper.h"

#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" ) // 设置入口地址 

#define STR_SERVICE_DESCRIPTION         _T("CollectDaemon Service")
#define STR_SERVICE_NAME                _T("CollectDaemon")
#define STR_SERVICE_DISPLAY_NAME        _T("CollectDaemon Service")
#define DELAY_BEFORE_RESTART            60000

void ProcMain();

void ServiceMain(int argc, _TCHAR** argv); 
void ControlHandler(DWORD request); 

void InstallService(LPTSTR lpServiceName, LPTSTR lpExePath);
void RunService(LPCTSTR lpServiceName);
void StopService(LPCTSTR lpServiceName);
void UnInstallService(LPCTSTR lpServiceName);

SERVICE_STATUS                  g_ServiceStatus; 
SERVICE_STATUS_HANDLE           g_hSvcStatus;

std::shared_ptr<CMutexLock>     g_spMutex;


void SentinelThread(PVOID lpParameter)
{
 	CGuardExe guard;
	guard.KillStart();
 	guard.ProcSentinel();
}

void MonitorThread(PVOID lpParameter)
{
	CMonitorCfg monitor;
	monitor.IsCfgChange();
}

void UpdateThread(PVOID lpParameter)
{
	CUpdateExe::GetInstance()->Receive();
}

void SendThread(PVOID lpParameter)
{
	CUpdateExe::GetInstance()->SendGWConfig();
}

int _tmain(int argc, _TCHAR* argv[])
{
	HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, STR_SERVICE_NAME);
	if(NULL != hMutex)
	{
		CloseHandle(hMutex);
		return -1;
	}
	g_spMutex.reset(new CMutexLock(STR_SERVICE_NAME));
	TCHAR lpFileName[MAX_PATH] = {0};
	GetModuleFileName(NULL, lpFileName, MAX_PATH);
	if (argc == 3)
	{
		if((!_tcsicmp(argv[1], _T("-i")) || !_tcsicmp(argv[1], _T("/i"))) && (!_tcsicmp(argv[2], _T("-r")) || !_tcsicmp(argv[2], _T("/r"))))
		{
			InstallService(STR_SERVICE_NAME, lpFileName);
			RunService(STR_SERVICE_NAME);
		}
		else if ((!_tcsicmp(argv[1], _T("-s")) || !_tcsicmp(argv[1], _T("/s"))) && (!_tcsicmp(argv[2], _T("-u")) || !_tcsicmp(argv[2], _T("/u"))))
		{
			StopService(STR_SERVICE_NAME);
			UnInstallService(STR_SERVICE_NAME);
		}
	}
	else if(argc > 1)
	{
		if(!_tcsicmp(argv[1], _T("-i")) || !_tcsicmp(argv[1], _T("/i")))
		{
			InstallService(STR_SERVICE_NAME, lpFileName);
		}
		else if(!_tcsicmp(argv[1], _T("-r")) || !_tcsicmp(argv[1], _T("/r")))
		{
			RunService(STR_SERVICE_NAME);
		}
		else if(!_tcsicmp(argv[1], _T("-s")) || !_tcsicmp(argv[1], _T("/s")))
		{
			StopService(STR_SERVICE_NAME);
		}
		else if(!_tcsicmp(argv[1], _T("-u")) || !_tcsicmp(argv[1], _T("/u")))
		{
			UnInstallService(STR_SERVICE_NAME);
		}
		else if(!_tcsicmp(argv[1], _T("-c")) || !_tcsicmp(argv[1], _T("/c")))
		{
			g_ServiceStatus.dwCurrentState = SERVICE_RUNNING; 
			ProcMain();
		}
	}
	else
	{
		SERVICE_TABLE_ENTRY ServiceTable[2];
		ServiceTable[0].lpServiceName = STR_SERVICE_NAME;
		ServiceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;

		ServiceTable[1].lpServiceName = NULL;
		ServiceTable[1].lpServiceProc = NULL;
		// Start the control dispatcher thread for our service
		StartServiceCtrlDispatcher(ServiceTable);
	}
	return 0;
}

void ServiceMain(int argc, _TCHAR** argv) 
{
	g_ServiceStatus.dwServiceType = SERVICE_WIN32; 
	g_ServiceStatus.dwCurrentState = 	SERVICE_START_PENDING; 
	g_ServiceStatus.dwControlsAccepted  = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	g_ServiceStatus.dwWin32ExitCode = NO_ERROR; 
	g_ServiceStatus.dwServiceSpecificExitCode = 0; 
	g_ServiceStatus.dwCheckPoint = 0; 
	g_ServiceStatus.dwWaitHint = 0; 

	g_hSvcStatus = RegisterServiceCtrlHandler(STR_SERVICE_NAME, (LPHANDLER_FUNCTION)ControlHandler); 
	if (g_hSvcStatus == (SERVICE_STATUS_HANDLE)0) 
	{ 
		LOG_ERROR << "Failed register a function to handle service control requests.";
		return; 
	}

	g_ServiceStatus.dwCurrentState = SERVICE_RUNNING; 
	SetServiceStatus(g_hSvcStatus, &g_ServiceStatus);

	LOG_INFO << "Begin service.";
	ProcMain();
}

void ProcMain()
{
	_beginthread(UpdateThread, 0, NULL);
	_beginthread(SentinelThread, 0, NULL);
	_beginthread(MonitorThread, 0, NULL);
	_beginthread(SendThread, 0, NULL);
	Sleep(INFINITE);
}

void ControlHandler(DWORD request) 
{ 
	switch(request) 
	{ 
	case SERVICE_CONTROL_STOP: 
		g_ServiceStatus.dwWin32ExitCode = NO_ERROR; 
		g_ServiceStatus.dwCurrentState = SERVICE_STOPPED; 
		break; 

	case SERVICE_CONTROL_SHUTDOWN: 
		g_ServiceStatus.dwWin32ExitCode = NO_ERROR; 
		g_ServiceStatus.dwCurrentState = SERVICE_STOPPED; 
		break; 

	default:
		break;
	} 

	SetServiceStatus (g_hSvcStatus, &g_ServiceStatus);
	return; 
}

void InstallService(LPTSTR lpServiceName, LPTSTR lpExePath)
{
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if(NULL != hSCManager)
	{
		//创建并安装服务
		std::string strExePath = _T("\"");
		strExePath += lpExePath;
		strExePath += _T("\"");

		SC_HANDLE hService = CreateService(hSCManager, lpServiceName, STR_SERVICE_DISPLAY_NAME, 
			SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, 
			SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, strExePath.c_str(), NULL, NULL, _T(""), 
			NULL, NULL); 
		if(NULL != hService)
		{
			SERVICE_DESCRIPTION sd;
			sd.lpDescription = STR_SERVICE_DESCRIPTION;
			ChangeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, &sd);

			SC_ACTION sa;
			sa.Type = SC_ACTION_RESTART;
			sa.Delay = DELAY_BEFORE_RESTART; //服务异常关闭后1分钟，重新启动服务
			SERVICE_FAILURE_ACTIONS fa;
			fa.dwResetPeriod = 50;
			fa.lpRebootMsg = NULL;
			fa.lpCommand = NULL;
			fa.cActions = 1;
			fa.lpsaActions = &sa;
			ChangeServiceConfig2(hService, SERVICE_CONFIG_FAILURE_ACTIONS, &fa);

			CloseServiceHandle(hService);
		}
		CloseServiceHandle(hSCManager);
	}
	else
	{
		LOG_ERROR << "Opens the specified service control manager database failed with: " << GetLastError();
	}
}

void RunService(LPCTSTR lpServiceName)
{
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);//与SCM建立连接
	if(NULL != hSCManager)
	{
		SC_HANDLE hService = OpenService(hSCManager, lpServiceName, SERVICE_ALL_ACCESS);//打开已有的服务
		if(NULL != hService)
		{
			StartService(hService, 0, NULL); //启动服务
			CloseServiceHandle(hService);
		}
		CloseServiceHandle(hSCManager);
	}
}

void StopService(LPCTSTR lpServiceName)
{
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if(NULL != hSCManager)
	{
		SC_HANDLE hService = OpenService(hSCManager, lpServiceName, SERVICE_ALL_ACCESS);
		if(NULL != hService)
		{
			SERVICE_STATUS serviceStatus;
			ControlService(hService, SERVICE_CONTROL_STOP, &serviceStatus); //停止服务
			CloseServiceHandle(hService);
		}
		CloseServiceHandle(hSCManager);
	}
}

void UnInstallService(LPCTSTR lpServiceName)
{
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if(NULL != hSCManager)
	{
		SC_HANDLE hService = OpenService(hSCManager, lpServiceName, SERVICE_ALL_ACCESS);
		if(NULL != hService)
		{
			DeleteService(hService); //删除服务
			CloseServiceHandle(hService);
		}
		CloseServiceHandle(hSCManager);
	}
}
