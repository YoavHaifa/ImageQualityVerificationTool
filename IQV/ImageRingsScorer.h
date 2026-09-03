#pragma once
#include "..\..\ImageRLib\TSharedImage.h"
#include "..\..\ImageRLib\Mask.h"
#include "..\..\yUtils\BoundHistogram.h"
#include "..\..\yUtils\TRange.h"
#include "ScoreTypes.h"
#include "RingInfo.h"
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

	// Score the given image; safe to call repeatedly on the same instance, one image at a time
	const CImageScore& Score(int iImage);

	// Files the score each scorer just computed into its own mResults, under iImage
	void RecordScores(int iImage);

	// Call once, after all images have been scored and recorded, to finalize each scorer's peaks
	void OnAllImagesScored();

	// Applies factor to every scorer's recorded scores (see CScoreTypeResults::ScaleScores) - the
	// same factor for every scorer, since it normalizes for the case's own pixel-value spread
	// rather than anything scorer-specific
	void ScaleScores(float factor);

	// Writes the case-wide pixel-value histogram (accumulated across all images scored so far)
	// to Histogram.csv in the case log dir. Call once, after all images have been scored.
	void LogHistogram();

	// The pixel-value histogram's main area (see CBoundHistogram::GetMainArea), for logging
	// alongside the rest of the case's summary info.
	STRange<int> GetHistogramMainArea(float cutPercent) const { return mHistogram.GetMainArea(cutPercent); }

	// The score+ring already recorded for iImage, under the currently active score type (gConfig.mScoreType)
	const CImageScore& GetCurrentScore(int iImage) const;

	// The score+ring already recorded for iImage, under the given score type
	const CImageScore& GetScore(EScoreType eScoreType, int iImage) const;

	// The image index with the highest score, under the currently active score type
	int GetImageWithMaxScore() const;

	// The image index holding the given peak severity order under the currently active score type, or -1 if not found
	int FindImageIndexOfPeak(int iWantedPeak) const;

	// This case's single worst (highest) score under the given score type, across every image
	// scored so far - same value CaseInfo.yaml logs as scorers > <name> > worst_score.
	float GetWorstScore(EScoreType eScoreType) const;

	// The full recorded score of the case's worst image under the given score type - unlike
	// GetWorstScore(), also carries the ring, source scorer (meaningful for AllMax), and which
	// original image it came from. See CScoreTypeResults::GetScoreAtMax().
	const CImageScore& GetScoreAtMax(EScoreType eScoreType) const;

	// The given scorer's own raw score for the given *original* image number, or 0 if that
	// scorer never scored that image. Used to find a source scorer's true (unweighted,
	// unscaled) raw score for whichever image produced another scorer's (e.g. AllMax's) max -
	// see COptimizer.
	float GetRawScoreAt(EScoreType eScoreType, int iOriginalImage) const;

	// Generic access to the scorers, so callers (e.g. per-scorer logging) don't need
	// to know the concrete set of score types
	int GetNScorers() const { return (int)mvScorers.size(); }
	class CScorerBase* GetScorerByIndex(int iScorer) const { return mvScorers[iScorer].get(); }

	// Number of rings in this case (mvRingMean has mnRings+1 entries, one per ring 0..mnRings).
	int GetNRings() const { return mnRings; }

	// Must be called once, before scoring the case's first image, so mpImages' compact
	// ring-mean-profile raster (see CArinetaImages::EnsureRingMeanProfile) is sized correctly up
	// front - every Score() call after this records its own row into it, regardless of
	// gConfig.mbDisplayCtPerRadius (this is cheap, and Review's use of it is a separate toggle).
	void PrepareRingMeanProfile(int nTotalImages);

	//float mScore = 0;
	//int miRingOfScore = -1;

private:
	void CreateScorers();

	// The scorer of the given type; its mResults holds the score+ring history across all images scored so far
	class CScorerBase* GetScorer(EScoreType eScoreType) const { return mvScorers[(int)eScoreType].get(); }

	void CollectRingsInfo();
	void ErodeValidArea();

	// Paints mpImages' CT-per-radius volume for the current image: each pixel gets its ring's
	// mean CT value (mvRingMean) instead of the raw pixel value, except pixels the erode mask
	// excluded ("illegal"), which get a constant 10 CT numbers below this image's lowest ring
	// mean, to stand out from any real value. Gated by gConfig.mbDisplayCtPerRadius; called once
	// mvRingMean and mErodedMask are final for this image.
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
	std::vector<std::unique_ptr<CScorerBase>> mvScorers;

	int miRingMeanProfileRow = 0; // next row to record into mpImages' compact ring-mean profile
};

