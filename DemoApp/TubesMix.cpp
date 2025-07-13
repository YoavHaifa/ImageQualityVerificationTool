#include "stdafx.h"
#include "TubesMix.h"
#include "..\..\yUtils\MyWindows.h"
#include <iostream>
#include <fstream>

CTubesMix::CTubesMix()
	: mMergeTexSizeS(450)
	, mMergeTexSizeZ(1024)
	, mnParamsInitialized(0)
	, mMarginsMm(20.0)
	, mpData(NULL)
	, msType("NULL")
	, mpfLog(NULL)
	, mpfCsv(NULL)
{
	mbAllParametersInitialized = ReadParametersFromFile();

	mnValues = mMergeTexSizeS * mMergeTexSizeZ;
	mpData = new float[mnValues];
	memset(mpData, 0, mnValues * sizeof(float));
}

const char* CTubesMix::CheckName(const string& sLine, const char* zName)
{
	const char* zStart = sLine.c_str();
	size_t nameLen = strlen(zName);
	size_t nameInLineLen = sLine.find(' ');
	if (nameLen != nameInLineLen)
		return NULL;

	if (strncmp(zName, zStart, nameLen) != 0)
		return NULL;

	mnParamsInitialized++;
	return sLine.c_str() + nameLen + 1;
}

void CTubesMix::Check4Int(const string& sLine, const char* zName, int& oValue)
{
	const char *zValue = CheckName(sLine, zName);
	if (zValue)
		oValue = atoi(zValue);
}

void CTubesMix::Check4Double(const string& sLine, const char* zName, double& oValue)
{
	const char *zValue = CheckName(sLine, zName);
	if (zValue)
		oValue = atof(zValue);
}

bool CTubesMix::ReadParametersFromFile()
{
	CString sfName("d:\\Log\\BP_calcTube1MergeWeight_width1024_height450.txt");
	ifstream file((const char*)sfName);
	if (!file.is_open())
	{
		//mLogger->LogError(LOGGER_LOC, L"Failed to open ECG file " + ecgFilename + L" while trying to read ECG data from the scan plan");
		return false;
	}

	string sLine;

	while (getline(file, sLine))
	{
		Check4Int(sLine, "mergeTexSizeS", mMergeTexSizeS);
		Check4Int(sLine, "mergeTexSizeZ", mMergeTexSizeZ);
		Check4Int(sLine, "nSlices", mnSlices);

		Check4Double(sLine, "totalTexSmm", mTotalTexSmm);
		Check4Double(sLine, "totalTexZmm", mTotalTexZmm);
		Check4Double(sLine, "distanceBetweenSensorsZ", mDistanceBetweenSensorsZ);
		Check4Double(sLine, "distanceBetweenXrts", mDistanceBetweenXrts);
		Check4Double(sLine, "tubeShiftZ_tube0", mTubeShiftZ_tube0);
		Check4Double(sLine, "tubeShiftZ_tube1", mTubeShiftZ_tube1);
		Check4Double(sLine, "DmsToCenterMm", mDmsToCenterMm);
		Check4Double(sLine, "xrtToCenterMm", mXrtToCenterMm);

		if (mnParamsInitialized == N_PARAMS_REQUIRED)
			return true;
	}

	char zBuf[128];
	sprintf_s(zBuf, sizeof(zBuf), "<CTubesMix::ReadParametersFromFile> Only %d parameters found - %d required",
		mnParamsInitialized, N_PARAMS_REQUIRED);
	CMyWindows::MessBox(zBuf, "Missing Parameters");
	return false;
}

bool CTubesMix::Dump()
{
	char zBuf[128];
	sprintf_s(zBuf, sizeof(zBuf), "d:\\Dump\\BP_TubesMergeWeights_%s_width%d_height%d.float.dat",
		(const char *)msType, mMergeTexSizeZ, mMergeTexSizeS);
	FILE* pf = NULL;
	fopen_s(&pf, zBuf, "wb");
	if (!pf)
	{
		CMyWindows::MessBox("Failed to open dump file", zBuf);
		return false;
	}
			
	size_t nWritten = fwrite(mpData, sizeof(float), mnValues, pf);
	fclose(pf);
		
	if (nWritten != mnValues)
	{
		CMyWindows::MessBox("Failed to write dump file", zBuf);
		return false;
	}
	return true;
}

