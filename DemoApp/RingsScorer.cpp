#include "stdafx.h"
#include "RingsScorer.h"
#include "RadiusImage.h"
#include "ArinetaImages.h"
#include "ImageRingsScorer.h"
#include "Config.h"
#include "DemoAppDlg.h"
#include <string>
#include <format>

using namespace std;

CRingsScorer::CRingsScorer(CArinetaImages* pImages)
	: mpImages(pImages)
{
	mpRadiusImage = new CRadiusImage(*pImages);
}
CRingsScorer::~CRingsScorer()
{
	delete mpRadiusImage;
}
float CRingsScorer::ScoreCurrentImage(int iImage, int& oAtRing)
{
	if (mbScoresComputed)
	{
		oAtRing = mvScores[iImage - 1].miRing;
		return mvScores[iImage - 1].mScore;
	}

	CImageRingScorer scorer(mpImages, iImage, mpRadiusImage);
	float score = scorer.Score();
	oAtRing = scorer.miRingOfScore;
	return score;

}
int CRingsScorer::ScoreAllImages()
{
	miFirst = mpImages->GetFirst();
	miLast = mpImages->GetLast();
	mStep = mpImages->GetStep();
	mnImages = mpImages->GetNFiles();

	for (int iImage = miFirst; iImage <= miLast; iImage += mStep)
	{
		mpImages->SetCurrent(iImage);
		int iRing = -1;
		float score = ScoreCurrentImage(iImage, iRing);
		mvScores.push_back(CImageScore(score, iRing));

		if (iImage == miFirst || score > mMaxScore)
		{
			mMaxScore = score;
			miImageWithMaxScore = iImage;
		}

		if (iImage % 10 == 0)
		{
			string s(format("Scoring image {}...", iImage));
			gConfig.PrintStatus(s.c_str());
		}
	}
	
	FindPeaks();
	OrderPeaks();
	Log();

	mbScoresComputed = true;
	miCurrentPeak = 1;

	string s(format("All {} images scored", mnImages));
	gConfig.PrintStatus(s.c_str());

	return miImageWithMaxScore;
}
void CRingsScorer::FindPeaks()
{
	for (int iImage = 1; iImage < mnImages - 1; iImage++)
	{
		float prev = mvScores[iImage - 1].mScore;
		float cur = mvScores[iImage].mScore;
		float next = mvScores[iImage + 1].mScore;
		if (prev > 0 && cur > 0 && next > 0) // All valid scores
		{
			if (cur >= prev && cur >= next)
			{
				mvScores[iImage].mbPeak = true;
				mnPeaks++;
			}
		}
	}
}
void CRingsScorer::OrderPeaks()
{
	mnPeaksOrdered = 0;
	mnRealPeaks = 0;
	while (mnPeaksOrdered < mnPeaks)
	{
		FindNextPixToOrder();
	}
}
void CRingsScorer::FindNextPixToOrder()
{
	int iNext = -1;
	float nextMaxScore = 0;
	for (int iImage = 1; iImage < mnImages-1; iImage++) // Peaks can not come in first or last image
	{
		if (mvScores[iImage].mbPeak && !mvScores[iImage].miPeak)
		{
			float score = mvScores[iImage].mScore;
			if (score > nextMaxScore)
			{
				nextMaxScore = score;
				iNext = iImage;
			}
		}
	}
	if (iNext >= 0)
	{
		if (!mvScores[iNext - 1].mbPeak)
			mnRealPeaks++;
		else
			gfLog.Printf("<CRingsScorer::FindNextPixToOrder> found wide peak %d at %d", mnRealPeaks, iNext);
		mvScores[iNext].miPeak = mnRealPeaks;
		mnPeaksOrdered++;
	}
}
void CRingsScorer::Log()
{
	FILE* pfLog = nullptr;
	fopen_s(&pfLog, "D:\\Log\\ScoreAllImages.csv", "w");
	if (!pfLog)
		return;

	fprintf(pfLog, "image, score, ring, peak\n");
	for (int iImage = 0; iImage < mnImages; iImage++)
	{
		int iOriginal = miFirst + iImage * mStep;
		fprintf(pfLog, "%d, %.2f, %d, %s, %d\n", iOriginal, 
			mvScores[iImage].mScore, 
			mvScores[iImage].miRing,
			mvScores[iImage].mbPeak ? "Peak" : "-",
			mvScores[iImage].miPeak
			);
	}
	fclose(pfLog);
}
void CRingsScorer::DisplayMaxPeak()
{
	LookForPeak(1);
}
void CRingsScorer::DisplayNextPeak()
{
	LookForPeak(miCurrentPeak + 1);
}
void CRingsScorer::DisplayPrevPeak()
{
	if (miCurrentPeak > 1)
		LookForPeak(miCurrentPeak - 1);
	else
		gConfig.PrintStatus("Max peak already displayed");
}
bool CRingsScorer::LookForPeak(int iWantedPeak)
{
	for (int iImage = 0; iImage < mnImages; iImage++)
	{
		if (mvScores[iImage].miPeak == iWantedPeak)
		{
			miCurrentPeak = iWantedPeak;
			miCurrentPeakImage = iImage;
			int iOriginal = miFirst + iImage * mStep;
			gfLog.Printf("<CRingsScorer::LookForPeak> Found peak %d at image %d original %d", iWantedPeak, iImage, iOriginal);

			string s(format("Fount peak {} at image {}", miCurrentPeak, iOriginal));
			gConfig.PrintStatus(s.c_str());
			gpDlg->OnCurrentSelectedByScorer(iOriginal);
			return true;
		}
	}

	gfLog.Printf("<CRingsScorer::LookForPeak> Failed to find peak %d", iWantedPeak);
	return false;
}