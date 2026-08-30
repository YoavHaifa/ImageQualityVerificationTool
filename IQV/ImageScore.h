#pragma once

class CImageScore
{
public:
	float mScore = 0; // Final score - after this scorer's weight is applied
	float mRawScore = 0; // Score before this scorer's weight is applied
	int miRing = -1; // Undefined ring is -1
	bool mbPeak = false; // Until peaks are identified
	int miPeak = 0;
};