void CTubesMix::ComputeInternalVariables()
{
	mZ_tube_0 = mDistanceBetweenXrts / 2.0 + mTubeShiftZ_tube0;
	mZ_tube_1 = -mDistanceBetweenXrts / 2.0 + mTubeShiftZ_tube1;

	mLocationOfLastZSensorCenterMm = (mnSlices / 2 - 0.5) * mDistanceBetweenSensorsZ;//in mm;
	mTexelSizeSMm = mTotalTexSmm / mMergeTexSizeS;	// in mm
	mTexelSizeZMm = mTotalTexZmm / mMergeTexSizeZ;	// in mm

	mFirstTexelLocationS = -mTotalTexSmm / 2 + mTexelSizeSMm / 2;
	mFirstTexelLocationZ = -mTotalTexZmm / 2 + mTexelSizeZMm / 2;

	mMinStartMix = mZ_tube_1 + mMarginsMm;
	mMaxEndMix = mZ_tube_0 - mMarginsMm;

	if (mpfLog)
	{
		fprintf(mpfLog, "<CTubesMix::ComputeInternalVariables>\n");
		fprintf(mpfLog, "z_tube_0 %f\n", mZ_tube_0);
		fprintf(mpfLog, "z_tube_1 %f\n", mZ_tube_1);
		fprintf(mpfLog, "locationOfLastZSensorCenter %f\n", mLocationOfLastZSensorCenterMm);
		fprintf(mpfLog, "texelSizeS %f\n", mTexelSizeSMm);
		fprintf(mpfLog, "texelSizeZ %f\n", mTexelSizeZMm);
		fprintf(mpfLog, "firstTexelLocationS %f\n", mFirstTexelLocationS);
		fprintf(mpfLog, "firstTexelLocationZ %f\n", mFirstTexelLocationZ);
		fprintf(mpfLog, "\n");
		fprintf(mpfLog, "mMarginsMm %f\n", mMarginsMm);
		fprintf(mpfLog, "mMinStartMix %f\n", mMinStartMix);
		fprintf(mpfLog, "mMaxEndMix %f\n", mMaxEndMix);
		fprintf(mpfLog, "\n");
	}
}

void CTubesMix::ComputeColVariables(int iS, bool bSaveMargins)
{
	double positionS = mFirstTexelLocationS + iS * mTexelSizeSMm;	// position of texel, in mm, from iso-center.

	// (abs(mZ_tube_1) + mZMixEnd) / (xrtToCenterMm - positionS) = (mLocationOfLastZSensorCenterMm + abs(mZ_tube_1)) / (DmsToCenterMm + xrtToCenterMm)
	mZMixEnd = (mLocationOfLastZSensorCenterMm + abs(mZ_tube_1)) / (mDmsToCenterMm + mXrtToCenterMm) * (mXrtToCenterMm - positionS) - abs(mZ_tube_1);
	mZMixStart = (mLocationOfLastZSensorCenterMm + abs(mZ_tube_0)) / (mDmsToCenterMm + mXrtToCenterMm) * (mXrtToCenterMm - positionS) - abs(mZ_tube_0);
	mZMixStart *= -1;

	if (bSaveMargins)
	{
		if (mZMixStart < mMinStartMix)
			mZMixStart = mMinStartMix;
		if (mZMixEnd > mMaxEndMix)
			mZMixEnd = mMaxEndMix;
	}

	mZRangeBetweenTubesMm = mZMixEnd - mZMixStart;

	if (mpfLog)
	{
		fprintf(mpfLog, "%3d: S %f - z_c %f - z_a %f\n", iS, positionS, mZMixEnd, mZMixStart);
	}

}

void CTubesMix::GrowNoMixArea()
{
	if (mpfLog)
		fprintf(mpfLog, "Start second pass, mergeTexSizeS %d, mergeTexSizeZ %d\n",
			mMergeTexSizeS, mMergeTexSizeZ);

	int iFirstDichotomic = -1;
	for (int iS = 0; iS < mMergeTexSizeS; iS++)
	{
		float* pLine = mpData + mMergeTexSizeZ * iS;
		int iLast1 = 0;
		int iFirst0 = mMergeTexSizeZ - 1;

		for (int iZ = 0; iZ < mMergeTexSizeZ - 1; iZ++)
		{
			if ((1.0f == pLine[iZ]) && (1.0f != pLine[iZ + 1]))
			{
				iLast1 = iZ;
			}
			if ((0.0f != pLine[iZ]) && (0.0f == pLine[iZ + 1]))
			{
				iFirst0 = iZ + 1;
			}
		}

		if (iLast1 + 1 == iFirst0 && iFirstDichotomic < 0)
			iFirstDichotomic = iS;
		int nGrow = 0;
		if (iLast1 + 1 < iFirst0)
		{
			pLine[iLast1 + 1] = 1.0f;
			nGrow++;
		}
		if (iLast1 + 1 < iFirst0 - 1)
		{
			pLine[iFirst0 - 1] = 0.0f;
			nGrow++;
		}

		if (mDebug && mpfLog)
		{
			fprintf(mpfLog, "Second pass %3d: %3d - %3d", iS, iLast1, iFirst0);
			if (nGrow > 0)
				fprintf(mpfLog, " Grew %d", nGrow);
			fprintf(mpfLog, "\n");
		}
	}

	if (mDebug && iFirstDichotomic > 0)
	{
		int n = mDebug;
		int iCorrect = iFirstDichotomic - 1;
		while (n-- > 0 && iCorrect >= 0)
		{
			if (mpfLog)
				fprintf(mpfLog, "<DEBUG> correcting line %d\n", iCorrect);

			float* pLine = mpData + mMergeTexSizeZ * iCorrect;
			for (int iZ = 0; iZ < mMergeTexSizeZ; iZ++)
			{
				if (iZ < mMergeTexSizeZ / 2)
					pLine[iZ] = 1;
				else
					pLine[iZ] = 0;
			}
			iCorrect--;
		}
	}
}

