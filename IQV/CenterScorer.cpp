#include "stdafx.h"
#include "CenterScorer.h"
#include "Config.h"

CCenterScorer::CCenterScorer(const std::vector<float>& vRingMean)
	: CScorerBase(vRingMean, EScoreType::Center)
{
}
void CCenterScorer::ComputeScore()
{
	if (gConfig.mnCentralRings < 1 || gConfig.mnOffCenterRings < 1)
		return;

	// Candidate local max/min within the central rings - mnCentralRings only gates
	// where we look for these, it does not bound the scored area (see ScoreFromCandidate)
	int iLocalMax = -1, iLocalMin = -1;
	for (int iRing = 0; iRing < gConfig.mnCentralRings; iRing++)
	{
		float value = mvRingMean[iRing];
		if (value == IGNORE_RING)
			continue;
		if (iLocalMax == -1 || value > mvRingMean[iLocalMax])
			iLocalMax = iRing;
		if (iLocalMin == -1 || value < mvRingMean[iLocalMin])
			iLocalMin = iRing;
	}

	if (iLocalMax != -1)
		ScoreFromCandidate(iLocalMax, true);
	if (iLocalMin != -1)
		ScoreFromCandidate(iLocalMin, false);
}
void CCenterScorer::ScoreFromCandidate(int iCandidate, bool bHigh)
{
	float sum = 0;
	for (int iRing = iCandidate; iRing <= mnRings; iRing++)
	{
		float value = mvRingMean[iRing];
		if (value == IGNORE_RING)
			break;

		// Surrounding window never dips back into the central rings - they may be
		// part of the same artifact - so it starts sliding out only once iRing passes them
		int iRangeStart = max(gConfig.mnCentralRings, iRing + 1);
		if (iRangeStart + gConfig.mnOffCenterRings > mnRings + 1)
			break; // not enough rings left to form a surrounding window

		STRange<int> range = ComputeDataRange(iRangeStart, gConfig.mnOffCenterRings, *mpRingsInfo);

		float dev;
		if (bHigh)
		{
			if (value <= range.mMax)
				break; // back in range - artifact ends here
			dev = value - range.mMax;
		}
		else
		{
			if (value >= range.mMin)
				break;
			dev = range.mMin - value;
		}

		mvRingScore[iRing] = dev;
		sum += dev;
	}

	// Extra weight for wider artifacts: the candidate ring carries the whole area's score
	if (sum > 0)
		mvRingScore[iCandidate] = sum;
}
