#pragma once

class CTubesMix
{
public:
	CTubesMix();

	void ComputeNominal();

	bool Dump();

private:
	int mSizeZ;
	int mSizeS;
	int mnValues;

	double mTotalTexZmm;
	double mTotalTexSmm;

	double mDistanceBetweenSensorsZ;
	double mDistanceBetweenXrts;
	double mTubeShiftZ_tube0;
	double mTubeShiftZ_tube1;

	int mMergeTexSizeZ;
	int mMergeTexSizeS;

	double mDmsToCenterMm;
	double mXrtToCenterMm;

	int mnSlices;

	CString msType;

	float* mpData;

	int mDebug;
	int mDump;
};

