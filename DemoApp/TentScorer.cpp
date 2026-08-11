#include "stdafx.h"
#include "TentScorer.h"
#include "Config.h"

typedef enum EDir
{
	DIR_Undef,
	DIR_Up,
	DIR_Down,
	DIR_Last
}EDir;


CTentScorer::CTentScorer(const std::vector<float>& vRingMean)
	: CScorerBase(vRingMean, EScoreType::Tent)
{
}
void CTentScorer::ComputeScore()
{
	// Look for all rings that are internal local mean
	float maxScore = 0;
	float prev = IGNORE_RING;
	EDir eDir = DIR_Undef;

	for (int i = 0; i < mnRings; i++)
	{
		float value = mvRingMean[i];
		if (value == IGNORE_RING)
		{
			eDir = DIR_Undef;
		}
		else if (prev != IGNORE_RING)
		{
			if (value > prev) // Going up here
			{
				if (eDir == DIR_Down) // Flex point for Local Min
					ComputeLocalMinScore(i-1);
				eDir = DIR_Up;
			}
			else if (value < prev)
			{
				if (eDir == DIR_Up) // Flex point for Local Max
					ComputeLocalMaxScore(i - 1);
				eDir = DIR_Down;
			}
		}

		prev = value;
	}
}
void CTentScorer::ComputeLocalMaxScore(int iRing)
{
	float value = mvRingMean[iRing];

	// Look for left leg of the tent
	int iPrev = iRing - 1;
	int iNext = iRing + 1;

	// Skip flat max area
	while (iPrev > 0 && mvRingMean[iPrev] == value)
		iPrev--;
	while (iNext < mnRings - 1 && mvRingMean[iNext] == value)
		iNext++;

	// Go back down as long as available - stop at the array edge or at an ignored ring
	while (iPrev > 0 && mvRingMean[iPrev - 1] != IGNORE_RING && mvRingMean[iPrev - 1] < mvRingMean[iPrev])
		iPrev--;

	// Go forward down as long as available - stop at the array edge or at an ignored ring
	while (iNext < mnRings - 1 && mvRingMean[iNext + 1] != IGNORE_RING && mvRingMean[iNext + 1] < mvRingMean[iNext])
		iNext++;

	float score = ((value - mvRingMean[iPrev]) + (value - mvRingMean[iNext])) / 2.0f;
	mvRingScore[iRing] = score;
	if (score > mScore.mScore)
	{
		mScore.mScore = score;
		mScore.miRing = iRing;
	}
}
void CTentScorer::ComputeLocalMinScore(int iRing)
{
	float value = mvRingMean[iRing];

	// Look for left leg of the tent
	int iPrev = iRing - 1;
	int iNext = iRing + 1;

	// Skip flat max area
	while (iPrev > 0 && mvRingMean[iPrev] == value)
		iPrev--;
	while (iNext < mnRings - 1 && mvRingMean[iNext] == value)
		iNext++;

	// Go back up as long as available - stop at the array edge or at an ignored ring
	while (iPrev > 0 && mvRingMean[iPrev - 1] != IGNORE_RING && mvRingMean[iPrev - 1] > mvRingMean[iPrev])
		iPrev--;

	// Go forward up as long as available - stop at the array edge or at an ignored ring
	while (iNext < mnRings - 1 && mvRingMean[iNext + 1] != IGNORE_RING && mvRingMean[iNext + 1] > mvRingMean[iNext])
		iNext++;

	float score = ((mvRingMean[iPrev] - value) + (mvRingMean[iNext] - value)) / 2.0f;
	mvRingScore[iRing] = score;
	if (score > mScore.mScore)
	{
		mScore.mScore = score;
		mScore.miRing = iRing;
	}
}