#include "stdafx.h"

#include <windows.h>
#include <stdio.h>
#include <iostream>

#include "common.h"

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: printToScreen
--
-- DATE: November 15, 2016
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Deric Mccadden
--
-- PROGRAMMER: Jamie Lee
--
-- INTERFACE: void printToScreen(LPCWSTR readBuffer, HWND hWnd) 
--
-- RETURNS: void.
--
-- NOTES:
-- This prints a tags data to the window for display to the user.
----------------------------------------------------------------------------------------------------------------------*/
void printToScreen(LPCWSTR readBuffer, HWND hWnd) {
	RECT rect;
	int width = 0;
	int height = 0;
	if (GetClientRect(hWnd, &rect))
	{
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;
	}
	HDC hdc;
	TEXTMETRIC tm;
	SIZE size;

	static int x = 10;
	static int y = 0;

	hdc = GetDC((HWND)hWnd);
	GetTextMetrics(hdc, &tm);

	wchar_t temp[2];
	for (size_t i = 0; i < wcslen(readBuffer); i++) {
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

		if (y + tm.tmHeight >= 70)
			y = 0;

		TextOut(hdc, x, y, temp, wcslen(temp));
		x += size.cx;
	}
	ReleaseDC((HWND)hWnd, hdc);
	readBuffer = '\0';
}

void addTag(std::wstring msg, HWND hWnd) {
	SendMessage(hWnd, LB_ADDSTRING, NULL, (LPARAM)msg.c_str());
}
