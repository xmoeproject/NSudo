// NSudoG.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include "M2MessageDialog.h"
#include "NSudoResourceManagement.h"

void NSudoPrintMsg(
	_In_opt_ HINSTANCE hInstance,
	_In_opt_ HWND hWnd,
	_In_ LPCWSTR lpContent)
{
	std::wstring DialogContent =
		g_ResourceManagement.GetLogoText() +
		lpContent +
		g_ResourceManagement.GetUTF8WithBOMStringResources(IDR_String_Links);
	
	M2MessageDialog(
		hInstance,
		hWnd,
		MAKEINTRESOURCE(IDI_NSUDO),
		L"NSudo",
		DialogContent.c_str());
}

HRESULT NSudoShowAboutDialog(
	_In_ HWND hwndParent)
{
	std::wstring DialogContent =
		g_ResourceManagement.GetLogoText() +
		g_ResourceManagement.GetUTF8WithBOMStringResources(IDR_String_CommandLineHelp) +
		g_ResourceManagement.GetUTF8WithBOMStringResources(IDR_String_Links);

	SetLastError(ERROR_SUCCESS);

	M2MessageDialog(
		g_ResourceManagement.Instance,
		hwndParent,
		MAKEINTRESOURCE(IDI_NSUDO),
		L"NSudo",
		DialogContent.c_str());

	return M2GetLastError();
}

int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nShowCmd)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nShowCmd);

	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	std::vector<std::wstring> command_args =
		g_ResourceManagement.GetCommandParameters();

	if (command_args.size() == 1)
	{
		command_args.push_back(L"-?");
	}
	
	NSUDO_MESSAGE message = NSudoCommandLineParser(
		g_ResourceManagement.IsElevated,
		true,
		command_args);
	if (NSUDO_MESSAGE::NEED_TO_SHOW_COMMAND_LINE_HELP == message)
	{
		NSudoShowAboutDialog(nullptr);
	}
	else if (NSUDO_MESSAGE::SUCCESS != message)
	{
		std::wstring Buffer = g_ResourceManagement.GetMessageString(
			message);
		NSudoPrintMsg(
			g_ResourceManagement.Instance,
			nullptr,
			Buffer.c_str());
		return -1;
	}

	return 0;
}
