#include "stdafx.h"
#include "AllMaxScorer.h"

CAllMaxScorer::CAllMaxScorer(const std::vector<float>& vRingMean, const std::vector<std::unique_ptr<CScorerBase>>* pAllScorers)
	: CScorerBase(vRingMean, EScoreType::AllMax)
	, mpAllScorers(pAllScorers)
{
	// Ignores its own weight - it's already built from its siblings' weighted scores
	mWeight = 1.0f;
}
void CAllMaxScorer::ComputeScore()
{
	float maxScore = 0;
	int iRingOfMax = -1;
	for (const auto& pSibling : *mpAllScorers)
	{
		if (pSibling.get() == this)
			continue;
		if (pSibling->mScore.mScore > maxScore)
		{
			maxScore = pSibling->mScore.mScore;
			iRingOfMax = pSibling->mScore.miRing;
		}
	}
	if (iRingOfMax >= 0)
		mvRingScore[iRingOfMax] = maxScore;
}
