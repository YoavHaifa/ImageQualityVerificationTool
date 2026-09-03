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
	, mWeight(gConfig.GetScorerWeight(eScoreType))
{
	mvRingScore.assign(vRingMean.size(), 0.0f);
}

void CScorerBase::Score(int iImage, const vector<CRingInfo>& vRingsInfo, bool bEnoughData)
{
	mScore = CImageScore(); // Start clean score for every new image
	std::fill(mvRingScore.begin(), mvRingScore.end(), 0.0f);

	if (bEnoughData)
	{
		// Scorer specific computations
		mpRingsInfo = &vRingsInfo;
		ComputeScore();

		// For all scorers again
		CorrectCenter(vRingsInfo);
	}
	FindMaxScorePerCurrentImage();

	// Weight this scorer's score before anyone else (e.g. CAllMaxScorer, later in the same
	// per-image loop) reads mScore.mScore, and before it's filed into mResults/logged
	mScore.mRawScore = mScore.mScore;
	mScore.mScore *= mWeight;

	// Files the score just computed by Score() into mResults, under iImage. mScore.meSourceType
	// is left at its default (N_SCORE_TYPES) for every scorer except CAllMaxScorer, which sets
	// it directly in its own ComputeScore().
	mResults.AddScore(mScore, iImage);
}
void CScorerBase::LogAllImages(int iFirst, int iStep) const
{
	string sfName(format("{}\\ScoreAllImages_{}.csv", gConfig.msCaseLogDir.c_str(), Name()));

	FILE* pfLog = nullptr;
	fopen_s(&pfLog, sfName.c_str(), "w");
	if (!pfLog)
		return;

	fprintf(pfLog, "image, raw_score, score, ring, peak, peak_order\n");
	for (int iImage = 0; iImage < mResults.NumImages(); iImage++)
	{
		int iOriginal = iFirst + iImage * iStep;
		const CImageScore& score = mResults[iImage];
		fprintf(pfLog, "%d, %.6f, %.6f, %d, %s, %d\n", iOriginal, score.mRawScore, score.mScore, score.miRing, score.mbPeak ? "Peak" : "-", score.miPeak);
	}
	fclose(pfLog);
}
bool CScorerBase::LoadSavedResults(const char* zCaseDir, float dataRangeFactor)
{
	string sfName(format("{}\\ScoreAllImages_{}.csv", zCaseDir, Name()));

	FILE* pf = nullptr;
	fopen_s(&pf, sfName.c_str(), "r");
	if (!pf)
		return false;

	char zLine[256];
	fgets(zLine, sizeof(zLine), pf); // header

	while (fgets(zLine, sizeof(zLine), pf))
	{
		int iOriginal, iRing;
		float rawScore, oldWeightedScore;
		if (sscanf_s(zLine, "%d, %f, %f, %d", &iOriginal, &rawScore, &oldWeightedScore, &iRing) == 4)
		{
			// Re-weight from the saved raw score with mWeight and dataRangeFactor as they are
			// *now*, rather than trust the saved weighted column - lets a changed
			// ScorerWeights.csv (or a changed data range correction) take effect on replay
			// without rescoring. AddScore's iImage becomes mResults.miImageWithMaxScore verbatim
			// if this is the best score so far, and a live scoring pass always stores the
			// *original* DICOM slice number there (see CImageRingsScorer::Score) - not a 0-based
			// index - so this must match, even though mvScores itself is still indexed by push order.
			CImageScore score;
			score.mRawScore = rawScore;
			score.mScore = rawScore * mWeight * dataRangeFactor;
			score.miRing = iRing;
			mResults.AddScore(score, iOriginal);
		}
	}
	fclose(pf);
	return true;
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
