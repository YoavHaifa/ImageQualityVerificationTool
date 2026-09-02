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
	EScoreType eSource = EScoreType::N_SCORE_TYPES;
	for (const auto& pSibling : *mpAllScorers)
	{
		if (pSibling.get() == this)
			continue;
		if (pSibling->mScore.mScore > maxScore)
		{
			maxScore = pSibling->mScore.mScore;
			iRingOfMax = pSibling->mScore.miRing;
			eSource = pSibling->GetScoreType();
		}
	}
	if (iRingOfMax >= 0)
	{
		mvRingScore[iRingOfMax] = maxScore;
		mScore.meSourceType = eSource; // FindMaxScorePerCurrentImage() only touches mScore/miRing
	}
}
bool CAllMaxScorer::LoadSavedResults(const char* zCaseDir, float /*dataRangeFactor*/)
{
	// Not needed here - built from siblings' mResults, which (loaded before this, in
	// registration order) already reflect both their own weight and this case's data range factor
	int nImages = 0;
	for (const auto& pSibling : *mpAllScorers)
		if (pSibling.get() != this)
			nImages = max(nImages, pSibling->mResults.NumImages());

	for (int iImage = 0; iImage < nImages; iImage++)
	{
		float maxScore = 0;
		int iRingOfMax = -1;
		int iOriginal = -1;
		EScoreType eSource = EScoreType::N_SCORE_TYPES;
		for (const auto& pSibling : *mpAllScorers)
		{
			if (pSibling.get() == this || iImage >= pSibling->mResults.NumImages())
				continue;

			const CImageScore& s = pSibling->mResults[iImage];
			// Every sibling at this push-order index shares the same original image number,
			// even one scored 0 - grab it unconditionally so this index is never skipped, or
			// every later index would drift out of the push-order == original image - 1
			// alignment that GetCurrentScore()/FindImageIndexOfPeak() rely on
			if (iOriginal < 0)
				iOriginal = s.miOriginalImage;
			if (s.mScore > maxScore)
			{
				maxScore = s.mScore;
				iRingOfMax = s.miRing;
				eSource = pSibling->GetScoreType();
			}
		}
		if (iOriginal >= 0)
		{
			CImageScore score;
			score.mScore = maxScore;
			score.mRawScore = maxScore;
			score.miRing = iRingOfMax;
			score.meSourceType = eSource;
			mResults.AddScore(score, iOriginal);
		}
	}
	return true;
}
