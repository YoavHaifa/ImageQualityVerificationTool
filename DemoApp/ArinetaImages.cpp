#include "stdafx.h"
#include "ArinetaImages.h"
#include "Config.h"
#include "..\..\yUtils\MyDicom.h"
#include "..\..\yUtils\MyWindows.h"
#include <string>
#include <format>
#include <algorithm>
#include <vector>

using namespace std;

const unsigned short RECON_SHFT_X_GROUP = 0x00a3;
const unsigned short RECON_SHFT_X_NUM = 0x1004;

const unsigned short RECON_SHFT_Y_GROUP = 0x00a3;
const unsigned short RECON_SHFT_Y_NUM = 0x1005;


CArinetaImages::CArinetaImages(const char* zfName)
	: CArchivesImages(zfName)
{

}
bool CArinetaImages::IsImageDicom(const char* zfName)
{
	if (!CMyDicom::IsDicom(zfName))
		return false;

	CMyDicom dicom(zfName);
	if (!dicom.HasTagRef(RECON_SHFT_X_GROUP, RECON_SHFT_X_NUM))
		return false;
	if (!dicom.HasTagRef(RECON_SHFT_Y_GROUP, RECON_SHFT_Y_NUM))
		return false;

	return true;
}
bool CArinetaImages::ComputeRotationCenter()
{
	mRotationCenter.mpImage = mCurrentImage.mpImage;
	CMyDicom* pDicom = mCurrentImage.mpDicom;
	if (!pDicom)
	{
		CMyWindows::MessBox("Failed to find Dicom data", "Data Error");
		return false;
	}

	float xOffset = 0;
	if (!GetFloatValueFromDicomString(RECON_SHFT_X_GROUP, RECON_SHFT_X_NUM, xOffset, "Rotation Center X Shift"))
		return false;

	float yOffset = 0;
	if (!GetFloatValueFromDicomString(RECON_SHFT_Y_GROUP, RECON_SHFT_Y_NUM, yOffset, "Rotation Center Y Shift"))
		return false;

	mRotationCenter.fx = (float)((mCurrentImage.mpImage->GetNCols() - 1) / 2.0 - xOffset / mCurrentImage.mpImage->mMmPerPixelWidth);
	mRotationCenter.fy = (float)((mCurrentImage.mpImage->GetNLines() - 1) / 2.0 + yOffset / mCurrentImage.mpImage->mMmPerPixelHeight);

	// Not drawn here - the viewer doesn't exist yet at this point in some flows (e.g. case
	// review), and DisplayScore() draws the correctly-sized circle for the displayed image
	// right after the caller shows it anyway, making an initial draw here redundant.
	return true;
}
bool CArinetaImages::GetFloatValueFromDicomString(unsigned short group, unsigned short num, float& value, const char* zFor)
{
	CMyDicom* pDicom = mCurrentImage.mpDicom;

	CString s;
	if (!pDicom->GetTextFieldAtTopLevel(group, num, s))
	{
		string sError (std::format("Failed to find Dicom data 0x{:04x} 0x{:04x} for {}", group, num, zFor));
		CMyWindows::MessBox(sError.c_str(), "Data Error");
		return false;
	}

	value = (float)atof(s);
	return true;
}
bool CArinetaImages::PrepareOnInit()
{
	gConfig.PrintStatus("Loading...");
	LoadFullRange();

	miFirst = GetFirst();
	miLast = GetLast();

	// LoadFullRange() has no way to report failure (it's void); detect it here instead - if the
	// first image never actually decoded (e.g. it fails deep in DICOM parsing despite passing
	// the sample-file screening check), nothing downstream can be trusted.
	if (!mpSharedVolume || !mpSharedVolume->GetImageStart(miFirst))
		return false;

	mnPixelsInImage = mnLinesInPage * mnCols;

	mpSharedVolume->Dump();

	string s(format("All {} images loaded", GetNFiles()));
	gConfig.PrintStatus(s.c_str());

	mnSliceWidth = min(gConfig.mnWantedSliceWidth, (unsigned short)GetNFiles());
	if (mnSliceWidth > 1)
		return ComputeWideImages();
	return true;
}
bool CArinetaImages::EnsureCtPerRadiusVolume()
{
	if (mpCtPerRadiusVolume)
		return true;
	return CreateSharedVolume("CtPerRadius", mpCtPerRadiusVolume);
}
bool CArinetaImages::ComputeWideImages()
{
	gConfig.PrintStatus("Computing wide images...");

	if (!mpWideVolume)
	{
		bool bOK = CreateSharedVolume("WideSlices", mpWideVolume);
		if (!bOK)
			return false;
	}

	mpWideVolume->Zero();

	// When gConfig.mbFilterWideImageRange is off, every value counts as "in range" - the loops
	// below then behave exactly like the old unconditional average (outOfRangeCount always 0).
	// When it's on, only values at least gConfig.mWideMinThreshold count: the first/last few
	// images in a series often cover a smaller radius than the rest, so far-out pixels there
	// aren't real data (read as low values) and would otherwise pull the average toward garbage.
	auto isInRange = [](short value)
	{
		if (!gConfig.mbFilterWideImageRange)
			return true;
		return value >= gConfig.mWideMinThreshold;
	};

	// Copy first input image to the sum buffer
	int iInput = miFirst;
	short* pInput0 = mpSharedVolume->GetImageStart(iInput++);
	if (!pInput0)
		return false;

	// Sum first n slices in sum buffer, tracking (per pixel) how many of the samples summed so
	// far are out of range. A wide pixel only gets a computed value once every sample in its
	// window is in range; otherwise it's left at 0 (already zeroed above).
	vector<int> sumBuf(mnPixelsInImage, 0);
	vector<int> outOfRangeCount(mnPixelsInImage, 0);
	int nValidInFirstImage = 0;
	for (int iPix = 0; iPix < mnPixelsInImage; iPix++)
	{
		if (isInRange(pInput0[iPix]))
		{
			sumBuf[iPix] = pInput0[iPix];
			nValidInFirstImage++;
		}
		else
			outOfRangeCount[iPix] = 1;
	}
	unsigned short nSummed = 1;
	CString snFirstImage;
	snFirstImage.Format("<ComputeWideImages> %d pixels valid in first image", nValidInFirstImage);
	CMyWindows::PrintStatus(snFirstImage);

	// Sum first images in sum buffer
	while (nSummed < mnSliceWidth)
	{
		short* pInput = mpSharedVolume->GetImageStart(iInput++);
		if (!pInput)
			return false;
		for (int iPix = 0; iPix < mnPixelsInImage; iPix++)
		{
			if (isInRange(pInput[iPix]))
				sumBuf[iPix] += pInput[iPix];
			else
				outOfRangeCount[iPix]++;
		}
		nSummed++;
	}

	// Compute first average "wide" image
	int iTarget = miFirst;
	short* pTarget = mpWideVolume->GetImageStart(iTarget++);
	for (int iPix = 0; iPix < mnPixelsInImage; iPix++)
	{
		if (outOfRangeCount[iPix] == 0)
			pTarget[iPix] = (short)(sumBuf[iPix] / nSummed);
	}

	// Copy first target until the sliding sum window can move
	int nSame = mnSliceWidth / 2 + 1; // Assuming "slice width" is odd
		// The first images are same, as we want them with full width to reduce noise
	int nCopy = nSame - 1;
	short* pFirstTarget = pTarget;
	for (int iCopy = 0; iCopy < nCopy; iCopy++)
	{
		short* pTarget = mpWideVolume->GetImageStart(iTarget++);
		memcpy(pTarget, pFirstTarget, mnPixelsInImage * sizeof(short));
	}

	// Main loop with sum-window - as long as there are new input images to add
	int iSub = miFirst;
	while (iInput <= miLast)
	{
		short* pInput = mpSharedVolume->GetImageStart(iInput++);
		short* pSub = mpSharedVolume->GetImageStart(iSub++);
		if (!pInput || !pSub)
			return false;
		short* pTarget = mpWideVolume->GetImageStart(iTarget++);

		for (int iPix = 0; iPix < mnPixelsInImage; iPix++)
		{
			if (isInRange(pInput[iPix]))
				sumBuf[iPix] += pInput[iPix];
			else
				outOfRangeCount[iPix]++;

			if (isInRange(pSub[iPix]))
				sumBuf[iPix] -= pSub[iPix];
			else
				outOfRangeCount[iPix]--;

			if (outOfRangeCount[iPix] == 0)
				pTarget[iPix] = (short)(sumBuf[iPix] / nSummed);
		}
	}

	// Fill the rest of the target images with the last proper wide target computed
	short* pPrevTarget = mpWideVolume->GetImageStart(iTarget-1);
	while (iTarget <= miLast)
	{
		short* pTarget = mpWideVolume->GetImageStart(iTarget++);
		memcpy(pTarget, pPrevTarget, mnPixelsInImage * sizeof(short));
	}

	mpWideVolume->Dump();

	string s(format("All {} wide images computed", GetNFiles()));
	gConfig.PrintStatus(s.c_str());
	return true;
}
short* CArinetaImages::GetImageRaster(int iImage)
{
	if (iImage < miFirst || iImage > miLast)
	{
		string s(format("<CArinetaImages::GetImageRaster> illegal index {}", iImage));
		CMyWindows::MessBox(s.c_str(), "SW Error");
		return NULL;
	}

	if (mnSliceWidth > 1)
		return mpWideVolume->GetImageStart(iImage);
	if (mpSharedVolume)
		return mpSharedVolume->GetImageStart(iImage);

	SetCurrent(iImage);
	return (short*)mCurrentImage.mpImage->GetDataStart();
}