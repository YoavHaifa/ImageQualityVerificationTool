#pragma once
#include "ImageScore.h"
#include <vector>

// Holds all per-score-type bookkeeping for one EScoreType: score+ring per image,
// which images are peaks, their severity order, and the image with the overall max score
class CScoreTypeResults
{
public:
	// weightedScore is what every comparison/display uses (mMaxScore, peak order, ...);
	// rawScore is kept alongside only so it can be logged and, on replay, re-weighted with
	// a possibly-changed weight without rescoring - see CScorerBase::LoadSavedResults
	void AddScore(float rawScore, float weightedScore, int iRing, int iImage);

	// Call once, after all images have been scored and added, to find peaks and rank them by severity
	void OnAllImagesScored();

	// Multiplies every recorded score - both raw and weighted - and mMaxScore by factor in
	// place - a uniform positive scale doesn't change peak order, only the numbers shown/
	// compared across cases. Applied equally to both fields to keep weighted == raw * weight.
	void ScaleScores(float factor);

	// Returns the image index holding the given peak severity order, or -1 if not found
	int FindImageIndexOfPeak(int iWantedPeak) const;

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
	int mnPeaks = 0;
	int mnRealPeaks = 0;
};
