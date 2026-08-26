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

	class CIQVManager* GetManager(void) { return mpManager; }
	int GetCurrentRank(void) const { return miCurrentRank; }
	int GetNumCases(void) const { return (int)mvCases.size(); }

private:
	void BuildCaseList(const char* zRootDir);
	void ComputeOrder();
	int FindScorerIndex(const char* zName) const;
	bool LoadCaseAtRank(int iRank);

	std::vector<CString> mvScorerNames;
	std::vector<CCaseInfo> mvCases;
	int miCurrentRank = 0;

	class CIQVManager* mpManager = nullptr; // owns the currently-displayed case's images/scorer
};
