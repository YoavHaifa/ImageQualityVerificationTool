#pragma once

// Accumulates per-ring pixel statistics (count, sum, min/max) while scanning one image
class CRingInfo
{
public:
	CRingInfo() {}
	void Add(int value);

	int mnPixelsInRaster = 0;
	int mnPixelsInRange = 0;
	float mSum = 0;
	float mDiff = 0;
	int mMin = 0;
	int mMax = 0;
};
