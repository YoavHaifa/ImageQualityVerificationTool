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
#include "..\..\yUtils\YamlParser.h"
#include <string>
#include <format>
#include <cstdio>
#include <cfloat>

using namespace std;

// The 3 "ring scorers" and the 3 regions each of them now tracks independently - shared by
// RunOnLabelDir() (populates SCaseResult::vPerRegion) and ComputeAndApplyNewWeights() (reads it),
// so both agree on the same iRingType*3+iRegion indexing.
static const EScoreType gaOptimizerRingTypes[3] = { EScoreType::MinMax, EScoreType::Tent, EScoreType::TentMin };
static const ERegion gaOptimizerRegions[3] = { ERegion::HighRes, ERegion::Border, ERegion::LowRes };

// Reads one flat "key: true"/"key: false" line from a case's CaseLabelInfo.yaml - false (not
// found, or not "true") if the key is missing entirely, which is exactly right for a Pass-labeled
// case (no CaseLabelInfo.yaml region flags ever get set there) as well as for an old case labeled
// before these flags existed at all.
static bool ReadYamlBool(CYamlLine* pRoot, const char* zKey)
{
	CString sVal;
	if (!pRoot->GetValue(zKey, sVal))
		return false;
	return sVal.CompareNoCase("true") == 0;
}

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

	CFilesList subDirs;
	CMyWindows::ListSubDirsInDir(sRoot, subDirs);

	POSITION pos = subDirs.GetHeadPosition();
	while (pos)
	{
		CString* psSubDir = subDirs.GetNext(pos);
		CString sSubDirName(CFileName::GetLastInPath(*psSubDir));

		CString sLabel = DetermineLabel(sSubDirName);
		if (sLabel.IsEmpty())
		{
			CBatchScorer::MyPrintStatus(format("Skipping \"{}\" - name doesn't start with pass or fail",
				(LPCTSTR)sSubDirName).c_str());
			continue;
		}

		RunOnLabelDir(*psSubDir, sLabel, sSubDirName);
	}

	WriteReports();

	return (int)mvResults.size();
}
CString COptimizer::DetermineLabel(const CString& sSubDirName)
{
	// Back to just two labels - which region(s) of a failed case actually show the problem now
	// lives in that case's own CaseLabelInfo.yaml (see CIQVDlg::SaveLabeledData), not the
	// sub-directory name, so any "fail..." name (old fail_center/fail_ring/fail_both
	// sub-directories included) is just "Fail" here.
	if (sSubDirName.Left(4).CompareNoCase("pass") == 0)
		return "Pass";
	if (sSubDirName.Left(4).CompareNoCase("fail") == 0)
		return "Fail";
	return CString();
}
bool COptimizer::IsRegionExpectedToFail(const SCaseResult& r, ERegion region)
{
	if (r.sLabel.CompareNoCase("Fail") != 0)
		return false;
	switch (region)
	{
	case ERegion::Center: return r.bFailedCenter;
	case ERegion::HighRes: return r.bFailedHR;
	case ERegion::Border: return r.bFailedBorder;
	case ERegion::LowRes: return r.bFailedLR;
	default: return false;
	}
}
bool COptimizer::IsAnyRingRegionExpectedToFail(const SCaseResult& r)
{
	if (r.sLabel.CompareNoCase("Fail") != 0)
		return false;
	return r.bFailedHR || r.bFailedBorder || r.bFailedLR;
}
void COptimizer::RunOnLabelDir(const char* zSubDir, const char* zLabel, const CString& sSubDirName)
{
	if (!CMyWindows::IsDirectory(zSubDir))
		return;

	CFilesList list;
	int nFound = CMyWindows::ListSampleFilesInDirTree(zSubDir, gConfig.msDicomFilePattern.c_str(), list);
	if (nFound < 1)
		return;

	CBatchScorer::ScreenNonImageSets(list);
	if (list.N() < 1)
		return;

	// Same nesting convention as CBatchScorer::RunOnDirTree - separate batch root per source
	// sub-directory (not per label - several sub-directories can share the same label), so a
	// same-named case under two different sub-directories can't collide in the log tree, and case
	// names come out relative to the sub-directory itself (the label is already its own report column).
	string sBatchRoot(format("Optimize_{}", (LPCTSTR)sSubDirName));

	// Wipe this run's own result sub-tree first - the training set (and which cases are in it)
	// can change between runs, so a stale case folder left over from an earlier run must not
	// linger and be mistaken for a current result.
	string sBatchLogDir(gConfig.msLogRoot + "\\" + sBatchRoot);
	CMyWindows::DeleteDirWithFiles(sBatchLogDir.c_str());

	gConfig.msBatchRootDir = sBatchRoot;
	gConfig.msBatchScanRootPath = zSubDir;

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

		// CaseLabelInfo.yaml lives right alongside the copied DICOM files (see
		// CIQVDlg::SaveLabeledData) - i.e. in the same directory as psfName, one of this case's own
		// sample files. Missing entirely (an old case labeled before this file existed) just leaves
		// every bFailed* flag at its default false - same as a genuine Pass case.
		CString sYamlName(CFileName::GetPath(*psfName) + "\\CaseLabelInfo.yaml");
		CYamlParser labelInfo;
		if (labelInfo.Parse(sYamlName))
		{
			CYamlLine* pRoot = labelInfo.GetRoot();
			result.bFailedCenter = ReadYamlBool(pRoot, "failed_center");
			result.bFailedHR = ReadYamlBool(pRoot, "failed_hr");
			result.bFailedBorder = ReadYamlBool(pRoot, "failed_border");
			result.bFailedLR = ReadYamlBool(pRoot, "failed_lr");
		}

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

			// Not every scorer is expected to catch every kind of labeled failure - a case whose
			// labeled region(s) this scorer isn't responsible for (e.g. a case flagged only in
			// LowRes, as far as the Center scorer is concerned) is judged exactly like an actual
			// Pass case here. AllMax is exempt from this narrowing - it's meant to catch every
			// failure type, since it's built from every sibling's own score.
			bool bExpectedFail = (type == EScoreType::AllMax) ? (result.sLabel.CompareNoCase("Fail") == 0)
				: (type == EScoreType::Center) ? IsRegionExpectedToFail(result, ERegion::Center)
				: IsAnyRingRegionExpectedToFail(result);
			pt.sExpectedVerdict = bExpectedFail ? "Fail" : "Pass";
			if (!bExpectedFail)
				pt.sAssessment = pt.bScoredPass ? "Correct Pass" : "False Positive";
			else
				pt.sAssessment = pt.bScoredPass ? "False Negative" : "Correct Fail";
		}

		// Same as above, but per individual region of each ring scorer, rather than merged across
		// its 3 regions - lets ComputeAndApplyNewWeights() tune each region's weight independently,
		// from that region's own score distribution, without a second scoring pass.
		result.vPerRegion.assign(9, SPerTypeResult());
		for (int iRingType = 0; iRingType < 3; iRingType++)
		{
			EScoreType type = gaOptimizerRingTypes[iRingType];
			for (int iRegion = 0; iRegion < 3; iRegion++)
			{
				ERegion region = gaOptimizerRegions[iRegion];
				const CImageScore& winner = pRingsScorer->GetScoreAtMax(type, region);
				SPerTypeResult& pt = result.vPerRegion[iRingType * 3 + iRegion];

				pt.score = winner.mScore;
				pt.bScoredPass = gConfig.IsPass(pt.score);
				pt.gap = pt.score - gConfig.mMaxAcceptableScore;
				pt.ring = winner.miRing;
				pt.originalImage = winner.miOriginalImage;

				// A single region has no AllMax-style sibling redirection - it's always its own
				// critical scorer, and its own winner.mRawScore is already that region's true
				// (unweighted) raw score, with no need for a separate GetRawScoreAt lookup.
				pt.sCriticalScorer = ScoreTypeName(type);
				pt.criticalRawScore = winner.mRawScore;

				bool bExpectedFail = IsRegionExpectedToFail(result, region);
				pt.sExpectedVerdict = bExpectedFail ? "Fail" : "Pass";
				if (!bExpectedFail)
					pt.sAssessment = pt.bScoredPass ? "Correct Pass" : "False Positive";
				else
					pt.sAssessment = pt.bScoredPass ? "False Negative" : "Correct Fail";
			}
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

		fprintf(pf, "label, expected, case, main area width, image, ring, critical scorer, critical raw score, score, verdict, gap, assessment\n");
		for (const SCaseResult& r : mvResults)
		{
			const SPerTypeResult& pt = r.vPerType[iType];
			fprintf(pf, "%s, %s, %s, %d, %d, %d, %s, %.6f, %.6f, %s, %.6f, %s\n",
				(LPCTSTR)r.sLabel, (LPCTSTR)pt.sExpectedVerdict, (LPCTSTR)r.sCaseName, r.mainAreaWidth,
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
COptimizer::SWeightResult COptimizer::ComputeOneWeight(const CString& sName, float oldWeight,
	const std::function<float(const SCaseResult&)>& getScore,
	const std::function<bool(const SCaseResult&)>& isExpectedToFail) const
{
	SWeightResult wr;
	wr.sScorer = sName;
	wr.oldWeight = oldWeight;
	wr.newWeight = oldWeight;

	// A case counts toward this scorer's "Pass" cohort (must never trip it) unless it's a failure
	// this scorer is actually expected to catch - see isExpectedToFail. E.g. a case labeled Fail
	// only in LowRes belongs in the Center scorer's "Pass" cohort, same as a real Pass case, since
	// Center isn't responsible for problems outside its own region.
	float passMax = -FLT_MAX;
	bool bAnyPass = false;
	float failMinOverall = FLT_MAX;
	bool bAnyFail = false;
	for (const SCaseResult& r : mvResults)
	{
		float score = getScore(r);
		if (!isExpectedToFail(r))
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
		return wr;
	}
	wr.passMax = passMax;

	float failMinAbove = FLT_MAX;
	bool bFoundAbove = false;
	for (const SCaseResult& r : mvResults)
	{
		if (!isExpectedToFail(r))
			continue;
		float score = getScore(r);
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
		wr.newWeight = oldWeight * (gConfig.mMaxAcceptableScore / target);

	return wr;
}
void COptimizer::ComputeAndApplyNewWeights(vector<SWeightResult>& results)
{
	// The 3 ring scorers (MinMax/Tent/TentMin) each get 3 independent weights now - one per region
	// (HighRes/Border/LowRes) - tuned from that region's own score distribution (SCaseResult::
	// vPerRegion) AND that region's own cohort (a case counts as "expected to fail" here only if
	// its own CaseLabelInfo.yaml flagged THIS region specifically, not just "Fail" overall).
	for (int iRingType = 0; iRingType < 3; iRingType++)
	{
		EScoreType type = gaOptimizerRingTypes[iRingType];
		for (int iRegion = 0; iRegion < 3; iRegion++)
		{
			ERegion region = gaOptimizerRegions[iRegion];
			int iSlot = iRingType * 3 + iRegion;

			CString sName;
			sName.Format("%s_%s", ScoreTypeName(type), RegionName(region));

			SWeightResult wr = ComputeOneWeight(sName, gConfig.GetScorerWeight(type, region),
				[iSlot](const SCaseResult& r) { return r.vPerRegion[iSlot].score; },
				[region](const SCaseResult& r) { return IsRegionExpectedToFail(r, region); });

			gConfig.SetScorerWeight(type, region, wr.newWeight);
			results.push_back(wr);
		}
	}

	// Center has no regions - one weight, same algorithm, cohort keyed off its own bFailedCenter flag.
	{
		EScoreType type = EScoreType::Center;
		SWeightResult wr = ComputeOneWeight(ScoreTypeName(type), gConfig.GetScorerWeight(type),
			[](const SCaseResult& r) { return r.vPerType[(int)EScoreType::Center].score; },
			[](const SCaseResult& r) { return IsRegionExpectedToFail(r, ERegion::Center); });

		gConfig.SetScorerWeight(type, wr.newWeight);
		results.push_back(wr);
	}

	// AllMax's own weight is fixed at 1.0, built from siblings - not tunable, so it never appears
	// in `results` at all (unlike before, where the type loop skipped it explicitly).

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
