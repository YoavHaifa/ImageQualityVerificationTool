#include "stdafx.h"
#include "ImageRingsScorer.h"
#include "Config.h"
#include "ArinetaImages.h"
#include "RadiusImage.h"
#include "TentScorer.h"
#include "..\..\yUtils\MyWindows.h"
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
	for (int iType = 0; iType < N_SCORE_TYPES; iType++)
		mvRingScoreByType[iType].assign(mnRings + 1, 0.0f);

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
		mvRingScoreByType[(int)EScoreType::MinMax][iRing] = diff;
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
	CTentScorer scorer(mvRingMean);
	scorer.Score();

	mvScoreByType[(int)EScoreType::Tent] = scorer.mScore.mScore;
	mvRingByType[(int)EScoreType::Tent] = scorer.mScore.miRing;
	mvRingScoreByType[(int)EScoreType::Tent] = scorer.mvRingScore;
}
void CImageRingScorer::Log()
{
	for (int iType = 0; iType < N_SCORE_TYPES; iType++)
	{
		string sDir(format("{}\\{}", gConfig.msScoreGraphsDir.c_str(), ScoreTypeName((EScoreType)iType)));
		CMyWindows::VerifyDirectory(sDir.c_str());

		string sfName(format("{}\\ImageScorer_{:03d}.csv", sDir.c_str(), miImage));
		FILE* pf = NULL;
		fopen_s(&pf, sfName.c_str(), "w");
		if (!pf)
			continue;

		const std::vector<float>& vScore = mvRingScoreByType[iType];
		fprintf(pf, "i, n check, n summed, sum, avg, diff, min, max, score\n");
		for (int iLog = 0; iLog < mnRings; iLog++)
			fprintf(pf, "%d, %d, %d, %.2f, %.2f, %.2f, %d, %d, %.2f\n",
				iLog, mvRingsInfo[iLog].mnPixelsInRaster,
				mvRingsInfo[iLog].mnPixelsInRange,
				mvRingsInfo[iLog].mSum,
				mvRingMean0[iLog],
				mvRingsInfo[iLog].mDiff,
				mvRingsInfo[iLog].mMin,
				mvRingsInfo[iLog].mMax,
				vScore[iLog]);
		fclose(pf);
	}
}