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
		oAtRing = mvScores[iImage - 1].mvRing[(int)gConfig.mScoreType];
		return mvScores[iImage - 1].mvScore[(int)gConfig.mScoreType];
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
		CImageRingScorer scorer(mpImages, iImage, mpRadiusImage);
		scorer.Score();
		mvScores.push_back(CImageScore1(scorer.mvScoreByType, scorer.mvRingByType));

		float activeScore = mvScores.back().mvScore[(int)gConfig.mScoreType];
		if (iImage == miFirst || activeScore > mMaxScore)
		{
			mMaxScore = activeScore;
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
	int iType = (int)gConfig.mScoreType;
	for (int iImage = 1; iImage < mnImages - 1; iImage++)
	{
		float prev = mvScores[iImage - 1].mvScore[iType];
		float cur = mvScores[iImage].mvScore[iType];
		float next = mvScores[iImage + 1].mvScore[iType];
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
	int iType = (int)gConfig.mScoreType;
	int iNext = -1;
	float nextMaxScore = 0;
	for (int iImage = 1; iImage < mnImages-1; iImage++) // Peaks can not come in first or last image
	{
		if (mvScores[iImage].mbPeak && !mvScores[iImage].miPeak)
		{
			float score = mvScores[iImage].mvScore[iType];
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
	string sfName(format("D:\\Log\\ScoreAllImages_{}.csv", ScoreTypeName(gConfig.mScoreType)));
	fopen_s(&pfLog, sfName.c_str(), "w");
	if (!pfLog)
		return;

	fprintf(pfLog, "image, minmax_score, minmax_ring, tent_score, tent_ring, peak, peak_order\n");
	for (int iImage = 0; iImage < mnImages; iImage++)
	{
		int iOriginal = miFirst + iImage * mStep;
		fprintf(pfLog, "%d, %.2f, %d, %.2f, %d, %s, %d\n", iOriginal,
			mvScores[iImage].mvScore[(int)EScoreType::MinMax],
			mvScores[iImage].mvRing[(int)EScoreType::MinMax],
			mvScores[iImage].mvScore[(int)EScoreType::Tent],
			mvScores[iImage].mvRing[(int)EScoreType::Tent],
			mvScores[iImage].mbPeak ? "Peak" : "-",
			mvScores[iImage].miPeak
			);
	}
	fclose(pfLog);
}
void CRingsScorer::ResetPeaks()
{
	for (auto& score : mvScores)
	{
		score.mbPeak = false;
		score.miPeak = 0;
	}
	mnPeaks = 0;
	mnPeaksOrdered = 0;
	mnRealPeaks = 0;
}
void CRingsScorer::OnActiveScoreTypeChanged()
{
	if (!mbScoresComputed)
		return;

	ResetPeaks();
	FindPeaks();
	OrderPeaks();
	Log();

	DisplayMaxPeak();
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