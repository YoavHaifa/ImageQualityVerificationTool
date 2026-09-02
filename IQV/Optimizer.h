#pragma once
#include <vector>

// Scores every labeled case under gConfig.msTrainingSetRoot (see the Label feature, File > Label)
// and reports, per case, how the current scoring configuration's verdict compares to the
// human-assigned label - lets scoring parameters be tuned by looking at false positives/negatives
// directly instead of guessing from a handful of cases.
class COptimizer
{
public:
	COptimizer();
	~COptimizer();

	// zRootDir must directly contain "Pass" and "Fail" sub-directories, each holding one or more
	// labeled DICOM sets (however deeply nested under them). Scores every one found, under the
	// currently active scoring configuration, and writes one summary report row per case to
	// <gConfig.msLogRoot>\TrainingSetReport_<active score type>.csv. Returns the number of cases
	// scored (both labels combined).
	int RunOnTrainingSet(const char* zRootDir);

	// This run's own report file, valid after RunOnTrainingSet() - lets a caller offer to open it.
	const CString& GetReportName(void) const { return msReportName; }

private:
	struct SCaseResult
	{
		CString sLabel; // "Pass" or "Fail", from which sub-directory the case was found under
		CString sCaseName;
		float score = 0;
		bool bScoredPass = false;
		float gap = 0; // score - gConfig.mMaxAcceptableScore (negative on the Pass side)
		CString sAssessment; // Correct Pass / Correct Fail / False Positive / False Negative
	};

	// Scores every case found under zLabelDir (e.g. <root>\Pass), appending one row to mvResults
	// per case actually scored. No-op (not an error) if zLabelDir doesn't exist.
	void RunOnLabelDir(const char* zLabelDir, const char* zLabel);

	void WriteReport();

	std::vector<SCaseResult> mvResults;
	CString msReportName;
};
