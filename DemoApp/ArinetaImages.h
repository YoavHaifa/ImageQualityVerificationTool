#pragma once
#include "..\..\ImageRLib\ArchivesImages.h"
#include "..\..\ImageRLib\DataCoordinates.h"

class CArinetaImages : public CArchivesImages
{
public:
	CArinetaImages(const char* zName);

	// Cheap check for whether zfName is an actual CT image, without loading a full image set:
	// true only if it's a DICOM file and carries the private fields ComputeRotationCenter()
	// needs. Some sets mix in non-image DICOM files (e.g. reports) that otherwise match the
	// sample-file pattern.
	static bool IsImageDicom(const char* zfName);

	bool ComputeRotationCenter();

	bool GetFloatValueFromDicomString(unsigned short group, unsigned short num, float& value, const char* zFor);

	CDataCoordinates& GetRotationCenter() { return mRotationCenter; }

	bool PrepareOnInit();

	short* GetImageRaster(int iImage);
	CTSharedImage<short>* GetSharedWideVolume() { return mpWideVolume; }

private:
	bool ComputeWideImages();

	CTSharedImage<short>* mpWideVolume = nullptr;

	int mnPixelsInImage = 1;

	int miFirst = -1;
	int miLast = -1;
	//int mStep;

	CDataCoordinates mRotationCenter;
	unsigned short mnSliceWidth = 1; // Number of consecutive input slices to average
};

