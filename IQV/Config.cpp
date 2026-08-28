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
	ReadFromFile();
	CMyWindows::VerifyDirectory(msLogRoot.c_str());
	if (mDebug)
		gfLog.Init("IQV_App");
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
	CXMLDump dumpFile(CMyWindows::GetApplicationPath() + "\\ReconTest.State.xml", "def");

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
	dumpFile.Write("score_type", (int)mScoreType);
	dumpFile.Write("version", msVersion.c_str());
	dumpFile.Write("log_root", msLogRoot.c_str());
	dumpFile.Write("log_image_ring_details", mbLogImageRingDetails);
	dumpFile.Write("dicom_file_pattern", msDicomFilePattern.c_str());
	dumpFile.Write("data_root", msDataRoot.c_str());
	dumpFile.Write("download_dir_name_filter", msDownloadDirNameFilter.c_str());
	dumpFile.Write("download_default_source", msDownloadDefaultSource.c_str());
	dumpFile.Write("developer_mode", mbDeveloperMode);
	dumpFile.Write("display_ct_per_radius", mbDisplayCtPerRadius);

	dumpFile.Write("debug", mDebug);
}
void CConfig::ReadFromFile()
{
	string sfName(CMyWindows::GetApplicationPath() + "\\ReconTest.State.xml");
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

	int iScoreType = (int)mScoreType;
	if (pRoot->GetValue("score_type", iScoreType))
		mScoreType = (EScoreType)iScoreType;

	pRoot->GetValue("version", msVersion);
	pRoot->GetValue("log_root", msLogRoot);
	pRoot->GetValue("log_image_ring_details", mbLogImageRingDetails);
	pRoot->GetValue("dicom_file_pattern", msDicomFilePattern);
	pRoot->GetValue("data_root", msDataRoot);
	pRoot->GetValue("download_dir_name_filter", msDownloadDirNameFilter);
	pRoot->GetValue("download_default_source", msDownloadDefaultSource);
	pRoot->GetValue("developer_mode", mbDeveloperMode);
	pRoot->GetValue("display_ct_per_radius", mbDisplayCtPerRadius);

	pRoot->GetValue("debug", mDebug);

}
void CConfig::PrintStatus(const char* zStatus)
{
	gfLog.Log("<PrintStatus>", zStatus);
	CMyWindows::PrintStatus(zStatus);
}
