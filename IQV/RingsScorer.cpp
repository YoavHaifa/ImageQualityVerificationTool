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
#include "..\..\yUtils\YamlParser.h"
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
	{
		// iImage is the original DICOM slice number - convert to the 0-based push-order index
		// mResults is actually indexed by (see CScoreTypeResults::AddScore). Assuming push order
		// == iImage - 1 (i.e. slice numbering always starts at 1) used to be wrong for any case
		// whose numbering doesn't start at 1 - e.g. a Label "Save Section" case, whose files keep
		// their original slice numbers (say 143-160) - and crashed with an out-of-range access.
		int iPushOrder = (iImage - miFirst) / mStep;
		return mpImageScorer->GetCurrentScore(iPushOrder);
	}

	return mpImageScorer->Score(iImage);
}
int CRingsScorer::ScoreAllImages()
{
	CMyWindows::PrintStatus("Scoring...");
	miFirst = mpImages->GetFirst();
	miLast = mpImages->GetLast();
	mStep = mpImages->GetStep();
	mnImages = mpImages->GetNFiles();

	// Drain any stale count left over from an earlier, unrelated action (e.g. a refused
	// "load old results" attempt just before this run) - GetMessBoxCount() is a sticky global
	// counter, so without this a leftover box from before this run started would otherwise be
	// mistaken below for a real per-image failure in *this* run.
	CMyWindows::GetMessBoxCount(nullptr);
	msLastAbortReason.Empty();

	mpImageScorer->PrepareRingMeanProfile(mnImages);

	for (int iImage = miFirst; iImage <= miLast; iImage += mStep)
	{
		mpImages->SetCurrent(iImage);
		mpImageScorer->Score(iImage);

		// A message box just fired for this image (e.g. ImageRLib's "GetData NULL" on
		// undecodable data) - abort the rest of this case rather than plow through more
		// images we already know can't be decoded. Capture its exact text so the caller can
		// report the real reason instead of a generic failure message.
		if (CMyWindows::GetMessBoxCount(&msLastAbortReason) > 0)
		{
			gfLog.Printf("<CRingsScorer::ScoreAllImages> Aborting at image %d: %s", iImage, (LPCTSTR)msLastAbortReason);
			return -1;
		}

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
	mpImages->DumpRingMeanProfile();

	mpImageScorer->OnAllImagesScored();

	// Normalize scores against the case's own pixel-value spread, so scores become comparable
	// across cases: a narrower main area means a more sharply-defined water phantom, so the same
	// raw ring deviation is more significant than in a case with a wide, blurry main area
	miMainAreaWidth = mpImageScorer->GetHistogramMainArea(gConfig.mHistogramCutPercent).Width();
	mDataRangeScoreFactor = ComputeDataRangeScoreFactor(miMainAreaWidth);
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
float CRingsScorer::ComputeDataRangeScoreFactor(int width)
{
	float widthF = (float)width;
	return 1000.0f / (widthF * widthF);
}
int CRingsScorer::LoadFromSavedResults(const char* zCaseDir)
{
	miFirst = mpImages->GetFirst();
	miLast = mpImages->GetLast();
	mStep = mpImages->GetStep();
	mnImages = mpImages->GetNFiles();

	// The main area width itself (not the full per-image histogram) is read back and re-run
	// through ComputeDataRangeScoreFactor() as it is *now* - rather than trusting the persisted
	// data_range_score_factor verbatim - so a later change to that formula takes effect on
	// replay of old cases too, without rescoring.
	CYamlParser parser;
	string sYamlName(string(zCaseDir) + "\\CaseInfo.yaml");
	if (parser.Parse(sYamlName.c_str()))
	{
		if (CYamlLine* pMainArea = parser.GetRoot()->GetFirst("main_area"))
			pMainArea->GetValue("width", miMainAreaWidth);
		mDataRangeScoreFactor = ComputeDataRangeScoreFactor(miMainAreaWidth);

		// Reconstruct (an approximation of) the CT-per-radius 3rd viewer column from the compact
		// per-image ring-mean profile saved when this case was originally scored - replay never
		// recomputes per-ring means itself, so this is the only way Review can show it at all.
		if (gConfig.mbDisplayCtPerRadiusInReview)
		{
			int nRings = 0;
			CString sProfileFile;
			parser.GetRoot()->GetValue("n_rings", nRings);
			parser.GetRoot()->GetValue("ring_mean_profile_file", sProfileFile);
			if (nRings > 0 && !sProfileFile.IsEmpty())
				mpImages->LoadCtPerRadiusVolumeFromProfile(sProfileFile, mnImages, nRings, *mpRadiusImage);
		}
	}

	// Every scorer's own LoadSavedResults() already handles a missing CSV gracefully (returns
	// false, leaves it unscored) - no need to pre-filter by name. Iterating in registration
	// order (not whatever order CaseInfo.yaml happens to list names in) also guarantees
	// CAllMaxScorer - always last, see CImageRingsScorer::CreateScorers() - loads only after
	// every sibling it depends on has already replayed its own (possibly newly-reweighted)
	// results; and since its override doesn't read a CSV at all, this also lets it compute
	// itself for a case scored before AllMax existed, not just ones that already had it.
	for (int iScorer = 0; iScorer < mpImageScorer->GetNScorers(); iScorer++)
		mpImageScorer->GetScorerByIndex(iScorer)->LoadSavedResults(zCaseDir, mDataRangeScoreFactor);

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
			fprintf(pfLog, ", %.6f, %d, %s, %d", score.mScore, score.miRing, score.mbPeak ? "Peak" : "-", score.miPeak);
		}
		fprintf(pfLog, "\n");
	}
	fclose(pfLog);

	gfLog.Printf("<CRingsScorer::Log> Saved %s", sfName.c_str());
}
void CRingsScorer::LogPerScorer()
{
	// One file per scorer, listing every image's score/ring/peak data - unlike Log() above,
	// this doesn't need to know the concrete set of score types, so adding or removing a
	// scorer needs no change here. Meant to let a later batch run's results be reloaded and
	// displayed without rescoring.
	for (int iScorer = 0; iScorer < mpImageScorer->GetNScorers(); iScorer++)
		mpImageScorer->GetScorerByIndex(iScorer)->LogAllImages(miFirst, mStep);

	gfLog.Printf("<CRingsScorer::LogPerScorer> Saved %d ScoreAllImages_<scorer>.csv file(s) under %s",
		mpImageScorer->GetNScorers(), gConfig.msCaseLogDir.c_str());
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
	fprintf(pfLog, "n_rings: %d\n", mpImageScorer->GetNRings());
	fprintf(pfLog, "ring_mean_profile_file: %s\n", (LPCTSTR)mpImages->GetRingMeanProfileDumpName());

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

	gfLog.Printf("<CRingsScorer::LogCaseInfo> Saved %s", sfName.c_str());
}
float CRingsScorer::GetWorstScore(EScoreType eScoreType) const
{
	return mpImageScorer->GetWorstScore(eScoreType);
}
const CImageScore& CRingsScorer::GetScoreAtMax(EScoreType eScoreType) const
{
	return mpImageScorer->GetScoreAtMax(eScoreType);
}
float CRingsScorer::GetRawScoreAt(EScoreType eScoreType, int iOriginalImage) const
{
	return mpImageScorer->GetRawScoreAt(eScoreType, iOriginalImage);
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