void CTubesMix::ComputeLineWeights(int iS)
{
	if (mpfCsv)
		fprintf(mpfCsv, "%d, %.3f, %.3f\n", iS, mZMixStart, mZMixEnd);

	float* pLine = mpData + iS * mMergeTexSizeZ;

	for (int iZ = 0; iZ < mMergeTexSizeZ; iZ++)
	{
		double positionZ = mFirstTexelLocationZ + iZ * mTexelSizeZMm;	// position of texel, in mm, from center.

		if (positionZ <= mZMixStart)	// w==1
		{
			if (iZ < mMergeTexSizeZ / 2)
				pLine[iZ] = 1.0f;
			else
				pLine[iZ] = 0.0f;
		}
		else if (positionZ >= mZMixEnd)
		{
			pLine[iZ] = 0.0f;
		}
		else
		{
			// pLine[iZ] = (float)(positionZ / (mZMixStart - mZMixEnd) + (mZMixEnd / (mZMixEnd - mZMixStart)));
			pLine[iZ] = (float)((mZMixEnd - positionZ) / mZRangeBetweenTubesMm);
		}
	}
}
void CTubesMix::CloseLog()
{
	if (mpfLog)
	{
		fclose(mpfLog);
		mpfLog = NULL;
	}
	if (mpfCsv)
	{
		fclose(mpfCsv);
		mpfCsv = NULL;
	}
}
void CTubesMix::OpenLog(const char* zFunc)
{
	char zBuf[128];
	CloseLog();

	// Log for debug
	sprintf_s(zBuf, sizeof(zBuf), "d:\\Log\\CTubesMix__%s_width%d_height%d.txt",
		zFunc, mMergeTexSizeZ, mMergeTexSizeS);
	fopen_s(&mpfLog, zBuf, "w");

	// CSV with limits of mix area in each line
	sprintf_s(zBuf, sizeof(zBuf), "d:\\Log\\CTubesMix__%s_width%d_height%d.csv",
		zFunc, mMergeTexSizeZ, mMergeTexSizeS);
	fopen_s(&mpfCsv, zBuf, "w");

	if (mpfLog)
	{
		fprintf(mpfLog, "<calcTube1MergeWeight>\n");
		fprintf(mpfLog, "mergeTexSizeZ %d\n", mMergeTexSizeZ);
		fprintf(mpfLog, "mergeTexSizeS %d\n", mMergeTexSizeS);
		fprintf(mpfLog, "totalTexZmm %f\n", mTotalTexZmm);
		fprintf(mpfLog, "totalTexSmm %f\n", mTotalTexSmm);
		fprintf(mpfLog, "bpP.distanceBetweenSensorsZ %f\n", mDistanceBetweenSensorsZ);
		fprintf(mpfLog, "debug %d 0x%x\n", mDebug, mDebug);
		fprintf(mpfLog, "\n");
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The function calculate Grid of weights: calcTube1MergeWeight
// The function executed only in case of Dual Tube shot
// Each pixel of the grid is calculates as if it is located directly on the path of the ray 
// And its weight is calculated as 1 if the weight is affected from the left tube only
// as 0 if the weight is affected from the right tube
// and lineary interpolated in case of both tubes incluence
//
//   left
//  ------
//          \  both
//           \        right
//             -------------
//
// Parameters:
//		buffer - Output Grid
//		mergeTexSizeZ - Size of the Grid in Z direction (XRT_MERGE_TEXTURE_RESOLUTION_Z = 512*4)
//		mergeTexSizeS - Size of the Grid in X direction (XRT_MERGE_TEXTURE_RESOLUTION_S	= 280*4)
//		totalTexZmm - 
//		totalTexSmm - 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CTubesMix::ComputeNominal()
{
	OpenLog("ComputeNominal");

	ComputeInternalVariables();

	for (int iS = 0; iS < mMergeTexSizeS; iS++)
	{
		ComputeColVariables(iS);
		ComputeLineWeights(iS);
	}

	// "grow" the area where weight==1 and the area where weight==0.
	GrowNoMixArea();

	msType = "Nominal1";
	Dump();
	CloseLog();
}

void CTubesMix::ComputeLimited()
{
	OpenLog("ComputeLimited");

	ComputeInternalVariables();

	for (int iS = 0; iS < mMergeTexSizeS; iS++)
	{
		ComputeColVariables(iS, true /*bSaveMargins*/);

		ComputeLineWeights(iS);
	}

	char zType[128];
	sprintf_s(zType, sizeof(zType), "Limited%.2fmm", mMarginsMm);
	msType = zType;
	Dump();
	CloseLog();
}