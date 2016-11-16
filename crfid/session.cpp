#include "stdafx.h"

#include <windows.h>
#include <iostream>
#include <sstream>

#include "SkyeTekAPI.h"
#include "SkyeTekProtocol.h"

#include "common.h"
#include "session.h"

bool connect(HWND hWnd, LPSKYETEK_READER** readers) {
	LPSKYETEK_DEVICE *devices = NULL;
	//LPSKYETEK_READER *readers = NULL;
	LPSKYETEK_TAG *lpTags = NULL;
	LPSKYETEK_DATA lpData = NULL;
	SKYETEK_STATUS st;
	unsigned int numDevices;
	unsigned int numReaders;
	int loops = 100;
	int totalReads = 0;
	int failedReads = 0;
	int failedLoops = 0;

	std::wstring out;
	std::wstringstream ss;

	// Discover reader

	printToScreen(L"Discovering reader...", hWnd);

	numDevices = SkyeTek_DiscoverDevices(&devices);
	if (numDevices == 0) {
		return false;
	}
	ss << L"Discovered " << numDevices << L" devices.\n";
	out = ss.str();
	printToScreen(out.c_str(), hWnd);
	ss.str(std::wstring());
	ss.clear();


	numReaders = SkyeTek_DiscoverReaders(devices, numDevices, readers);
	if (numReaders == 0)
	{
		SkyeTek_FreeDevices(devices, numDevices);
		return false;
	}
	ss << L"Found reader: " << (*readers)[0]->readerName << L"\nOn device: ";
	ss << (*readers)[0]->lpDevice->type << L"\n";// << L"[" << readers[0]->lpDevice->address << L"]\n";
	out = ss.str();
	printToScreen(out.c_str(), hWnd);
	ss.str(std::wstring());
	ss.clear();


	// Get baud if serial
	if (_tcscmp((*readers)[0]->lpDevice->type, SKYETEK_SERIAL_DEVICE_TYPE) == 0)
	{
		st = SkyeTek_GetSystemParameter((*readers)[0], SYS_BAUD, &lpData);
		if (st != SKYETEK_SUCCESS)
		{
			SkyeTek_FreeReaders((*readers), numReaders);
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
		ss << L"current baud: " << currentBaud << L"\n";
	}

	// Set additional timeout
	SkyeTek_SetAdditionalTimeout((*readers)[0]->lpDevice, 5000);

	out = ss.str();
	printToScreen(out.c_str(), hWnd);
	ss.str(std::wstring());
	ss.clear();

	return true;

}
