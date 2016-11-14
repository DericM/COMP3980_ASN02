// crfid.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include "crfid.h"

#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>

#include "SkyeTekAPI.h"
#include "SkyeTekProtocol.h"

#pragma warning (disable: 4096)
#pragma warning (disable: 4996)

#define MAX_LOADSTRING 100
#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 400

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
char str[80] = "";
wchar_t szBuff[256];




// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int connect(HWND hWnd);
void printToScreen(char* readBuffer, LPVOID hWnd);
void printToScreen2(LPWSTR str, LPVOID hWnd);
SKYETEK_STATUS ReadTagData(LPSKYETEK_READER lpReader, LPSKYETEK_TAG lpTag);
void output(TCHAR *sz, ...);




int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CRFID, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CRFID));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CRFID));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_CRFID);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, 
							 szTitle, 
							 WS_OVERLAPPEDWINDOW,
							 CW_USEDEFAULT, 
							 0, 
							 CW_USEDEFAULT, 
							 0, 
							 nullptr, 
							 nullptr, 
							 hInstance, 
							 nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
			case IDM_CONNECT:
				connect(hWnd);
				break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}




int connect(HWND hWnd) {
	LPSKYETEK_DEVICE *devices = NULL;
	LPSKYETEK_READER *readers = NULL;
	LPSKYETEK_TAG *lpTags = NULL;
	LPSKYETEK_DATA lpData = NULL;
	SKYETEK_STATUS st;
	unsigned short count;
	unsigned int numDevices;
	unsigned int numReaders;
	int loops = 100;
	int totalReads = 0;
	int failedReads = 0;
	int failedLoops = 0;

	std::string out;
	std::stringstream ss;
	
	// Discover reader

	printToScreen("Discovering reader...", hWnd);
	
	numDevices = SkyeTek_DiscoverDevices(&devices);
	if (numDevices == 0){
		return 0;
	}
	ss << "Discovered " << numDevices << " devices.";
	out = ss.str();
	printToScreen(strdup(out.c_str()), hWnd);
	ss.str(std::string());
	ss.clear();


	numReaders = SkyeTek_DiscoverReaders(devices, numDevices, &readers);
	if (numReaders == 0)
	{
		SkyeTek_FreeDevices(devices, numDevices);
		return 0;
	}
	ss << "Found reader: " << readers[0]->friendly << "\nOn device: ";
	ss << readers[0]->lpDevice->type << "[" << readers[0]->lpDevice->address << "]\n";
	out = ss.str();
	printToScreen(strdup(out.c_str()), hWnd);
	ss.str(std::string());
	ss.clear();

	
	// Get baud if serial
	if (_tcscmp(readers[0]->lpDevice->type, SKYETEK_SERIAL_DEVICE_TYPE) == 0)
	{
		st = SkyeTek_GetSystemParameter(readers[0], SYS_BAUD, &lpData);
		if (st != SKYETEK_SUCCESS)
		{
			output(_T("*** ERROR: could not get SYS_BAUD: %s\n"),
				SkyeTek_GetStatusMessage(st));
			SkyeTek_FreeReaders(readers, numReaders);
			SkyeTek_FreeDevices(devices, numDevices);
			//fclose(fp);
			return 0;
		}
		int currentBaud = 0;
		if (lpData->data[0] == 0)
			currentBaud = 9600;
		else if (lpData->data[0] == 1)
			currentBaud = 19200;
		else if (lpData->data[0] == 2)
			currentBaud = 38400;
		else if (lpData->data[0] == 3)
			currentBaud = 57600;
		else if (lpData->data[0] == 4)
			currentBaud = 115200;
		output(_T("Baud rate: %d [0x%02d]\n"), currentBaud, lpData->data[0]);
		SkyeTek_FreeData(lpData);
		lpData = NULL;
	}

	// Set additional timeout
	SkyeTek_SetAdditionalTimeout(readers[0]->lpDevice, 5000);
	output(_T("Added 5 seconds of additional timeout on device\n"));


	ss << "Looping " << loops << "times\n";
	out = ss.str();
	printToScreen(strdup(out.c_str()), hWnd);
	ss.str(std::string());
	ss.clear();
	// Loop, discover tags and read data
	//output(_T("Looping %d times...\n"), loops);
	for (int i = 0; i < loops; i++)
	{
		// Debug
		//output(_T("Loop %d\n"), i);
		ss << "Loop " << i;
		out = ss.str();
		printToScreen(strdup(out.c_str()), hWnd);
		ss.str(std::string());
		ss.clear();

		// If it is an M9, set retries to zero
		if (_tcscmp(readers[0]->model, _T("M9")) == 0)
		{
			lpData = SkyeTek_AllocateData(1);
			lpData->data[0] = 0;
			st = SkyeTek_SetSystemParameter(readers[0], SYS_COMMAND_RETRY, lpData);
			SkyeTek_FreeData(lpData);
			lpData = NULL;
			if (st != SKYETEK_SUCCESS)
			{
				//output(_T("*** ERROR: failed to set M9 retries to 0: %s\n"), STPV3_LookupResponse(st));
				failedLoops++;
				continue;
			}
			//output(_T("Set M9 retries to zero for get tags\n"));
		}

		// Discover all tags
		lpTags = NULL;
		count = 0;
		st = SkyeTek_GetTags(readers[0], AUTO_DETECT, &lpTags, &count);
		if (st == SKYETEK_TIMEOUT)
		{
			//output(_T("*** WARNING: SkyeTek_GetTags timed out: %s\n"), readers[0]->friendly);
			ss << "*** WARNING: SkyeTek_GetTags timed out: " << readers[0]->friendly << "\n";
			out = ss.str();
			printToScreen(strdup(out.c_str()), hWnd);
			ss.str(std::string());
			ss.clear();
			failedLoops++;
			continue;
		}
		else if (st != SKYETEK_SUCCESS)
		{
			//output(_T("*** ERROR: SkyeTek_GetTags failed: %s\n"), STPV3_LookupResponse(st));
			ss << "*** ERROR: SkyeTek_GetTags failed: " << STPV3_LookupResponse(st) << "\n";
			out = ss.str();
			printToScreen(strdup(out.c_str()), hWnd);
			ss.str(std::string());
			ss.clear();
			failedLoops++;
			continue;
		}

		// Loop through tags and read each if it has data
		//output(_T("Discovered %d tags\n"), count);
		for (unsigned short ix = 0; ix < count; ix++)
		{
			//output(_T("Discovered tag: %s [%s]\n"), lpTags[ix]->friendly,
			//	SkyeTek_GetTagTypeNameFromType(lpTags[ix]->type));

			ss << "Discovered tag: " << lpTags[ix]->friendly;
			ss << "[" << SkyeTek_GetTagTypeNameFromType(lpTags[ix]->type) << "]";
			out = ss.str();
			printToScreen(strdup(out.c_str()), hWnd);
			ss.str(std::string());
			ss.clear();


			// Don't attempt to read EM4X22 tags
			if ((lpTags[ix]->type & 0xFFF0) != EM4X22_AUTO)
			{
				//output(_T("Reading tag: %s [%s]\n"), lpTags[ix]->friendly,
				//	SkyeTek_GetTagTypeNameFromType(lpTags[ix]->type));

				ss << "Reading tag: " << lpTags[ix]->friendly;
				ss << "[" << SkyeTek_GetTagTypeNameFromType(lpTags[ix]->type) << "]";
				out = ss.str();
				printToScreen(strdup(out.c_str()), hWnd);
				ss.str(std::string());
				ss.clear();

				st = ReadTagData(readers[0], lpTags[ix]);
				totalReads++;
				if (st != SKYETEK_SUCCESS)
				{
					failedLoops++;
					failedReads++;
				}
			}
		}

		// Free tags
		SkyeTek_FreeTags(readers[0], lpTags, count);

	} // End loop
	return 1;

}




