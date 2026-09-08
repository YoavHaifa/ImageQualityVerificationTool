#include "stdafx.h"
#include "ImageRingsScorer.h"
#include "Config.h"
#include "ArinetaImages.h"
#include "RadiusImage.h"
#include "MinMaxScorer.h"
#include "TentScorer.h"
#include "CenterScorer.h"
#include "..\..\yUtils\MyWindows.h"
#include <string>
#include <cmath>
#include <format>
#include "..\..\ImageRLib\Mask.h"


using namespace std;

static const ERegion gaRingScorerRegions[3] = { ERegion::HighRes, ERegion::Border, ERegion::LowRes };

CImageRingsScorer::CImageRingsScorer(CArinetaImages* pImages, CRadiusImage* pRadiusImage)
	: mpImages(pImages)
	, mpRadiusImage(pRadiusImage)
	, mHistogram("PixelValues", gConfig.mHistogramMin, gConfig.mHistogramMax)
	, mErodedMask(pImages->GetNLines(), pImages->GetNCols())
{
	mnRings = (int)mpRadiusImage->mMaxRadius;

	// Sized once - never resized again, so the scorers' cached references/sizes stay valid for reuse
	mvRingMean.resize(mnRings + 1);
	CreateScorers();
	CreateResults();
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

	if (gConfig.mbDisplayCtPerRadius)
		FillCtPerRadiusImage();
	mpImages->RecordRingMeanProfileRow(miRingMeanProfileRow++, mvRingMean);

	bool bEnoughData = mnPixelsWithinThreshold >= gConfig.mnMinPixelsInMask;
	for (auto& pScorer : mvScorers)
		pScorer->ComputeRingScores(mvRingsInfo, bEnoughData);

	RecordRegionResults(iImage);
	RecordCenterResult(iImage);
	RecordAllMaxResult(iImage); // must run after the above - depends on every other result for this image

	if (gConfig.mbLogImageRingDetails)
		Log();

	return GetActiveScore(gConfig.mScoreType);
}
void CImageRingsScorer::OnAllImagesScored()
{
	for (CScorerResult& r : mvResults)
		r.mResults.OnAllImagesScored();
}
void CImageRingsScorer::ScaleScores(float factor)
{
	for (CScorerResult& r : mvResults)
		r.mResults.ScaleScores(factor);
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

	gfLog.Printf("<CImageRingsScorer::LogHistogram> Saved %s", sfName.c_str());
}
const CImageScore& CImageRingsScorer::GetCurrentScore(int iPushOrder) const
{
	return GetActiveScore(gConfig.mScoreType, iPushOrder);
}
const CImageScore& CImageRingsScorer::GetScore(EScoreType eScoreType, int iImage) const
{
	static CImageScore empty;
	const CImageScore* pBest = nullptr;
	for (const CScorerResult& r : mvResults)
	{
		if (r.meScoreType != eScoreType)
			continue;
		const CImageScore& cand = r.mResults[iImage];
		if (!pBest || cand.mScore > pBest->mScore)
			pBest = &cand;
	}
	return pBest ? *pBest : empty;
}
int CImageRingsScorer::GetImageWithMaxScore() const
{
	// Same image GetScoreAtMax(gConfig.mScoreType) itself points to - see CScoreTypeResults::AddScore
	return GetScoreAtMax(gConfig.mScoreType).miOriginalImage;
}
int CImageRingsScorer::FindImageIndexOfPeak(int iWantedPeak) const
{
	CScoreTypeResults merged = MergeResults(gConfig.mScoreType, /*bActiveOnly=*/true);
	return merged.FindImageIndexOfPeak(iWantedPeak);
}
int CImageRingsScorer::GetActiveImageWithMaxScore() const
{
	CScoreTypeResults merged = MergeResults(gConfig.mScoreType, /*bActiveOnly=*/true);
	if (merged.miImageWithMaxScore >= 0)
		return merged.miImageWithMaxScore;

	// Nothing is currently active at all (e.g. every region disabled) - land on the comprehensive
	// worst image rather than nowhere; its score will legitimately show as 0 under the active
	// filter, but that's far better than an invalid/negative position.
	return GetImageWithMaxScore();
}
float CImageRingsScorer::GetWorstScore(EScoreType eScoreType) const
{
	float best = 0;
	bool bFirst = true;
	for (const CScorerResult& r : mvResults)
	{
		if (r.meScoreType != eScoreType)
			continue;
		if (bFirst || r.mResults.mMaxScore > best)
		{
			best = r.mResults.mMaxScore;
			bFirst = false;
		}
	}
	return best;
}
float CImageRingsScorer::GetWorstScore(EScoreType eScoreType, ERegion region) const
{
	for (const CScorerResult& r : mvResults)
		if (r.meScoreType == eScoreType && r.meRegion == region)
			return r.mResults.mMaxScore;
	return 0;
}
void CImageRingsScorer::PrepareRingMeanProfile(int nTotalImages)
{
	miRingMeanProfileRow = 0;
	mpImages->EnsureRingMeanProfile(nTotalImages, mnRings);
}
const CImageScore& CImageRingsScorer::GetScoreAtMax(EScoreType eScoreType) const
{
	static CImageScore empty;
	// A type's overall worst image is always achieved within whichever single region has the
	// highest individual max - max(A,B,C) taken image-by-image can never exceed max(max(A),
	// max(B), max(C)) taken separately, so there's no need to rescan every image to merge this.
	const CScorerResult* pBest = nullptr;
	for (const CScorerResult& r : mvResults)
	{
		if (r.meScoreType != eScoreType)
			continue;
		if (!pBest || r.mResults.mMaxScore > pBest->mResults.mMaxScore)
			pBest = &r;
	}
	return pBest ? pBest->mResults.GetScoreAtMax() : empty;
}
const CImageScore& CImageRingsScorer::GetScoreAtMax(EScoreType eScoreType, ERegion region) const
{
	static CImageScore empty;
	for (const CScorerResult& r : mvResults)
		if (r.meScoreType == eScoreType && r.meRegion == region)
			return r.mResults.GetScoreAtMax();
	return empty;
}
float CImageRingsScorer::GetRawScoreAt(EScoreType eScoreType, int iOriginalImage) const
{
	// Unlike GetScoreAtMax(), this needs whichever region actually won AT THIS SPECIFIC image (not
	// necessarily the same region as the type's overall worst) - see CScoreTypeResults::AddScore's
	// per-image weighted mScore, which is exactly what this comparison already tracks elsewhere.
	const CImageScore* pBest = nullptr;
	for (const CScorerResult& r : mvResults)
	{
		if (r.meScoreType != eScoreType)
			continue;
		const CImageScore* pCand = r.mResults.FindByOriginalImage(iOriginalImage);
		if (!pCand)
			continue;
		if (!pBest || pCand->mScore > pBest->mScore)
			pBest = pCand;
	}
	return pBest ? pBest->mRawScore : 0.0f;
}
float CImageRingsScorer::GetWeightForRing(EScoreType eScoreType, int iRing) const
{
	if (iRing < 0 || !IsRingScorerType(eScoreType))
		return gConfig.GetScorerWeight(eScoreType);

	ERegion region = gConfig.ClassifyRing(iRing);
	if (region == ERegion::Center)
		return 1.0f; // ring scorers don't own the Center region - CCenterScorer does

	return gConfig.GetScorerWeight(eScoreType, region);
}
void CImageRingsScorer::ReplayAllMaxResult()
{
	CScorerResult* pAllMax = FindResult(EScoreType::AllMax);

	int nImages = 0;
	for (const CScorerResult& r : mvResults)
		if (r.meScoreType != EScoreType::AllMax)
			nImages = max(nImages, r.mResults.NumImages());

	for (int iImage = 0; iImage < nImages; iImage++)
	{
		CImageScore score = ComputeAllMaxScore(iImage, /*bActiveOnly=*/false);
		if (score.miOriginalImage < 0)
			continue;
		pAllMax->mResults.AddScore(score, score.miOriginalImage);
	}
}
CScorerResult* CImageRingsScorer::FindResult(EScoreType type)
{
	for (CScorerResult& r : mvResults)
		if (r.meScoreType == type)
			return &r;
	return nullptr;
}
const CScorerResult* CImageRingsScorer::FindResult(EScoreType type) const
{
	for (const CScorerResult& r : mvResults)
		if (r.meScoreType == type)
			return &r;
	return nullptr;
}
CScorerResult* CImageRingsScorer::FindResult(EScoreType type, ERegion region)
{
	for (CScorerResult& r : mvResults)
		if (r.meScoreType == type && r.meRegion == region)
			return &r;
	return nullptr;
}
void CImageRingsScorer::RecordRegionResults(int iImage)
{
	for (auto& pScorer : mvScorers)
	{
		EScoreType type = pScorer->GetScoreType();
		if (!IsRingScorerType(type))
			continue;

		for (ERegion region : gaRingScorerRegions)
		{
			float maxScore = 0;
			int iRingOfMax = -1;
			for (int iRing = 0; iRing <= mnRings; iRing++)
			{
				if (gConfig.ClassifyRing(iRing) != region)
					continue;
				if (pScorer->mvRingScore[iRing] > maxScore)
				{
					maxScore = pScorer->mvRingScore[iRing];
					iRingOfMax = iRing;
				}
			}

			CImageScore score;
			score.mRawScore = maxScore;
			score.miRing = iRingOfMax;
			score.mScore = maxScore * gConfig.GetScorerWeight(type, region);

			CScorerResult* pResult = FindResult(type, region);
			pResult->mScore = score;
			pResult->mResults.AddScore(score, iImage);
		}
	}
}
void CImageRingsScorer::RecordCenterResult(int iImage)
{
	for (auto& pScorer : mvScorers)
	{
		if (pScorer->GetScoreType() != EScoreType::Center)
			continue;

		float maxScore = 0;
		int iRingOfMax = -1;
		for (int iRing = 0; iRing <= mnRings; iRing++)
		{
			if (pScorer->mvRingScore[iRing] > maxScore)
			{
				maxScore = pScorer->mvRingScore[iRing];
				iRingOfMax = iRing;
			}
		}

		CImageScore score;
		score.mRawScore = maxScore;
		score.miRing = iRingOfMax;
		score.mScore = maxScore * gConfig.GetScorerWeight(EScoreType::Center);

		CScorerResult* pResult = FindResult(EScoreType::Center);
		pResult->mScore = score;
		pResult->mResults.AddScore(score, iImage);
	}
}
void CImageRingsScorer::RecordAllMaxResult(int iImage)
{
	CScorerResult* pAllMax = FindResult(EScoreType::AllMax);
	CImageScore score = ComputeAllMaxScoreLive(/*bActiveOnly=*/false);
	score.mRawScore = score.mScore; // AllMax's weight is always 1.0 - raw == weighted
	pAllMax->mScore = score;
	pAllMax->mResults.AddScore(score, iImage);
}
CImageScore CImageRingsScorer::ComputeAllMaxScore(int iImage, bool bActiveOnly) const
{
	CImageScore best;
	EScoreType eSource = EScoreType::N_SCORE_TYPES;
	bool bFirst = true;
	for (const CScorerResult& r : mvResults)
	{
		if (r.meScoreType == EScoreType::AllMax)
			continue;
		if (bActiveOnly && !r.IsActive())
			continue;
		if (iImage >= r.mResults.NumImages())
			continue; // this result hasn't scored this many images (e.g. a missing CSV on replay)

		const CImageScore& cand = r.mResults[iImage];
		if (bFirst || cand.mScore > best.mScore)
		{
			best = cand;
			eSource = r.meScoreType;
			bFirst = false;
		}
	}
	best.meSourceType = eSource;
	return best;
}
CImageScore CImageRingsScorer::ComputeAllMaxScoreLive(bool bActiveOnly) const
{
	CImageScore best;
	EScoreType eSource = EScoreType::N_SCORE_TYPES;
	bool bFirst = true;
	for (const CScorerResult& r : mvResults)
	{
		if (r.meScoreType == EScoreType::AllMax)
			continue;
		if (bActiveOnly && !r.IsActive())
			continue;
		if (bFirst || r.mScore.mScore > best.mScore)
		{
			best = r.mScore;
			eSource = r.meScoreType;
			bFirst = false;
		}
	}
	best.meSourceType = eSource;
	return best;
}
const CImageScore& CImageRingsScorer::GetActiveScore(EScoreType type) const
{
	static CImageScore empty;
	if (type == EScoreType::AllMax)
	{
		// AllMax's own stored value (mScore/mResults) is comprehensive - for this live,
		// active-region view, recompute fresh so a currently-disabled region correctly drops out
		// of AllMax's own live verdict too.
		mLastActiveAllMax = ComputeAllMaxScoreLive(/*bActiveOnly=*/true);
		return mLastActiveAllMax;
	}

	const CImageScore* pBest = nullptr;
	for (const CScorerResult& r : mvResults)
	{
		if (r.meScoreType != type || !r.IsActive())
			continue;
		if (!pBest || r.mScore.mScore > pBest->mScore)
			pBest = &r.mScore;
	}
	return pBest ? *pBest : empty;
}
const CImageScore& CImageRingsScorer::GetActiveScore(EScoreType type, int iPushOrder) const
{
	static CImageScore empty;
	if (type == EScoreType::AllMax)
	{
		mLastActiveAllMax = ComputeAllMaxScore(iPushOrder, /*bActiveOnly=*/true);
		return mLastActiveAllMax;
	}

	const CImageScore* pBest = nullptr;
	for (const CScorerResult& r : mvResults)
	{
		if (r.meScoreType != type || !r.IsActive())
			continue;
		const CImageScore& cand = r.mResults[iPushOrder];
		if (!pBest || cand.mScore > pBest->mScore)
			pBest = &cand;
	}
	return pBest ? *pBest : empty;
}
CScoreTypeResults CImageRingsScorer::MergeResults(EScoreType type, bool bActiveOnly) const
{
	CScoreTypeResults merged;

	if (type == EScoreType::AllMax && bActiveOnly)
	{
		// AllMax's own stored history is comprehensive (see RecordAllMaxResult) - for an
		// active-region-filtered peak list, recompute per image from its siblings instead of
		// reusing that stored history, same reasoning as GetActiveScore's AllMax special case.
		int nImages = 0;
		for (const CScorerResult& r : mvResults)
			if (r.meScoreType != EScoreType::AllMax)
				nImages = max(nImages, r.mResults.NumImages());

		for (int i = 0; i < nImages; i++)
		{
			CImageScore score = ComputeAllMaxScore(i, /*bActiveOnly=*/true);
			if (score.miOriginalImage < 0)
				continue;
			merged.AddScore(score, score.miOriginalImage);
		}
		merged.OnAllImagesScored();
		return merged;
	}

	std::vector<const CScorerResult*> matching;
	for (const CScorerResult& r : mvResults)
	{
		if (r.meScoreType != type)
			continue;
		if (bActiveOnly && !r.IsActive())
			continue;
		matching.push_back(&r);
	}

	if (matching.empty())
		return merged;

	int nImages = matching[0]->mResults.NumImages();
	for (int i = 0; i < nImages; i++)
	{
		const CImageScore* pBest = &matching[0]->mResults[i];
		for (size_t k = 1; k < matching.size(); k++)
		{
			const CImageScore& cand = matching[k]->mResults[i];
			if (cand.mScore > pBest->mScore)
				pBest = &cand;
		}
		merged.AddScore(*pBest, pBest->miOriginalImage);
	}
	merged.OnAllImagesScored();
	return merged;
}
void CImageRingsScorer::CreateScorers()
{
	// The one place that knows the concrete compute-object types - everything else operates on
	// them generically. AllMax is not a compute object any more - see CreateResults()/
	// RecordAllMaxResult, it's a pure aggregation over the other results.
	mvScorers.clear();
	mvScorers.push_back(std::make_unique<CMinMaxScorer>(mvRingMean));
	mvScorers.push_back(std::make_unique<CTentScorer>(mvRingMean));
	mvScorers.push_back(std::make_unique<CTentMinScorer>(mvRingMean));
	mvScorers.push_back(std::make_unique<CCenterScorer>(mvRingMean));
}
void CImageRingsScorer::CreateResults()
{
	mvResults.clear();

	static const EScoreType aRingTypes[3] = { EScoreType::MinMax, EScoreType::Tent, EScoreType::TentMin };
	for (EScoreType type : aRingTypes)
		for (ERegion region : gaRingScorerRegions)
		{
			CString sName;
			sName.Format("%s_%s", ScoreTypeName(type), RegionName(region));
			mvResults.emplace_back(type, region, sName);
		}

	mvResults.emplace_back(EScoreType::Center, ERegion::Center, CString(ScoreTypeName(EScoreType::Center)));
	mvResults.emplace_back(EScoreType::AllMax, ERegion::N_REGIONS, CString(ScoreTypeName(EScoreType::AllMax)));
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
	thresholdMask.Threshold(pImageRaster, gConfig.mMinThreshold, gConfig.mMaxThreshold);
	mErodedMask.FastErode(thresholdMask, gConfig.mErodeLevel);
	unsigned char* mpMask = mErodedMask.GetMaskRaster();

	// Check all pixels in image - every region is always scored here, regardless of the "Review
	// Regions" checkboxes. Scoring is exhaustive over every scorer type AND every region; a
	// disabled region only narrows the live GetActiveScore()/GetCurrentScore() view, the same way
	// review already picks among all-already-computed scorer types without re-scoring.
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
void CImageRingsScorer::FillCtPerRadiusImage()
{
	if (!mpImages->EnsureCtPerRadiusVolume())
		return;

	short* pTarget = mpImages->GetSharedCtPerRadiusVolume()->GetImageStart(miImage);
	if (!pTarget)
		return;

	float minRingMean = IGNORE_RING;
	for (int iRing = 0; iRing <= mnRings; iRing++)
	{
		if (mvRingMean[iRing] == IGNORE_RING)
			continue;
		if (minRingMean == IGNORE_RING || mvRingMean[iRing] < minRingMean)
			minRingMean = mvRingMean[iRing];
	}
	short illegalValue = (short)(minRingMean - 10);

	int nToCheck = mpRadiusImage->mnPixels;
	float* pRadiusRaster = mpRadiusImage->GetData();
	const unsigned char* pMask = mErodedMask.GetMaskRaster();
	for (int i = 0; i < nToCheck; i++)
	{
		if (!pMask[i])
		{
			pTarget[i] = illegalValue;
			continue;
		}
		int iRadius = (int)pRadiusRaster[i];
		pTarget[i] = (short)mvRingMean[iRadius];
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
			fprintf(pf, "%d, %d, %d, %.6f, %.6f, %.6f, %d, %d, %.6f\n",
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
