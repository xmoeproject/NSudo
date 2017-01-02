﻿// NSudo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

using namespace M2;

HINSTANCE g_hInstance;

HANDLE g_hOut;
HANDLE g_hIn;
HANDLE g_hErr;

wchar_t g_ExePath[MAX_PATH];
wchar_t g_AppPath[MAX_PATH];
wchar_t g_ShortCutListPath[MAX_PATH];

int g_argc;
wchar_t** g_argv;

bool g_GUIMode;

CNSudo *g_pNSudo = nullptr;

namespace ProjectInfo
{
	wchar_t VersionText[] = L"NSudo 4.2 (Build 1612)";
}

// CreateProcessWithToken 和 CreateProcessAsUser 的选项
const DWORD dwFlags = CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT;

// 为编译通过而禁用的警告
#if _MSC_VER >= 1200
#pragma warning(push)
#pragma warning(disable:4191) // 从“type of expression”到“type required”的不安全转换(等级 3)
#endif

// 开启对话框Per-Monitor DPI Aware支持（Win10可用）
inline int EnablePerMonitorDialogScaling()
{
	typedef int(WINAPI *PFN_EnablePerMonitorDialogScaling)();

	PFN_EnablePerMonitorDialogScaling pFunc = nullptr;

	LdrGetProcedureAddress(
		GetModuleHandleW(L"user32.dll"),
		nullptr, 2577,
		reinterpret_cast<PVOID*>(&pFunc));

	return (pFunc ? pFunc() : -1);
}

#if _MSC_VER >= 1200
#pragma warning(pop)
#endif

bool SuCreateProcessLegacy(
	_In_opt_ HANDLE hToken,
	_Inout_ LPWSTR lpCommandLine)
{
	bool bRet = true;

	STARTUPINFOW StartupInfo = { 0 };
	PROCESS_INFORMATION ProcessInfo = { 0 };
	wchar_t szBuf[512], szSystemDirectory[MAX_PATH];

	//设置进程所在桌面
	StartupInfo.lpDesktop = L"WinSta0\\Default";

	//获取系统目录
	GetSystemDirectoryW(szSystemDirectory, MAX_PATH);

	//生成命令行	
	wsprintfW(szBuf,
		L"%s\\cmd.exe /c start \"%s\\cmd.exe\" %s",
		szSystemDirectory, szSystemDirectory, lpCommandLine);

	//启动进程
	if (!CreateProcessAsUserW(
		hToken,
		nullptr,
		szBuf,
		nullptr,
		nullptr,
		FALSE,
		dwFlags,
		nullptr,
		g_AppPath,
		&StartupInfo,
		&ProcessInfo))
	{
		if (!CreateProcessWithTokenW(
			hToken,
			LOGON_WITH_PROFILE,
			nullptr,
			szBuf,
			dwFlags,
			nullptr,
			g_AppPath,
			&StartupInfo,
			&ProcessInfo))
		{
			bRet = false;
		}
	}

	//关闭句柄
	if (bRet)
	{
		NtClose(ProcessInfo.hProcess);
		NtClose(ProcessInfo.hThread);
	}

	//返回结果
	return bRet;
}

bool SuCreateProcess(
	_In_opt_ HANDLE hToken,
	_Inout_ LPWSTR lpCommandLine)
{
	bool bRet = true;

	STARTUPINFOW StartupInfo = { 0 };
	PROCESS_INFORMATION ProcessInfo = { 0 };
	wchar_t szBuf[512];

	//设置进程所在桌面
	StartupInfo.lpDesktop = L"WinSta0\\Default";

	//生成命令行	
	wsprintfW(szBuf, L"%s -Execute %s", g_ExePath, lpCommandLine);

	//启动进程
	if (!CreateProcessAsUserW(
		hToken, 
		nullptr, 
		szBuf,
		nullptr,
		nullptr,
		FALSE, 
		dwFlags,
		nullptr,
		g_AppPath, 
		&StartupInfo,
		&ProcessInfo))
	{
		if (!CreateProcessWithTokenW(
			hToken,
			LOGON_WITH_PROFILE, 
			nullptr,
			szBuf,
			dwFlags,
			nullptr, 
			g_AppPath, 
			&StartupInfo,
			&ProcessInfo))
		{
			bRet = false;
		}
	}

	//关闭句柄
	if (bRet)
	{
		NtClose(ProcessInfo.hProcess);
		NtClose(ProcessInfo.hThread);
	}

	//返回结果
	return bRet;
}

