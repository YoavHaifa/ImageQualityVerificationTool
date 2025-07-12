#pragma once
#include <string>

using namespace std;

class CTubesMix
{
public:
	CTubesMix();

	bool ReadParametersFromFile();

	void ComputeNominal();

	bool Dump();

private:
	const char* CheckName(const string& sLine, const char* zName); // If name found - return name length, else 0
	void Check4Int(const string& line, const char* zName, int& value);
	void Check4Double(const string& line, const char* zName, double& value);

	int mnParamsInitialized;
	static const int N_PARAMS_REQUIRED = 11;
	bool mbAllParametersInitialized;

	int mMergeTexSizeZ;
	int mMergeTexSizeS;
	int mnValues;

	double mTotalTexZmm;
	double mTotalTexSmm;

	double mDistanceBetweenSensorsZ;
	double mDistanceBetweenXrts;
	double mTubeShiftZ_tube0;
	double mTubeShiftZ_tube1;

	double mDmsToCenterMm;
	double mXrtToCenterMm;

	int mnSlices;

	CString msType;

	float* mpData;

	int mDebug;
	int mDump;
};

