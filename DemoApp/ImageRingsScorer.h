#pragma once
#include "..\..\ImageRLib\TSharedImage.h"
#include <vector>

// Compute the rings scores for a single image

class CRingInfo
{
public:
	CRingInfo() {}
	void Add(int value)
	{
		mnPixelsInRange++;
		mSum += value;
	}

	int mnPixelsInRaster = 0;
	int mnPixelsInRange = 0;
	float mSum = 0;
	float mDiff = 0;
};

class CImageRingScorer
{
public:
	CImageRingScorer(CTImage<short>* pImage, class CRadiusImage* pRadiusImage);
	~CImageRingScorer();

	float Score();

	float mScore = 0;
	int miRingOfScore = -1;

private:
	void CollectRingsInfo(std::vector<float>& vMean);

	CTImage<short>* mpImage = nullptr;
	class CRadiusImage* mpRadiusImage;

	int mnRings = 0;
	int mnPixelsWithinThreshold = 0;

	static constexpr float IGNORE_RING = -100.0;
	bool mbComputeByDiff = false;

	bool mbLog = true;
	void ComputeScoreByDiff(std::vector<float>& vRingMean);
	void ComputeScoreByMinMaxDiff(std::vector<float>& vRingMean);
};