void SuInitialize()
{
	g_hInstance = GetModuleHandleW(nullptr);

	g_hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	g_hIn = GetStdHandle(STD_INPUT_HANDLE);
	g_hErr = GetStdHandle(STD_ERROR_HANDLE);

	g_argv = CommandLineToArgvW(GetCommandLineW(), &g_argc);

	GetModuleFileNameW(nullptr, g_ExePath, MAX_PATH);

	wcscpy_s(g_AppPath, MAX_PATH, g_ExePath);
	wcsrchr(g_AppPath, L'\\')[0] = L'\0';

	wcscpy_s(g_ShortCutListPath, MAX_PATH, g_AppPath);
	wcscat_s(g_ShortCutListPath, MAX_PATH, L"\\ShortCutList.ini");

	g_GUIMode = (g_argc == 1);

	g_pNSudo = new CNSudo();
}

void SuMUIPrintMsg(
	_In_opt_ HINSTANCE hInstance,
	_In_opt_ HWND hWnd,
	_In_ UINT uID)
{
	DWORD result;
	CPtr<wchar_t*> szBuffer;
	if (szBuffer.Alloc(2048 * sizeof(wchar_t)))
	{
		LoadStringW(hInstance, uID, szBuffer, 2048);
		if (g_GUIMode) TaskDialog(
			hWnd, hInstance, L"NSudo", nullptr, szBuffer, 0, nullptr, nullptr);
		else WriteConsoleW(g_hOut, szBuffer, (DWORD)wcslen(szBuffer), &result, nullptr);
	}
}

void SuMUIInsComboBox(
	_In_opt_ HINSTANCE hInstance,
	_In_opt_ HWND hWnd,
	_In_ UINT uID)
{
	CPtr<wchar_t*> szBuffer;
	if (szBuffer.Alloc(260 * sizeof(wchar_t)))
	{
		LoadStringW(hInstance, uID, szBuffer, 260);
		SendMessageW(hWnd, CB_INSERTSTRING, 0, (LPARAM)(wchar_t*)szBuffer);
	}
}

void NSudoBrowseDialog(
	_In_opt_ HWND hWnd,
	_Out_ wchar_t* szPath)
{
	OPENFILENAME ofn = { 0 };

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.nMaxFile = MAX_PATH;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.lpstrFile = szPath;
	ofn.Flags = OFN_HIDEREADONLY | OFN_CREATEPROMPT;

	GetOpenFileNameW(&ofn);
}

bool SuMUICompare(
	_In_opt_ HINSTANCE hInstance,
	_In_ UINT uID,
	_In_ LPCWSTR lpText)
{
	bool bRet = false;
	CPtr<wchar_t*> szBuffer;
	if (szBuffer.Alloc(2048 * sizeof(wchar_t)))
	{
		LoadStringW(hInstance, uID, szBuffer, 2048);		
		bRet = (_wcsicmp(szBuffer, lpText) == 0);
	}
	return bRet;
}


void SuGUIRun(
	_In_ HWND hDlg,
	_In_ LPCWSTR szUser,
	_In_ bool bEnableAllPrivileges,
	_In_ LPCWSTR szCMDLine)
{
	if (_wcsicmp(L"", szCMDLine) == 0)
	{
		SuMUIPrintMsg(g_hInstance, hDlg, IDS_ERRTEXTBOX);
	}
	else
	{
		wchar_t *szBuffer = nullptr;

		wchar_t szPath[MAX_PATH];
		GetPrivateProfileStringW(
			szCMDLine, L"CommandLine", L"", szPath, MAX_PATH, g_ShortCutListPath);

		szBuffer = (wcscmp(szPath, L"") != 0 ? szPath : const_cast<wchar_t*>(szCMDLine));

		// 模拟为System权限
		if (g_pNSudo->ImpersonateAsSystem())
		{
			CToken *pToken = nullptr;

			// 获取用户令牌
			if (SuMUICompare(g_hInstance, IDS_TI, szUser))
			{
				g_pNSudo->GetTrustedInstallerToken(&pToken);
			}
			else if (SuMUICompare(g_hInstance, IDS_SYSTEM, szUser))
			{
				g_pNSudo->GetSystemToken(&pToken);
			}
			else if (SuMUICompare(g_hInstance, IDS_CURRENTPROCESS, szUser))
			{
				g_pNSudo->GetCurrentToken(&pToken);
			}
			else if (SuMUICompare(g_hInstance, IDS_CURRENTUSER, szUser))
			{
				g_pNSudo->GetCurrentUserToken(M2GetCurrentSessionID(), &pToken);
			}

			// 如果勾选启用全部特权，则对令牌启用全部特权
			if (pToken && bEnableAllPrivileges) pToken->SetPrivilege(EnableAll);	
			
			if (!SuCreateProcess(*pToken, szBuffer))
			{
				SuMUIPrintMsg(g_hInstance, NULL, IDS_ERRSUDO);
			}

			if (pToken) delete pToken;

			g_pNSudo->RevertToSelf();
		}	
	}
}

