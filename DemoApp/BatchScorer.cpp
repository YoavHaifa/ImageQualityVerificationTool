#include "stdafx.h"
#include "BatchScorer.h"
#include "IQVManager.h"
#include "Config.h"
#include "..\..\yUtils\FilesList.h"
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
		CString* psfName = list.GetNext(pos);

		string s(format("Batch scoring case {}/{}: {}", nScored + 1, nTotal, (LPCTSTR)*psfName));
		gConfig.PrintStatus(s.c_str());

		CIQVManager manager;
		if (manager.LoadAndScore(*psfName))
			nScored++;
	}

	return nScored;
}
