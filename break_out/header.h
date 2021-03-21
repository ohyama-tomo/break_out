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
