#include "stdafx.h"
#include "BatchScorer.h"
#include "IQVManager.h"
#include "Config.h"
#include "..\..\yUtils\FilesList.h"
#include "..\..\yUtils\MyWindows.h"
#include <string>
#include <format>
#include <cstdio>

using namespace std;

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

	return Run(list);
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
		if (manager.LoadAndScore(*psfName, iCase))
			nScored++;
	}

	string sDone(format("Batch scoring complete: {}/{} case(s) scored", nScored, nTotal));
	MyPrintStatus(sDone.c_str());

	return nScored;
}
