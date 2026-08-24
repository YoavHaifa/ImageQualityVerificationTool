#include "stdafx.h"
#include "BatchScorer.h"
#include "IQVManager.h"
#include "Config.h"
#include "..\..\yUtils\FilesList.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\yUtils\FileName.h"
#include <string>
#include <format>
#include <cstdio>

using namespace std;

// Some cases in the wild (e.g. an unexpected/corrupt DICOM layout) crash deep inside the
// decode/imaging pipeline rather than failing cleanly - deliberately no local C++ objects here
// (SEH requires that) so one bad case can be caught and skipped without taking the whole batch
// run down with it.
static bool SafeLoadAndScore(CIQVManager* pManager, const char* zFileName, int iCaseIndex)
{
	__try
	{
		return pManager->LoadAndScore(zFileName, iCaseIndex);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
}
CBatchScorer::CBatchScorer()
{
}
CBatchScorer::~CBatchScorer()
{
}
void CBatchScorer::MyPrintStatus(const char* zText)
{
	CMyWindows::PrintStatus1(zText);
	printf("%s\n", zText);
}
int CBatchScorer::RunOnDirTree(const char* zRootDir)
{
	MyPrintStatus("Looking for Dicom Sets");

	CFilesList list;
	int nFound = CMyWindows::ListSampleFilesInDirTree(zRootDir, gConfig.msDicomFilePattern.c_str(), list);

	string s(format("Found {} case(s) under {}", nFound, zRootDir));
	printf("%s\n", s.c_str());

	// Set for the duration of this run so every case nests under the same log subdirectory
	// without threading it through each LoadAndScore() call; cleared again once done so it
	// doesn't leak into any later interactive/review flow.
	gConfig.msBatchRootDir = (LPCTSTR)CFileName::GetLastInPath(zRootDir);
	msLogDir = CString(gConfig.msLogRoot.c_str()) + "\\" + gConfig.msBatchRootDir.c_str();
	int nScored = Run(list);
	gConfig.msBatchRootDir.clear();

	return nScored;
}
int CBatchScorer::Run(const CFilesList& list)
{
	int nTotal = list.N();
	int nScored = 0;
	int iCase = 0;

	POSITION pos = list.GetHeadPosition();
	while (pos)
	{
		iCase++;
		string sBatch(format("Batch scoring: case {}/{}", iCase, nTotal));
		MyPrintStatus(sBatch.c_str());

		CString* psfName = list.GetNext(pos);

		CIQVManager manager;
		if (SafeLoadAndScore(&manager, *psfName, iCase))
			nScored++;
		else
			MyPrintStatus("Case data could not be loaded - skipped without saving");
	}

	string sDone(format("Batch scoring complete: {}/{} case(s) scored", nScored, nTotal));
	MyPrintStatus(sDone.c_str());

	return nScored;
}
