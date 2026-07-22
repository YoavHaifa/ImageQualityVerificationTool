#include "stdafx.h"
#include "ImageRingsScorer.h"
#include "Config.h"
#include "RadiusImage.h"
#include <string>
#include <cmath>
#include "..\..\ImageRLib\Mask.h"


using namespace std;

CImageRingScorer::CImageRingScorer(CTImage<unsigned short>* pImage, CRadiusImage* pRadiusImage)
	: mpImage(pImage)
	, mpRadiusImage(pRadiusImage)
{
	mnRings = (int)mpRadiusImage->mMaxRadius;
}
CImageRingScorer::~CImageRingScorer()
{
}
float CImageRingScorer::Score()
{
	vector<float> vRingMean0(mnRings + 1);
	CollectRingsInfo(vRingMean0);

	// Expand area of illegal samples
	vector<float> vRingMean(mnRings + 1);
	vRingMean[0] = vRingMean0[0];
	vRingMean[mnRings] = vRingMean0[mnRings];
	for (int iR = 1; iR < mnRings; iR++)
	{
		float prev = vRingMean0[iR - 1];
		float next = vRingMean0[iR + 1];
		if (prev == IGNORE_RING && next == IGNORE_RING)
			vRingMean[iR] = IGNORE_RING;
		else
			vRingMean[iR] = vRingMean0[iR];
	}

	if (mbComputeByDiff)
		ComputeScoreByDiff(vRingMean);
	else
		ComputeScoreByMinMaxDiff(vRingMean);

	return mScore;
}
void CImageRingScorer::CollectRingsInfo(vector<float>& vMean)
{
	int nToCheck = mpRadiusImage->mnPixels;
	float* pRadiusRaster = mpRadiusImage->GetData();
	unsigned short* pImageRaster = mpImage->GetData();
	vector<CRingInfo> vRingsInfo(mnRings+1);

	int nLines = mpImage->GetNLines();
	int nCols = mpImage->GetNCols();
	CMask thresholdMask(nLines, nCols);
	CMask erodedMask(nLines, nCols);
	thresholdMask.Threshold(pImageRaster, gConfig.mMinThreshold, gConfig.mMaxThreshold);
	erodedMask.FastErode(thresholdMask, gConfig.mErodeLevel);
	unsigned char* mpMask = erodedMask.GetMaskRaster();

	// Check all pixels in image
	for (int i = 0; i < nToCheck; i++)
	{
		int iRadius = (int)pRadiusRaster[i];
		vRingsInfo[iRadius].mnPixelsInRaster++;
		unsigned short value = pImageRaster[i];
		if (mpMask[i])
		{
			mnPixelsWithinThreshold++;
			vRingsInfo[iRadius].Add(value);
		}
	}

	for (int iRing = 0; iRing < mnRings; iRing++)
	{
		int nSummed = vRingsInfo[iRing].mnPixelsInRange;
		if (nSummed < 2)
		{
			vMean[iRing] = IGNORE_RING;
		}
		else if (nSummed < (vRingsInfo[iRing].mnPixelsInRaster / 2) && nSummed < 50)
		{
			vMean[iRing] = IGNORE_RING;
		}
		else
		{
			vMean[iRing] = vRingsInfo[iRing].mSum / nSummed;
			if (iRing > 0)
			{
				float prev = vMean[iRing - 1];
				if (prev != IGNORE_RING)
					vRingsInfo[iRing].mDiff = abs(vMean[iRing] - prev);
			}
		}
	}

	if (mbLog)
	{
		string sfName("d:\\Log\\ImageScorer.csv");
		FILE* pf = NULL;
		fopen_s(&pf, sfName.c_str(), "w");
		if (!pf)
			return;

		fprintf(pf, "i, n check, n summed, sum, avg, diff\n");
		for (int iLog = 0; iLog < mnRings; iLog++)
			fprintf(pf, "%d, %d, %d, %.2f, %.2f, %.2f\n",
				iLog, vRingsInfo[iLog].mnPixelsInRaster, vRingsInfo[iLog].mnPixelsInRange,
				vRingsInfo[iLog].mSum, vMean[iLog], vRingsInfo[iLog].mDiff);
		fclose(pf);
	}
}
void CImageRingScorer::ComputeScoreByDiff(vector<float>& vRingMean)
{
	// Compute score on rings
	// As first trial, just find the heighest jump in the function
	// where number of pixels in range is at least 50% OR bigger han 20
	float maxDiff = 0;
	for (int iRing = 1; iRing < mnRings - 2; iRing++)
	{
		float mean = vRingMean[iRing];
		float nextMean = vRingMean[iRing + 3];
		if (mean != IGNORE_RING && nextMean != IGNORE_RING)
		{
			float absDiff = abs(nextMean - mean);
			if (absDiff > maxDiff)
			{
				maxDiff = absDiff;
				miRingOfScore = iRing;
			}
		}
	}
	mScore = maxDiff;
}
void CImageRingScorer::ComputeScoreByMinMaxDiff(vector<float>& vRingMean)
{
	//vector<float> vMax5(mnRings + 1);
	//vector<float> vMin5(mnRings + 1);

	float maxDiff = 0;
	for (int iRing = 0; iRing <= mnRings; iRing++)
	{
		int iStart = max(0, iRing - 2);
		int iLast = min(iRing + 2, mnRings);
		float maxVal = IGNORE_RING;
		float minVal = IGNORE_RING;
		for (int iVal = iStart; iVal <= iLast; iVal++)
		{
			float value = vRingMean[iVal];
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
		if (diff > maxDiff)
		{
			maxDiff = diff;
			miRingOfScore = iRing;
		}

	}
	mScore = maxDiff;
}