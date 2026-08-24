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

	// Range of the per-case pixel-value histogram (research tool); same CT_BIAS convention as
	// mMinThreshold/mMaxThreshold, so it's directly comparable to raw pixel values
	short mHistogramMin = 924;
	short mHistogramMax = 1124;

	// Cutoff, as a percentage of the histogram's peak count, used to find the "main area" (the
	// range around the peak that's still at least this fraction of it) - see CBoundHistogram::GetMainArea
	float mHistogramCutPercent = 20.0f;
	unsigned short mErodeLevel = 5;
	unsigned short mnWantedSliceWidth = 11; // Number of consecutive input slices to average

	int mnCentralRings = 2; // Number of innermost rings scored as "central"
	int mnOffCenterRings = 2; // Number of rings beyond the central rings scored as "off-center"

	EScoreType mScoreType = EScoreType::MinMax;

	int mDebug = 0xff;

	// Bump this when a change is expected to affect scoring results. Used e.g. to name
	// baseline result snapshots ("<msLogRoot>_<msVersion>") for regression comparison.
	std::string msVersion = "0.8.1";

	std::string msLogRoot = "d:\\IQV_Log";
	std::string msCaseLogDir; // <msLogRoot>\[<batch root name>\]<current case name>[_<miCaseIndex>], set by SetCurrentCase

	// Position (1-based) of the current case within a batch run, or 0 outside of batch scoring.
	// Case names collide often enough in real data (same immediate/parent folder name reused
	// across different sets) that msCaseLogDir alone isn't reliably unique - appending this
	// disambiguates the case log directory, and per-case files that Excel won't let you have two
	// same-named copies of open at once (e.g. Histogram_<miCaseIndex>.csv) can use it too.
	int miCaseIndex = 0;

	// Name of the current batch run's own log subdirectory (nests under msLogRoot; see
	// SetCurrentCase), or empty outside of batch scoring. Set directly by CBatchScorer for the
	// duration of a run rather than passed to every case - not read from/written to file.
	std::string msBatchRootDir;

	// Per-scorer directory with one ring-detail CSV per image (heavy: one dir + N files per scorer);
	// off by default so it doesn't blow up when batch-scoring many sets.
	bool mbLogImageRingDetails = false;

	// Filename pattern identifying a directory's DICOM image files, when scanning a directory tree for sets to score
	std::string msDicomFilePattern = "I00*";

	// Shows developer-only menu sections (Process/Set/Get/Test); real users only need File/Help.
	// Defaults to on so this doesn't change what's visible on a dev machine until explicitly turned off.
	bool mbDeveloperMode = true;

	void SetCurrentCase(const char* zCaseName, int iCaseIndex = 0);

	void SaveToFile();
	void ReadFromFile();

	void PrintStatus(const char* zStatus);
};

static constexpr float IGNORE_RING = -100.0;

extern CConfig gConfig;
extern CFileLogger gfLog;
