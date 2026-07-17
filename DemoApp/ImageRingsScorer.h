#pragma once
#include "..\..\ImageRLib\TSharedImage.h"

// Compute the rings scores for a single image

class CRingInfo
{
public:
	CRingInfo() {}
	void Add(int value)
	{
		mnValues++;
		mSum += value;
	}

	int mnValues = 0;
	float mSum = 0;
};

class CImageRingScorer
{
public:
	CImageRingScorer(CTImage<unsigned short>* pImage, class CRadiusImage* pRadiusImage);
	~CImageRingScorer();

	float Score();

private:
	void CollectRingsInfo();

	CTImage<unsigned short>* mpImage = nullptr;
	class CRadiusImage* mpRadiusImage;

	int mnRings = 0;
	int mnPixelsWithinThreshold = 0;

	static constexpr int umMinThreshold = -50;
	static constexpr int umMaxThreshold = 150;

	bool mbLog = true;
};

