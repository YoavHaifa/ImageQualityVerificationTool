#include "stdafx.h"
#include "ArinetaImages.h"
#include "DemoAppDlg.h"
#include "Config.h"
#include "..\..\yUtils\MyDicom.h"
#include "..\..\yUtils\MyWindows.h"
#include <string>
#include <format>
#include <algorithm>

using namespace std;

const unsigned short RECON_SHFT_X_GROUP = 0x00a3;
const unsigned short RECON_SHFT_X_NUM = 0x1004;

const unsigned short RECON_SHFT_Y_GROUP = 0x00a3;
const unsigned short RECON_SHFT_Y_NUM = 0x1005;


CArinetaImages::CArinetaImages(const char* zfName)
	: CArchivesImages(zfName)
{

}
bool CArinetaImages::ComputeRotationCenter(CDemoAppDlg* pDlg)
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

	pDlg->DisplayCircle(mRotationCenter, 100.0f);
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
	CMyWindows::PrintStatus("Loading...");
	LoadFullRange();

	mnPixelsInImage = mnLinesInPage * mnCols;

	miFirst = GetFirst();
	miLast = GetLast();

	mpSharedVolume->Dump();

	string s(format("All {} images loaded", GetNFiles()));
	CMyWindows::PrintStatus(s.c_str());

	mnSliceWidth = min(gConfig.mnWantedSliceWidth, (unsigned short)GetNFiles());
	if (mnSliceWidth > 1)
		ComputeWideImages();
	return true;
}
void CArinetaImages::ComputeWideImages()
{
	CMyWindows::PrintStatus("Computing wide images...");

	if (!mpWideVolume)
	{
		bool bOK = CreateSharedVolume("WideSlices", mpWideVolume);
		if (!bOK)
			return;
	}

	mpWideVolume->Zero();

	// Sum first n slices in sum buffer
	int* pSumBuf = new int[mnPixelsInImage];

	// Copy first input image to the sum buffer
	int iInput = miFirst;
	short* pInput0 = mpSharedVolume->GetImageStart(iInput++);
	for (int iPix = 0; iPix < mnPixelsInImage; iPix++)
		pSumBuf[iPix] = pInput0[iPix];
	unsigned short nSummed = 1;

	// Sum first images in sum buffer
	while (nSummed < mnSliceWidth)
	{
		short* pInput = mpSharedVolume->GetImageStart(iInput++);
		for (int iPix = 0; iPix < mnPixelsInImage; iPix++)
			pSumBuf[iPix] += pInput[iPix];
		nSummed++;
	}

	// Compute first average "wide" image 
	int iTarget = miFirst;
	short* pTarget = mpWideVolume->GetImageStart(iTarget++);
	for (int iPix = 0; iPix < mnPixelsInImage; iPix++)
		pTarget[iPix] = (short)(pSumBuf[iPix] / nSummed);

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
		short* pTarget = mpWideVolume->GetImageStart(iTarget++);

		for (int iPix = 0; iPix < mnPixelsInImage; iPix++)
		{
			pSumBuf[iPix] += (pInput[iPix] - pSub[iPix]);
			pTarget[iPix] = (short)(pSumBuf[iPix] / nSummed);
		}
	}

	// Fill the rest of the target images with the last proper wide target computed
	short* pPrevTarget = mpWideVolume->GetImageStart(iTarget-1);
	while (iTarget <= miLast)
	{
		short* pTarget = mpWideVolume->GetImageStart(iTarget++);
		memcpy(pTarget, pPrevTarget, mnPixelsInImage * sizeof(short));
	}

	delete[] pSumBuf;
	mpSharedVolume->Dump();

	string s(format("All {} wide images computed", GetNFiles()));
	CMyWindows::PrintStatus(s.c_str());
}