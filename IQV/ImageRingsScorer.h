#pragma once
#include "..\..\ImageRLib\TSharedImage.h"
#include "..\..\ImageRLib\Mask.h"
#include "..\..\yUtils\BoundHistogram.h"
#include "..\..\yUtils\TRange.h"
#include "ScoreTypes.h"
#include "RingInfo.h"
#include "ScorerResult.h"
#include <vector>
#include <memory>

// Compute the rings scores for a single image

class CScorerBase;
class CImageScore;

class CImageRingsScorer
{
public:
	CImageRingsScorer(class CArinetaImages* pImages, class CRadiusImage* pRadiusImage);
	~CImageRingsScorer();

	// Score the given image; safe to call repeatedly on the same instance, one image at a time.
	// The returned score is the LIVE, active-region view of gConfig.mScoreType - for a ring scorer
	// (MinMax/Tent/TentMin), the max over whichever of its 3 regions currently pass
	// gConfig.IsRegionEnabled (see GetActiveScore) - so toggling a "Review Regions" checkbox
	// changes what's shown here immediately, without rescoring. Every region is still fully
	// scored and recorded every time regardless (see CollectRingsInfo) - "at scoring stage we do
	// not have to check region limits, we have to score all regions" - only this one, most-visible
	// query is filtered by what's currently active; every other query below (GetScore/
	// GetScoreAtMax/GetWorstScore/peak-finding/GetRawScoreAt) stays comprehensive (every region,
	// always), matching how it always worked before regions existed.
	const CImageScore& Score(int iImage);

	// Call once, after all images have been scored and recorded, to finalize each result's peaks
	void OnAllImagesScored();

	// Applies factor to every result's recorded scores (see CScoreTypeResults::ScaleScores) - the
	// same factor for every one, since it normalizes for the case's own pixel-value spread rather
	// than anything scorer- or region-specific
	void ScaleScores(float factor);

	// Writes the case-wide pixel-value histogram (accumulated across all images scored so far)
	// to Histogram.csv in the case log dir. Call once, after all images have been scored.
	void LogHistogram();

	// The pixel-value histogram's main area (see CBoundHistogram::GetMainArea), for logging
	// alongside the rest of the case's summary info.
	STRange<int> GetHistogramMainArea(float cutPercent) const { return mHistogram.GetMainArea(cutPercent); }

	// Same "live, active-region" view as Score() above, but for an already-scored image (Case/
	// Batch Review's replay path) - see CRingsScorer::ScoreCurrentImage. iPushOrder is 0-based, in
	// scoring order (NOT the original DICOM slice number).
	const CImageScore& GetCurrentScore(int iPushOrder) const;

	// Comprehensive (every region, always - see the Score() doc comment above) - the score+ring
	// already recorded for iImage (push order), under the given score type.
	const CImageScore& GetScore(EScoreType eScoreType, int iImage) const;

	// Comprehensive. The image (original DICOM slice number) with the highest score, under the
	// currently active score type.
	int GetImageWithMaxScore() const;

	// Active-region-filtered equivalent of GetImageWithMaxScore() above - the original image
	// (DICOM slice number) with the highest ACTIVE score under the currently active score type, or
	// -1 if none of its regions are currently enabled. Used when a case is first opened/replayed
	// (see CRingsScorer::ScoreAllImages/LoadFromSavedResults and CBatchReviewer), so landing on a
	// case never shows a score of 0 just because its real severity happens to be in a
	// currently-disabled region.
	int GetActiveImageWithMaxScore() const;

	// Active-region-filtered (unlike every other query on this list) - the image index (push
	// order) holding the given peak severity order under the currently active score type,
	// considering only its currently-enabled regions, or -1 if not found - see MergeResults. This
	// is what CRingsScorer::DisplayMaxPeak/Next/Prev actually navigate with, so toggling a "Review
	// Regions" checkbox changes what "next peak" means immediately, the same way picking a
	// different scorer type already does (see CIQVDlg::OnBnClickedCheckReviewRegion).
	int FindImageIndexOfPeak(int iWantedPeak) const;

	// Comprehensive. This case's single worst (highest) score under the given score type, across
	// every image scored so far - same value CaseInfo.yaml logs as scorers > <name> > worst_score.
	float GetWorstScore(EScoreType eScoreType) const;

	// Same, but for exactly ONE region of a ring scorer - this case's own single worst score for
	// just that (type,region) pair, across every image scored so far. Used to log CaseInfo.yaml's
	// per-region breakdown (see CRingsScorer::LogCaseInfo), which CBatchReviewer reads back to
	// compute an active-region-filtered case ordering without needing to reopen every case.
	float GetWorstScore(EScoreType eScoreType, ERegion region) const;

	// Comprehensive. The full recorded score of the case's worst image under the given score type -
	// unlike GetWorstScore(), also carries the ring, source scorer (meaningful for AllMax), and
	// which original image it came from.
	const CImageScore& GetScoreAtMax(EScoreType eScoreType) const;

	// Same, but for exactly ONE region of a ring scorer, rather than merged across its 3 regions -
	// this case's own single worst score for just that (type,region) pair. Used by COptimizer to
	// tune each region's weight independently, from that region's own score distribution. Only
	// meaningful when IsRingScorerType(eScoreType); returns an empty score for Center/AllMax
	// (they have no per-region breakdown - use the type-only overload above for those).
	const CImageScore& GetScoreAtMax(EScoreType eScoreType, ERegion region) const;

