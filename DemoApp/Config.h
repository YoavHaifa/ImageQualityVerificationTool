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

	int mnCentralRings = 2; // Number of innermost rings scored as "central"
	int mnOffCenterRings = 2; // Number of rings beyond the central rings scored as "off-center"

	EScoreType mScoreType = EScoreType::MinMax;

	int mDebug = 0xff;

	std::string msLogRoot = "d:\\IQV_Log";
	std::string msCaseLogDir; // <msLogRoot>\<current case name>, set by SetCurrentCase

	void SetCurrentCase(const char* zCaseName);

	void SaveToFile();
	void ReadFromFile();

	void PrintStatus(const char* zStatus);
};

static constexpr float IGNORE_RING = -100.0;

extern CConfig gConfig;
extern CFileLogger gfLog;
