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
	CImageRingScorer(class CArinetaImages* pImages, int iImage, class CRadiusImage* pRadiusImage);
	~CImageRingScorer();

	float Score();

	float mScore = 0;
	int miRingOfScore = -1;

private:
	void CollectRingsInfo();
	void ComputeScoreByDiff();
	void ComputeScoreByMinMaxDiff();
	void Log();

	class CArinetaImages* mpImages = nullptr;
	class CRadiusImage* mpRadiusImage;
	int miImage = -1;

	int mnRings = 0;
	int mnPixelsWithinThreshold = 0;

	static constexpr float IGNORE_RING = -100.0;
	bool mbComputeByDiff = false;

	std::vector<float> mvRingMean;
	std::vector<float> mvRingMean0;
	std::vector<CRingInfo> mvRingsInfo;

	bool mbLog = true;
};

