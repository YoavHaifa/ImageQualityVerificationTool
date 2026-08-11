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
	mpImageScorer = new CImageRingsScorer(pImages, mpRadiusImage);
}
CRingsScorer::~CRingsScorer()
{
	delete mpImageScorer;
	delete mpRadiusImage;
}
float CRingsScorer::ScoreCurrentImage(int iImage, int& oAtRing)
{
	if (mbScoresComputed)
	{
		const CImageScore& score = mvScoreResults[(int)gConfig.mScoreType][iImage - 1];
		oAtRing = score.miRing;
		return score.mScore;
	}

	float score = mpImageScorer->Score(iImage);
	oAtRing = mpImageScorer->miRingOfScore;
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
		mpImageScorer->Score(iImage);

		for (int iType = 0; iType < N_SCORE_TYPES; iType++)
			mvScoreResults[iType].AddScore(mpImageScorer->mvScoreByType[iType], mpImageScorer->mvRingByType[iType], iImage);

		if (iImage % 10 == 0)
		{
			string s(format("Scoring image {}...", iImage));
			gConfig.PrintStatus(s.c_str());
		}
	}

	for (int iType = 0; iType < N_SCORE_TYPES; iType++)
	{
		mvScoreResults[iType].FindPeaks();
		mvScoreResults[iType].OrderPeaks();
	}
	Log();

	mbScoresComputed = true;
	miCurrentPeak = 1;

	string s(format("All {} images scored", mnImages));
	gConfig.PrintStatus(s.c_str());

	return mvScoreResults[(int)gConfig.mScoreType].miImageWithMaxScore;
}
void CRingsScorer::Log()
{
	FILE* pfLog = nullptr;
	string sfName("D:\\Log\\ScoreAllImages.csv");
	fopen_s(&pfLog, sfName.c_str(), "w");
	if (!pfLog)
		return;

	fprintf(pfLog, "image");
	for (int iType = 0; iType < N_SCORE_TYPES; iType++)
	{
		const char* zName = ScoreTypeName((EScoreType)iType);
		fprintf(pfLog, ", %s_score, %s_ring, %s_peak, %s_peak_order", zName, zName, zName, zName);
	}
	fprintf(pfLog, "\n");

	for (int iImage = 0; iImage < mnImages; iImage++)
	{
		int iOriginal = miFirst + iImage * mStep;
		fprintf(pfLog, "%d", iOriginal);
		for (int iType = 0; iType < N_SCORE_TYPES; iType++)
		{
			const CImageScore& score = mvScoreResults[iType][iImage];
			fprintf(pfLog, ", %.2f, %d, %s, %d", score.mScore, score.miRing, score.mbPeak ? "Peak" : "-", score.miPeak);
		}
		fprintf(pfLog, "\n");
	}
	fclose(pfLog);
}
void CRingsScorer::OnActiveScoreTypeChanged()
{
	if (!mbScoresComputed)
		return;

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
	int iImage = mvScoreResults[(int)gConfig.mScoreType].FindImageIndexOfPeak(iWantedPeak);
	if (iImage < 0)
	{
		gfLog.Printf("<CRingsScorer::LookForPeak> Failed to find peak %d", iWantedPeak);
		return false;
	}

	miCurrentPeak = iWantedPeak;
	miCurrentPeakImage = iImage;
	int iOriginal = miFirst + iImage * mStep;
	gfLog.Printf("<CRingsScorer::LookForPeak> Found peak %d at image %d original %d", iWantedPeak, iImage, iOriginal);

	string s(format("Fount peak {} at image {}", miCurrentPeak, iOriginal));
	gConfig.PrintStatus(s.c_str());
	gpDlg->OnCurrentSelectedByScorer(iOriginal);
	return true;
}