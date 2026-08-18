#include "stdafx.h"
#include "RingsScorer.h"
#include "RadiusImage.h"
#include "ArinetaImages.h"
#include "ImageRingsScorer.h"
#include "ImageScore.h"
#include "ScorerBase.h"
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
const CImageScore& CRingsScorer::ScoreCurrentImage(int iImage)
{
	if (mbScoresComputed)
		return mpImageScorer->GetCurrentScore(iImage);

	return mpImageScorer->Score(iImage);
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

		if (iImage % 10 == 0)
		{
			string s(format("Scoring image {}...", iImage));
			gConfig.PrintStatus(s.c_str());
		}
	}

	mpImageScorer->OnAllImagesScored();
	Log();
	LogPerScorer();
	LogCaseInfo();

	mbScoresComputed = true;
	miCurrentPeak = 1;

	string s(format("All {} images scored", mnImages));
	gConfig.PrintStatus(s.c_str());

	return mpImageScorer->GetImageWithMaxScore();
}
void CRingsScorer::Log()
{
	FILE* pfLog = nullptr;
	string sfName(gConfig.msCaseLogDir + "\\ScoreAllImages.csv");
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
			const CImageScore& score = mpImageScorer->GetScore((EScoreType)iType, iImage);
			fprintf(pfLog, ", %.2f, %d, %s, %d", score.mScore, score.miRing, score.mbPeak ? "Peak" : "-", score.miPeak);
		}
		fprintf(pfLog, "\n");
	}
	fclose(pfLog);
}
void CRingsScorer::LogPerScorer()
{
	// One file per scorer, listing every image's score/ring/peak data - unlike Log() above,
	// this doesn't need to know the concrete set of score types, so adding or removing a
	// scorer needs no change here. Meant to let a later batch run's results be reloaded and
	// displayed without rescoring.
	for (int iScorer = 0; iScorer < mpImageScorer->GetNScorers(); iScorer++)
		mpImageScorer->GetScorerByIndex(iScorer)->LogAllImages(miFirst, mStep);
}
void CRingsScorer::LogCaseInfo()
{
	// Lets a case be found and its scores reviewed later without reopening it through the
	// GUI's File Open and rescoring it.
	FILE* pfLog = nullptr;
	string sfName(gConfig.msCaseLogDir + "\\CaseInfo.yaml");
	fopen_s(&pfLog, sfName.c_str(), "w");
	if (!pfLog)
		return;

	fprintf(pfLog, "case_path: %s\n", (LPCTSTR)mpImages->GetPath());
	fprintf(pfLog, "n_images: %d\n", mnImages);
	fprintf(pfLog, "scorers:\n");
	for (int iScorer = 0; iScorer < mpImageScorer->GetNScorers(); iScorer++)
	{
		CScorerBase* pScorer = mpImageScorer->GetScorerByIndex(iScorer);
		fprintf(pfLog, "  %s:\n", pScorer->Name());
		fprintf(pfLog, "    worst_score: %.2f\n", pScorer->mResults.mMaxScore);
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
	int iImage = mpImageScorer->FindImageIndexOfPeak(iWantedPeak);
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