/**
* This reads all of the memory on a tag.
* @param lpReader Handle to the reader
* @param lpTag Handle to the tag
* @return SKYETEK_SUCCESS if it succeeds, or the failure status otherwise
*/
SKYETEK_STATUS ReadTagData(LPSKYETEK_READER lpReader, LPSKYETEK_TAG lpTag)
{
	// Variables used
	SKYETEK_STATUS st;
	LPSKYETEK_DATA lpData = NULL;
	SKYETEK_ADDRESS addr;
	SKYETEK_MEMORY mem;
	unsigned long length;
	unsigned char data[2048];
	bool didFail = false;
	unsigned char stat = 0;

	// If it is an M9, set retries to 5
	if (_tcscmp(lpReader->model, _T("M9")) == 0)
	{
		lpData = SkyeTek_AllocateData(1);
		lpData->data[0] = 32;
		st = SkyeTek_SetSystemParameter(lpReader, SYS_COMMAND_RETRY, lpData);
		SkyeTek_FreeData(lpData);
		lpData = NULL;
		if (st != SKYETEK_SUCCESS)
		{
			output(_T("*** ERROR: failed to set M9 retries to 32: %s\n"), STPV3_LookupResponse(st));
			return st;
		}
		output(_T("Set M9 retries to 32 for read tag\n"));
	}

	// Get memory info
	memset(&mem, 0, sizeof(SKYETEK_MEMORY));
	st = SkyeTek_GetTagInfo(lpReader, lpTag, &mem);
	if (st != SKYETEK_SUCCESS)
	{
		output(_T("*** ERROR: failed to get tag info: %s\n"), STPV3_LookupResponse(st));
		return st;
	}

	// Allocate the memory now that we know how much it has
	memset(data, 0, 2048);
	length = (mem.maxBlock - mem.startBlock + 1) * mem.bytesPerBlock;
	if (length > 2048)
	{
		output(_T("*** ERROR: Tag data length is too big: %ld is greater than max 2048\n"), length);
		return SKYETEK_FAILURE;
	}
	output(_T("Reading %d %d-byte blocks for %d bytes total\n"),
		(mem.maxBlock - mem.startBlock + 1), mem.bytesPerBlock, length);

	// NOTE: From this point forward, partial read results can be returned
	// so the calling function will need to determine how to handle the 
	// partial read and when to delete the allocated memory

	// Initialize address based on tag type
	memset(&addr, 0, sizeof(SKYETEK_ADDRESS));
	addr.start = mem.startBlock;
	addr.blocks = 1;

	// Now we loop but lock and release each time so as not to starve the rest of the app
	unsigned char *ptr = data;
	for (; addr.start <= mem.maxBlock; addr.start++)
	{

		// Read data
		st = SkyeTek_ReadTagData(lpReader, lpTag, &addr, 0, 0, &lpData);
		if (st != SKYETEK_SUCCESS)
		{
			didFail = true;
			ptr += mem.bytesPerBlock;
			output(_T("Page 0x%.2X -> Read Fail (%s)\n"),
				addr.start, SkyeTek_GetStatusMessage(st));
			continue;
		}

		// Get lock status
		stat = 0;
		st = SkyeTek_GetLockStatus(lpReader, lpTag, &addr, &stat);
		if (st != SKYETEK_SUCCESS)
		{
			didFail = true;
			ptr += mem.bytesPerBlock;
			output(_T("Page 0x%.2X -> Read Pass, Get Lock Fail (%s)\n"),
				addr.start, SkyeTek_GetStatusMessage(st));
			continue;
		}

		// Copy over data to buffer
		output(_T("Page 0x%.2X -> Read Pass, Lock Pass (%s)\n"),
			addr.start, stat ? _T("Locked") : _T("Unlocked"));
		memcpy(ptr, lpData->data, lpData->size);
		ptr += mem.bytesPerBlock;
		SkyeTek_FreeData(lpData);
		lpData = NULL;

	} // End loop

	  // Report data
	lpData = SkyeTek_AllocateData((int)length);
	SkyeTek_CopyBuffer(lpData, data, length);
	TCHAR *str = SkyeTek_GetStringFromData(lpData);
	output(_T("Tag data for %s: %s\n"), lpTag->friendly, str);
	SkyeTek_FreeString(str);
	SkyeTek_FreeData(lpData);

	// Return
	if (didFail)
		return SKYETEK_FAILURE;
	else
		return SKYETEK_SUCCESS;
}




