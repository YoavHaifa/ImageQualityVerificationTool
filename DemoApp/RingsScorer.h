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
};


class CRingsScorer
{
public:
	CRingsScorer(class CArinetaImages* pImages);
	~CRingsScorer();

	float ScoreCurrentImage(int& oAtRing);

	// Compute score of all images
	// Return miPos of image with max score (to be displayed)
	int ScoreAllImages();

private:
	std::vector<CImageScore> mvScores;
	bool mbScoresComputed = false;

	float mMaxScore = 0;
	int miImageWithMaxScore = -1;

	class CArinetaImages* mpImages = nullptr;
	class CRadiusImage* mpRadiusImage = nullptr;
	class CImageRingScorer* mpImageScorer = nullptr;
};

