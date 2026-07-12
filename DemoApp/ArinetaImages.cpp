#include "stdafx.h"
#include "ArinetaImages.h"
#include "..\..\yUtils\MyDicom.h"
#include "..\..\yUtils\MyWindows.h"
#include <string>
#include <format>

using namespace std;

const unsigned short RECON_SHFT_X_GROUP = 0x00a3;
const unsigned short RECON_SHFT_X_NUM = 0x1004;

const unsigned short RECON_SHFT_Y_GROUP = 0x00a3;
const unsigned short RECON_SHFT_Y_NUM = 0x1005;


CArinetaImages::CArinetaImages(const char* zfName)
	: CArchivesImages(zfName)
{

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