HRESULT CALLBACK SuAboutDialogCallback(
	HWND hWnd,
	UINT uNotification,
	WPARAM wParam,
	LPARAM lParam,
	LONG_PTR dwRefData)
{
	IIntendToIgnoreThisVariable(wParam);
	IIntendToIgnoreThisVariable(dwRefData);
	
	HRESULT hr = S_OK;

	switch (uNotification)
	{
	case TDN_HYPERLINK_CLICKED:
		ShellExecuteW(hWnd, L"open", (LPCWSTR)lParam, nullptr, nullptr, SW_SHOW);
		break;
	}

	return hr;
}

HRESULT SuShowAboutDialog(
	_In_ HWND hwndParent,
	_In_ HINSTANCE hInstance)
{
	TASKDIALOGCONFIG tdConfig;
	memset(&tdConfig, 0, sizeof(tdConfig));

	tdConfig.cbSize = sizeof(TASKDIALOGCONFIG);
	tdConfig.hwndParent = hwndParent;
	tdConfig.hInstance = hInstance;
	tdConfig.pfCallback = SuAboutDialogCallback;
	tdConfig.pszWindowTitle = L"NSudo";
	tdConfig.pszMainIcon = MAKEINTRESOURCE(IDI_NSUDO);
	tdConfig.pszMainInstruction = ProjectInfo::VersionText;
	tdConfig.pszContent = MAKEINTRESOURCE(IDS_ABOUT);
	tdConfig.pszFooterIcon = TD_INFORMATION_ICON;
	tdConfig.pszFooter = 
		L"<a href=\""
		L"http://shang.qq.com/wpa/qunwpa?idkey=29940ed5c8b2363efcf8a1c376f280c4a46c4e356d5533af48541418ff13ada2"
		L"\">官方群(Offical QQ Group): 466078631</a>\r\n"
		L"<a href=\""
		L"https://m2team.github.io/NSudo"
		L"\">项目首页(Project Site): https://m2team.github.io/NSudo</a>";
	tdConfig.pszExpandedControlText = MAKEINTRESOURCE(IDS_ABOUT_FOOTERTITLE);
	tdConfig.pszExpandedInformation = MAKEINTRESOURCE(IDS_HELP);
	tdConfig.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION |TDF_ENABLE_HYPERLINKS | TDF_EXPAND_FOOTER_AREA;

	return TaskDialogIndirect(&tdConfig, nullptr, nullptr, nullptr);
}


