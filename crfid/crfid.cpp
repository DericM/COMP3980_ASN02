// crfid.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include "crfid.h"

#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <string>

#include "SkyeTekAPI.h"
#include "SkyeTekProtocol.h"


#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
char str[80] = "";




// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int connect();





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
				connect();
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




int connect() {
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



	/*
	// Discover reader

	Debug.WriteLine("Discovering reader...");
	numDevices = SkyeTek_DiscoverDevices(&devices);
	if (numDevices == 0){
		return 0;
	}
	cout << "Discovered " << numDevices << " devices" << endl;
	numReaders = SkyeTek_DiscoverReaders(devices, numDevices, &readers);
	if (numReaders == 0)
	{
		SkyeTek_FreeDevices(devices, numDevices);
		return 0;
	}
	cout << "Found reader: " << readers[0]->friendly << endl;
	cout << "On device: " << readers[0]->lpDevice->type;
	cout << "[" << readers[0]->lpDevice->address << "]" << endl;

	*/

	return 1;

}








/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: printToScreen
--
-- DATE: October 05, 2016
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Deric Mccadden
--
-- PROGRAMMER: Deric Mccadden
--
-- INTERFACE: void printToScreen(char readBuffer[], DWORD dwRead, LPVOID hwnd)
--
-- RETURNS: void.
--
-- NOTES:
-- This prints characters to the window for display to the user.
----------------------------------------------------------------------------------------------------------------------*/
/*
void printToScreen(char readBuffer[], DWORD dwRead, LPVOID hwnd) {
	HDC hdc;
	RECT rect;
	TEXTMETRIC tm;
	SIZE size;

	static unsigned x = 0;
	static unsigned y = 0;

	hdc = GetDC((HWND)hwnd);
	//string str;

	sprintf_s(str, "%c", readBuffer[0]); // Convert char to string
	GetTextMetrics(hdc, &tm);

	TextOut(hdc, x, y, (LPCWSTR)str, strlen(str));
	GetTextExtentPoint32(hdc, (LPCWSTR)str, strlen(str), &size);

	x += size.cx;
	if (x >= WINDOW_WIDTH - 20) {
		x = 0;
		y = y + tm.tmHeight + tm.tmExternalLeading;
	}

	OutputDebugStringA("received: ");
	OutputDebugStringA(str);
	OutputDebugStringA("\n");
	ReleaseDC((HWND)hwnd, hdc);
	readBuffer = '\0';
}*/