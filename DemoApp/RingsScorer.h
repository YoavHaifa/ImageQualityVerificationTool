#pragma once
#include "ScoreTypes.h"
#include <vector>

class CImageScore
{
public:
	static constexpr int N_SCORE_TYPES = (int)EScoreType::N_SCORE_TYPES;

	CImageScore(const float scores[], const int rings[])
	{
		for (int i = 0; i < N_SCORE_TYPES; i++)
		{
			mvScore[i] = scores[i];
			mvRing[i] = rings[i];
		}
	}
	float mvScore[N_SCORE_TYPES] = {};
	int mvRing[N_SCORE_TYPES] = {}; // Undefined ring is -1, set by caller
	bool mbPeak = false; // Until peaks are identified
	int miPeak = 0;
};


class CRingsScorer
{
public:
	CRingsScorer(class CArinetaImages* pImages);
	~CRingsScorer();

	float ScoreCurrentImage(int iImage, int& oAtRing);

	// Compute score of all images
	// Return miPos of image with max score (to be displayed)
	int ScoreAllImages();

	// Navigation in peaks by severity order
	void DisplayMaxPeak();
	void DisplayNextPeak();
	void DisplayPrevPeak();

	// Re-derive peaks from the already-cached per-image scores for the newly active score type
	void OnActiveScoreTypeChanged();

private:
	void FindPeaks();
	void OrderPeaks();
	void FindNextPixToOrder();
	void ResetPeaks();
	void Log();

	bool LookForPeak(int iWantedPeak);

	int miFirst = 0;
	int miLast = 0;
	int mStep = 1;
	int mnImages = 0;
	int mnPeaks = 0;
	int mnPeaksOrdered = 0;
	int mnRealPeaks = 0;

	int miCurrentPeak = 0;
	int miCurrentPeakImage = 0;

	std::vector<CImageScore> mvScores;
	bool mbScoresComputed = false;

	float mMaxScore = 0;
	int miImageWithMaxScore = -1;

	class CArinetaImages* mpImages = nullptr;
	class CRadiusImage* mpRadiusImage = nullptr;
	// class CImageRingScorer* mpImageScorer = nullptr;
};