INT_PTR CALLBACK DialogCallBack(
	HWND hDlg, 
	UINT message,
	WPARAM wParam,
	LPARAM lParam)
{
	IIntendToIgnoreThisVariable(lParam);
	
	HWND hUserName = GetDlgItem(hDlg, IDC_UserName);
	HWND hCheckBox = GetDlgItem(hDlg, IDC_Check_EnableAllPrivileges);
	HWND hszPath = GetDlgItem(hDlg, IDC_szPath);

	wchar_t szCMDLine[MAX_PATH], szUser[260], szBuffer[512];
	
	switch (message)
	{
	case WM_CLOSE:
		EndDialog(hDlg, 0);
		break;
	case WM_INITDIALOG:
		// Show NSudo Logo
		SendMessageW(
			GetDlgItem(hDlg, IDC_NSudoLogo),
			STM_SETIMAGE,
			IMAGE_ICON,
			LPARAM(LoadImageW(
				g_hInstance,
				MAKEINTRESOURCE(IDI_NSUDO),
				IMAGE_ICON,
				64,
				64,
				LR_COPYFROMRESOURCE)));

		//Show Warning Icon
		SendMessageW(
			GetDlgItem(hDlg, IDC_Icon),
			STM_SETIMAGE,
			IMAGE_ICON,
			LPARAM(LoadIconW(NULL, IDI_WARNING)));

		SuMUIInsComboBox(g_hInstance, hUserName, IDS_TI);
		SuMUIInsComboBox(g_hInstance, hUserName, IDS_SYSTEM);
		SuMUIInsComboBox(g_hInstance, hUserName, IDS_CURRENTPROCESS);
		SuMUIInsComboBox(g_hInstance, hUserName, IDS_CURRENTUSER);
		SendMessageW(hUserName, CB_SETCURSEL, 3, 0); //设置默认项"TrustedInstaller"

		{
			wchar_t szItem[260], szBuf[32768];
			DWORD dwLength = GetPrivateProfileSectionNamesW(szBuf, 32768, g_ShortCutListPath);

			for (DWORD i = 0, j = 0; i < dwLength; i++, j++)
			{
				if (szBuf[i] != L'\0')
				{
					szItem[j] = szBuf[i];
				}
				else
				{
					szItem[j] = L'\0';
					SendMessageW(hszPath, CB_INSERTSTRING, 0, (LPARAM)szItem);
					j = (DWORD)-1;				
				}
			}
		}
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_Run:
			GetDlgItemTextW(hDlg, IDC_UserName, szUser, sizeof(szUser));
			GetDlgItemTextW(hDlg, IDC_szPath, szCMDLine, sizeof(szCMDLine));

			SuGUIRun(hDlg, szUser, (SendMessageW(hCheckBox, BM_GETCHECK, 0, 0) == BST_CHECKED), szCMDLine);
			break;
		case IDC_About:
			SuShowAboutDialog(hDlg, g_hInstance);
			break;
		case IDC_Browse:		
			szBuffer[1] = L'\0';
			NSudoBrowseDialog(hDlg, &szBuffer[1]);

			if (szBuffer[1] != L'\0')
			{
				szBuffer[0] = L'\"';
				wcscat_s(szBuffer, 512, L"\"");

				SetDlgItemTextW(hDlg, IDC_szPath, szBuffer);
			}

			break;
		}
		break;
	case WM_DROPFILES:
	{	
		DragQueryFileW((HDROP)wParam, 0, &szBuffer[1], sizeof(szBuffer) - sizeof(wchar_t));

		if (!(GetFileAttributesW(&szBuffer[1]) & FILE_ATTRIBUTE_DIRECTORY))
		{
			szBuffer[0] = L'\"';
			wcscat_s(szBuffer, 512, L"\"");

			SetDlgItemTextW(hDlg, IDC_szPath, szBuffer);
		}

		DragFinish((HDROP)wParam);

		break;
	}
	}

	return 0;
}

