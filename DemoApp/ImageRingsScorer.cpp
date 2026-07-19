#include "stdafx.h"
#include "ImageRingsScorer.h"
#include "RadiusImage.h"
#include <string>
#include <cmath>


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

	// Compute score on rings
	// As first trial, just find the heighest jump in the function
	// where number of pixels in range is at least 50% OR bigger han 20
	float maxDiff = 0;
	for (int iRing = 1; iRing < mnRings - 2; iRing++)
	{
		float mean = vRingMean[iRing];
		float nextMean = vRingMean[iRing+3];
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
	return mScore;
}
void CImageRingScorer::CollectRingsInfo(vector<float>& vMean)
{
	int nToCheck = mpRadiusImage->mnPixels;
	float* pRadiusRaster = mpRadiusImage->GetData();
	unsigned short* pImageRaster = mpImage->GetData();
	vector<CRingInfo> vRingsInfo(mnRings+1);

	// Checvk all pixels in image
	for (int i = 0; i < nToCheck; i++)
	{
		int iRadius = (int)pRadiusRaster[i];
		vRingsInfo[iRadius].mnPixelsInRaster++;
		unsigned short value = pImageRaster[i];
		if (value >= umMinThreshold && value <= umMaxThreshold)
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