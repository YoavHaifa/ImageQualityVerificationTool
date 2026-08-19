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
void CConfig::SetCurrentCase(const char* zCaseName)
{
	msCaseLogDir = msLogRoot + "\\" + zCaseName;
	CMyWindows::VerifyDirectory(msCaseLogDir.c_str());
}
void CConfig::SaveToFile()
{
	CXMLDump dumpFile(CMyWindows::GetApplicationPath() + "\\ReconTest.State.xml", "def");

	dumpFile.Write("min_ct_threshold", mMinThreshold - CT_BIAS);
	dumpFile.Write("max_ct_threshold", mMaxThreshold - CT_BIAS);
	dumpFile.Write("mask_erode_level", mErodeLevel);
	dumpFile.Write("slice_width", mnWantedSliceWidth);
	dumpFile.Write("n_central_rings", mnCentralRings);
	dumpFile.Write("n_off_center_rings", mnOffCenterRings);
	dumpFile.Write("score_type", (int)mScoreType);
	dumpFile.Write("log_root", msLogRoot.c_str());
	dumpFile.Write("log_image_ring_details", mbLogImageRingDetails);
	dumpFile.Write("dicom_file_pattern", msDicomFilePattern.c_str());
	dumpFile.Write("developer_mode", mbDeveloperMode);

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
	pRoot->GetValue("mask_erode_level", mErodeLevel);
	pRoot->GetValue("slice_width", mnWantedSliceWidth);
	pRoot->GetValue("n_central_rings", mnCentralRings);
	pRoot->GetValue("n_off_center_rings", mnOffCenterRings);

	int iScoreType = (int)mScoreType;
	if (pRoot->GetValue("score_type", iScoreType))
		mScoreType = (EScoreType)iScoreType;

	CString sLogRoot;
	if (pRoot->GetValue("log_root", sLogRoot))
		msLogRoot = (const char*)sLogRoot;

	pRoot->GetValue("log_image_ring_details", mbLogImageRingDetails);

	CString sDicomFilePattern;
	if (pRoot->GetValue("dicom_file_pattern", sDicomFilePattern))
		msDicomFilePattern = (const char*)sDicomFilePattern;

	pRoot->GetValue("developer_mode", mbDeveloperMode);

	pRoot->GetValue("debug", mDebug);

}
void CConfig::PrintStatus(const char* zStatus)
{
	gfLog.Log("<PrintStatus>", zStatus);
	CMyWindows::PrintStatus(zStatus);
}
