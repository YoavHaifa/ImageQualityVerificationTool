#include "stdafx.h"
#include "ScorerBase.h"
#include "Config.h"
#include <string>
#include <format>

using namespace std;

CScorerBase::CScorerBase(const vector<float>& vRingMean, EScoreType eScoreType)
	: mvRingMean(vRingMean)
	, mnRings((int)vRingMean.size() - 1)
	, meScoreType(eScoreType)
{
	mvRingScore.assign(vRingMean.size(), 0.0f);
}

void CScorerBase::Score(int iImage, const vector<CRingInfo>& vRingsInfo)
{
	mScore = CImageScore(); // Start clean score for every new image
	std::fill(mvRingScore.begin(), mvRingScore.end(), 0.0f);

	// Scorer specific computations
	ComputeScore();

	// For all scorers again
	CorrectCenter(vRingsInfo);
	FindMaxScorePerCurrentImage();

	// Files the score just computed by Score() into mResults, under iImage
	mResults.AddScore(mScore.mScore, mScore.miRing, iImage);
}
void CScorerBase::LogAllImages(int iFirst, int iStep) const
{
	string sfName(format("{}\\ScoreAllImages_{}.csv", gConfig.msCaseLogDir.c_str(), Name()));

	FILE* pfLog = nullptr;
	fopen_s(&pfLog, sfName.c_str(), "w");
	if (!pfLog)
		return;

	fprintf(pfLog, "image, score, ring, peak, peak_order\n");
	for (int iImage = 0; iImage < mResults.NumImages(); iImage++)
	{
		int iOriginal = iFirst + iImage * iStep;
		const CImageScore& score = mResults[iImage];
		fprintf(pfLog, "%d, %.2f, %d, %s, %d\n", iOriginal, score.mScore, score.miRing, score.mbPeak ? "Peak" : "-", score.miPeak);
	}
	fclose(pfLog);
}
void CScorerBase::FindMaxScorePerCurrentImage()
{
	mScore.mScore = 0;
	mScore.miRing = -1;
	for (int iRing = 0; iRing <= mnRings; iRing++)
	{
		if (mvRingScore[iRing] > mScore.mScore)
		{
			mScore.mScore = mvRingScore[iRing];
			mScore.miRing = iRing;
		}
	}
}
void CScorerBase::CorrectCenter(const vector<CRingInfo>& vRingsInfo)
{
	if (gConfig.mnCentralRings < 1 || gConfig.mnOffCenterRings < 1)
		return;

	// Find data range off-center area
	int iFirstRingOffCenter = gConfig.mnCentralRings;
	SRange<int> offDataRange = ComputeDataRange(iFirstRingOffCenter, gConfig.mnOffCenterRings, vRingsInfo);

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
SRange<int> CScorerBase::ComputeDataRange(int iFrom, int n, const std::vector<CRingInfo>& vRingsInfo)
{
	SRange<int> range(vRingsInfo[iFrom].mMin, vRingsInfo[iFrom].mMax);
	
	for (int iRing = iFrom + 1; iRing < iFrom + n; iRing++)
		range.Add(vRingsInfo[iRing].mMin, vRingsInfo[iRing].mMax);
	return range;
}
