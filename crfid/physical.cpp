#include "stdafx.h"

#include <sstream>
#include <vector>

#include "SkyeTekAPI.h"
#include "SkyeTekProtocol.h"

#include "common.h"
#include "physical.h"


/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: ReadTagData
--
-- DATE: November 15, 2016
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Deric Mccadden
--
-- PROGRAMMER: Jamie Lee
--
-- INTERFACE: SKYETEK_STATUS ReadTagData(const LPSKYETEK_READER lpReader, const LPSKYETEK_TAG lpTag)
--
-- RETURNS: SKYETEK_STATUS.
--
-- NOTES:
-- This reads all of the memory on a tag.
----------------------------------------------------------------------------------------------------------------------*/
/**
* This reads all of the memory on a tag.
* @param lpReader Handle to the reader
* @param lpTag Handle to the tag
* @return SKYETEK_SUCCESS if it succeeds, or the failure status otherwise
*/
SKYETEK_STATUS ReadTagData(const LPSKYETEK_READER lpReader, const LPSKYETEK_TAG lpTag)
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

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: ReadTag
--
-- DATE: November 15, 2016
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Deric Mccadden
--
-- PROGRAMMER: Jamie Lee
--
-- INTERFACE: void ReadTag(const LPSKYETEK_READER* readers, std::vector<std::wstring>& vecTag, HWND hWnd, HWND hListBox)
--
-- RETURNS: void.
--
-- NOTES:
-- This reads all of the memory on a tag.
----------------------------------------------------------------------------------------------------------------------*/
void ReadTag(const LPSKYETEK_READER* readers, std::vector<std::wstring>& vecTag, HWND hWnd, HWND hListBox)
{
	std::wstring out;
	std::wstringstream ss;
	LPSKYETEK_DATA lpData = NULL;
	LPSKYETEK_TAG *lpTags = NULL;
	SKYETEK_STATUS st;
	unsigned short count;
	int totalReads = 0;

	ss.str(std::wstring());
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
		ss << L"*** WARNING: SkyeTek_GetTags timed out: " << readers[0]->friendly << L"\n";
		out = ss.str();
		printToScreen(out.c_str(), hWnd);
		ss.str(std::wstring());
		ss.clear();
		return;
	}
	else if (st != SKYETEK_SUCCESS)
	{
		ss << L"*** ERROR: SkyeTek_GetTags failed: " << STPV3_LookupResponse(st) << L"\n";
		out = ss.str();
		printToScreen(out.c_str(), hWnd);
		ss.str(std::wstring());
		ss.clear();
		return;
	}

	for (unsigned short ix = 0; ix < count; ix++)
	{
		ss << L"Discovered tag: [" << SkyeTek_GetTagTypeNameFromType(lpTags[ix]->type) << L"]\t";
		ss << L" - " << lpTags[ix]->friendly << L"\n";
		out = ss.str();
		//addTag(out, hWnd);
		ss.str(std::wstring());
		ss.clear();

		// Don't attempt to read EM4X22 tags
		if ((lpTags[ix]->type & 0xFFF0) != EM4X22_AUTO)
		{
			ss << L"Reading tag: [" << SkyeTek_GetTagTypeNameFromType(lpTags[ix]->type) << L"]\t";
			ss << L" - " << lpTags[ix]->friendly << L"\n";
			out = ss.str();
			if (std::find(vecTag.begin(), vecTag.end(), lpTags[ix]->friendly) == vecTag.end()) {
				vecTag.push_back(lpTags[ix]->friendly);
				addTag(out, hListBox);
			}
			ss.str(std::wstring());
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
