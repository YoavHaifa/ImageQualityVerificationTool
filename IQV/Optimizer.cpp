#include "stdafx.h"
#include "Optimizer.h"
#include "BatchScorer.h"
#include "IQVManager.h"
#include "RingsScorer.h"
#include "Config.h"
#include "..\..\yUtils\FilesList.h"
#include "..\..\yUtils\MyWindows.h"
#include <string>
#include <format>
#include <cstdio>

using namespace std;

// Some cases in the wild (e.g. an unexpected/corrupt DICOM layout) crash deep inside the
// decode/imaging pipeline rather than failing cleanly - deliberately no local C++ objects here
// (SEH requires that) so one bad case can be caught and skipped without taking the whole run
// down with it. Mirrors CBatchScorer.cpp's own SafeLoadAndScore.
static bool SafeLoadAndScore(CIQVManager* pManager, const char* zFileName, int iCaseIndex)
{
	__try
	{
		return pManager->LoadAndScore(zFileName, iCaseIndex);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
}
COptimizer::COptimizer()
{
}
COptimizer::~COptimizer()
{
}
int COptimizer::RunOnTrainingSet(const char* zRootDir)
{
	mvResults.clear();

	CString sRoot(zRootDir);
	while (!sRoot.IsEmpty() && (sRoot.Right(1) == "\\" || sRoot.Right(1) == "/"))
		sRoot = sRoot.Left(sRoot.GetLength() - 1);

	RunOnLabelDir(sRoot + "\\Pass", "Pass");
	RunOnLabelDir(sRoot + "\\Fail", "Fail");

	WriteReport();

	return (int)mvResults.size();
}
void COptimizer::RunOnLabelDir(const char* zLabelDir, const char* zLabel)
{
	if (!CMyWindows::IsDirectory(zLabelDir))
		return;

	CFilesList list;
	int nFound = CMyWindows::ListSampleFilesInDirTree(zLabelDir, gConfig.msDicomFilePattern.c_str(), list);
	if (nFound < 1)
		return;

	CBatchScorer::ScreenNonImageSets(list);
	if (list.N() < 1)
		return;

	// Same nesting convention as CBatchScorer::RunOnDirTree - separate batch root per label, so
	// a same-named case under both Pass and Fail can't collide in the log tree, and case names
	// come out relative to the label directory (the label itself is already its own report column).
	string sBatchRoot(format("Optimize_{}", zLabel));
	gConfig.msBatchRootDir = sBatchRoot;
	gConfig.msBatchScanRootPath = zLabelDir;

	int nTotal = list.N();
	int iCase = 0;
	POSITION pos = list.GetHeadPosition();
	while (pos)
	{
		iCase++;
		string sStatus(format("Scoring training data ({}): case {}/{}", zLabel, iCase, nTotal));
		gConfig.PrintStatus(sStatus.c_str());

		CString* psfName = list.GetNext(pos);

		CIQVManager manager;
		if (!SafeLoadAndScore(&manager, *psfName, iCase))
		{
			gConfig.PrintStatus("Case data could not be loaded - skipped without saving");
			continue;
		}

		SCaseResult result;
		result.sLabel = zLabel;

		// gConfig.msCaseLogDir was just composed (via LoadImages(), same junction-aware naming
		// batch scoring uses) as "<sBatchRoot>\<case name>[_<index>]" - strip the batch-root
		// prefix back off to get just the case's own name for the report.
		CString sRelative(gConfig.GetCaseRelativeLogDir().c_str());
		CString sPrefix(CString(sBatchRoot.c_str()) + "\\");
		result.sCaseName = (sRelative.Left(sPrefix.GetLength()) == sPrefix)
			? sRelative.Mid(sPrefix.GetLength())
			: sRelative;

		result.score = manager.GetRingsScorer()->GetWorstScore(gConfig.mScoreType);
		result.bScoredPass = gConfig.IsPass(result.score);
		result.gap = result.score - gConfig.mMaxAcceptableScore;

		bool bTrueLabelPass = (result.sLabel == "Pass");
		if (bTrueLabelPass)
			result.sAssessment = result.bScoredPass ? "Correct Pass" : "False Positive";
		else
			result.sAssessment = result.bScoredPass ? "False Negative" : "Correct Fail";

		mvResults.push_back(result);
	}

	gConfig.msBatchRootDir.clear();
	gConfig.msBatchScanRootPath.clear();
}
void COptimizer::WriteReport()
{
	string sfName(format("{}\\TrainingSetReport_{}.csv", gConfig.msLogRoot, ScoreTypeName(gConfig.mScoreType)));
	msReportName = sfName.c_str();

	FILE* pf = nullptr;
	fopen_s(&pf, sfName.c_str(), "w");
	if (!pf)
		return;

	fprintf(pf, "label, case, score, verdict, gap, assessment\n");
	for (const SCaseResult& r : mvResults)
		fprintf(pf, "%s, %s, %.2f, %s, %.2f, %s\n",
			(LPCTSTR)r.sLabel, (LPCTSTR)r.sCaseName, r.score,
			r.bScoredPass ? "Pass" : "Fail", r.gap, (LPCTSTR)r.sAssessment);
	fclose(pf);
}
