#include "stdafx.h"
#include "BatchReviewer.h"
#include "IQVManager.h"
#include "Config.h"
#include "..\..\yUtils\MyFolderDialog.h"
#include "..\..\yUtils\MyWindows.h"
#include "..\..\yUtils\FilesList.h"
#include "..\..\yUtils\FileName.h"
#include "..\..\yUtils\YamlParser.h"
#include <algorithm>
#include <format>

CBatchReviewer::CBatchReviewer()
{
}
CBatchReviewer::~CBatchReviewer()
{
	delete mpManager;
}
bool CBatchReviewer::Init()
{
	CMyFolderDialog dlg("Select Root Directory for Batch Review");
	if (!dlg.DoModal())
		return false;

	return Init(dlg.msFolderName);
}
bool CBatchReviewer::Init(const char* zRootDir)
{
	BuildCaseList(zRootDir);
	if (mvCases.empty())
	{
		CMyWindows::MessBox("No reviewable cases (with CaseInfo.yaml) found under the selected directory.",
			"Open Batch Scoring");
		return false;
	}

	ComputeOrder();

	return DisplayWorstCase();
}
void CBatchReviewer::BuildCaseList(const char* zRootDir)
{
	mvCases.clear();
	mvScorerNames.clear();

	CFilesList dirs;
	CMyWindows::ListSubDirsInDir(zRootDir, dirs);

	POSITION pos = dirs.GetHeadPosition();
	while (pos)
	{
		CString* psDir = dirs.GetNext(pos);

		CString sYamlName(*psDir + "\\CaseInfo.yaml");
		if (!CFileName::Exist(sYamlName))
			continue;

		CYamlParser parser;
		if (!parser.Parse(sYamlName))
			continue;

		CYamlLine* pScorers = parser.GetRoot()->GetFirst("scorers");
		if (!pScorers)
			continue;

		CCaseInfo caseInfo;
		caseInfo.msCaseDir = *psDir;

		POSITION posScorer = pScorers->GetHeadPosition();
		while (posScorer)
		{
			CYamlLine* pScorer = pScorers->GetNext(posScorer);
			CString sName = pScorer->Key();

			int iScorer = FindScorerIndex(sName);
			if (iScorer < 0)
			{
				// First time seeing this scorer name - add it as a new column, backfilling
				// cases already collected with 0 for it
				iScorer = (int)mvScorerNames.size();
				mvScorerNames.push_back(sName);
				for (CCaseInfo& c : mvCases)
					c.mvWorstScore.push_back(0.0f);
			}

			float worstScore = 0;
			pScorer->GetValue("worst_score", worstScore);

			while ((int)caseInfo.mvWorstScore.size() <= iScorer)
				caseInfo.mvWorstScore.push_back(0.0f);
			caseInfo.mvWorstScore[iScorer] = worstScore;
		}

		mvCases.push_back(caseInfo);
	}
}
void CBatchReviewer::ComputeOrder()
{
	for (CCaseInfo& c : mvCases)
	{
		while (c.mvWorstScore.size() < mvScorerNames.size())
			c.mvWorstScore.push_back(0.0f);
		c.mvOrder.assign(mvScorerNames.size(), 0);
	}

	for (int iScorer = 0; iScorer < (int)mvScorerNames.size(); iScorer++)
	{
		std::vector<int> vIndices(mvCases.size());
		for (int i = 0; i < (int)mvCases.size(); i++)
			vIndices[i] = i;

		// Worst (highest) score first, like CScoreTypeResults' peak severity order
		std::sort(vIndices.begin(), vIndices.end(), [this, iScorer](int a, int b)
			{
				return mvCases[a].mvWorstScore[iScorer] > mvCases[b].mvWorstScore[iScorer];
			});

		for (int iRank = 0; iRank < (int)vIndices.size(); iRank++)
			mvCases[vIndices[iRank]].mvOrder[iScorer] = iRank + 1;
	}
}
int CBatchReviewer::FindScorerIndex(const char* zName) const
{
	for (int i = 0; i < (int)mvScorerNames.size(); i++)
		if (mvScorerNames[i] == zName)
			return i;
	return -1;
}
const CCaseInfo* CBatchReviewer::FindCaseAtRank(int iRank) const
{
	int iScorer = FindScorerIndex(ScoreTypeName(gConfig.mScoreType));
	if (iScorer < 0)
		return nullptr;

	if (iRank < 1 || iRank > (int)mvCases.size())
		return nullptr;

	for (const CCaseInfo& c : mvCases)
		if (c.mvOrder[iScorer] == iRank)
			return &c;
	return nullptr;
}
bool CBatchReviewer::LoadCaseAtRank(int iRank)
{
	const CCaseInfo* pTarget = FindCaseAtRank(iRank);
	if (!pTarget)
		return false;

	delete mpManager;
	mpManager = new CIQVManager();
	if (!mpManager->LoadFromSavedResults(pTarget->msCaseDir))
	{
		delete mpManager;
		mpManager = nullptr;
		return false;
	}

	miCurrentRank = iRank;
	return true;
}
bool CBatchReviewer::DisplayWorstCase()
{
	return LoadCaseAtRank(1);
}
bool CBatchReviewer::DisplayNextCase()
{
	if (miCurrentRank >= (int)mvCases.size())
	{
		gConfig.PrintStatus("Best case already displayed");
		return false;
	}

	const char* zScorerName = ScoreTypeName(gConfig.mScoreType);
	int iScorer = FindScorerIndex(zScorerName);
	const CCaseInfo* pNext = FindCaseAtRank(miCurrentRank + 1);
	if (iScorer < 0 || !pNext || pNext->mvWorstScore[iScorer] <= 0)
	{
		gConfig.PrintStatus(std::format("no more relevant scores for {}", zScorerName).c_str());
		return false;
	}

	return LoadCaseAtRank(miCurrentRank + 1);
}
bool CBatchReviewer::DisplayPrevCase()
{
	if (miCurrentRank <= 1)
	{
		gConfig.PrintStatus("Worst case already displayed");
		return false;
	}
	return LoadCaseAtRank(miCurrentRank - 1);
}
