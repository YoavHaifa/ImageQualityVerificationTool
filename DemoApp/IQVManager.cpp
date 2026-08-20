#include "stdafx.h"
#include "IQVManager.h"
#include "ArinetaImages.h"
#include "RingsScorer.h"
#include "Config.h"
#include "DemoAppDlg.h"
#include "..\..\yUtils\FileName.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\yUtils\YamlParser.h"
#include <format>
#include <vector>

using namespace std;

CIQVManager::CIQVManager()
{
}
CIQVManager::~CIQVManager()
{
	delete mpRingsScorer;
	delete mpImages;
}
bool CIQVManager::LoadImages(const char* zImageFileName)
{
	CArinetaImages::SetDebug(0xff);
	mpImages = new CArinetaImages(zImageFileName);

	// The images' immediate directory is usually a generic name (e.g. "Dicom"),
	// so use its parent directory's name as the case name instead.
	CString sDicomDir(mpImages->GetPath());
	if (!sDicomDir.IsEmpty() && (sDicomDir.Right(1) == "\\" || sDicomDir.Right(1) == "/"))
		sDicomDir = sDicomDir.Left(sDicomDir.GetLength() - 1);
	gConfig.SetCurrentCase(CFileName::GetLastDirName(sDicomDir));

	// "Current Case" reflects whichever case is loading right now, in every flow (single-open,
	// batch, review)
	if (gpDlg)
		gpDlg->SetDlgItemText(IDC_STATIC_IMAGESET, GetSetInfo().c_str());

	mpImages->ComputeRotationCenter();
	mpImages->PrepareOnInit();

	return true;
}
bool CIQVManager::LoadAndScore(const char* zImageFileName)
{
	if (!LoadImages(zImageFileName))
		return false;

	mpRingsScorer = new CRingsScorer(mpImages);
	miScoredPosition = mpRingsScorer->ScoreAllImages();

	return true;
}
bool CIQVManager::ResolveCaseSampleFile(const char* zCaseDir, CString& osSampleFile)
{
	CYamlParser parser;
	string sYamlName(string(zCaseDir) + "\\CaseInfo.yaml");
	if (!parser.Parse(sYamlName.c_str()))
		return false;

	CString sCasePath;
	if (!parser.GetRoot()->GetValue("case_path", sCasePath))
		return false;

	osSampleFile = CMyWindows::GetFirstFileName(sCasePath + "\\" + gConfig.msDicomFilePattern.c_str());
	return !osSampleFile.IsEmpty();
}
bool CIQVManager::LoadFromSavedResults(const char* zCaseDir)
{
	CString sSampleFile;
	if (!ResolveCaseSampleFile(zCaseDir, sSampleFile))
		return false;

	if (!LoadImages(sSampleFile))
		return false;

	CYamlParser parser;
	string sYamlName(string(zCaseDir) + "\\CaseInfo.yaml");
	parser.Parse(sYamlName.c_str()); // already validated by ResolveCaseSampleFile above

	vector<CString> vScorerNames;
	CYamlLine* pScorers = parser.GetRoot()->GetFirst("scorers");
	if (pScorers)
	{
		POSITION pos = pScorers->GetHeadPosition();
		while (pos)
			vScorerNames.push_back(pScorers->GetNext(pos)->Key());
	}

	mpRingsScorer = new CRingsScorer(mpImages);
	miScoredPosition = mpRingsScorer->LoadFromSavedResults(zCaseDir, vScorerNames);

	return true;
}
string CIQVManager::GetSetInfo(void)
{
	if (!mpImages)
		return string();

	return format("Set: {}  ({} images)", (LPCTSTR)mpImages->GetPath(), mpImages->GetNFiles());
}
