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

#include "common.h"
#include "session.h"
#include "physical.h"

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
LPSKYETEK_READER* readers = NULL;
bool isConnected = false;
bool start = true;
std::vector<std::wstring> vecTag;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

DWORD WINAPI ReadThread(LPVOID hwnd);

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
   int width = 0;
   int height = 0;
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
	   (int)(width * 0.5 - 100 - 30),
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
	   (int)(width * 0.5 + 30),
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
				isConnected = connect(hWnd, &readers);
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

DWORD WINAPI ReadThread(LPVOID hwnd)
{
	HWND _hwnd = (HWND)hwnd;
	// Create the overlapped event. Must be closed before exiting
	// to avoid a handle leak.
	LPSECURITY_ATTRIBUTES lpEventAttributes = NULL;
	BOOL bManualReset = FALSE;
	BOOL bInitialState = FALSE;
	LPCTSTR lpName = NULL;

	while (true)
	{
		if (isConnected && start)
			ReadTag(readers, vecTag, _hwnd, listBox);
	}

	return 0;
}
