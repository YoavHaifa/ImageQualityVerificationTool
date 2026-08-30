#pragma once
#include <string>

// Loads a DICOM image set and runs the ring-quality scoring on it, independently of any UI.
class CIQVManager
{
public:
	CIQVManager();
	~CIQVManager();

	// iCaseIndex is this case's 1-based position within a batch run (0 outside of batch scoring);
	// see CConfig::miCaseIndex.
	bool LoadAndScore(const char* zImageFileName, int iCaseIndex = 0);

	// Loads a case's already-saved scoring results instead of rescoring: reloads the real
	// images (for display) from CaseInfo.yaml's case_path, but reads scores/rings back from
	// the saved per-scorer CSVs rather than recomputing them. zCaseDir is the log directory
	// that CRingsScorer wrote them into originally (contains CaseInfo.yaml and the CSVs).
	bool LoadFromSavedResults(const char* zCaseDir);

	// Reads just enough of zCaseDir\CaseInfo.yaml to name one real DICOM file for the case,
	// without loading any images yet - lets a caller set up a viewer for that file name
	// before LoadFromSavedResults() runs.
	bool ResolveCaseSampleFile(const char* zCaseDir, CString& osSampleFile);

	class CArinetaImages* GetImages(void) { return mpImages; }
	class CRingsScorer* GetRingsScorer(void) { return mpRingsScorer; }
	int GetScoredPosition(void) { return miScoredPosition; }

	std::string GetSetInfo(void);

private:
	// Common to both LoadAndScore and LoadFromSavedResults: loads the real images, sets up
	// the case's log directory, and computes the rotation center.
	bool LoadImages(const char* zImageFileName, int iCaseIndex = 0);

	// Refuses to replay a case whose CaseInfo.yaml csv_version doesn't match gConfig.mCsvVersion -
	// its per-scorer CSVs would otherwise be parsed under a column layout this build doesn't write
	bool CheckCsvVersion(const char* zCaseDir);

	// Composes a case name from the real "junction" directories (ones with more than one real
	// subdirectory) between sRoot and sSetDir, using at each junction whichever child was
	// actually taken toward sSetDir. Returns "SingleSet" if there's no junction at all, i.e.
	// sSetDir is the only set anywhere under sRoot.
	static CString ComposeCaseNameFromJunctions(const CString& sRoot, const CString& sSetDir);

	class CArinetaImages* mpImages = nullptr;
	class CRingsScorer* mpRingsScorer = nullptr;
	int miScoredPosition = 0;
};
