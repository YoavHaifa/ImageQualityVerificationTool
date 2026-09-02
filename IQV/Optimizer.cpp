#include "stdafx.h"
#include "Optimizer.h"
#include "BatchScorer.h"
#include "IQVManager.h"
#include "RingsScorer.h"
#include "ImageScore.h"
#include "Config.h"
#include "..\..\yUtils\FilesList.h"
#include "..\..\yUtils\MyWindows.h"
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
		const CImageScore& winner = pRingsScorer->GetScoreAtMax(gConfig.mScoreType);
		result.score = winner.mScore;
		result.bScoredPass = gConfig.IsPass(result.score);
		result.gap = result.score - gConfig.mMaxAcceptableScore;

		// AllMax stamps meSourceType with whichever sibling actually produced its winning score;
		// every other scorer leaves it at N_SCORE_TYPES, meaning it's its own (only) source.
		EScoreType eCriticalScorer = (winner.meSourceType != EScoreType::N_SCORE_TYPES)
			? winner.meSourceType : gConfig.mScoreType;
		result.sCriticalScorer = ScoreTypeName(eCriticalScorer);
		result.criticalRawScore = pRingsScorer->GetRawScoreAt(eCriticalScorer, winner.miOriginalImage);

		// Every individual scorer's own final score for this case - cheap (already computed),
		// and lets OptimizeWeights() compute new weights later without rescoring.
		result.vScorerScores.assign((int)EScoreType::N_SCORE_TYPES, 0.0f);
		for (int iType = 0; iType < (int)EScoreType::N_SCORE_TYPES; iType++)
			result.vScorerScores[iType] = pRingsScorer->GetWorstScore((EScoreType)iType);

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

	fprintf(pf, "label, case, critical scorer, critical raw score, score, verdict, gap, assessment\n");
	for (const SCaseResult& r : mvResults)
		fprintf(pf, "%s, %s, %s, %.2f, %.2f, %s, %.2f, %s\n",
			(LPCTSTR)r.sLabel, (LPCTSTR)r.sCaseName,
			(LPCTSTR)r.sCriticalScorer, r.criticalRawScore,
			r.score, r.bScoredPass ? "Pass" : "Fail", r.gap, (LPCTSTR)r.sAssessment);
	fclose(pf);
}
int COptimizer::OptimizeWeights(const char* zRootDir)
{
	// Pass 1: score everything with today's weights - both the "before" baseline and the source
	// data new weights are computed from (no need to rescore for that - see vScorerScores).
	RunOnTrainingSet(zRootDir);

	// Preserve the before-pass results and weights before either gets overwritten below.
	CString sBeforeReport(InsertBeforeExtension(msReportName, "_before_optimize"));
	CopyFile(msReportName, sBeforeReport, FALSE);

	CString sWeightsFile(gConfig.GetScorerWeightsFileName().c_str());
	CString sWeightsBackup(InsertBeforeExtension(sWeightsFile, "_before_optimize"));
	CopyFile(sWeightsFile, sWeightsBackup, FALSE);

	vector<SWeightResult> weightResults;
	ComputeAndApplyNewWeights(weightResults);
	WriteWeightsReport(weightResults);

	// Pass 2: rescore everything with the new weights now active, so the report reflects them.
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
			float score = r.vScorerScores[iType];
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
			float score = r.vScorerScores[iType];
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
	string sfName(format("{}\\WeightOptimization.csv", gConfig.msLogRoot));
	msWeightsReportName = sfName.c_str();

	FILE* pf = nullptr;
	fopen_s(&pf, sfName.c_str(), "w");
	if (!pf)
		return;

	fprintf(pf, "scorer, old weight, new weight, pass max, fail target, separated, note\n");
	for (const SWeightResult& wr : results)
	{
		if (!wr.bHasData)
		{
			fprintf(pf, "%s, %.2f, %.2f, , , , no data - unchanged\n",
				(LPCTSTR)wr.sScorer, wr.oldWeight, wr.newWeight);
			continue;
		}
		fprintf(pf, "%s, %.2f, %.2f, %.2f, %.2f, %s, %s\n",
			(LPCTSTR)wr.sScorer, wr.oldWeight, wr.newWeight, wr.passMax, wr.failTarget,
			wr.bSeparated ? "yes" : "no",
			wr.bSeparated ? "" : "no separation - weight lowered so nothing fails");
	}
	fclose(pf);
}
