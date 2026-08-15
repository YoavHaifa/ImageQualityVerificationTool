#pragma once
#include "ImageScore.h"
#include <vector>

// Holds all per-score-type bookkeeping for one EScoreType: score+ring per image,
// which images are peaks, their severity order, and the image with the overall max score
class CScoreTypeResults
{
public:
	void AddScore(float score, int iRing, int iImage);

	// Call once, after all images have been scored and added, to find peaks and rank them by severity
	void OnAllImagesScored();

	// Returns the image index holding the given peak severity order, or -1 if not found
	int FindImageIndexOfPeak(int iWantedPeak) const;

	const CImageScore& operator[](int iImage) const { return mvScores[iImage]; }
	int NumImages() const { return (int)mvScores.size(); }

	float mMaxScore = 0;
	int miImageWithMaxScore = -1;

private:
	void FindPeaks();
	void OrderPeaks();
	bool FindNextPixToOrder();

	std::vector<CImageScore> mvScores;
	int mnPeaks = 0;
	int mnRealPeaks = 0;
};
