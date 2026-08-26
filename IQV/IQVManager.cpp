#include "stdafx.h"
#include "IQVManager.h"
#include "ArinetaImages.h"
#include "RingsScorer.h"
#include "Config.h"
#include "IQVDlg.h"
#include "..\..\yUtils\FileName.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\yUtils\FilesList.h"
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

	CString sDicomDir(mpImages->GetPath());
	if (!sDicomDir.IsEmpty() && (sDicomDir.Right(1) == "\\" || sDicomDir.Right(1) == "/"))
		sDicomDir = sDicomDir.Left(sDicomDir.GetLength() - 1);

	CString sCaseName;
	if (!gConfig.msBatchScanRootPath.empty())
		sCaseName = ComposeCaseNameFromJunctions(gConfig.msBatchScanRootPath.c_str(), sDicomDir);
	else
	{
		// No batch-scan root to compose a junction-based name from (single open, or reviewing
		// already-saved results) - fall back to the simple rule: the images' immediate directory
		// is sometimes just a generic wrapper name ("Dicom"), in which case its parent directory
		// holds the significant, case-identifying name instead.
		sCaseName = CFileName::GetLastInPath(sDicomDir);
		if (sCaseName.CompareNoCase("Dicom") == 0)
			sCaseName = CFileName::GetLastDirName(sDicomDir);
	}
	gConfig.SetCurrentCase(sCaseName, iCaseIndex);

	// "Current Case" reflects whichever case is loading right now, in every flow (single-open,
	// batch, review)
	if (gpDlg)
		gpDlg->SetDlgItemText(IDC_STATIC_IMAGESET, GetSetInfo().c_str());

	mpImages->ComputeRotationCenter();

	return mpImages->PrepareOnInit();
}
CString CIQVManager::ComposeCaseNameFromJunctions(const CString& sRoot, const CString& sSetDir)
{
	if (!CFileName::IsSubDir(sSetDir, sRoot))
		return CFileName::GetLastInPath(sSetDir); // shouldn't happen - sSetDir wasn't found under sRoot

	CString sRelative(sSetDir.Mid(sRoot.GetLength()));
	while (!sRelative.IsEmpty() && (sRelative[0] == '\\' || sRelative[0] == '/'))
		sRelative = sRelative.Mid(1);

	CString sCaseName;
	CString sCurrentDir(sRoot);
	while (!sRelative.IsEmpty())
	{
		int iSep = sRelative.FindOneOf("\\/");
		CString sSegment((iSep < 0) ? sRelative : sRelative.Left(iSep));
		sRelative = (iSep < 0) ? CString() : sRelative.Mid(iSep + 1);

		// A real junction: sCurrentDir had more than one real subdirectory to choose from -
		// sSegment is whichever one was actually taken toward sSetDir, so it's the informative part
		CFilesList subDirs;
		CMyWindows::ListSubDirsInDir(sCurrentDir, subDirs);
		if (subDirs.N() > 1)
			sCaseName += (sCaseName.IsEmpty() ? CString() : CString("_")) + sSegment;

		sCurrentDir += "\\" + sSegment;
	}

	if (sCaseName.IsEmpty())
		sCaseName = "SingleSet"; // no junction anywhere - sSetDir is the only set under sRoot

	return sCaseName;
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
