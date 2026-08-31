#include "stdafx.h"
#include "RingsScorer.h"
#include "RadiusImage.h"
#include "ArinetaImages.h"
#include "ImageRingsScorer.h"
#include "ImageScore.h"
#include "ScorerBase.h"
#include "Config.h"
#include "IQVDlg.h"
#include "..\..\yUtils\MyWindows.h"
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
	CMyWindows::PrintStatus("Scoring...");
	miFirst = mpImages->GetFirst();
	miLast = mpImages->GetLast();
	mStep = mpImages->GetStep();
	mnImages = mpImages->GetNFiles();

	for (int iImage = miFirst; iImage <= miLast; iImage += mStep)
	{
		mpImages->SetCurrent(iImage);
		mpImageScorer->Score(iImage);

		// A message box just fired for this image (e.g. ImageRLib's "GetData NULL" on
		// undecodable data) - abort the rest of this case rather than plow through more
		// images we already know can't be decoded
		if (CMyWindows::GetMessBoxCount(nullptr) > 0)
			return -1;

		if (iImage % 10 == 0)
		{
			string s(format("Scoring image {}...", iImage));
			gConfig.PrintStatus(s.c_str());
		}
	}

	// Unconditional, purely so the volume is always available for offline debugging (and, if
	// gConfig.mbAvoidSharedMemory is on, for the viewer to display from instead of live shared
	// memory) - the wide volume dumps itself the same way, right after ComputeWideImages() computes it
	mpImages->DumpCtPerRadiusVolume();

	mpImageScorer->OnAllImagesScored();

	// Normalize scores against the case's own pixel-value spread, so scores become comparable
	// across cases: a narrower main area means a more sharply-defined water phantom, so the same
	// raw ring deviation is more significant than in a case with a wide, blurry main area
	miMainAreaWidth = mpImageScorer->GetHistogramMainArea(gConfig.mHistogramCutPercent).Width();
	float widthF = (float)miMainAreaWidth;
	mDataRangeScoreFactor = 1000.0f / (widthF * widthF);
	mpImageScorer->ScaleScores(mDataRangeScoreFactor);

	mpImageScorer->LogHistogram();
	Log();
	LogPerScorer();
	LogCaseInfo();

	mbScoresComputed = true;
	miCurrentPeak = 1;

	string s(format("All {} images scored", mnImages));
	gConfig.PrintStatus(s.c_str());

	return mpImageScorer->GetImageWithMaxScore();
}
int CRingsScorer::LoadFromSavedResults(const char* zCaseDir)
{
	miFirst = mpImages->GetFirst();
	miLast = mpImages->GetLast();
	mStep = mpImages->GetStep();
	mnImages = mpImages->GetNFiles();

	// Every scorer's own LoadSavedResults() already handles a missing CSV gracefully (returns
	// false, leaves it unscored) - no need to pre-filter by name. Iterating in registration
	// order (not whatever order CaseInfo.yaml happens to list names in) also guarantees
	// CAllMaxScorer - always last, see CImageRingsScorer::CreateScorers() - loads only after
	// every sibling it depends on has already replayed its own (possibly newly-reweighted)
	// results; and since its override doesn't read a CSV at all, this also lets it compute
	// itself for a case scored before AllMax existed, not just ones that already had it.
	for (int iScorer = 0; iScorer < mpImageScorer->GetNScorers(); iScorer++)
		mpImageScorer->GetScorerByIndex(iScorer)->LoadSavedResults(zCaseDir);

	// Peaks weren't read back from the CSVs - recompute them from the replayed scores,
	// same as a live run does once every image has been scored
	mpImageScorer->OnAllImagesScored();

	mbScoresComputed = true;
	miCurrentPeak = 1;

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

	fprintf(pfLog, "csv_version: %d\n", gConfig.mCsvVersion);
	fprintf(pfLog, "case_path: %s\n", (LPCTSTR)mpImages->GetPath());
	fprintf(pfLog, "n_images: %d\n", mnImages);

	STRange<int> histogramMainArea = mpImageScorer->GetHistogramMainArea(gConfig.mHistogramCutPercent);
	fprintf(pfLog, "main_area:\n");
	fprintf(pfLog, "  min: %d\n", histogramMainArea.mMin);
	fprintf(pfLog, "  max: %d\n", histogramMainArea.mMax);
	fprintf(pfLog, "  width: %d\n", miMainAreaWidth);
	fprintf(pfLog, "data_range_score_factor: %.4f\n", mDataRangeScoreFactor);

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
		gConfig.PrintStatus(format("no more relevant scores for {}", ScoreTypeName(gConfig.mScoreType)).c_str());
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