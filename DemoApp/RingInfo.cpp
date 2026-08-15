#include "RingInfo.h"

void CRingInfo::Add(int value)
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
