#include "stdafx.h"
#include "Config.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\yUtils\FileName.h"
#include "..\..\yUtils\XmlParse.h"
#include "..\..\yUtils\XmlDump.h"

using namespace std;

CConfig gConfig;
CFileLogger gfLog;

CConfig::CConfig()
{
}
void CConfig::Init()
{
	ComputeConfigDir();
	ReadFromFile();
	CMyWindows::VerifyDirectory(msLogRoot.c_str());
	CMyWindows::VerifyDirectory("d:\\MyLog");
	CMyWindows::VerifyDirectory("d:\\MyLog\\Images");
	VerifyTrainingSetRoot();
	if (mDebug)
	{
		gfLog.Init("IQV_App");
		gfLog.Log("<CConfig::Init()> version", msVersion);
	}
	LoadScorerWeights();
}
void CConfig::VerifyTrainingSetRoot()
{
	if (msTrainingSetRoot.size() < 3 || msTrainingSetRoot[1] != ':')
	{
		CMyWindows::VerifyDirectory(msTrainingSetRoot.c_str()); // not a drive-letter path - best effort
		return;
	}

	CString sDriveRoot(msTrainingSetRoot.substr(0, 3).c_str()); // e.g. "d:\"
	CString sRest(msTrainingSetRoot.substr(3).c_str());
	CString sOutPath;
	CMyWindows::VerifyDirectoryPath(sDriveRoot, sRest, sOutPath);
}
void CConfig::ComputeConfigDir()
{
	CString sAppPath(CMyWindows::GetApplicationPath());

	// Strip a trailing separator so the last path component can be found reliably
	while (sAppPath.GetLength() > 0 && (sAppPath.Right(1) == "\\" || sAppPath.Right(1) == "/"))
		sAppPath = sAppPath.Left(sAppPath.GetLength() - 1);

	msConfigDir = (LPCTSTR)sAppPath;

	int iSep = sAppPath.ReverseFind('\\');
	CString sLastDir = (iSep >= 0) ? sAppPath.Mid(iSep + 1) : sAppPath;
	if (iSep >= 0 && sLastDir.CompareNoCase("Debug") == 0)
	{
		CString sReleaseDir(sAppPath.Left(iSep) + "\\Release");
		if (CMyWindows::IsDirectory(sReleaseDir))
			msConfigDir = (LPCTSTR)sReleaseDir;
	}
}
// HighRes/Border/LowRes -> 0/1/2 within a scorer type's 3-slot row (see mvScorerWeights).
// Center isn't stored here at all - CImageRingsScorer::GetWeightForRing leaves a ring scorer's
// Center-region rings unweighted, so this fallback (same slot as HighRes) is never actually read for Center.
static int RegionSlotIndex(ERegion region)
{
	switch (region)
	{
	case ERegion::Border: return 1;
	case ERegion::LowRes: return 2;
	default: return 0; // HighRes, and the Center fallback described above
	}
}
void CConfig::LoadScorerWeights()
{
	mvScorerWeights.assign((int)EScoreType::N_SCORE_TYPES * 3, 1.0f);

	string sfName = GetScorerWeightsFileName();
	if (!CFileName::Exist(sfName.c_str()))
	{
		SaveScorerWeights();
		return;
	}

	FILE* pf = nullptr;
	fopen_s(&pf, sfName.c_str(), "r");
	if (!pf)
		return;

	char zLine[128];
	fgets(zLine, sizeof(zLine), pf); // header
	bool bMigratedOldFormat = false;
	while (fgets(zLine, sizeof(zLine), pf))
	{
		int iCode;
		char zName[64];
		float wHighRes, wBorder, wLowRes;
		int nParsed = sscanf_s(zLine, "%d, %63[^,], %f, %f, %f", &iCode, zName, (unsigned)sizeof(zName),
			&wHighRes, &wBorder, &wLowRes);
		if (iCode < 0 || iCode >= (int)EScoreType::N_SCORE_TYPES)
			continue;

		if (nParsed == 5)
		{
			mvScorerWeights[iCode * 3 + 0] = wHighRes;
			mvScorerWeights[iCode * 3 + 1] = wBorder;
			mvScorerWeights[iCode * 3 + 2] = wLowRes;
		}
		else if (nParsed == 3)
		{
			// Old single-weight-column format (from before per-region weights) - migrate forward
			// by starting all 3 regions at whatever value was already tuned there
			SetScorerWeight((EScoreType)iCode, wHighRes);
			bMigratedOldFormat = true;
		}
		else
		{
			gfLog.Printf("<CConfig::LoadScorerWeights> Unrecognized line (parsed %d of 5 fields): %s", nParsed, zLine);
		}
	}
	fclose(pf);

	// Persist the migration immediately, rather than leaving the on-disk file in the old format
	// (and re-migrating it in memory) until the next time something else happens to call
	// SaveScorerWeights() (e.g. Optimize Scorer Weights) - SaveScorerWeights() always writes the
	// current 5-column format, so this brings the file itself up to date right away.
	if (bMigratedOldFormat)
		SaveScorerWeights();
}
void CConfig::SaveScorerWeights() const
{
	string sfName = GetScorerWeightsFileName();
	FILE* pfOut = nullptr;
	fopen_s(&pfOut, sfName.c_str(), "w");
	if (!pfOut)
		return;

	fprintf(pfOut, "code, name, weight_highres, weight_border, weight_lowres\n");
	for (int i = 0; i < (int)EScoreType::N_SCORE_TYPES; i++)
		fprintf(pfOut, "%d, %s, %.6f, %.6f, %.6f\n", i, ScoreTypeName((EScoreType)i),
			mvScorerWeights[i * 3 + 0], mvScorerWeights[i * 3 + 1], mvScorerWeights[i * 3 + 2]);
	fclose(pfOut);
}
void CConfig::SetScorerWeight(EScoreType type, float weight)
{
	int i = (int)type;
	if (i < 0 || i >= (int)EScoreType::N_SCORE_TYPES)
		return;
	mvScorerWeights[i * 3 + 0] = weight;
	mvScorerWeights[i * 3 + 1] = weight;
	mvScorerWeights[i * 3 + 2] = weight;
}
void CConfig::SetScorerWeight(EScoreType type, ERegion region, float weight)
{
	int i = (int)type;
	if (i < 0 || i >= (int)EScoreType::N_SCORE_TYPES)
		return;
	mvScorerWeights[i * 3 + RegionSlotIndex(region)] = weight;
}
float CConfig::ComputeCertaintyFraction(float score) const
{
	const float minFractionAtThreshold = 0.2f;
	float half = mMaxAcceptableScore / 2.0f;
	float oneAndHalf = mMaxAcceptableScore * 1.5f;

	if (score <= mMaxAcceptableScore)
	{
		// Pass side: 1.0 at/below half the threshold, shrinking to the floor right at the threshold
		if (score <= half)
			return 1.0f;
		float t = (score - half) / (mMaxAcceptableScore - half);
		return 1.0f - t * (1.0f - minFractionAtThreshold);
	}
	// Fail side: mirror image - floor right above the threshold, growing to 1.0 by 1.5x it
	if (score >= oneAndHalf)
		return 1.0f;
	float t = (score - mMaxAcceptableScore) / (oneAndHalf - mMaxAcceptableScore);
	return minFractionAtThreshold + t * (1.0f - minFractionAtThreshold);
}
float CConfig::GetScorerWeight(EScoreType type) const
{
	return GetScorerWeight(type, ERegion::HighRes);
}
float CConfig::GetScorerWeight(EScoreType type, ERegion region) const
{
	int i = (int)type;
	if (i < 0 || i >= (int)EScoreType::N_SCORE_TYPES)
		return 1.0f;
	return mvScorerWeights[i * 3 + RegionSlotIndex(region)];
}
ERegion CConfig::ClassifyRing(int iRing) const
{
	if (iRing < mnCentralRings)
		return ERegion::Center;
	if (iRing <= miLastHighResolutionRing)
		return ERegion::HighRes;
	if (iRing < miFirstLowResolutionRing)
		return ERegion::Border;
	return ERegion::LowRes;
}
bool CConfig::IsRegionEnabled(ERegion region) const
{
	switch (region)
	{
	case ERegion::Center: return mbReviewCenter;
	case ERegion::HighRes: return mbReviewHighRes;
	case ERegion::Border: return mbReviewHRLRBorder;
	case ERegion::LowRes: return mbReviewLowRes;
	default: return true;
	}
}
void CConfig::SetCurrentCase(const char* zCaseName, int iCaseIndex)
{
	miCaseIndex = iCaseIndex;
	msCaseLogDir = msLogRoot;
	if (!msBatchRootDir.empty())
	{
		msCaseLogDir += "\\" + msBatchRootDir;
		CMyWindows::VerifyDirectory(msCaseLogDir.c_str()); // CreateDirectory() isn't recursive
	}
	msCaseLogDir += string("\\") + zCaseName;
	if (iCaseIndex > 0)
		msCaseLogDir += "_" + std::to_string(iCaseIndex);
	CMyWindows::VerifyDirectory(msCaseLogDir.c_str());

	gfLog.Printf("<CConfig::SetCurrentCase> Case directory: %s", msCaseLogDir.c_str());
}
void CConfig::SaveToFile()
{
	CXMLDump dumpFile((msConfigDir + "\\ReconTest.State.xml").c_str(), "def");

	dumpFile.Write("min_ct_threshold", mMinThreshold - CT_BIAS);
	dumpFile.Write("max_ct_threshold", mMaxThreshold - CT_BIAS);
	dumpFile.Write("wide_min_threshold", mWideMinThreshold - CT_BIAS);
	dumpFile.Write("filter_wide_image_range", mbFilterWideImageRange);
	dumpFile.Write("histogram_min", mHistogramMin - CT_BIAS);
	dumpFile.Write("histogram_max", mHistogramMax - CT_BIAS);
	dumpFile.Write("histogram_cut_percent", mHistogramCutPercent);
	dumpFile.Write("mask_erode_level", mErodeLevel);
	dumpFile.Write("slice_width", mnWantedSliceWidth);
	dumpFile.Write("n_central_rings", mnCentralRings);
	dumpFile.Write("n_off_center_rings", mnOffCenterRings);
	dumpFile.Write("min_pixels_in_mask", mnMinPixelsInMask);
	dumpFile.Write("last_high_resolution_ring", miLastHighResolutionRing);
	dumpFile.Write("first_low_resolution_ring", miFirstLowResolutionRing);
	dumpFile.Write("review_center", mbReviewCenter);
	dumpFile.Write("review_high_res", mbReviewHighRes);
	dumpFile.Write("review_hr_lr_border", mbReviewHRLRBorder);
	dumpFile.Write("review_low_res", mbReviewLowRes);
	dumpFile.Write("score_type", (int)mScoreType);
	dumpFile.Write("max_acceptable_score", mMaxAcceptableScore);
	dumpFile.Write("version", msVersion.c_str());
	dumpFile.Write("log_root", msLogRoot.c_str());
	dumpFile.Write("log_image_ring_details", mbLogImageRingDetails);
	dumpFile.Write("dicom_file_pattern", msDicomFilePattern.c_str());
	dumpFile.Write("data_root", msDataRoot.c_str());
	dumpFile.Write("download_dir_name_filter", msDownloadDirNameFilter.c_str());
	dumpFile.Write("download_default_source", msDownloadDefaultSource.c_str());
	dumpFile.Write("developer_mode", mbDeveloperMode);
	dumpFile.Write("display_ct_per_radius", mbDisplayCtPerRadius);
	dumpFile.Write("display_ct_per_radius_in_review", mbDisplayCtPerRadiusInReview);
	dumpFile.Write("avoid_shared_memory", mbAvoidSharedMemory);
	dumpFile.Write("collect_data_for_training", mbCollectDataForTraining);
	dumpFile.Write("training_set_root", msTrainingSetRoot.c_str());
	dumpFile.Write("saved_section_length", mSavedSectionLength);
	dumpFile.Write("operator_name", msOperatorName.c_str());

	dumpFile.Write("debug", mDebug);
}
void CConfig::ReadFromFile()
{
	string sfName(msConfigDir + "\\ReconTest.State.xml");
	if (!CFileName::Exist(sfName.c_str()))
		return;

	CXMLParse fParse(sfName.c_str());
	CXMLParseNode* pRoot = fParse.GetRoot();
	if (!pRoot)
	{
		CMyWindows::MessBox("<CConfig::ReadFromFile> Error: fParse.GetRoot failed", "Bad state file");
		return;
	}

	if (pRoot->GetValue("min_ct_threshold", mMinThreshold))
		mMinThreshold += CT_BIAS;
	if (pRoot->GetValue("max_ct_threshold", mMaxThreshold))
		mMaxThreshold += CT_BIAS;
	if (pRoot->GetValue("wide_min_threshold", mWideMinThreshold))
		mWideMinThreshold += CT_BIAS;
	pRoot->GetValue("filter_wide_image_range", mbFilterWideImageRange);
	if (pRoot->GetValue("histogram_min", mHistogramMin))
		mHistogramMin += CT_BIAS;
	if (pRoot->GetValue("histogram_max", mHistogramMax))
		mHistogramMax += CT_BIAS;
	pRoot->GetValue("histogram_cut_percent", mHistogramCutPercent);
	pRoot->GetValue("mask_erode_level", mErodeLevel);
	pRoot->GetValue("slice_width", mnWantedSliceWidth);
	pRoot->GetValue("n_central_rings", mnCentralRings);
	pRoot->GetValue("n_off_center_rings", mnOffCenterRings);
	pRoot->GetValue("min_pixels_in_mask", mnMinPixelsInMask);
	pRoot->GetValue("last_high_resolution_ring", miLastHighResolutionRing);
	pRoot->GetValue("first_low_resolution_ring", miFirstLowResolutionRing);
	pRoot->GetValue("review_center", mbReviewCenter);
	pRoot->GetValue("review_high_res", mbReviewHighRes);
	pRoot->GetValue("review_hr_lr_border", mbReviewHRLRBorder);
	pRoot->GetValue("review_low_res", mbReviewLowRes);

	int iScoreType = (int)mScoreType;
	if (pRoot->GetValue("score_type", iScoreType))
		mScoreType = (EScoreType)iScoreType;
	pRoot->GetValue("max_acceptable_score", mMaxAcceptableScore);

	pRoot->GetValue("version", msVersion);
	pRoot->GetValue("log_root", msLogRoot);
	pRoot->GetValue("log_image_ring_details", mbLogImageRingDetails);
	pRoot->GetValue("dicom_file_pattern", msDicomFilePattern);
	pRoot->GetValue("data_root", msDataRoot);
	pRoot->GetValue("download_dir_name_filter", msDownloadDirNameFilter);
	pRoot->GetValue("download_default_source", msDownloadDefaultSource);
	pRoot->GetValue("developer_mode", mbDeveloperMode);
	pRoot->GetValue("display_ct_per_radius", mbDisplayCtPerRadius);
	pRoot->GetValue("display_ct_per_radius_in_review", mbDisplayCtPerRadiusInReview);
	pRoot->GetValue("avoid_shared_memory", mbAvoidSharedMemory);
	pRoot->GetValue("collect_data_for_training", mbCollectDataForTraining);
	pRoot->GetValue("training_set_root", msTrainingSetRoot);
	pRoot->GetValue("saved_section_length", mSavedSectionLength);
	pRoot->GetValue("operator_name", msOperatorName);

	pRoot->GetValue("debug", mDebug);

}
string CConfig::GetCaseRelativeLogDir() const
{
	if (msCaseLogDir.size() > msLogRoot.size() && msCaseLogDir.compare(0, msLogRoot.size(), msLogRoot) == 0)
	{
		string sRelative(msCaseLogDir.substr(msLogRoot.size()));
		while (!sRelative.empty() && (sRelative.front() == '\\' || sRelative.front() == '/'))
			sRelative.erase(0, 1);
		return sRelative;
	}
	return msCaseLogDir; // shouldn't normally happen - msCaseLogDir is always built from msLogRoot
}
void CConfig::PrintStatus(const char* zStatus)
{
	gfLog.Log("<PrintStatus>", zStatus);
	CMyWindows::PrintStatus(zStatus);
}
