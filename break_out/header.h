#pragma once
#include <Windows.h>
#include <mmsystem.h>
#include <digitalv.h>
#include <vector>
#include <random>
#include <string>
#include <iostream>
void LoadSE(MCI_OPEN_PARMS open, MCI_PLAY_PARMS play, const char* resource);
void PlaySE(MCIDEVICEID id, MCI_PLAY_PARMS play);
std::wstring String2WString(std::string s){
	int bufsize = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, (wchar_t*)NULL, 0);
	wchar_t* buffer = new wchar_t[bufsize];
	std::wstring result(buffer, buffer+bufsize-1);
	delete[] buffer;
	return result;
}