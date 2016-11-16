// crfid.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include "crfid.h"

#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "SkyeTekAPI.h"
#include "SkyeTekProtocol.h"

#pragma warning (disable: 4096)
#pragma warning (disable: 4996)

#define MAX_LOADSTRING 100
#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 400
#define IDC_START_BUTTON 200
#define IDC_RESET_BUTTON 201

// Global Variables:
HWND hWnd;
HWND listBox;
HWND startBtn;
HWND resetBtn;
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
char str[80] = "";
wchar_t szBuff[256];
HANDLE hThrd;
DWORD threadId;
HANDLE hEventRead;
int width, height;
LPSKYETEK_READER *readers = NULL;
bool isConnected = false;
bool start = true;
std::vector<std::wstring> vecTag;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int connect(HWND hWnd);
void addTag(std::string out, LPVOID hWnd);
void printToScreen(char* readBuffer, LPVOID hWnd);
SKYETEK_STATUS ReadTagData(LPSKYETEK_READER lpReader, LPSKYETEK_TAG lpTag);
DWORD WINAPI ReadThread(LPVOID hwnd);
void ReadTag();

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

	// Create Read thread
	hThrd = CreateThread(NULL, 0, ReadThread, (LPVOID)hWnd, 0, &threadId);
	if (hThrd)
	{
		CloseHandle(hThrd);
	}

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

   hWnd = CreateWindowW(szWindowClass, 
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

   // Get the width and height
   RECT rect;
   if (GetClientRect(hWnd, &rect))
   {
	   width = rect.right - rect.left;
	   height = rect.bottom - rect.top;
   }

   listBox = CreateWindow(TEXT("LISTBOX"),
	   TEXT(""),
	   WS_CHILD | WS_VISIBLE | WS_BORDER,
	   10,
	   70,
	   width - 20,
	   height - 120,
	   hWnd,
	   NULL,
	   NULL,
	   NULL);

   startBtn = CreateWindow(L"BUTTON",
	   L"STOP",
	   WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
	   width * 0.5 - 100 - 30,
	   70 + height - 120 + 10,
	   100,
	   30,
	   hWnd,
	   (HMENU)IDC_START_BUTTON,
	   GetModuleHandle(NULL),
	   NULL);

   resetBtn = CreateWindow(L"BUTTON",
	   L"RESET",
	   WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
	   width * 0.5 + 30,
	   70 + height - 120 + 10,
	   100,
	   30,
	   hWnd,
	   (HMENU)IDC_RESET_BUTTON,
	   GetModuleHandle(NULL),
	   NULL);

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
			case IDC_START_BUTTON:
				if (start)
				{
					start = false;
					SendMessage(startBtn, WM_SETTEXT, NULL, (LPARAM)L"START");
				}
				else
				{
					start = true;
					SendMessage(startBtn, WM_SETTEXT, NULL, (LPARAM)L"STOP");
				}
				break;
			case IDC_RESET_BUTTON:
				SendMessage(listBox, LB_RESETCONTENT, NULL, 0);
				vecTag.clear();
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
	//LPSKYETEK_READER *readers = NULL;
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
	ss << "Discovered " << numDevices << " devices.\n";
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
		SkyeTek_FreeData(lpData);
		lpData = NULL;
	}

	// Set additional timeout
	SkyeTek_SetAdditionalTimeout(readers[0]->lpDevice, 5000);


	//ss << "Looping " << loops << "times\n";
	out = ss.str();
	printToScreen(strdup(out.c_str()), hWnd);
	ss.str(std::string());
	ss.clear();

	isConnected = true;
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
			return st;
		}
	}

	// Get memory info
	memset(&mem, 0, sizeof(SKYETEK_MEMORY));
	st = SkyeTek_GetTagInfo(lpReader, lpTag, &mem);
	if (st != SKYETEK_SUCCESS)
	{
		return st;
	}

	// Allocate the memory now that we know how much it has
	memset(data, 0, 2048);
	length = (mem.maxBlock - mem.startBlock + 1) * mem.bytesPerBlock;
	if (length > 2048)
	{
		return SKYETEK_FAILURE;
	}

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
			continue;
		}

		// Get lock status
		stat = 0;
		st = SkyeTek_GetLockStatus(lpReader, lpTag, &addr, &stat);
		if (st != SKYETEK_SUCCESS)
		{
			didFail = true;
			ptr += mem.bytesPerBlock;
			continue;
		}

		// Copy over data to buffer
		memcpy(ptr, lpData->data, lpData->size);
		ptr += mem.bytesPerBlock;
		SkyeTek_FreeData(lpData);
		lpData = NULL;

	} // End loop

	  // Report data
	lpData = SkyeTek_AllocateData((int)length);
	SkyeTek_CopyBuffer(lpData, data, length);
	TCHAR *str = SkyeTek_GetStringFromData(lpData);
	SkyeTek_FreeString(str);
	SkyeTek_FreeData(lpData);

	// Return
	if (didFail)
		return SKYETEK_FAILURE;
	else
		return SKYETEK_SUCCESS;
}

