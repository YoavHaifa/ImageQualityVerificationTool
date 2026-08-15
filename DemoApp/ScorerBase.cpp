#include "stdafx.h"
#include "ScorerBase.h"
#include "Config.h"

using namespace std;

CScorerBase::CScorerBase(const vector<float>& vRingMean, EScoreType eScoreType)
	: mvRingMean(vRingMean)
	, mnRings((int)vRingMean.size() - 1)
	, meScoreType(eScoreType)
{
	mvRingScore.assign(vRingMean.size(), 0.0f);
}

void CScorerBase::Score(int iImage)
{
	mScore = CImageScore();
	std::fill(mvRingScore.begin(), mvRingScore.end(), 0.0f);
	ComputeScore();

	// Files the score just computed by Score() into mResults, under iImage
	mResults.AddScore(mScore.mScore, mScore.miRing, iImage);
}

void CScorerBase::CorrectCenter()
{
	if (gConfig.mnCentralRings < 1 || gConfig.mnOffCenterRings < 1)
		return;

	// Find min and max of off-center area
	//int iFirstRingOffCenter = gConfig.mnCentralRings;
	//float minOff = vRingMean[iFirstRingOffCenter].
}
