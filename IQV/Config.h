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

	// Lower bound used to detect "out of range" pixels when building wide (averaged) slices -
	// distinct from mMinThreshold, which governs ring validity during scoring, not this. Same
	// CT_BIAS convention. No upper bound: unusually high values are still real data here.
	short mWideMinThreshold = 500;

	// When on, a wide-image pixel is only computed (by averaging mnWantedSliceWidth consecutive
	// raw samples) if every one of those samples is at least mWideMinThreshold; otherwise it's
	// left at 0. The first/last few images in a series often cover a smaller radius than the
	// rest, so far-out pixels there aren't real data (read as low values) and would otherwise
	// pull the average toward garbage. On by default - preserves the previous unconditional averaging.
	bool mbFilterWideImageRange = true;

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

	// Full path of the directory tree the current batch run was scanned from (see
	// CBatchScorer::RunOnDirTree), or empty outside of batch scoring. Used by
	// CIQVManager::LoadImages to compose a case name from the real "junction" directories
	// between this root and each set - not read from/written to file.
	std::string msBatchScanRootPath;

	// Per-scorer directory with one ring-detail CSV per image (heavy: one dir + N files per scorer);
	// off by default so it doesn't blow up when batch-scoring many sets.
	bool mbLogImageRingDetails = false;

	// Filename pattern identifying a directory's DICOM image files, when scanning a directory tree for sets to score
	std::string msDicomFilePattern = "I00*";

	// Local root under which "Utils > Download Data" mirrors DICOM sets fetched from the network -
	// see CDataDownloader
	std::string msDataRoot = "d:\\IQV_Data";

	// "Utils > Download Data" only searches under directories whose name contains this
	// (case-insensitive) for DICOM sets to fetch, rather than the whole source tree
	std::string msDownloadDirNameFilter = "Water";

	// Network location offered as the default starting point for "Utils > Download Data"
	std::string msDownloadDefaultSource = "\\\\192.168.110.219\\Production";

	// Shows developer-only menu sections (Process/Set/Get/Test); real users only need File/Help.
	// Defaults to on so this doesn't change what's visible on a dev machine until explicitly turned off.
	bool mbDeveloperMode = true;

	// Optional 3rd viewer column: for each pixel, its ring's mean CT value for the currently
	// scored image, instead of the raw pixel value - a visual "what this would look like if
	// perfectly radially symmetric" reference, for spotting ring artifacts (illegal/eroded
	// pixels get a constant 10 below the image's lowest ring mean, to stand out). Only populated
	// during live scoring (not Case/Batch Review, which doesn't recompute per-ring means).
	bool mbDisplayCtPerRadius = true;

	void SetCurrentCase(const char* zCaseName, int iCaseIndex = 0);

	void SaveToFile();
	void ReadFromFile();

	void PrintStatus(const char* zStatus);
};

static constexpr float IGNORE_RING = -100.0;

extern CConfig gConfig;
extern CFileLogger gfLog;
