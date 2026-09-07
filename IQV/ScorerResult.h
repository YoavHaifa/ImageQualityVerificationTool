#pragma once
#include "ImageScore.h"
#include "ScoreTypeResults.h"
#include "ScoreTypes.h"

// One independently tracked (scorer type, region) result. CImageRingsScorer holds a flat list of
// these - one for Center, one for AllMax, and one PER REGION (HighRes/Border/LowRes) for each of
// the 3 ring scorers (MinMax/Tent/TentMin, see IsRingScorerType) - instead of bookkeeping living
// inside each scorer object (that used to mean exactly one CImageScore/CScoreTypeResults per
// EScoreType). This is what makes reviewing one region exactly as complete as reviewing one
// scorer type already was: each region's history is independently computed, recorded and saved,
// so selecting among them at review time never needs to rescore or reconstruct data that was
// already discarded.
//
// Scoring itself (see CImageRingsScorer::Score) always fills every one of these, regardless of
// which regions are currently enabled - "at scoring stage we do not have to check region limits,
// we have to score all regions" (Yoav). IsActive() is consulted only by the live "what's on
// screen right now" queries (GetActiveScore et al) - comprehensive queries (GetScore/
// GetScoreAtMax/GetWorstScore/peak-finding/GetRawScoreAt, and training/tuning via COptimizer)
// deliberately ignore it and always consider every region.
class CScorerResult
{
public:
	CScorerResult(EScoreType eType, ERegion eRegion, const CString& sName);

	EScoreType meScoreType;
	ERegion meRegion; // ERegion::Center for Center's own result, HighRes/Border/LowRes for a ring
		// scorer's 3 results; N_REGIONS only for AllMax, which isn't tied to one specific region
	CString msName; // e.g. "MinMax_HighRes", "Center", "AllMax" - used for ScoreAllImages_<name>.csv

	// AllMax's own single result is always active (it's the aggregator, not a region - what feeds
	// INTO it is filtered per-input instead); every other result (Center's own, and a ring
	// scorer's 3) is active iff gConfig.IsRegionEnabled(meRegion) - see the "Review Regions"
	// checkboxes. Center is not special here - it's just as toggleable as any other region.
	bool IsActive() const;

	CImageScore mScore; // this image's own score - set fresh every CImageRingsScorer::Score() call
	CScoreTypeResults mResults; // across-images history/peaks - see CScoreTypeResults

	// Writes/replays ScoreAllImages_<msName>.csv - same format CScorerBase::LogAllImages/
	// LoadSavedResults used to have when bookkeeping lived on the scorer object, just now keyed by
	// (type,region) instead. weight is resolved by the caller (CImageRingsScorer knows whether
	// this is a ring-scorer region or a plain single-weight type; this class doesn't need to).
	// EXCEPT AllMax, which is never replayed this way - see CImageRingsScorer::ReplayAllMaxResult.
	void LogAllImages(int iFirst, int iStep) const;
	bool LoadSavedResults(const char* zCaseDir, float weight, float dataRangeFactor);
};
