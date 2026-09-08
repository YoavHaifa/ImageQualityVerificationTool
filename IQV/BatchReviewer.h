#pragma once
#include "CaseInfo.h"
#include <vector>

// Manages interactively leafing between many already-scored cases under a root directory
// (each case being a log directory CRingsScorer wrote, the same shape CCaseReviewer opens
// one of). Builds a worst-to-best severity order per scorer - mirroring how
// CScoreTypeResults orders peaks within one case, but one case per position instead of one
// image per position - and leafs a case at a time, reusing CIQVManager::LoadFromSavedResults
// to actually load each case picked.
class CBatchReviewer
{
public:
	CBatchReviewer();
	~CBatchReviewer();

	// Prompts the user for a root directory containing one case log directory per case, then
	// calls Init(zRootDir) below. Returns false if canceled.
	bool Init();

	// Builds the case list under zRootDir and each scorer's worst-to-best order, then loads
	// the worst case for the currently active scorer (gConfig.mScoreType). Returns false if
	// no valid cases were found, or the worst case failed to load.
	bool Init(const char* zRootDir);

	// Leaf to the next/previous case in severity order (for gConfig.mScoreType), or back to
	// the worst. Returns false if there's nowhere to go or the target case failed to load.
	bool DisplayNextCase();
	bool DisplayPrevCase();
	bool DisplayWorstCase();

	// Re-ranks every case by its ACTIVE-region-filtered severity (see GetActiveWorstScore) - call
	// this whenever the "Review Regions" checkboxes change (CIQVDlg::OnBnClickedCheckReviewRegion),
	// before DisplayWorstCase()/re-displaying, so "worst case"/"next case" always reflects what's
	// currently enabled without reopening or rescoring any case. Also called once by Init().
	void ComputeOrder();

	class CIQVManager* GetManager(void) { return mpManager; }
	int GetCurrentRank(void) const { return miCurrentRank; }
	int GetNumCases(void) const { return (int)mvCases.size(); }

private:
	void BuildCaseList(const char* zRootDir);
	int FindScorerIndex(const char* zName) const;
	const CCaseInfo* FindCaseAtRank(int iRank) const;
	bool LoadCaseAtRank(int iRank);

	// The ACTIVE-region-filtered worst score for one case, under the scorer at mvScorerNames[iScorer] -
	// unlike CCaseInfo::mvWorstScore[iScorer] (comprehensive, every region), this considers only
	// whichever regions gConfig.IsRegionEnabled() currently allows: for a ring scorer (MinMax/Tent/
	// TentMin), the max of its own 3 regions' worst scores that are enabled; for Center, its own
	// worst score if Center is enabled, else 0; for AllMax, the max of every OTHER scorer's own
	// active-filtered worst score (same aggregation CImageRingsScorer::ComputeAllMaxScore already
	// does one level down, per image instead of per case).
	float GetActiveWorstScore(const CCaseInfo& c, int iScorer) const;

	std::vector<CString> mvScorerNames;
	std::vector<CCaseInfo> mvCases;
	int miCurrentRank = 0;

	class CIQVManager* mpManager = nullptr; // owns the currently-displayed case's images/scorer
};
