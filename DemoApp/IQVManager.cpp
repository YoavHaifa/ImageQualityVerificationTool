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
bool CIQVManager::LoadImages(const char* zImageFileName, int iCaseIndex)
{
	CArinetaImages::SetDebug(0xff);
	mpImages = new CArinetaImages(zImageFileName);

	// The images' immediate directory is sometimes just a generic wrapper name ("Dicom") - only
	// then does its parent directory hold the significant, case-identifying name instead.
	CString sDicomDir(mpImages->GetPath());
	if (!sDicomDir.IsEmpty() && (sDicomDir.Right(1) == "\\" || sDicomDir.Right(1) == "/"))
		sDicomDir = sDicomDir.Left(sDicomDir.GetLength() - 1);
	CString sCaseName = CFileName::GetLastInPath(sDicomDir);
	if (sCaseName.CompareNoCase("Dicom") == 0)
		sCaseName = CFileName::GetLastDirName(sDicomDir);
	gConfig.SetCurrentCase(sCaseName, iCaseIndex);

	// "Current Case" reflects whichever case is loading right now, in every flow (single-open,
	// batch, review)
	if (gpDlg)
		gpDlg->SetDlgItemText(IDC_STATIC_IMAGESET, GetSetInfo().c_str());

	mpImages->ComputeRotationCenter();

	return mpImages->PrepareOnInit();
}
bool CIQVManager::LoadAndScore(const char* zImageFileName, int iCaseIndex)
{
	// Some cases (e.g. an unexpected directory layout) have images ImageRLib can't actually
	// decode - it pops a blocking "GetData NULL" box per bad image rather than failing cleanly.
	// Suppress those for the duration of this case and treat any as "case data not found":
	// abort without saving, instead of the user having to dismiss one box per bad image.
	CMyWindows::DisableMessBox();

	bool bOK = LoadImages(zImageFileName, iCaseIndex);
	if (bOK && CMyWindows::GetMessBoxCount(nullptr) > 0)
		bOK = false;

	if (bOK)
	{
		mpRingsScorer = new CRingsScorer(mpImages);
		miScoredPosition = mpRingsScorer->ScoreAllImages();
		bOK = (miScoredPosition >= 0);
	}

	CMyWindows::EnableMessBox(nullptr);

	if (!bOK)
		gConfig.PrintStatus("Case data could not be loaded - skipping without saving");

	return bOK;
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
