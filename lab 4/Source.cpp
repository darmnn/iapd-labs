#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <Windows.h>
#include <conio.h>
#include <vfw.h>
#include <SetupAPI.h>
#include <devguid.h>

//	F11 - start/stop caming video

using namespace std;

#pragma comment(lib,"vfw32.lib")
#pragma comment(lib, "setupapi.lib")

HHOOK hHookKeyboard;
HWND hWinCapture;

bool winIsHidden = false;
bool winIsCapturing = false;

int fileNum = 1;

void startRecording(HWND hWinCapture)
{
	WCHAR* fileName;
	HANDLE hFile;
	fileName = new WCHAR[10];

	do
	{
		fileName[0] = fileName[1] = fileName[2] = fileName[3] = fileName[4] = '\0';
		_itow(fileNum, fileName, 10);
		wcscat(fileName, L".avi");
		fileNum++;
	} while (CreateFileW(fileName, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL) != INVALID_HANDLE_VALUE);

	wcout << L"FILE: " << fileName << endl;

	capFileSetCaptureFile(hWinCapture, fileName);

	delete fileName;
	capCaptureSequence(hWinCapture);
	winIsCapturing = true;
}

void stopRecording(HWND hWinCapture)
{
	capCaptureAbort(hWinCapture);
	winIsCapturing = false;
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{

	KBDLLHOOKSTRUCT* ks = (KBDLLHOOKSTRUCT*)lParam;
	cout << ks->vkCode;

	if (ks->vkCode == 0x1B)
		TerminateProcess(GetCurrentProcess(), NO_ERROR);

	if (ks->vkCode == 72 && ks->flags == 128)	//h = hide. flags == 128 - transition state. The value is 0 if the key is pressed and 1 if it is 
												//being released
	{
		if (winIsHidden == true)
		{
			ShowWindow(hWinCapture, SW_NORMAL);
			winIsHidden = false;
		}
		else
		{
			ShowWindow(hWinCapture, SW_HIDE);
			winIsHidden = true;
		}
		return -1;
	}

	if (ks->vkCode == 122 && ks->flags == 128)	//f11 = start video
	{
		CAPTUREPARMS captureParams;		
		capCaptureGetSetup(hWinCapture, &captureParams, sizeof(CAPTUREPARMS));
		
		captureParams.fMakeUserHitOKToCapture = FALSE;
		captureParams.fYield = TRUE;
		captureParams.fCaptureAudio = TRUE;		
		captureParams.fAbortLeftMouse = FALSE;
		captureParams.fAbortRightMouse = FALSE;
		captureParams.dwRequestMicroSecPerFrame = 40000;

		capCaptureSetSetup(hWinCapture, &captureParams, sizeof(CAPTUREPARMS));


		if (winIsCapturing)
			stopRecording(hWinCapture);
		else
			startRecording(hWinCapture);
		return -1;
	}
	return CallNextHookEx(hHookKeyboard, nCode, wParam, lParam);
}

int main()
{
	system("chcp 1251");
	system("cls");
	int i;
	DWORD numberOfDevices = 0;
	WCHAR name[256];
	WCHAR description[256];

	for (i = 0; i < 10; ++i)
	{
		if (capGetDriverDescriptionW(i,		//index of the capture driver
			name,		//pointer to a buffer containing string corresponding to the capture driver name
			255,		//length, in bytes, of the buffer pointed to by name
			description,		//pointer to a buffer containing string corresponding to the description of the capture driver
			255))		//length, in bytes, of the buffer pointed to by description
		{
			numberOfDevices++;
			wcout << name << endl;
			wcout << description << endl;
		}
	}
	if (numberOfDevices == 0)
	{
		cout << "No webcamera installed";
		_getch();
		return 1;
	}

	SP_DEVINFO_DATA DeviceInfoData = { 0 };
	HDEVINFO DeviceInfoSet = SetupDiGetClassDevsA(&GUID_DEVCLASS_CAMERA, "USB", NULL, DIGCF_PRESENT);
	if (DeviceInfoSet == INVALID_HANDLE_VALUE) {
		exit(1);
	}
	DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	SetupDiEnumDeviceInfo(DeviceInfoSet, 0, &DeviceInfoData);
	char deviceName[256];
	char deviceMfg[256];
	SetupDiGetDeviceRegistryPropertyA(DeviceInfoSet, &DeviceInfoData, SPDRP_FRIENDLYNAME, NULL, (PBYTE)deviceName, sizeof(deviceName), 0);
	SetupDiGetDeviceRegistryPropertyA(DeviceInfoSet, &DeviceInfoData, SPDRP_MFG, NULL, (PBYTE)deviceMfg, sizeof(deviceMfg), 0);
	SetupDiDestroyDeviceInfoList(DeviceInfoSet);
	cout << "Vendor: " << deviceMfg << '\n'
		<< "Name: " << deviceName << endl;

	Sleep(4000);

	HWND hWnd = GetForegroundWindow();
	
	ShowWindow(hWnd,SW_HIDE);	

	if (!(hWinCapture = capCreateCaptureWindowW(L"Capture",	
		WS_VISIBLE, 100, 100, 640, 440, hWnd, 0)))
		return 1;


	bool flag = FALSE;
	while (!flag)
		flag = capDriverConnect(hWinCapture,0);

	hHookKeyboard = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc,GetModuleHandle(NULL),0);
				  
	capPreviewScale(hWinCapture, TRUE);
	capPreviewRate(hWinCapture, 50);
	capPreview(hWinCapture, TRUE);		

	MSG msg = { 0 };
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	capDriverDisconnect(hWinCapture);
	DestroyWindow(hWinCapture);
	return 0;
}