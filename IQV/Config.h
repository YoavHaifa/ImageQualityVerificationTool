#pragma once
#include <string>
#include <vector>
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

	int mnCentralRings = 3; // Number of innermost rings scored as "central"
	int mnOffCenterRings = 2; // Number of rings beyond the central rings scored as "off-center"

	// Below this many in-mask pixels, an image is scored 0 by every scorer rather than
	// trusted - e.g. wide images sharing a mask with the very first slice, which has little data
	int mnMinPixelsInMask = 1000;

	// Some of Arineta's scanners have lesser-quality off-center detectors - data out there
	// shouldn't be reported as a ring artifact. When on, CImageRingsScorer::CollectRingsInfo()
	// simply never enters any pixel whose ring index is above mLowResolutionDistanceFromCenterPixels
	// into that ring's statistics - same as a ring with too few valid pixels, it ends up IGNORE_RING
	// and every scorer already skips those. The displayed image itself is not masked.
	bool mbIgnoreLowResolutionArea = true;
	int mLowResolutionDistanceFromCenterPixels = 256;

	EScoreType mScoreType = EScoreType::MinMax;

	// Global pass/fail cutoff on the displayed score, regardless of which scorer is active.
	// A score above this is a "Failure", at or below it is a "Pass".
	float mMaxAcceptableScore = 10.0f;

	bool IsPass(float score) const { return score <= mMaxAcceptableScore; }

	// How strongly the given score supports its own pass/fail verdict, as a 0..1 fill fraction
	// of just that one color (see CIQVDlg::PaintPassFailIndicator - pass shows only green, fail
	// shows only red, no mixing). Symmetric around the threshold: right at it (from either side)
	// certainty is at its lowest (0.2, never all the way to 0 - a bare verdict still shows some
	// color) and grows to 1.0 by half the threshold below it (a clean pass) or 1.5x the
	// threshold above it (a clean fail).
	float ComputeCertaintyFraction(float score) const;

	// Not read from/written to ReconTest.State.xml - a hardcoded build-time constant. Bump this
	// whenever ScoreAllImages_<Name>.csv's column layout changes (see CScorerBase::LogAllImages/
	// LoadSavedResults). Written into each case's CaseInfo.yaml as csv_version when scored;
	// CIQVManager::LoadFromSavedResults refuses to replay a case whose csv_version doesn't match,
	// since its CSVs would be parsed under the wrong column layout otherwise.
	int mCsvVersion = 2;

	int mDebug = 0xff;

	// Bump this when a change is expected to affect scoring results. Used e.g. to name
	// baseline result snapshots ("<msLogRoot>_<msVersion>") for regression comparison.
	std::string msVersion = "0.8.5";

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

	// Shows the "Label" menu (File>Label - save the current case/section as pass/fail training
	// data), for collecting labeled data to tune the app with. Off for real end users eventually,
	// on by default for now since only internal people label data at this point.
	bool mbCollectDataForTraining = true;

	// Root under which labeled DICOM sets are copied verbatim (as-is, no reprocessing) - one
	// sub-tree "Pass", one "Fail", each holding one directory per labeled save, named the same
	// relative way as under msLogRoot (see GetCaseRelativeLogDir) so multiple source roots
	// reusing the same case name don't collide.
	std::string msTrainingSetRoot = "d:\\IQV_Data\\TrainingSet";

	// Number of images copied by "Save Section" - centered on the currently displayed image,
	// clipped to the case's own first/last image
	int mSavedSectionLength = 21;

	// msCaseLogDir with the msLogRoot prefix stripped off - i.e. [<msBatchRootDir>\]<case name>
	// [_<index>]. Reused to mirror the same case-identifying relative structure under
	// msTrainingSetRoot (see the Label feature) - computed fresh each time since msCaseLogDir
	// only exists after SetCurrentCase(), rather than kept as its own field.
	std::string GetCaseRelativeLogDir() const;

	// Shows developer-only menu sections (Process/Set/Get/Test); real users only need File/Help.
	// Defaults to on so this doesn't change what's visible on a dev machine until explicitly turned off.
	bool mbDeveloperMode = true;

	// Optional 3rd viewer column: for each pixel, its ring's mean CT value for the currently
	// scored image, instead of the raw pixel value - a visual "what this would look like if
	// perfectly radially symmetric" reference, for spotting ring artifacts (illegal/eroded
	// pixels get a constant 10 below the image's lowest ring mean, to stand out). Only populated
	// during live scoring (not Case/Batch Review, which doesn't recompute per-ring means).
	bool mbDisplayCtPerRadius = true;

	// Work-around for a machine (Gilad's) where the wide/CT-per-radius shared-memory volumes
	// display as all-zero in ImageR even though the computed data itself is correct on disk -
	// root cause not yet found. When on, CIQVDlg::DisplayVolume() dumps each volume to a file
	// (see CMyImage::Dump/umsLastSavedImageName) and has ImageR open that file instead of
	// sharing the live memory-mapped volume.
	bool mbAvoidSharedMemory = false;

	// Where ReconTest.State.xml and ScorerWeights.csv live - computed once in Init(), before
	// ReadFromFile()/LoadScorerWeights() need it. Not read from/written to file itself.
	// Normally the running exe's own directory, EXCEPT: if that directory is named "Debug" and
	// has a sibling "Release" directory, the Release one is used instead - so a Debug build run
	// during development shares the same config/weights as the Release build, rather than each
	// drifting its own separate copy.
	std::string msConfigDir;

	void SetCurrentCase(const char* zCaseName, int iCaseIndex = 0);

	void SaveToFile();
	void ReadFromFile();

	void PrintStatus(const char* zStatus);

	// Per-scorer weight, brings different scorers' scores to a similar scale before they're
	// compared (e.g. by CAllMaxScorer) or displayed. Read from ScorerWeights.csv (app directory,
	// created with every weight defaulted to 1.0 if missing) once at startup - changing a weight
	// there needs an app restart to take effect, but Case/Batch Review's replay path (which
	// re-weights each scorer's saved raw score, not just replaying the old weighted one) then
	// reflects it without rescoring.
	float GetScorerWeight(EScoreType type) const;

	// Where ScorerWeights.csv lives (msConfigDir\ScorerWeights.csv) - exposed so a caller (e.g.
	// COptimizer) can back the file up before overwriting it.
	std::string GetScorerWeightsFileName() const { return msConfigDir + "\\ScorerWeights.csv"; }

	// Overwrites one scorer's in-memory weight immediately - unlike a hand-edited ScorerWeights.csv,
	// this takes effect on the very next CScorerBase constructed (e.g. the next LoadAndScore() call),
	// no restart needed. Doesn't touch disk by itself - call SaveScorerWeights() too to persist it.
	void SetScorerWeight(EScoreType type, float weight);

	// Rewrites ScorerWeights.csv from the current in-memory weights - e.g. after SetScorerWeight().
	void SaveScorerWeights() const;

private:
	void ComputeConfigDir();
	void LoadScorerWeights();

	// msTrainingSetRoot can be nested more than one level deep (default d:\IQV_Data\TrainingSet)
	// under directories that may not exist yet - CreateDirectory (and so CMyWindows::
	// VerifyDirectory) only ever creates one level, so this walks up from the drive root instead
	// of assuming the parent already exists (same technique CDataDownloader already relies on).
	void VerifyTrainingSetRoot();

	std::vector<float> mvScorerWeights;
};

static constexpr float IGNORE_RING = -100.0;

extern CConfig gConfig;
extern CFileLogger gfLog;