int main()
{	
	SECURITY_CAPABILITIES SecurityCapabilities = { 0 };

	SuGenerateRandomAppContainerSid(
		&SecurityCapabilities.AppContainerSid);
	
	SuGenerateAppContainerCapabilitiy(
		&SecurityCapabilities.Capabilities,
		&SecurityCapabilities.CapabilityCount);






	HANDLE hToken = nullptr,hNewToken=nullptr;

	SuQueryCurrentProcessToken(&hToken);

	NTSTATUS status = SuCreateAppContainerToken(&hNewToken, hToken, &SecurityCapabilities);

	HRESULT hr = (HRESULT)RtlNtStatusToDosError(status);

	wchar_t szCMD[] = L"cmd /k";

	STARTUPINFOW StartupInfo = { 0 };
	PROCESS_INFORMATION ProcessInfo = { 0 };

	if (CreateProcessAsUserW(
		hNewToken, NULL, szCMD, nullptr, nullptr, FALSE,
		CREATE_NEW_CONSOLE,
		nullptr, nullptr, &StartupInfo, &ProcessInfo))
	{
		
	}


	status = hr;
	
	
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	
	SuInitialize();

	if (!g_pNSudo) return 1;

	bool bElevated = (g_pNSudo->GetAvailableLevel() > 0);

	if (g_GUIMode)
	{
		if (bElevated)
		{
			FreeConsole();

			EnablePerMonitorDialogScaling();

			ChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
			ChangeWindowMessageFilter(0x0049, MSGFLT_ADD); // WM_COPYGLOBALDATA

			DialogBoxParamW(
				g_hInstance,
				MAKEINTRESOURCEW(IDD_NSudoDlg),
				nullptr,
				DialogCallBack,
				0L);
		}
		else
		{				
			SHELLEXECUTEINFOW ExecInfo = { 0 };
			ExecInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
			ExecInfo.lpFile = g_ExePath;
			ExecInfo.lpVerb = L"runas";
			ExecInfo.nShow = SW_NORMAL;

			ShellExecuteExW(&ExecInfo);

			return (int)GetLastError();
		}
	}
	else
	{
		// 执行宿主
		if (wcscmp(L"-Execute", g_argv[1]) == 0)
		{
			SHELLEXECUTEINFOW ExecInfo = { 0 };
			ExecInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
			ExecInfo.lpVerb = L"open";
			ExecInfo.lpFile = g_argv[2];
			ExecInfo.nShow = SW_NORMAL;

			ShellExecuteExW(&ExecInfo);

			return (int)GetLastError();
		}
		
		DWORD result;
		WriteConsoleW(g_hOut, ProjectInfo::VersionText, (DWORD)wcslen(ProjectInfo::VersionText), &result, nullptr);
		WriteConsoleW(g_hOut, L"\n", (DWORD)wcslen(L"\n"), &result, nullptr);

		SuMUIPrintMsg(g_hInstance, NULL, IDS_ABOUT);

		// 如果参数是 /? 或 -?,则显示帮助
		if (g_argc == 2 && 
			(g_argv[1][0] == L'-' || g_argv[1][0] == L'/') && 
			g_argv[1][1] == '?')
		{
			SuMUIPrintMsg(g_hInstance, NULL, IDS_HELP);
			return 0;
		}

		// 如果未提权或者模拟System权限失败
		if (!(bElevated && g_pNSudo->ImpersonateAsSystem()))
		{
			SuMUIPrintMsg(g_hInstance, NULL, IDS_ERRNOTHELD);
			return 0;
		}

		bool bCMDLineArgEnable = true;
		bool bArgErr = false;

		wchar_t *szBuffer = nullptr;

		CToken *pToken = nullptr;
		CToken *pTempToken = nullptr;

		for (int i = 1; i < g_argc; i++)
		{
			//判断是参数还是要执行的命令行或常见任务
			if (g_argv[i][0] == L'-' || g_argv[i][0] == L'/')
			{
				switch (g_argv[i][1])
				{			
				case 'U':
				case 'u':
				{
					if (g_argv[i][2] == L':')
					{
						switch (g_argv[i][3])
						{
						case 'T':
						case 't':
							g_pNSudo->GetTrustedInstallerToken(&pToken);
							break;
						case 'S':
						case 's':
							g_pNSudo->GetSystemToken(&pToken);
							break;
						case 'C':
						case 'c':
							g_pNSudo->GetCurrentUserToken(M2GetCurrentSessionID(), &pToken);
							break;
						case 'P':
						case 'p':
							g_pNSudo->GetCurrentToken(&pToken);
							break;
						case 'D':
						case 'd':
							if (g_pNSudo->GetCurrentToken(&pTempToken))
							{
								pTempToken->MakeLUA(&pToken);
								delete pTempToken;
							}
							break;
						default:
							bArgErr = true;
							break;
						}
					}
					break;
				}
				case 'P':
				case 'p':
				{
					if (pToken)
					{
						if (g_argv[i][2] == L':')
						{
							switch (g_argv[i][3])
							{
							case 'E':
							case 'e':
								pToken->SetPrivilege(EnableAll);
								break;
							case 'D':
							case 'd':
								pToken->SetPrivilege(RemoveMost);
								break;
							default:
								bArgErr = true;
								break;
							}
						}
					}
					break;
				}
				case 'M':
				case 'm':
				{
					if (pToken)
					{
						if (g_argv[i][2] == L':')
						{
							switch (g_argv[i][3])
							{
							case 'S':
							case 's':
								pToken->SetIL(IntegrityLevel::System);
								break;
							case 'H':
							case 'h':
								pToken->SetIL(IntegrityLevel::High);
								break;
							case 'M':
							case 'm':
								pToken->SetIL(IntegrityLevel::Medium);
								break;
							case 'L':
							case 'l':
								pToken->SetIL(IntegrityLevel::Low);
								break;
							default:
								bArgErr = true;
								break;
							}
						}
					}

					break;
				}
				default:
					break;
				}
			}
			else
			{
				if (bCMDLineArgEnable)
				{
					wchar_t szPath[MAX_PATH];

					GetPrivateProfileStringW(
						g_argv[i], L"CommandLine", L"", szPath, MAX_PATH, g_ShortCutListPath);

					wcscmp(szPath, L"") != 0 ?
						szBuffer = szPath : szBuffer = (g_argv[i]);

					if (szBuffer) bCMDLineArgEnable = false;
				}
			}
		}

		if (pToken == nullptr)
		{
			if (g_pNSudo->GetTrustedInstallerToken(&pToken))
			{
				pToken->SetPrivilege(EnableAll);
			}
		}

		if (bCMDLineArgEnable || bArgErr)
		{
			SuMUIPrintMsg(g_hInstance, NULL, IDS_ERRARG);
			return -1;
		}
		else
		{
			if (!(pToken && SuCreateProcess(*pToken, szBuffer)))
			{
				SuMUIPrintMsg(g_hInstance, NULL, IDS_ERRSUDO);
			}
		}

		if (pToken) delete pToken;

		g_pNSudo->RevertToSelf();
	}
	
	delete g_pNSudo;

	return 0;
}