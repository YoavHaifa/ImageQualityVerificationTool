#pragma once
#include "ScoreTypes.h"
#include <vector>

class CRingsScorer
{
public:
	CRingsScorer(class CArinetaImages* pImages);
	~CRingsScorer();

	const class CImageScore& ScoreCurrentImage(int iImage);

	// Compute score of all images
	// Return miPos of image with max score (to be displayed)
	int ScoreAllImages();

	// Navigation in peaks by severity order
	void DisplayMaxPeak();
	void DisplayNextPeak();
	void DisplayPrevPeak();

	// Jump to the max peak of the newly active score type (peaks for all types are already computed)
	void OnActiveScoreTypeChanged();

private:
	static constexpr int N_SCORE_TYPES = (int)EScoreType::N_SCORE_TYPES;

	void Log();

	bool LookForPeak(int iWantedPeak);

	int miFirst = 0;
	int miLast = 0;
	int mStep = 1;
	int mnImages = 0;

	int miCurrentPeak = 0;
	int miCurrentPeakImage = 0;

	bool mbScoresComputed = false;

	class CArinetaImages* mpImages = nullptr;
	class CRadiusImage* mpRadiusImage = nullptr;
	class CImageRingsScorer* mpImageScorer = nullptr;
};