	// Comprehensive. The given scorer type's own raw score for the given *original* image number
	// (whichever of its regions actually produced that image's own recorded score), or 0 if never
	// scored. Used to find a source scorer's true (unweighted, unscaled) raw score for whichever
	// image produced another scorer's (e.g. AllMax's) max - see COptimizer.
	float GetRawScoreAt(EScoreType eScoreType, int iOriginalImage) const;

	// The weight actually applied to a score at ring iRing, for the given scorer type - see
	// CConfig::GetScorerWeight(type, region). Used by the main dialog's score-detail display.
	float GetWeightForRing(EScoreType eScoreType, int iRing) const;

	// Generic access to the flat per-(type,region) result list, so callers (per-scorer logging,
	// replay) don't need to know the concrete set of score types/regions.
	int GetNScorers() const { return (int)mvResults.size(); }
	class CScorerResult* GetScorerByIndex(int iScorer) { return &mvResults[iScorer]; }

	// Rebuilds AllMax's own history from its (by then already replayed) siblings' mResults - call
	// once, after every other result's LoadSavedResults() has run. See ComputeAllMaxScore.
	void ReplayAllMaxResult();

	// Number of rings in this case (mvRingMean has mnRings+1 entries, one per ring 0..mnRings).
	int GetNRings() const { return mnRings; }

	// Must be called once, before scoring the case's first image, so mpImages' compact
	// ring-mean-profile raster (see CArinetaImages::EnsureRingMeanProfile) is sized correctly up
	// front - every Score() call after this records its own row into it, regardless of
	// gConfig.mbDisplayCtPerRadius (this is cheap, and Review's use of it is a separate toggle).
	void PrepareRingMeanProfile(int nTotalImages);

private:
	void CreateScorers();
	void CreateResults();

	CScorerResult* FindResult(EScoreType type); // Center/AllMax - one region-less result
	const CScorerResult* FindResult(EScoreType type) const;
	CScorerResult* FindResult(EScoreType type, ERegion region); // a ring scorer's one region

	void RecordRegionResults(int iImage); // MinMax/Tent/TentMin -> 9 entries, comprehensive
	void RecordCenterResult(int iImage);
	void RecordAllMaxResult(int iImage); // comprehensive - see ComputeAllMaxScore(bActiveOnly=false)

	// AllMax's own value: max over every OTHER result's score, EXCLUDING AllMax itself.
	// bActiveOnly true restricts that to currently-active results (see GetActiveScore) - false is
	// the comprehensive, always-stored value RecordAllMaxResult/ReplayAllMaxResult use.
	CImageScore ComputeAllMaxScore(int iImage, bool bActiveOnly) const; // historical, by push-order index
	CImageScore ComputeAllMaxScoreLive(bool bActiveOnly) const; // from the CURRENT (just-scored) image only

	// The live "what's currently active" merge for gConfig.mScoreType - see Score()/GetCurrentScore().
	const CImageScore& GetActiveScore(EScoreType type) const; // current (just-scored) image
	const CImageScore& GetActiveScore(EScoreType type, int iPushOrder) const; // historical, by push-order

	// Merges every CScorerResult matching type (max per image, across all its regions) into a
	// fresh, properly peak-ordered CScoreTypeResults - lets FindImageIndexOfPeak() work over "all
	// of this type's regions together" without needing to track that merge incrementally. Cheap:
	// just a max-scan over already-computed per-image scores, no rescoring - same principle as
	// picking a different already-scored scorer type needs no rescoring either. bActiveOnly
	// restricts the merge to currently-active regions (see CScorerResult::IsActive) - what
	// FindImageIndexOfPeak() actually wants, so toggling a "Review Regions" checkbox repositions
	// peak navigation immediately, the same way picking a different scorer type already does.
	CScoreTypeResults MergeResults(EScoreType type, bool bActiveOnly) const;

	void CollectRingsInfo();
	void ErodeValidArea();

	// Paints mpImages' CT-per-radius volume for the current image: each pixel gets its ring's
	// mean CT value (mvRingMean) instead of the raw pixel value, except pixels the erode mask
	// excluded ("illegal"), which get a constant 10 CT numbers below this image's lowest ring
	// mean, to stand out. Gated by gConfig.mbDisplayCtPerRadius; called once mvRingMean and
	// mErodedMask are final for this image.
	void FillCtPerRadiusImage();

	void Log();

	class CArinetaImages* mpImages = nullptr;
	class CRadiusImage* mpRadiusImage;
	int miImage = -1;

	int mnRings = 0;
	int mnPixelsWithinThreshold = 0;

	CBoundHistogram mHistogram;

	// Filled fresh by CollectRingsInfo() each image; reused by FillCtPerRadiusImage() to mark
	// pixels the erode step excluded, without redoing the threshold+erode pass
	CMask mErodedMask;

	std::vector<float> mvRingMean0;
	std::vector<float> mvRingMean;
	std::vector<CRingInfo> mvRingsInfo;
	std::vector<std::unique_ptr<CScorerBase>> mvScorers; // MinMax/Tent/TentMin/Center - compute objects only, no AllMax
	std::vector<CScorerResult> mvResults; // flat: 3x(MinMax,Tent,TentMin) regions + Center + AllMax = 11

	// mutable: AllMax's live "active" value is recomputed fresh on every GetActiveScore() call
	// (rather than stored, unlike every other result) so a currently-disabled region correctly
	// drops out of AllMax's own live verdict too - this just gives that fresh value a stable
	// address to return a const reference to.
	mutable CImageScore mLastActiveAllMax;

	int miRingMeanProfileRow = 0; // next row to record into mpImages' compact ring-mean profile
};
