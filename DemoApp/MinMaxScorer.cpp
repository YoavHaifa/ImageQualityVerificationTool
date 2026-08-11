#include "stdafx.h"
#include "MinMaxScorer.h"
#include "Config.h"

CMinMaxScorer::CMinMaxScorer(const std::vector<float>& vRingMean)
	: CScorerBase(vRingMean, EScoreType::MinMax)
{
}
void CMinMaxScorer::ComputeScore()
{
	float maxDiff = 0;
	int iRingOfScore = -1;
	for (int iRing = 0; iRing <= mnRings; iRing++)
	{
		int iStart = max(0, iRing - 2);
		int iLast = min(iRing + 2, mnRings);
		float maxVal = IGNORE_RING;
		float minVal = IGNORE_RING;
		for (int iVal = iStart; iVal <= iLast; iVal++)
		{
			float value = mvRingMean[iVal];
			if (value != IGNORE_RING)
			{
				if (maxVal == IGNORE_RING)
				{
					maxVal = value;
					minVal = value;
				}
				else
				{
					if (value > maxVal)
						maxVal = value;
					else if (value < minVal)
						minVal = value;
				}
			}
		}
		float diff = maxVal - minVal;
		mvRingScore[iRing] = diff;
		if (diff > maxDiff)
		{
			maxDiff = diff;
			iRingOfScore = iRing;
		}
	}
	mScore.mScore = maxDiff;
	mScore.miRing = iRingOfScore;
}
