#include "stdafx.h"
#include "ImageRingsScorer.h"
#include "RadiusImage.h"
#include <vector>
#include <string>


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
	CollectRingsInfo();
	return 0;
}
void CImageRingScorer::CollectRingsInfo()
{
	int nToCheck = mpRadiusImage->mnPixels;
	float* pRadiusRaster = mpRadiusImage->GetData();
	unsigned short* pImageRaster = mpImage->GetData();
	vector<CRingInfo> vRingsInfo(mnRings);

	// Checvk all pixels in image
	for (int i = 0; i < nToCheck; i++)
	{
		unsigned short value = pImageRaster[i];
		if (value >= umMinThreshold && value <= umMaxThreshold)
		{
			mnPixelsWithinThreshold++;
			int iRadius = (int)pRadiusRaster[i];
			vRingsInfo[iRadius].Add(value);
		}
	}

	vector<float> vMean(mnRings+1);
	for (int iRing = 0; iRing < mnRings; iRing++)
	{
		int nInRing = vRingsInfo[iRing].mnValues;
		if (nInRing > 0)
			vMean[iRing] = vRingsInfo[iRing].mSum / nInRing;
		else
			vMean[iRing] = 0;
	}

	if (mbLog)
	{
		string sfName("d:\\Log\\ImageScorer.csv");
		FILE* pf = NULL;
		fopen_s(&pf, sfName.c_str(), "w");
		if (!pf)
			return;

		fprintf(pf, "i, n, sum, avg\n");
		for (int iLog = 0; iLog < mnRings; iLog++)
			fprintf(pf, "%d, %d, %.2f, %.2f\n",
				iLog, vRingsInfo[iLog].mnValues, vRingsInfo[iLog].mSum, vMean[iLog]);
		fclose(pf);
	}

}