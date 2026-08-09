#include "stdafx.h"
#include "ImageRingsScorer.h"
#include "Config.h"
#include "ArinetaImages.h"
#include "RadiusImage.h"
#include <string>
#include <cmath>
#include <format>
#include "..\..\ImageRLib\Mask.h"


using namespace std;

CImageRingScorer::CImageRingScorer(CArinetaImages* pImages, int iImage, CRadiusImage* pRadiusImage)
	: mpImages(pImages)
	, miImage(iImage)
	, mpRadiusImage(pRadiusImage)
{
	mnRings = (int)mpRadiusImage->mMaxRadius;
}
CImageRingScorer::~CImageRingScorer()
{
}
float CImageRingScorer::Score()
{
	CollectRingsInfo();

	// Expand area of illegal samples
	mvRingMean.resize(mnRings + 1);
	mvRingScore.resize(mnRings + 1);

	mvRingMean[0] = mvRingMean0[0];
	mvRingMean[mnRings] = mvRingMean0[mnRings];
	for (int iR = 1; iR < mnRings; iR++)
	{
		float prev = mvRingMean0[iR - 1];
		float next = mvRingMean0[iR + 1];
		if (prev == IGNORE_RING && next == IGNORE_RING)
			mvRingMean[iR] = IGNORE_RING;
		else
			mvRingMean[iR] = mvRingMean0[iR];
	}

	ComputeScoreByMinMaxDiff();
	ComputeScoreByTent();

	mScore = mvScoreByType[(int)gConfig.mScoreType];
	miRingOfScore = mvRingByType[(int)gConfig.mScoreType];

	if (mbLog)
		Log();

	return mScore;
}
void CImageRingScorer::CollectRingsInfo()
{
	mvRingMean0.resize(mnRings + 1);
	int nToCheck = mpRadiusImage->mnPixels;
	float* pRadiusRaster = mpRadiusImage->GetData();
	short* pImageRaster = mpImages->GetImageRaster(miImage);
	mvRingsInfo.resize(mnRings+1);

	int nLines = mpImages->GetNLines();
	int nCols = mpImages->GetNCols();
	CMask thresholdMask(nLines, nCols);
	CMask erodedMask(nLines, nCols);
	thresholdMask.Threshold(pImageRaster, gConfig.mMinThreshold, gConfig.mMaxThreshold);
	erodedMask.FastErode(thresholdMask, gConfig.mErodeLevel);
	unsigned char* mpMask = erodedMask.GetMaskRaster();

	// Check all pixels in image
	for (int i = 0; i < nToCheck; i++)
	{
		int iRadius = (int)pRadiusRaster[i];
		mvRingsInfo[iRadius].mnPixelsInRaster++;
		short value = pImageRaster[i];
		if (mpMask[i])
		{
			mnPixelsWithinThreshold++;
			mvRingsInfo[iRadius].Add(value);
		}
	}

	for (int iRing = 0; iRing < mnRings; iRing++)
	{
		int nSummed = mvRingsInfo[iRing].mnPixelsInRange;
		if (nSummed < 2)
		{
			mvRingMean0[iRing] = IGNORE_RING;
		}
		else if (nSummed < (mvRingsInfo[iRing].mnPixelsInRaster / 2) && nSummed < 50)
		{
			mvRingMean0[iRing] = IGNORE_RING;
		}
		else
		{
			mvRingMean0[iRing] = mvRingsInfo[iRing].mSum / nSummed;
			if (iRing > 0)
			{
				float prev = mvRingMean0[iRing - 1];
				if (prev != IGNORE_RING)
					mvRingsInfo[iRing].mDiff = abs(mvRingMean0[iRing] - prev);
			}
		}
	}
}
void CImageRingScorer::ComputeScoreByDiff()
{
	// Compute score on rings
	// As first trial, just find the heighest jump in the function
	// where number of pixels in range is at least 50% OR bigger han 20
	float maxDiff = 0;
	for (int iRing = 1; iRing < mnRings - 2; iRing++)
	{
		float mean = mvRingMean[iRing];
		float nextMean = mvRingMean[iRing + 3];
		if (mean != IGNORE_RING && nextMean != IGNORE_RING)
		{
			float absDiff = abs(nextMean - mean);
			mvRingScore[iRing] = absDiff;
			if (absDiff > maxDiff)
			{
				maxDiff = absDiff;
				miRingOfScore = iRing;
			}
		}
	}
	mScore = maxDiff;
}
void CImageRingScorer::ComputeScoreByMinMaxDiff()
{
	//vector<float> vMax5(mnRings + 1);
	//vector<float> vMin5(mnRings + 1);

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
	mvScoreByType[(int)EScoreType::MinMax] = maxDiff;
	mvRingByType[(int)EScoreType::MinMax] = iRingOfScore;
}
void CImageRingScorer::ComputeScoreByTent()
{
	mvRingScoreTent.assign(mnRings + 1, 0.0f);

	// Valid (non-ignored) ring positions, in radius order
	std::vector<int> validIdx;
	for (int iRing = 0; iRing <= mnRings; iRing++)
		if (mvRingMean[iRing] != IGNORE_RING)
			validIdx.push_back(iRing);

	// Group consecutive equal-value rings into flat segments - a flat area is one unit,
	// the same way a run of consecutive equal-scoring images is treated as a single "wide peak"
	struct SSegment
	{
		size_t mkStart, mkEnd; // indices into validIdx
		float mValue;
	};
	std::vector<SSegment> segments;
	for (size_t k = 0; k < validIdx.size(); )
	{
		size_t j = k;
		while (j + 1 < validIdx.size() && mvRingMean[validIdx[j + 1]] == mvRingMean[validIdx[k]])
			j++;
		segments.push_back({ k, j, mvRingMean[validIdx[k]] });
		k = j + 1;
	}

	// Classify interior segments as local max/min (adjacent segments always differ in value)
	struct SExtremum { int miRing; float mValue; bool mbMax; };
	std::vector<SExtremum> extrema;
	for (size_t s = 1; s + 1 < segments.size(); s++)
	{
		float prev = segments[s - 1].mValue;
		float cur = segments[s].mValue;
		float next = segments[s + 1].mValue;
		bool bMax = cur > prev && cur > next;
		bool bMin = cur < prev && cur < next;
		if (bMax || bMin)
		{
			size_t kMid = (segments[s].mkStart + segments[s].mkEnd) / 2;
			extrema.push_back({ validIdx[kMid], cur, bMax });
		}
	}

	float maxTent = 0;
	int iRingOfTent = -1;
	for (size_t k = 1; k + 1 < extrema.size(); k++)
	{
		if (extrema[k - 1].mbMax == extrema[k].mbMax || extrema[k + 1].mbMax == extrema[k].mbMax)
			continue; // needs an opposite-type extremum on both sides to form a tent

		float diffLeft = abs(extrema[k].mValue - extrema[k - 1].mValue);
		float diffRight = abs(extrema[k].mValue - extrema[k + 1].mValue);
		float tent = (diffLeft + diffRight) / 2.0f;

		mvRingScoreTent[extrema[k].miRing] = tent;
		if (tent > maxTent)
		{
			maxTent = tent;
			iRingOfTent = extrema[k].miRing;
		}
	}

	mvScoreByType[(int)EScoreType::Tent] = maxTent;
	mvRingByType[(int)EScoreType::Tent] = iRingOfTent;
}
void CImageRingScorer::Log()
{
	string sfName(format("{}\\ImageScorer_{:03d}.csv", gConfig.msScoreGraphsDir.c_str(), miImage));
	FILE* pf = NULL;
	fopen_s(&pf, sfName.c_str(), "w");
	if (!pf)
		return;

	fprintf(pf, "i, n check, n summed, sum, avg, diff, min, max, minmax_score, tent_score\n");
	for (int iLog = 0; iLog < mnRings; iLog++)
		fprintf(pf, "%d, %d, %d, %.2f, %.2f, %.2f, %d, %d, %.2f, %.2f\n",
			iLog, mvRingsInfo[iLog].mnPixelsInRaster,
			mvRingsInfo[iLog].mnPixelsInRange,
			mvRingsInfo[iLog].mSum,
			mvRingMean0[iLog],
			mvRingsInfo[iLog].mDiff,
			mvRingsInfo[iLog].mMin,
			mvRingsInfo[iLog].mMax,
			mvRingScore[iLog],
			mvRingScoreTent[iLog]);
	fclose(pf);

}