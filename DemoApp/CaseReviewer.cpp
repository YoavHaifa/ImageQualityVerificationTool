#include "stdafx.h"
#include "CaseReviewer.h"
#include "IQVManager.h"
#include "..\..\yUtils\MyFolderDialog.h"
#include "..\..\yUtils\MyWindows.h"

CCaseReviewer::CCaseReviewer()
{
}
CCaseReviewer::~CCaseReviewer()
{
	delete mpManager;
}
bool CCaseReviewer::SelectCase()
{
	CMyFolderDialog dlg("Select Case Directory to Review");
	if (!dlg.DoModal())
		return false;

	msCaseDir = dlg.msFolderName;

	mpManager = new CIQVManager();
	CString sSampleFile;
	if (!mpManager->ResolveCaseSampleFile(msCaseDir, sSampleFile))
	{
		CMyWindows::MessBox("Selected directory does not contain a valid, reviewable case "
			"(expecting CaseInfo.yaml and its original images to still be available).",
			"Open Case Scoring");
		return false;
	}

	return true;
}
bool CCaseReviewer::LoadCase()
{
	return mpManager->LoadFromSavedResults(msCaseDir);
}
