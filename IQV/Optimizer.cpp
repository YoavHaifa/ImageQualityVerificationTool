#include "stdafx.h"
#include "Optimizer.h"
#include "BatchScorer.h"
#include "IQVManager.h"
#include "RingsScorer.h"
#include "ImageScore.h"
#include "Config.h"
#include "..\..\yUtils\FilesList.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\yUtils\FileName.h"
#include <string>
#include <format>
#include <cstdio>
#include <cfloat>

using namespace std;

// "<dir>\Name.csv" + "_before_optimize" -> "<dir>\Name_before_optimize.csv" (appended at the end
// if there's no extension to insert before).
static CString InsertBeforeExtension(const CString& sPath, const char* zSuffix)
{
	int iDot = sPath.ReverseFind('.');
	if (iDot < 0)
		return sPath + zSuffix;
	return sPath.Left(iDot) + zSuffix + sPath.Mid(iDot);
}

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

	msReportDir = (gConfig.msLogRoot + "\\TrainingSetReport").c_str();
	CMyWindows::VerifyDirectory(msReportDir);

	CString sRoot(zRootDir);
	while (!sRoot.IsEmpty() && (sRoot.Right(1) == "\\" || sRoot.Right(1) == "/"))
		sRoot = sRoot.Left(sRoot.GetLength() - 1);

	RunOnLabelDir(sRoot + "\\Pass", "Pass");
	RunOnLabelDir(sRoot + "\\Fail", "Fail");

	WriteReports();

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

	// Wipe this run's own result sub-tree first - the training set (and which cases are in it)
	// can change between runs, so a stale case folder left over from an earlier run must not
	// linger and be mistaken for a current result.
	string sBatchLogDir(gConfig.msLogRoot + "\\" + sBatchRoot);
	CMyWindows::DeleteDirWithFiles(sBatchLogDir.c_str());

	gConfig.msBatchRootDir = sBatchRoot;
	gConfig.msBatchScanRootPath = zLabelDir;

	int nTotal = list.N();
	int iCase = 0;
	POSITION pos = list.GetHeadPosition();
	while (pos)
	{
		iCase++;
		string sStatus(format("Scoring training data ({}): case {}/{}", zLabel, iCase, nTotal));
		CBatchScorer::MyPrintStatus(sStatus.c_str());

		CString* psfName = list.GetNext(pos);

		CIQVManager manager;
		if (!SafeLoadAndScore(&manager, *psfName, iCase))
		{
			CBatchScorer::MyPrintStatus("Case data could not be loaded - skipped without saving");
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

		CRingsScorer* pRingsScorer = manager.GetRingsScorer();
		result.mainAreaWidth = pRingsScorer->GetMainAreaWidth();

		bool bTrueLabelPass = (result.sLabel == "Pass");
		result.vPerType.assign((int)EScoreType::N_SCORE_TYPES, SPerTypeResult());
		for (int iType = 0; iType < (int)EScoreType::N_SCORE_TYPES; iType++)
		{
			EScoreType type = (EScoreType)iType;
			const CImageScore& winner = pRingsScorer->GetScoreAtMax(type);
			SPerTypeResult& pt = result.vPerType[iType];

			pt.score = winner.mScore;
			pt.bScoredPass = gConfig.IsPass(pt.score);
			pt.gap = pt.score - gConfig.mMaxAcceptableScore;
			pt.ring = winner.miRing;
			pt.originalImage = winner.miOriginalImage;

			// AllMax stamps meSourceType with whichever sibling actually produced its winning
			// score; every other scorer leaves it at N_SCORE_TYPES, meaning it's its own source.
			EScoreType eCriticalScorer = (winner.meSourceType != EScoreType::N_SCORE_TYPES)
				? winner.meSourceType : type;
			pt.sCriticalScorer = ScoreTypeName(eCriticalScorer);
			pt.criticalRawScore = pRingsScorer->GetRawScoreAt(eCriticalScorer, winner.miOriginalImage);

			if (bTrueLabelPass)
				pt.sAssessment = pt.bScoredPass ? "Correct Pass" : "False Positive";
			else
				pt.sAssessment = pt.bScoredPass ? "False Negative" : "Correct Fail";
		}

		mvResults.push_back(result);
	}

	gConfig.msBatchRootDir.clear();
	gConfig.msBatchScanRootPath.clear();
}
void COptimizer::WriteReports()
{
	for (int iType = 0; iType < (int)EScoreType::N_SCORE_TYPES; iType++)
	{
		EScoreType type = (EScoreType)iType;
		string sfName(format("{}\\TrainingSetReport_{}.csv", (LPCTSTR)msReportDir, ScoreTypeName(type)));

		FILE* pf = nullptr;
		fopen_s(&pf, sfName.c_str(), "w");
		if (!pf)
		{
			// Most likely cause: this exact file is still open in Excel from an earlier "Open it
			// now?" - silently skipping would leave a stale report on disk with no indication
			// the fresh results were never written.
			CMyWindows::MessBox(format("Failed to write {} - is it open in Excel or another program? "
				"Results for this scorer were NOT updated - the file on disk is stale.", sfName).c_str(),
				"Score Training Data");
			continue;
		}

		fprintf(pf, "label, case, main area width, image, ring, critical scorer, critical raw score, score, verdict, gap, assessment\n");
		for (const SCaseResult& r : mvResults)
		{
			const SPerTypeResult& pt = r.vPerType[iType];
			fprintf(pf, "%s, %s, %d, %d, %d, %s, %.6f, %.6f, %s, %.6f, %s\n",
				(LPCTSTR)r.sLabel, (LPCTSTR)r.sCaseName, r.mainAreaWidth,
				pt.originalImage, pt.ring, (LPCTSTR)pt.sCriticalScorer, pt.criticalRawScore,
				pt.score, pt.bScoredPass ? "Pass" : "Fail", pt.gap, (LPCTSTR)pt.sAssessment);
		}
		fclose(pf);
	}
}
int COptimizer::OptimizeWeights(const char* zRootDir)
{
	// Pass 1: score everything with today's weights - both the "before" baseline and the source
	// data new weights are computed from (no need to rescore for that - see SCaseResult::vPerType).
	RunOnTrainingSet(zRootDir);

	// Preserve the before-pass reports and weights before either gets overwritten below.
	for (int iType = 0; iType < (int)EScoreType::N_SCORE_TYPES; iType++)
	{
		CString sReport(format("{}\\TrainingSetReport_{}.csv", (LPCTSTR)msReportDir, ScoreTypeName((EScoreType)iType)).c_str());
		CopyFile(sReport, InsertBeforeExtension(sReport, "_before_optimize"), FALSE);
	}

	CString sWeightsFile(gConfig.GetScorerWeightsFileName().c_str());
	CString sWeightsBackup(msReportDir + "\\" + CFileName::GetLastInPath(sWeightsFile));
	sWeightsBackup = InsertBeforeExtension(sWeightsBackup, "_before_optimize");
	CopyFile(sWeightsFile, sWeightsBackup, FALSE);

	vector<SWeightResult> weightResults;
	ComputeAndApplyNewWeights(weightResults);
	WriteWeightsReport(weightResults);

	// Pass 2: rescore everything with the new weights now active, so the reports reflect them.
	return RunOnTrainingSet(zRootDir);
}
void COptimizer::ComputeAndApplyNewWeights(vector<SWeightResult>& results)
{
	for (int iType = 0; iType < (int)EScoreType::N_SCORE_TYPES; iType++)
	{
		EScoreType type = (EScoreType)iType;
		if (type == EScoreType::AllMax)
			continue; // AllMax's own weight is fixed at 1.0, built from siblings - not tunable

		SWeightResult wr;
		wr.sScorer = ScoreTypeName(type);
		wr.oldWeight = gConfig.GetScorerWeight(type);
		wr.newWeight = wr.oldWeight;

		float passMax = -FLT_MAX;
		bool bAnyPass = false;
		float failMinOverall = FLT_MAX;
		bool bAnyFail = false;
		for (const SCaseResult& r : mvResults)
		{
			float score = r.vPerType[iType].score;
			if (r.sLabel == "Pass")
			{
				if (!bAnyPass || score > passMax)
					passMax = score;
				bAnyPass = true;
			}
			else
			{
				if (!bAnyFail || score < failMinOverall)
					failMinOverall = score;
				bAnyFail = true;
			}
		}

		if (!bAnyPass || !bAnyFail)
		{
			wr.bHasData = false;
			results.push_back(wr);
			continue;
		}
		wr.passMax = passMax;

		float failMinAbove = FLT_MAX;
		bool bFoundAbove = false;
		for (const SCaseResult& r : mvResults)
		{
			if (r.sLabel != "Fail")
				continue;
			float score = r.vPerType[iType].score;
			if (score > passMax && score < failMinAbove)
			{
				failMinAbove = score;
				bFoundAbove = true;
			}
		}

		float target;
		if (bFoundAbove)
		{
			wr.bSeparated = true;
			wr.failTarget = failMinAbove;
			target = (passMax + failMinAbove) / 2.0f;
		}
		else
		{
			// No Fail case scores above even the worst Pass case - every Fail case this scorer
			// could catch would also mean failing that Pass case, and a false positive is never
			// acceptable (see the normal case above). So instead of catching anything, lower the
			// weight just enough that even the worst Pass case (the overall max here, since no
			// Fail scored higher) stays at/below threshold - this scorer flags nothing as Fail
			// rather than risk a false positive.
			wr.bSeparated = false;
			wr.failTarget = failMinOverall; // logged for visibility only - not the target used below
			target = passMax;
		}

		if (target > 0.0001f)
			wr.newWeight = wr.oldWeight * (gConfig.mMaxAcceptableScore / target);

		gConfig.SetScorerWeight(type, wr.newWeight);
		results.push_back(wr);
	}

	gConfig.SaveScorerWeights();
}
void COptimizer::WriteWeightsReport(const vector<SWeightResult>& results)
{
	string sfName(format("{}\\WeightOptimization.csv", (LPCTSTR)msReportDir));
	msWeightsReportName = sfName.c_str();

	FILE* pf = nullptr;
	fopen_s(&pf, sfName.c_str(), "w");
	if (!pf)
	{
		CMyWindows::MessBox(format("Failed to write {} - is it open in Excel or another program?",
			sfName).c_str(), "Optimize Scorer Weights");
		return;
	}

	fprintf(pf, "scorer, old weight, new weight, pass max, fail target, separated, note\n");
	for (const SWeightResult& wr : results)
	{
		if (!wr.bHasData)
		{
			fprintf(pf, "%s, %.6f, %.6f, , , , no data - unchanged\n",
				(LPCTSTR)wr.sScorer, wr.oldWeight, wr.newWeight);
			continue;
		}
		fprintf(pf, "%s, %.6f, %.6f, %.6f, %.6f, %s, %s\n",
			(LPCTSTR)wr.sScorer, wr.oldWeight, wr.newWeight, wr.passMax, wr.failTarget,
			wr.bSeparated ? "yes" : "no",
			wr.bSeparated ? "" : "no separation - weight lowered so nothing fails");
	}
	fclose(pf);
}
