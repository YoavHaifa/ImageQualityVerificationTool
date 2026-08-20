#include "stdafx.h"
#include "BatchScorer.h"
#include "IQVManager.h"
#include "Config.h"
#include "..\..\yUtils\FilesList.h"
#include "..\..\yUtils\MyWindows.h"
#include <string>
#include <format>

using namespace std;

CBatchScorer::CBatchScorer()
{
}
CBatchScorer::~CBatchScorer()
{
}
int CBatchScorer::Run(const CFilesList& list)
{
	int nTotal = list.N();
	int nScored = 0;

	POSITION pos = list.GetHeadPosition();
	while (pos)
	{
		string sBatch(format("Batch scoring: case {}/{}", nScored + 1, nTotal));
		CMyWindows::PrintStatus1(sBatch.c_str());

		CString* psfName = list.GetNext(pos);

		CIQVManager manager;
		if (manager.LoadAndScore(*psfName))
			nScored++;
	}

	string sDone(format("Batch scoring complete: {}/{} case(s) scored", nScored, nTotal));
	CMyWindows::PrintStatus1(sDone.c_str());

	return nScored;
}
