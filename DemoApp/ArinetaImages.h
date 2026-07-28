#pragma once
#include "..\..\ImageRLib\ArchivesImages.h"
#include "..\..\ImageRLib\DataCoordinates.h"

class CArinetaImages : public CArchivesImages
{
public:
	CArinetaImages(const char* zName);

	bool ComputeRotationCenter(class CDemoAppDlg* pDlg);

	bool GetFloatValueFromDicomString(unsigned short group, unsigned short num, float& value, const char* zFor);

	CDataCoordinates& GetRotationCenter() { return mRotationCenter; }

	bool PrepareOnInit();

	short* GetImageRaster(int iImage);

private:
	void ComputeWideImages();

	CTSharedImage<short>* mpWideVolume = nullptr;

	int mnPixelsInImage = 1;

	int miFirst = -1;
	int miLast = -1;
	//int mStep;

	CDataCoordinates mRotationCenter;
	unsigned short mnSliceWidth = 1; // Number of consecutive input slices to average
};

