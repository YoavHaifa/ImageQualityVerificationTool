#include "stdafx.h"
#include "ScoreTypeResults.h"
#include "Config.h"

void CScoreTypeResults::AddScore(const CImageScore& score, int iImage)
{
	CImageScore stored = score;
	stored.miOriginalImage = iImage;
	mvScores.push_back(stored);
	if (mvScores.size() == 1 || score.mScore > mMaxScore)
	{
		mMaxScore = score.mScore;
		miImageWithMaxScore = iImage;
	}
}
void CScoreTypeResults::OnAllImagesScored()
{
	FindPeaks();
	OrderPeaks();
}
void CScoreTypeResults::ScaleScores(float factor)
{
	// mRawScore is deliberately left untouched - it stays the real, physical raw score
	// (e.g. tent amplitude) regardless of this case's data range factor
	for (CImageScore& score : mvScores)
		score.mScore *= factor;
	mMaxScore *= factor;
}
void CScoreTypeResults::FindPeaks()
{
	int nImages = (int)mvScores.size();
	for (int iImage = 1; iImage < nImages - 1; iImage++)
	{
		float prev = mvScores[iImage - 1].mScore;
		float cur = mvScores[iImage].mScore;
		float next = mvScores[iImage + 1].mScore;
		if (prev > 0 && cur > 0 && next > 0) // All valid scores
		{
			if (cur >= prev && cur >= next)
			{
				mvScores[iImage].mbPeak = true;
				mnPeaks++;
			}
		}
	}
}
void CScoreTypeResults::OrderPeaks()
{
	mnRealPeaks = 0;
	int nPeaksOrdered = 0;
	while (nPeaksOrdered < mnPeaks)
	{
		if (!FindNextPixToOrder())
			break;
		nPeaksOrdered++;
	}
}
bool CScoreTypeResults::FindNextPixToOrder()
{
	int nImages = (int)mvScores.size();
	int iNext = -1;
	float nextMaxScore = 0;
	for (int iImage = 1; iImage < nImages - 1; iImage++) // Peaks can not come in first or last image
	{
		if (mvScores[iImage].mbPeak && !mvScores[iImage].miPeak)
		{
			float score = mvScores[iImage].mScore;
			if (score > nextMaxScore)
			{
				nextMaxScore = score;
				iNext = iImage;
			}
		}
	}
	if (iNext < 0)
		return false;

	if (!mvScores[iNext - 1].mbPeak)
		mnRealPeaks++;
	else
		gfLog.Printf("<CScoreTypeResults::FindNextPixToOrder> found wide peak %d at %d", mnRealPeaks, iNext);
	mvScores[iNext].miPeak = mnRealPeaks;
	return true;
}
int CScoreTypeResults::FindImageIndexOfPeak(int iWantedPeak) const
{
	for (int iImage = 0; iImage < (int)mvScores.size(); iImage++)
		if (mvScores[iImage].miPeak == iWantedPeak)
			return iImage;
	return -1;
}
