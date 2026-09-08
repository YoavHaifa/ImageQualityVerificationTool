#pragma once
#include "ScoreTypes.h"
#include <vector>
#include <functional>

// Scores every labeled case under gConfig.msTrainingSetRoot (see the Label feature, File > Label)
// and reports, per case and per scorer, how the current scoring configuration's verdict compares
// to the human-assigned label - lets scoring parameters be tuned by looking at false positives/
// negatives directly instead of guessing from a handful of cases.
class COptimizer
{
public:
	COptimizer();
	~COptimizer();

	// zRootDir may directly contain any number of sub-directories, each holding one or more
	// labeled DICOM sets (however deeply nested under them) - every one is scored, with its label
	// determined by its own name's prefix (case-insensitive): "pass" or "fail" (see
	// DetermineLabel()) - a sub-directory whose name doesn't start with either is skipped
	// (reported via status). Scores every case found under every scorer type
	// (not just whichever is currently active), and writes one report per type to
	// <gConfig.msLogRoot>\TrainingSetReport\TrainingSetReport_<type>.csv. Returns the number of
	// cases scored (every label combined).
	int RunOnTrainingSet(const char* zRootDir);

	// The directory all of this class's reports are written into, valid after RunOnTrainingSet() -
	// lets a caller offer to open it.
	const CString& GetReportDir(void) const { return msReportDir; }

	// Optimizes every individual scorer's weight (not AllMax's own - it's fixed at 1.0, built
	// from siblings) from a single scoring pass over zRootDir, then rescores everything again
	// with the new weights so the resulting reports reflect them. See the .cpp for the algorithm.
	// Backs up both ScorerWeights.csv and the "before" results reports (each as
	// "..._before_optimize...") so old vs new can be compared. Returns the number of cases scored
	// in the final (new-weights) pass.
	int OptimizeWeights(const char* zRootDir);

	// This run's weight-optimization report (old/new weight, pass max, fail target per scorer),
	// valid after OptimizeWeights().
	const CString& GetWeightsReportName(void) const { return msWeightsReportName; }

private:
	// This case's outcome under one particular scorer type - see RunOnLabelDir
	struct SPerTypeResult
	{
		float score = 0;
		bool bScoredPass = false;
		float gap = 0; // score - gConfig.mMaxAcceptableScore (negative on the Pass side)

		// "Pass" or "Fail" - what this specific scorer is actually expected to produce for this
		// case, per IsRegionExpectedToFail()/IsAnyRingRegionExpectedToFail() (not necessarily the
		// case's own raw label - e.g. a case labeled Fail only in LowRes expects Pass from the
		// Center scorer, since Center isn't responsible for problems outside its own region).
		// sAssessment, and ComputeAndApplyNewWeights()'s cohort split, are both driven by this
		// rather than by sLabel directly.
		CString sExpectedVerdict;

		CString sAssessment; // Correct Pass / Correct Fail / False Positive / False Negative
		int ring = -1; // the ring that produced `score`
		int originalImage = -1; // the DICOM slice that produced `score`

		// Which scorer actually produced `score` - this type itself, except for AllMax, where
		// it's whichever sibling actually won (AllMax isn't an independent measurement).
		CString sCriticalScorer;
		float criticalRawScore = 0; // that scorer's own true (unweighted, unscaled) raw score
	};

	struct SCaseResult
	{
		CString sLabel; // "Pass" or "Fail" - see DetermineLabel()
		CString sCaseName;
		int mainAreaWidth = 0; // this case's own pixel-histogram main area width (same for every type)

		// This case's outcome under every scorer type, indexed by (int)EScoreType - lets one
		// scoring pass produce every type's report, and OptimizeWeights() compute new weights,
		// without rescoring.
		std::vector<SPerTypeResult> vPerType;

		// This case's outcome for each of the 3 ring scorers' (MinMax/Tent/TentMin) 3 regions
		// (HighRes/Border/LowRes) - unlike vPerType above (comprehensive, merged across a ring
		// scorer's regions), this is one specific region's own score. Indexed
		// iRingType*3+iRegion, where iRingType is 0/1/2 for MinMax/Tent/TentMin (not (int)EScoreType
		// - Center/AllMax have no regions and aren't in here) and iRegion is 0/1/2 for
		// HighRes/Border/LowRes - see ComputeAndApplyNewWeights(), which is the only reader.
		std::vector<SPerTypeResult> vPerRegion;

		// Which region(s) this case's own CaseLabelInfo.yaml flagged as showing the problem (see
		// CIQVDlg::SaveLabeledData) - read directly from that file, alongside the DICOM images
		// themselves, by RunOnLabelDir(). Always false for a Pass-labeled case (the region dialog
		// is Fail-only, so the YAML's own flags are already false there too - this is just read
		// back, not re-derived from sLabel). See IsRegionExpectedToFail().
		bool bFailedCenter = false;
		bool bFailedHR = false;
		bool bFailedBorder = false;
		bool bFailedLR = false;
	};

