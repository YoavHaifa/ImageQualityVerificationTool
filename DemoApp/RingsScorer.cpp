#include "stdafx.h"
#include "RingsScorer.h"
#include "RadiusImage.h"
#include "ArinetaImages.h"
#include "ImageRingsScorer.h"
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
float CRingsScorer::ScoreCurrentImage(int& oAtRing)
{
	if (mbScoresComputed)
	{
		int iCurrent = mpImages->GetCurrentPosition();
		oAtRing = mvScores[iCurrent - 1].miRing;
		return mvScores[iCurrent - 1].mScore;
	}

	CTImage<unsigned short>* pImage = mpImages->GetImage();
	
	CImageRingScorer scorer(pImage, mpRadiusImage);
	float score = scorer.Score();
	oAtRing = scorer.miRingOfScore;
	return score;

}
int CRingsScorer::ScoreAllImages()
{
	int iFirst = mpImages->GetFirst();
	int iLast = mpImages->GetLast();
	int step = mpImages->GetStep();
	int nImages = mpImages->GetNFiles();

	FILE* pfLog;
	fopen_s(&pfLog, "D:\\Log\\ScoreAllImages.csv", "w");
	if (pfLog)
		fprintf(pfLog, "image, score, ring\n");

	for (int iImage = iFirst; iImage <= iLast; iImage += step)
	{
		mpImages->SetCurrent(iImage);
		int iRing = -1;
		float score = ScoreCurrentImage(iRing);
		mvScores.push_back(CImageScore(score, iRing));

		if (pfLog)
			fprintf(pfLog, "%d, %.2f, %d\n", iImage, score, iRing);

		if (iImage == iFirst || score > mMaxScore)
		{
			mMaxScore = score;
			miImageWithMaxScore = iImage;
		}

		if (iImage % 10 == 0)
		{
			string s(format("Scoring image {}...", iImage));
			CMyWindows::PrintStatus(s.c_str());
		}
	}
	mbScoresComputed = true;
	if (pfLog)
		fclose(pfLog);

	string s(format("All {} images scored", nImages));
	CMyWindows::PrintStatus(s.c_str());

	mpImages->SetCurrent(miImageWithMaxScore);
	return miImageWithMaxScore;
}