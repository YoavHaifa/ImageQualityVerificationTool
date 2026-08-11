#pragma once
#include "..\..\ImageRLib\TSharedImage.h"
#include <vector>
#include <memory>

// Compute the rings scores for a single image

class CScorerBase;

class CRingInfo
{
public:
	CRingInfo() {}
	void Add(int value)
	{
		mnPixelsInRange++;
		mSum += value;
		if (mnPixelsInRange == 1)
		{
			mMin = value;
			mMax = value;
		}
		else
		{
			if (value < mMin)
				mMin = value;
			if (value > mMax)
				mMax = value;
		}
	}

	int mnPixelsInRaster = 0;
	int mnPixelsInRange = 0;
	float mSum = 0;
	float mDiff = 0;
	int mMin = 0;
	int mMax = 0;
};

class CImageRingsScorer
{
public:
	CImageRingsScorer(class CArinetaImages* pImages, class CRadiusImage* pRadiusImage);
	~CImageRingsScorer();

	// Score the given image; safe to call repeatedly on the same instance, one image at a time
	float Score(int iImage);

	float mScore = 0;
	int miRingOfScore = -1;

	std::vector<float> mvScoreByType;
	std::vector<int> mvRingByType;

private:
	void CollectRingsInfo();
	void CreateScorers();
	void Log();

	class CArinetaImages* mpImages = nullptr;
	class CRadiusImage* mpRadiusImage;
	int miImage = -1;

	int mnRings = 0;
	int mnPixelsWithinThreshold = 0;

	std::vector<float> mvRingMean0;
	std::vector<float> mvRingMean;
	std::vector<CRingInfo> mvRingsInfo;
	std::vector<std::unique_ptr<CScorerBase>> mvScorers;

	bool mbLog = true;
};