	// One scorer's weight-optimization outcome - see ComputeAndApplyNewWeights()
	struct SWeightResult
	{
		CString sScorer;
		float oldWeight = 1.0f;
		float newWeight = 1.0f;
		float passMax = 0;
		float failTarget = 0; // lowest Fail score above passMax, or (if none) the lowest Fail score overall
		bool bSeparated = true; // false if no Fail case scored above passMax (couldn't cleanly separate)
		bool bHasData = true; // false if either cohort was empty for this scorer - weight left unchanged
	};

	// Determines a training-set sub-directory's label from its own name's prefix
	// (case-insensitive): "pass" -> "Pass", "fail" -> "Fail" (this includes an old
	// fail_center/fail_ring/fail_both sub-directory - which region(s) of a failed case show the
	// problem now lives in that case's own CaseLabelInfo.yaml, see CIQVDlg::SaveLabeledData, not the
	// directory name). Returns an empty string if the name starts with neither - callers should
	// skip such a directory rather than guess.
	static CString DetermineLabel(const CString& sSubDirName);

	// Whether this case is expected to fail in the given region specifically (Center for the
	// Center scorer, or one of HighRes/Border/LowRes for a ring scorer's own per-region cohort) -
	// reads the case's own labeled region flags (SCaseResult::bFailed*, from CaseLabelInfo.yaml),
	// not just its coarse Pass/Fail label. A Pass-labeled case is never expected to fail anywhere
	// (its region flags are already all false anyway - see CIQVDlg::SaveLabeledData).
	// NOTE (2026-09-08): replaces an earlier IsExpectedToFail(EScoreType, CString) that had gone
	// silently dormant - it matched against the OLD "Fail_Center"/"Fail_Ring"/"Fail_Both" directory
	// names, which DetermineLabel() stopped producing once labeling moved to CaseLabelInfo.yaml
	// (see [[project-four-region-labeling]]) - so every non-AllMax cohort came back empty
	// ("no data - unchanged" in WeightOptimization.csv) for every training run since. Caught while
	// wiring up per-region tuning tonight.
	static bool IsRegionExpectedToFail(const SCaseResult& r, ERegion region);

	// Whether this case is expected to fail ANY of a ring scorer's 3 regions (HighRes/Border/
	// LowRes) - used for a ring scorer's own COMPREHENSIVE assessment (vPerType, merged across its
	// regions, same as TrainingSetReport_<type>.csv already reports), as opposed to one specific
	// region's own cohort (IsRegionExpectedToFail).
	static bool IsAnyRingRegionExpectedToFail(const SCaseResult& r);

	// Scores every case found under zSubDir (e.g. <root>\fail_ring_2), appending one row to
	// mvResults per case actually scored, tagged with the given zLabel. sSubDirName (the
	// directory's own name, e.g. "fail_ring_2") - not zLabel - names this run's own log
	// sub-tree, so two different sub-directories sharing the same label never collide.
	// No-op (not an error) if zSubDir doesn't exist.
	void RunOnLabelDir(const char* zSubDir, const char* zLabel, const CString& sSubDirName);

	// Writes one CSV per scorer type into msReportDir (created if needed).
	void WriteReports();

	// Computes one weight-optimization outcome via the midpoint algorithm: finds the highest Pass
	// score and the lowest Fail score above it (getScore extracts whichever score matters - a
	// whole type's comprehensive score, or one specific region's own score), targets the midpoint
	// between them (so gConfig.mMaxAcceptableScore lands exactly there), and rescales oldWeight
	// accordingly - catches every Fail case without failing any Pass case, whenever such a clean
	// separation exists. A false positive (failing a Pass case) is never acceptable - so if no Fail
	// case scores above the Pass max, there's no safe split at all: the weight is instead lowered
	// just enough that even the worst Pass case stays at/below threshold, meaning this scorer
	// flags nothing as Fail rather than risk one. Doesn't touch gConfig or mvResults itself -
	// ComputeAndApplyNewWeights() (the only caller) applies/persists the returned weight.
	// isExpectedToFail decides cohort membership (Pass vs Fail) for each case - the caller passes
	// whichever notion applies (IsRegionExpectedToFail for one region, IsAnyRingRegionExpectedToFail
	// for a ring type's own comprehensive weight, or a simple Fail-label check for AllMax-like
	// "catches everything" semantics), so this method itself doesn't need to know which scorer or
	// region it's tuning.
	SWeightResult ComputeOneWeight(const CString& sName, float oldWeight,
		const std::function<float(const SCaseResult&)>& getScore,
		const std::function<bool(const SCaseResult&)>& isExpectedToFail) const;

	// For each of the 3 ring scorers' (MinMax/Tent/TentMin) 3 regions (HighRes/Border/LowRes),
	// and separately for Center (which has no regions), computes and applies (SetScorerWeight)
	// a new weight via ComputeOneWeight() - see there for the algorithm. AllMax's weight stays
	// fixed at 1.0, built from siblings - not tunable. Persists every weight at the end
	// (SaveScorerWeights). Requires mvResults to already be populated (see RunOnLabelDir) - does
	// not itself score anything.
	void ComputeAndApplyNewWeights(std::vector<SWeightResult>& results);

	void WriteWeightsReport(const std::vector<SWeightResult>& results);

	std::vector<SCaseResult> mvResults;
	CString msReportDir; // <gConfig.msLogRoot>\TrainingSetReport - every report of this class's lives here
	CString msWeightsReportName;
};
