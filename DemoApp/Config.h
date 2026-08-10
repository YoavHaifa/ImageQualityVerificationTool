#pragma once
#include <string>
#include "..\..\yUtils\FileLogger.h"
#include "ScoreTypes.h"

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

	EScoreType mScoreType = EScoreType::MinMax;

	int mDebug = 0xff;

	std::string msScoreGraphsDir = "d:\\Log\\IQV_Graphs";

	void SaveToFile();
	void ReadFromFile();

	void PrintStatus(const char* zStatus);
};

static constexpr float IGNORE_RING = -100.0;

extern CConfig gConfig;
extern CFileLogger gfLog;
