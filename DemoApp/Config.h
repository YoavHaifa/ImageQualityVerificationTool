#pragma once

class CConfig
{
public:
	short mMinThreshold = 980;
	short mMaxThreshold = 1050;
	unsigned short mErodeLevel = 5;
	unsigned short mnWantedSliceWidth = 5; // Number of consecutive input slices to average


};

extern CConfig gConfig;
