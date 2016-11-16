#pragma once

SKYETEK_STATUS ReadTagData(const LPSKYETEK_READER lpReader, const LPSKYETEK_TAG lpTag);
void ReadTag(const LPSKYETEK_READER* readers, std::vector<std::wstring>& vecTag, HWND hWnd, HWND hListBox);