void printToScreen(char* readBuffer, LPVOID hWnd) {
	HDC hdc;
	RECT rect;
	TEXTMETRIC tm;
	SIZE size;

	static unsigned x = 0;
	static unsigned y = 0;

	hdc = GetDC((HWND)hWnd);
	GetTextMetrics(hdc, &tm);


	for (int i = 0; i < strlen(readBuffer); i++) {
		wchar_t temp[2];
		swprintf_s(temp, sizeof(temp) / sizeof(wchar_t), L"%c", readBuffer[i]);

		TextOut(hdc, x, y, temp, wcslen(temp));
		GetTextExtentPoint32(hdc, temp, wcslen(temp), &size);

		x += size.cx;
		if (x >= WINDOW_WIDTH - 20) {
			x = 0;
			y = y + tm.tmHeight + tm.tmExternalLeading;
		}
	}

	//OutputDebugStringA("received: ");
	OutputDebugStringA(str);
	OutputDebugStringA("\n");
	OutputDebugStringA(readBuffer);
	OutputDebugStringA("\n");
	ReleaseDC((HWND)hWnd, hdc);
	readBuffer = '\0';
}

void printToScreen2(LPWSTR str, LPVOID hWnd) {
	HDC hdc;
	RECT rect;
	TEXTMETRIC tm;
	SIZE size;

	static unsigned x2 = 0;
	static unsigned y2 = 0;

	hdc = GetDC((HWND)hWnd);
	GetTextMetrics(hdc, &tm);

	TextOut(hdc, x2, y2, str, wcslen(str));
	GetTextExtentPoint32(hdc, str, wcslen(str), &size);

	x2 += size.cx;
	if (x2 >= WINDOW_WIDTH - 20) {
		x2 = 0;
		y2 = y2 + tm.tmHeight + tm.tmExternalLeading;
	}

	//OutputDebugStringA("received: ");
	/*OutputDebugStringA(str);
	OutputDebugStringA("\n");
	OutputDebugStringA(readBuffer);
	OutputDebugStringA("\n");*/
	ReleaseDC((HWND)hWnd, hdc);
	str = '\0';
}




void output(TCHAR *sz, ...)
{
	va_list args;

	if (sz == NULL)
		return;

	TCHAR msg[2048];
	memset(msg, 0, 2048 * sizeof(TCHAR));
	TCHAR str[2048];
	memset(str, 0, 2048 * sizeof(TCHAR));
	TCHAR timestr[16];
	SYSTEMTIME st;

	GetLocalTime(&st);
	memset(timestr, 0, 16 * sizeof(TCHAR));
	_stprintf(timestr, _T("%d:%02d:%02d.%03d"), st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

	va_start(args, sz);
	_vsntprintf(str, 2047, sz, args);
	va_end(args);

	_stprintf(msg, _T("%s: %s"), timestr, str);
	_tprintf(msg);

	OutputDebugStringA((LPCSTR)msg);
	//debug(msg);
}