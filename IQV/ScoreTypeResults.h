#pragma once
#include "ImageScore.h"
#include <vector>

// Holds all per-score-type bookkeeping for one EScoreType: score+ring per image,
// which images are peaks, their severity order, and the image with the overall max score
class CScoreTypeResults
{
public:
	// Files score under iImage (stamped onto the stored copy as miOriginalImage). score.mScore
	// (the weighted value) is what every comparison/display uses (mMaxScore, peak order, ...);
	// score.mRawScore is kept alongside only so it can be logged and, on replay, re-weighted
	// with a possibly-changed weight without rescoring - see CScorerBase::LoadSavedResults.
	void AddScore(const CImageScore& score, int iImage);

	// Call once, after all images have been scored and added, to find peaks and rank them by severity
	void OnAllImagesScored();

	// Multiplies every recorded (weighted) score and mMaxScore by factor in place - a uniform
	// positive scale doesn't change peak order, only the numbers shown/compared across cases.
	// mRawScore is untouched - it stays the real, physical raw score.
	void ScaleScores(float factor);

	// Returns the image index holding the given peak severity order, or -1 if not found
	int FindImageIndexOfPeak(int iWantedPeak) const;

	// The full recorded score (ring, source type, raw score, ...) of the case's single worst
	// (mMaxScore) image - not just the float mMaxScore itself. Only meaningful once at least one
	// score has been added.
	const CImageScore& GetScoreAtMax() const { return mvScores[miPushOrderOfMaxScore]; }

	// This scorer's own recorded score for the given *original* DICOM slice number (not a push-order
	// index - see CImageScore::miOriginalImage), or nullptr if that image was never scored here.
	// Lets a caller look up one scorer's score for the same image another scorer's max came from -
	// see COptimizer, which uses this to find a source scorer's true raw score for the report.
	const CImageScore* FindByOriginalImage(int iOriginal) const;

	const CImageScore& operator[](int iImage) const { return mvScores[iImage]; }
	int NumImages() const { return (int)mvScores.size(); }

	float mMaxScore = 0;
	int miImageWithMaxScore = -1;

private:
	// Operations to perform when scores are compute on all images
	void FindPeaks();
	void OrderPeaks();

	bool FindNextPixToOrder();

	std::vector<CImageScore> mvScores;
	int miPushOrderOfMaxScore = -1; // Index into mvScores of the entry holding mMaxScore
	int mnPeaks = 0;
	int mnRealPeaks = 0;
};
