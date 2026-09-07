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
void CScorerBase::ComputeRingScores(const vector<CRingInfo>& vRingsInfo, bool bEnoughData)
{
	std::fill(mvRingScore.begin(), mvRingScore.end(), 0.0f);

	if (bEnoughData)
	{
		mpRingsInfo = &vRingsInfo;
		ComputeScore();
		CorrectCenter(vRingsInfo);
	}
}
void CScorerBase::CorrectCenter(const vector<CRingInfo>& vRingsInfo)
{
	if (gConfig.mnCentralRings < 1 || gConfig.mnOffCenterRings < 1)
		return;

	// Find data range off-center area
	int iFirstRingOffCenter = gConfig.mnCentralRings;
	STRange<int> offDataRange = ComputeDataRange(iFirstRingOffCenter, gConfig.mnOffCenterRings, vRingsInfo);

	for (int iCentralRing = 0; iCentralRing < gConfig.mnCentralRings; iCentralRing++)
	{
		if (mvRingScore[iCentralRing] == 0)
			continue;

		float deviation = offDataRange.AbsDeviation(mvRingMean[iCentralRing]);
		if (mvRingScore[iCentralRing] > deviation)
		{
			gfLog.Printf("<CScorerBase::CorrectCenter> ring %d score clipped from %.2f to %.2f", iCentralRing, mvRingScore[iCentralRing], deviation);
			mvRingScore[iCentralRing] = deviation;
		}
	}

}
STRange<int> CScorerBase::ComputeDataRange(int iFrom, int n, const std::vector<CRingInfo>& vRingsInfo)
{
	STRange<int> range(vRingsInfo[iFrom].mMin, vRingsInfo[iFrom].mMax);

	for (int iRing = iFrom + 1; iRing < iFrom + n; iRing++)
		range.Add(vRingsInfo[iRing].mMin, vRingsInfo[iRing].mMax);
	return range;
}
