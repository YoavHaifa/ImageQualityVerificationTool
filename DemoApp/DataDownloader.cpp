#include "stdafx.h"
#include "DataDownloader.h"
#include "ArinetaImages.h"
#include "Config.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\yUtils\FilesList.h"
#include "..\..\yUtils\FileName.h"
#include <string>
#include <format>

using namespace std;

CDataDownloader::CDataDownloader()
{
}
CDataDownloader::~CDataDownloader()
{
}
void CDataDownloader::MyPrintStatus(const char* zText)
{
	CMyWindows::PrintStatus1(zText);
	printf("%s\n", zText);
}
int CDataDownloader::DownloadFromRoot(const char* zRootDir)
{
	string sFilter(gConfig.msDownloadDirNameFilter);
	string s0(format("Looking for \"{}\" directories under {}", sFilter, zRootDir));
	MyPrintStatus(s0.c_str());

	CFilesList waterDirs;
	int nWaterDirs = CMyWindows::ListDirsByNameInDirTree(zRootDir, sFilter.c_str(), waterDirs);
	if (nWaterDirs < 1)
	{
		string s(format("No \"{}\" directories found", sFilter));
		MyPrintStatus(s.c_str());
		return 0;
	}

	CString sRoot(zRootDir);
	while (!sRoot.IsEmpty() && (sRoot.Right(1) == "\\" || sRoot.Right(1) == "/"))
		sRoot = sRoot.Left(sRoot.GetLength() - 1);

	CString sRootLeaf(CFileName::GetLastInPath(sRoot));
	CString sDestRoot;
	if (!CMyWindows::VerifyDirectoryPath(gConfig.msDataRoot.c_str(), sRootLeaf, sDestRoot))
	{
		MyPrintStatus("Failed to create local destination root");
		return 0;
	}

	int nCopied = 0;
	POSITION pos = waterDirs.GetHeadPosition();
	while (pos)
	{
		CString* psWaterDir = waterDirs.GetNext(pos);

		CFilesList sets;
		CMyWindows::ListSampleFilesInDirTree(*psWaterDir, gConfig.msDicomFilePattern.c_str(), sets);

		POSITION posSet = sets.GetHeadPosition();
		while (posSet)
		{
			CString* psSample = sets.GetNext(posSet);
			if (CArinetaImages::IsImageDicom(*psSample))
				CopyOneSet(sRoot, sDestRoot, *psSample, nCopied);
		}
	}

	string s(format("Copied {} image set(s) to {}", nCopied, (LPCTSTR)sDestRoot));
	MyPrintStatus(s.c_str());
	return nCopied;
}
void CDataDownloader::CopyOneSet(const CString& sRoot, const CString& sDestRoot, const CString& sSampleFile, int& onCopied)
{
	CFileName fSample(sSampleFile);
	CString sSetDir(fSample.Path());
	while (!sSetDir.IsEmpty() && (sSetDir.Right(1) == "\\" || sSetDir.Right(1) == "/"))
		sSetDir = sSetDir.Left(sSetDir.GetLength() - 1);

	if (!CFileName::IsSubDir(sSetDir, sRoot))
		return; // shouldn't happen - sSetDir was found while scanning under sRoot

	CString sRelative(sSetDir.Mid(sRoot.GetLength()));
	while (!sRelative.IsEmpty() && (sRelative[0] == '\\' || sRelative[0] == '/'))
		sRelative = sRelative.Mid(1);

	CString sDestDir;
	if (!CMyWindows::VerifyDirectoryPath(sDestRoot, sRelative, sDestDir))
	{
		string s(format("Failed to create destination directory for {}", (LPCTSTR)sSetDir));
		MyPrintStatus(s.c_str());
		return;
	}

	string s(format("Copying {} -> {}", (LPCTSTR)sSetDir, (LPCTSTR)sDestDir));
	MyPrintStatus(s.c_str());

	CMyWindows::CopyDiretoryTree(sSetDir, sDestDir);
	onCopied++;
}
