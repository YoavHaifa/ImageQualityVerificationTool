#pragma once

class CImageScore
{
public:
	float mScore = 0;
	int miRing = -1; // Undefined ring is -1
	bool mbPeak = false; // Until peaks are identified
	int miPeak = 0;
};

