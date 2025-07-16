#pragma once
#include <string>

using namespace std;

class CTubesMix
{
public:
	CTubesMix();

	bool ReadParametersFromFile();

	void ComputeNominal();
	void ComputeLimited(int marginsMm);

	bool Dump();

private:
	void ComputeInternalVariables();
	void ComputeColVariables(int iS, bool bSaveMargins = false);
	void ComputeLineWeights(int iS);
	void GrowNoMixArea();
	void OpenLog(const char* zFunc);
	void CloseLog();

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

	double mMarginsMm;	// Width of area on the inner side of the tube with no mix
						// Within these margins - only the closer tube is used
	double mMinStartMix;
	double mMaxEndMix;

	// Internal parameters converted to members
	double mZ_tube_0;
	double mZ_tube_1;

	double mLocationOfLastZSensorCenterMm;//in mm;
	double mTexelSizeSMm;	// in mm
	double mTexelSizeZMm;	// in mm

	double mFirstTexelLocationS;
	double mFirstTexelLocationZ;

	double mZMixStart;
	double mZMixEnd;
	double mZRangeBetweenTubesMm;


	CString msType;

	float* mpData;

	// Debug
	int mDebug;
	int mDump;
	FILE* mpfLog;
	FILE* mpfCsv;
};

