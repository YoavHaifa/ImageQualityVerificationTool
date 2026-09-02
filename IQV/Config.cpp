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
void CConfig::LoadScorerWeights()
{
	mvScorerWeights.assign((int)EScoreType::N_SCORE_TYPES, 1.0f);

	string sfName(msConfigDir + "\\ScorerWeights.csv");
	if (!CFileName::Exist(sfName.c_str()))
	{
		FILE* pfOut = nullptr;
		fopen_s(&pfOut, sfName.c_str(), "w");
		if (pfOut)
		{
			fprintf(pfOut, "code, name, weight\n");
			for (int i = 0; i < (int)EScoreType::N_SCORE_TYPES; i++)
				fprintf(pfOut, "%d, %s, %.2f\n", i, ScoreTypeName((EScoreType)i), mvScorerWeights[i]);
			fclose(pfOut);
		}
		return;
	}

	FILE* pf = nullptr;
	fopen_s(&pf, sfName.c_str(), "r");
	if (!pf)
		return;

	char zLine[128];
	fgets(zLine, sizeof(zLine), pf); // header
	while (fgets(zLine, sizeof(zLine), pf))
	{
		int iCode;
		char zName[64];
		float weight;
		if (sscanf_s(zLine, "%d, %63[^,], %f", &iCode, zName, (unsigned)sizeof(zName), &weight) == 3
			&& iCode >= 0 && iCode < (int)mvScorerWeights.size())
			mvScorerWeights[iCode] = weight;
	}
	fclose(pf);
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
	int i = (int)type;
	return (i >= 0 && i < (int)mvScorerWeights.size()) ? mvScorerWeights[i] : 1.0f;
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
	dumpFile.Write("ignore_low_resolution_area", mbIgnoreLowResolutionArea);
	dumpFile.Write("low_resolution_distance_from_center_pixels", mLowResolutionDistanceFromCenterPixels);
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
	dumpFile.Write("avoid_shared_memory", mbAvoidSharedMemory);
	dumpFile.Write("collect_data_for_training", mbCollectDataForTraining);
	dumpFile.Write("training_set_root", msTrainingSetRoot.c_str());
	dumpFile.Write("saved_section_length", mSavedSectionLength);

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
	pRoot->GetValue("ignore_low_resolution_area", mbIgnoreLowResolutionArea);
	pRoot->GetValue("low_resolution_distance_from_center_pixels", mLowResolutionDistanceFromCenterPixels);

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
	pRoot->GetValue("avoid_shared_memory", mbAvoidSharedMemory);
	pRoot->GetValue("collect_data_for_training", mbCollectDataForTraining);
	pRoot->GetValue("training_set_root", msTrainingSetRoot);
	pRoot->GetValue("saved_section_length", mSavedSectionLength);

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
