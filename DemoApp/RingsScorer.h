#pragma once
#include <vector>

class CImageScore
{
public:
	CImageScore(float score, int iRing)
		: mScore(score)
		, miRing(iRing)
	{}
	float mScore = 0;
	int miRing = -1; // Undefined
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

private:
	void FindPeaks();
	void OrderPeaks();
	void FindNextPixToOrder();
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

