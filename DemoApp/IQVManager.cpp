#include "stdafx.h"
#include "IQVManager.h"
#include "ArinetaImages.h"
#include "RingsScorer.h"
#include "Config.h"
#include "DemoAppDlg.h"
#include "..\..\yUtils\FileName.h"
#include <format>

using namespace std;

CIQVManager::CIQVManager()
{
}
CIQVManager::~CIQVManager()
{
	delete mpRingsScorer;
	delete mpImages;
}
bool CIQVManager::LoadAndScore(const char* zImageFileName, CDemoAppDlg* pDlg)
{
	CArinetaImages::SetDebug(0xff);
	mpImages = new CArinetaImages(zImageFileName);

	// The images' immediate directory is usually a generic name (e.g. "Dicom"),
	// so use its parent directory's name as the case name instead.
	CString sDicomDir(mpImages->GetPath());
	if (!sDicomDir.IsEmpty() && (sDicomDir.Right(1) == "\\" || sDicomDir.Right(1) == "/"))
		sDicomDir = sDicomDir.Left(sDicomDir.GetLength() - 1);
	gConfig.SetCurrentCase(CFileName::GetLastDirName(sDicomDir));

	// "Current Case" reflects whichever case is loading/scoring right now, in both the
	// single-open and batch flows - independent of pDlg, which is only for the debug circle below
	if (gpDlg)
		gpDlg->SetDlgItemText(IDC_STATIC_IMAGESET, GetSetInfo().c_str());

	mpImages->ComputeRotationCenter(pDlg);
	mpImages->PrepareOnInit();

	mpRingsScorer = new CRingsScorer(mpImages);
	miScoredPosition = mpRingsScorer->ScoreAllImages();

	return true;
}
string CIQVManager::GetSetInfo(void)
{
	if (!mpImages)
		return string();

	return format("Set: {}  ({} images)", (LPCTSTR)mpImages->GetPath(), mpImages->GetNFiles());
}
