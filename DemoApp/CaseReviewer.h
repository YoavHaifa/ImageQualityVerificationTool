#pragma once

// Manages interactive review of a case's already-saved scoring results (from a prior
// live-open or batch run), without rescoring - the counterpart to CIQVManager::LoadAndScore
// for revisiting a case rather than freshly scoring one.
//
// Used in two steps: SelectCase() prompts for and validates the case directory (cheap, no
// image loading yet); LoadCase() then does the actual image/score loading.
class CCaseReviewer
{
public:
	CCaseReviewer();
	~CCaseReviewer();

	// Prompts the user to pick a case's log directory (containing CaseInfo.yaml and its
	// saved CSVs) and validates it's a reviewable case. Returns false if the user cancels
	// or the directory isn't valid.
	bool SelectCase();

	// Loads the real images and replays the saved scores into a CRingsScorer, for the case
	// picked by SelectCase().
	bool LoadCase();

	class CIQVManager* GetManager(void) { return mpManager; }

private:
	class CIQVManager* mpManager = nullptr;
	CString msCaseDir;
};
