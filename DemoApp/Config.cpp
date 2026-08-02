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
	CMyWindows::VerifyDirectory(msScoreGraphsDir.c_str());
}
void CConfig::Init()
{
	ReadFromFile();
	if (mDebug)
		gfLog.Init("IQV_App");
}
void CConfig::SaveToFile()
{
	CXMLDump dumpFile(CMyWindows::GetApplicationPath() + "\\ReconTest.State.xml", "def");

	dumpFile.Write("min_ct_threshold", mMinThreshold - CT_BIAS);
	dumpFile.Write("max_ct_threshold", mMaxThreshold - CT_BIAS);
	dumpFile.Write("mask_erode_level", mErodeLevel);
	dumpFile.Write("slice_width", mnWantedSliceWidth);

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
	//pRoot->GetValue("slice_width", mnWantedSliceWidth);

	pRoot->GetValue("debug", mDebug);

}
