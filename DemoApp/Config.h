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
	short mMaxThreshold = 1080;
	unsigned short mErodeLevel = 5;
	unsigned short mnWantedSliceWidth = 11; // Number of consecutive input slices to average

	int mnCentralRings = 2; // Number of innermost rings scored as "central"
	int mnOffCenterRings = 2; // Number of rings beyond the central rings scored as "off-center"

	EScoreType mScoreType = EScoreType::MinMax;

	int mDebug = 0xff;

	// Bump this when a change is expected to affect scoring results. Used e.g. to name
	// baseline result snapshots ("<msLogRoot>_<msVersion>") for regression comparison.
	std::string msVersion = "0.8";

	std::string msLogRoot = "d:\\IQV_Log";
	std::string msCaseLogDir; // <msLogRoot>\<current case name>, set by SetCurrentCase

	// Per-scorer directory with one ring-detail CSV per image (heavy: one dir + N files per scorer);
	// off by default so it doesn't blow up when batch-scoring many sets.
	bool mbLogImageRingDetails = false;

	// Filename pattern identifying a directory's DICOM image files, when scanning a directory tree for sets to score
	std::string msDicomFilePattern = "I00*";

	// Shows developer-only menu sections (Process/Set/Get/Test); real users only need File/Help.
	// Defaults to on so this doesn't change what's visible on a dev machine until explicitly turned off.
	bool mbDeveloperMode = true;

	void SetCurrentCase(const char* zCaseName);

	void SaveToFile();
	void ReadFromFile();

	void PrintStatus(const char* zStatus);
};

static constexpr float IGNORE_RING = -100.0;

extern CConfig gConfig;
extern CFileLogger gfLog;
