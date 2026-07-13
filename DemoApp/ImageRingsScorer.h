#pragma once
#include <vector>

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

	std::vector<CRingInfo>* mpvRingInfo;

	static constexpr int umMinThreshold = -50;
	static constexpr int umMaxThreshold = 150;

};

