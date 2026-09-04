#pragma once
#include "ScoreTypes.h"
#include <vector>

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
	// determined by its own name's prefix (case-insensitive): "pass", "fail_center", "fail_ring",
	// or "fail_both" (see DetermineLabel()) - a sub-directory whose name doesn't start with any of
	// these is skipped (reported via status). Scores every case found under every scorer type
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
		// case, per IsExpectedToFail() (not necessarily the case's own raw label - e.g. a
		// Fail_Ring case expects Pass from the Center scorer, since Center isn't responsible for
		// ring-only problems). sAssessment, and ComputeAndApplyNewWeights()'s cohort split, are
		// both driven by this rather than by sLabel directly.
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
		CString sLabel; // "Pass", "Fail_Center", "Fail_Ring", or "Fail_Both" - see DetermineLabel()
		CString sCaseName;
		int mainAreaWidth = 0; // this case's own pixel-histogram main area width (same for every type)

		// This case's outcome under every scorer type, indexed by (int)EScoreType - lets one
		// scoring pass produce every type's report, and OptimizeWeights() compute new weights,
		// without rescoring.
		std::vector<SPerTypeResult> vPerType;
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
	// (case-insensitive): "pass" -> "Pass", "fail_center" -> "Fail_Center", "fail_ring" ->
	// "Fail_Ring", "fail_both" -> "Fail_Both". Returns an empty string if the name doesn't start
	// with any of these - callers should skip such a directory rather than guess.
	static CString DetermineLabel(const CString& sSubDirName);

	// Whether the given scorer type is actually expected to fail a case carrying this label.
	// Every scorer is expected to fail every Fail label, EXCEPT: the Center scorer only targets
	// central artifacts, so it isn't expected to fail Fail_Ring (no center problem); the ring
	// scorers (MinMax/Tent/TentMin) only target off-center ring artifacts, so they aren't
	// expected to fail Fail_Center (no ring problem). AllMax is exempt from this narrowing - it's
	// meant to catch every failure type, since it's built from every sibling's own score.
	static bool IsExpectedToFail(EScoreType type, const CString& sLabel);

	// Scores every case found under zSubDir (e.g. <root>\fail_ring_2), appending one row to
	// mvResults per case actually scored, tagged with the given zLabel. sSubDirName (the
	// directory's own name, e.g. "fail_ring_2") - not zLabel - names this run's own log
	// sub-tree, so two different sub-directories sharing the same label never collide.
	// No-op (not an error) if zSubDir doesn't exist.
	void RunOnLabelDir(const char* zSubDir, const char* zLabel, const CString& sSubDirName);

	// Writes one CSV per scorer type into msReportDir (created if needed).
	void WriteReports();

	// For each individual scorer type: finds the highest Pass score and the lowest Fail score
	// above it, targets the midpoint between them (so gConfig.mMaxAcceptableScore lands exactly
	// there), and rescales that scorer's weight accordingly - catches every Fail case without
	// failing any Pass case, whenever such a clean separation exists. A false positive (failing a
	// Pass case) is never acceptable - so if no Fail case scores above the Pass max, there's no
	// safe split at all: the weight is instead lowered just enough that even the worst Pass case
	// stays at/below threshold, meaning this scorer flags nothing as Fail rather than risk one.
	// Applies each new weight to gConfig immediately (SetScorerWeight) and
	// persists them all at the end (SaveScorerWeights). Requires mvResults to already be
	// populated (see RunOnLabelDir) - does not itself score anything.
	void ComputeAndApplyNewWeights(std::vector<SWeightResult>& results);

	void WriteWeightsReport(const std::vector<SWeightResult>& results);

	std::vector<SCaseResult> mvResults;
	CString msReportDir; // <gConfig.msLogRoot>\TrainingSetReport - every report of this class's lives here
	CString msWeightsReportName;
};
