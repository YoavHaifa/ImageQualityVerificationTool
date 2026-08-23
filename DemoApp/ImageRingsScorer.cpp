#include "stdafx.h"
#include "ImageRingsScorer.h"
#include "Config.h"
#include "ArinetaImages.h"
#include "RadiusImage.h"
#include "MinMaxScorer.h"
#include "TentScorer.h"
#include "..\..\yUtils\MyWindows.h"
#include <string>
#include <cmath>
#include <format>
#include "..\..\ImageRLib\Mask.h"


using namespace std;

CImageRingsScorer::CImageRingsScorer(CArinetaImages* pImages, CRadiusImage* pRadiusImage)
	: mpImages(pImages)
	, mpRadiusImage(pRadiusImage)
	, mHistogram("PixelValues", gConfig.mHistogramMin, gConfig.mHistogramMax)
{
	mnRings = (int)mpRadiusImage->mMaxRadius;

	// Sized once - never resized again, so the scorers' cached references/sizes stay valid for reuse
	mvRingMean.resize(mnRings + 1);
	CreateScorers();
}
CImageRingsScorer::~CImageRingsScorer()
{
}
const CImageScore& CImageRingsScorer::Score(int iImage)
{
	miImage = iImage;

	CollectRingsInfo();

	// Expand area of illegal samples
	ErodeValidArea();

	for (auto& pScorer : mvScorers)
		pScorer->Score(iImage, mvRingsInfo);

	if (gConfig.mbLogImageRingDetails)
		Log();

	return GetScorer(gConfig.mScoreType)->mScore;
}
void CImageRingsScorer::OnAllImagesScored()
{
	for (auto& pScorer : mvScorers)
		pScorer->OnAllImagesScored();
}
void CImageRingsScorer::ScaleScores(float factor)
{
	for (auto& pScorer : mvScorers)
		pScorer->ScaleScores(factor);
}
void CImageRingsScorer::LogHistogram()
{
	// Suffixed by case index (when scoring as part of a batch) so several cases' Histogram.csv
	// don't collide on name - Excel refuses to have two same-named workbooks open at once, even
	// from different folders.
	string sfName = gConfig.miCaseIndex > 0
		? format("{}\\Histogram_{}.csv", gConfig.msCaseLogDir.c_str(), gConfig.miCaseIndex)
		: format("{}\\Histogram.csv", gConfig.msCaseLogDir.c_str());
	FILE* pf = NULL;
	fopen_s(&pf, sfName.c_str(), "w");
	if (!pf)
		return;

	fprintf(pf, "value, count\n");
	for (int i = 0; i < mHistogram.mLen; i++)
		fprintf(pf, "%d, %d\n", (int)(mHistogram.mBase + i * mHistogram.mDelta), mHistogram.mpCounters[i]);
	fclose(pf);
}
const CImageScore& CImageRingsScorer::GetCurrentScore(int iImage) const
{
	return GetScorer(gConfig.mScoreType)->mResults[iImage - 1];
}
const CImageScore& CImageRingsScorer::GetScore(EScoreType eScoreType, int iImage) const
{
	return GetScorer(eScoreType)->mResults[iImage];
}
int CImageRingsScorer::GetImageWithMaxScore() const
{
	return GetScorer(gConfig.mScoreType)->mResults.miImageWithMaxScore;
}
int CImageRingsScorer::FindImageIndexOfPeak(int iWantedPeak) const
{
	return GetScorer(gConfig.mScoreType)->mResults.FindImageIndexOfPeak(iWantedPeak);
}
void CImageRingsScorer::CreateScorers()
{
	// The one place that knows the concrete scorer types - everything else operates on them generically
	mvScorers.clear();
	mvScorers.push_back(std::make_unique<CMinMaxScorer>(mvRingMean));
	mvScorers.push_back(std::make_unique<CTentScorer>(mvRingMean));
}
void CImageRingsScorer::CollectRingsInfo()
{
	mnPixelsWithinThreshold = 0;
	mvRingMean0.resize(mnRings + 1);
	int nToCheck = mpRadiusImage->mnPixels;
	float* pRadiusRaster = mpRadiusImage->GetData();
	short* pImageRaster = mpImages->GetImageRaster(miImage);
	mvRingsInfo.assign(mnRings + 1, CRingInfo()); // reset, not just resize - Add() accumulates per call

	mHistogram.Add(pImageRaster, nToCheck);

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
void CImageRingsScorer::ErodeValidArea()
{
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
}
void CImageRingsScorer::Log()
{
	for (const auto& pScorer : mvScorers)
	{
		string sDir(format("{}\\{}", gConfig.msCaseLogDir.c_str(), pScorer->Name()));
		CMyWindows::VerifyDirectory(sDir.c_str());

		string sfName(format("{}\\ImageScorer_{:03d}.csv", sDir.c_str(), miImage));
		FILE* pf = NULL;
		fopen_s(&pf, sfName.c_str(), "w");
		if (!pf)
			continue;

		const std::vector<float>& vScore = pScorer->mvRingScore;
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