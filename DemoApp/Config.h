#pragma once
#include <string>

class CConfig
{
public:
	CConfig();

	static const short CT_BIAS = 1024;
	short mMinThreshold = 980;
	short mMaxThreshold = 1050;
	unsigned short mErodeLevel = 5;
	unsigned short mnWantedSliceWidth = 11; // Number of consecutive input slices to average

	std::string msScoreGraphsDir = "d:\\Log\\IQV_Graphs";

	void SaveToFile();
	void ReadFromFile();

};

extern CConfig gConfig;
