#pragma once
#include <string>
#include "..\..\yUtils\FileLogger.h"

class CConfig
{
public:
	CConfig();
	void Init();

	static const short CT_BIAS = 1024;
	short mMinThreshold = 980;
	short mMaxThreshold = 1050;
	unsigned short mErodeLevel = 5;
	unsigned short mnWantedSliceWidth = 11; // Number of consecutive input slices to average

	int mDebug = 0xff;

	std::string msScoreGraphsDir = "d:\\Log\\IQV_Graphs";

	void SaveToFile();
	void ReadFromFile();

};

extern CConfig gConfig;
extern CFileLogger gfLog;
