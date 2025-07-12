#include "stdafx.h"
#include "TubesMix.h"
#include "..\..\yUtils\MyWindows.h"
#include <iostream>
#include <fstream>

CTubesMix::CTubesMix()
	: mMergeTexSizeS(450)
	, mMergeTexSizeZ(1024)
	, mnParamsInitialized(0)
	, mpData(NULL)
	, msType("NULL")
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
	FILE* pfLog = NULL;
	char zBuf[128];
	sprintf_s(zBuf, sizeof(zBuf), "d:\\Log\\CTubesMix__ComputeNominal_width%d_height%d.txt",
		mMergeTexSizeZ, mMergeTexSizeS);
	fopen_s(&pfLog, zBuf, "w");

	if (pfLog)
	{
		fprintf(pfLog, "<calcTube1MergeWeight>\n");
		fprintf(pfLog, "mergeTexSizeZ %d\n", mMergeTexSizeZ);
		fprintf(pfLog, "mergeTexSizeS %d\n", mMergeTexSizeS);
		fprintf(pfLog, "totalTexZmm %f\n", mTotalTexZmm);
		fprintf(pfLog, "totalTexSmm %f\n", mTotalTexSmm);
		fprintf(pfLog, "bpP.distanceBetweenSensorsZ %f\n", mDistanceBetweenSensorsZ);
		fprintf(pfLog, "debug %d 0x%x\n", mDebug, mDebug);
		fprintf(pfLog, "\n");
	}


	double z_tube_0 = mDistanceBetweenXrts / 2.0 + mTubeShiftZ_tube0;
	double z_tube_1 = -mDistanceBetweenXrts / 2.0 + mTubeShiftZ_tube1;

	double locationOfLastZSensorCenter = (mnSlices / 2 - 0.5) * mDistanceBetweenSensorsZ;//in mm;
		//                    ReconFOV      512*4
	double texelSizeS = mTotalTexSmm / mMergeTexSizeS;	// in mm
	double texelSizeZ = mTotalTexZmm / mMergeTexSizeZ;	// in mm

	double firstTexelLocationS = -mTotalTexSmm / 2 + texelSizeS / 2;
	double firstTexelLocationZ = -mTotalTexZmm / 2 + texelSizeZ / 2;

	if (pfLog)
	{
		fprintf(pfLog, "z_tube_0 %f\n", z_tube_0);
		fprintf(pfLog, "z_tube_1 %f\n", z_tube_1);
		fprintf(pfLog, "locationOfLastZSensorCenter %f\n", locationOfLastZSensorCenter);
		fprintf(pfLog, "texelSizeS %f\n", texelSizeS);
		fprintf(pfLog, "texelSizeZ %f\n", texelSizeZ);
		fprintf(pfLog, "firstTexelLocationS %f\n", firstTexelLocationS);
		fprintf(pfLog, "firstTexelLocationZ %f\n", firstTexelLocationZ);
		fprintf(pfLog, "\n");
	}

	for (int iS = 0; iS < mMergeTexSizeS; iS++)
	{
		double positionS = firstTexelLocationS + iS * texelSizeS;	// position of texel, in mm, from iso-center.

		// (abs(z_tube_1) + z_c_edge) / (xrtToCenterMm - positionS) = (locationOfLastZSensorCenter + abs(z_tube_1)) / (DmsToCenterMm + xrtToCenterMm)
		double z_c_edge = (locationOfLastZSensorCenter + abs(z_tube_1)) / (mDmsToCenterMm + mXrtToCenterMm) * (mXrtToCenterMm - positionS) - abs(z_tube_1);
		double z_a_edge = (locationOfLastZSensorCenter + abs(z_tube_0)) / (mDmsToCenterMm + mXrtToCenterMm) * (mXrtToCenterMm - positionS) - abs(z_tube_0);
		z_a_edge *= -1;
		if (pfLog)
		{
			fprintf(pfLog, "%3d: S %f - z_c %f - z_a %f\n", iS, positionS, z_c_edge, z_a_edge);
		}

		for (int iZ = 0; iZ < mMergeTexSizeZ; iZ++)
		{
			double positionZ = firstTexelLocationZ + iZ * texelSizeZ;	// position of texel, in mm, from center.

			if (positionZ <= z_a_edge)	// w==1
			{
				if (iZ < mMergeTexSizeZ / 2)
					mpData[mMergeTexSizeZ * iS + iZ] = 1.0f;
				else
					mpData[mMergeTexSizeZ * iS + iZ] = 0.0f;
			}
			else if (positionZ >= z_c_edge)
			{
				mpData[mMergeTexSizeZ * iS + iZ] = 0.0f;
			}
			else
			{
				mpData[mMergeTexSizeZ * iS + iZ] = (float)(positionZ / (z_a_edge - z_c_edge) + (z_c_edge / (z_c_edge - z_a_edge)));
			}
		}
	}

	if (pfLog)
		fprintf(pfLog, "Start second pass, mergeTexSizeS %d, mergeTexSizeZ %d\n",
			mMergeTexSizeS, mMergeTexSizeZ);

	// "grow" the area where weight==1 and the area where weight==0.
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

		if (mDebug && pfLog)
		{
			fprintf(pfLog, "Second pass %3d: %3d - %3d", iS, iLast1, iFirst0);
			if (nGrow > 0)
				fprintf(pfLog, " Grew %d", nGrow);
			fprintf(pfLog, "\n");
		}
	}

	if (mDebug && iFirstDichotomic > 0)
	{
		int n = mDebug;
		int iCorrect = iFirstDichotomic - 1;
		while (n-- > 0 && iCorrect >= 0)
		{
			if (pfLog)
				fprintf(pfLog, "<DEBUG> correcting line %d\n", iCorrect);

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
	if (pfLog)
		fclose(pfLog);

	msType = "Nominal";
	if (mDump)
		Dump();
}