DWORD WINAPI ReadThread(LPVOID hwnd)
{
	HWND _hwnd = (HWND)hwnd;
	// Create the overlapped event. Must be closed before exiting
	// to avoid a handle leak.
	//hEventRead = CreateEvent(NULL, TRUE, FALSE, NULL);
	LPSECURITY_ATTRIBUTES lpEventAttributes = NULL;
	BOOL bManualReset = FALSE;
	BOOL bInitialState = FALSE;
	LPCTSTR lpName = NULL;
	hEventRead = CreateEvent(lpEventAttributes, bManualReset, bInitialState, lpName);

	DWORD dwRes;
	int a = 0;
	while (true)
	{
		if (isConnected && start)
			ReadTag();
	}

	return 0;
}

void ReadTag()
{
	std::string out;
	std::stringstream ss;
	LPSKYETEK_DATA lpData = NULL;
	LPSKYETEK_TAG *lpTags = NULL;
	SKYETEK_STATUS st;
	unsigned short count;
	int totalReads = 0;

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
			return;
		}
	}

	// Discover all tags
	lpTags = NULL;
	count = 0;
	st = SkyeTek_GetTags(readers[0], AUTO_DETECT, &lpTags, &count);
	if (st == SKYETEK_TIMEOUT)
	{
		ss << "*** WARNING: SkyeTek_GetTags timed out: " << readers[0]->friendly << "\n";
		out = ss.str();
		printToScreen(strdup(out.c_str()), hWnd);
		ss.str(std::string());
		ss.clear();
		return;
	}
	else if (st != SKYETEK_SUCCESS)
	{
		ss << "*** ERROR: SkyeTek_GetTags failed: " << STPV3_LookupResponse(st) << "\n";
		out = ss.str();
		printToScreen(strdup(out.c_str()), hWnd);
		ss.str(std::string());
		ss.clear();
		return;
	}

	for (unsigned short ix = 0; ix < count; ix++)
	{
		ss << "Discovered tag: " << lpTags[ix]->friendly;
		ss << "[" << SkyeTek_GetTagTypeNameFromType(lpTags[ix]->type) << "]\n";
		out = ss.str();
		//addTag(out, hWnd);
		ss.str(std::string());
		ss.clear();

		// Don't attempt to read EM4X22 tags
		if ((lpTags[ix]->type & 0xFFF0) != EM4X22_AUTO)
		{
			ss << "Reading tag: " << lpTags[ix]->friendly;
			ss << "[" << SkyeTek_GetTagTypeNameFromType(lpTags[ix]->type) << "]\n";
			out = ss.str();
			if (std::find(vecTag.begin(), vecTag.end(), lpTags[ix]->friendly) == vecTag.end()) {
				vecTag.push_back(lpTags[ix]->friendly);
				addTag(out, hWnd);
			}
			ss.str(std::string());
			ss.clear();

			st = ReadTagData(readers[0], lpTags[ix]);
			totalReads++;
			if (st != SKYETEK_SUCCESS)
			{
			}
		}
	}

	// Free tags
	SkyeTek_FreeTags(readers[0], lpTags, count);
}

void addTag(std::string out, LPVOID hWnd) {
	std::wstring msg(out.begin(), out.end());
	SendMessage(listBox, LB_ADDSTRING, NULL, (LPARAM)msg.c_str());
}

void printToScreen(char* readBuffer, LPVOID hWnd) {
	HDC hdc;
	RECT rect;
	TEXTMETRIC tm;
	SIZE size;

	static unsigned x = 10;
	static unsigned y = 0;

	hdc = GetDC((HWND)hWnd);
	GetTextMetrics(hdc, &tm);


	for (int i = 0; i < strlen(readBuffer); i++) {
		wchar_t temp[2];
		swprintf_s(temp, sizeof(temp) / sizeof(wchar_t), L"%c", readBuffer[i]);
		GetTextExtentPoint32(hdc, temp, wcslen(temp), &size);

		if (x + size.cx >= width)
		{
			x = 10;
			y += tm.tmHeight + tm.tmExternalLeading; // next line
		}

		if (!wcscmp(temp, L"\n")) // handle as a new line.
		{
			x = 10;
			y += tm.tmHeight + tm.tmExternalLeading; // next line
		}

		TextOut(hdc, x, y, temp, wcslen(temp));
		x += size.cx;
	}
	ReleaseDC((HWND)hWnd, hdc);
	readBuffer = '\0';
}
