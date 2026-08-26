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

	// Sized/positioned the same as the wide volume, but only ever populated by the scorer
	// (CImageRingsScorer::FillCtPerRadiusImage, gated by gConfig.mbDisplayCtPerRadius) - null
	// until that first happens, e.g. always in Case/Batch Review, which doesn't rescore.
	CTSharedImage<short>* GetSharedCtPerRadiusVolume() { return mpCtPerRadiusVolume; }

	// Lazily creates the CT-per-radius volume if it doesn't exist yet. Returns false only if
	// the underlying image isn't ready yet (see CArchivesImages::CreateSharedVolume).
	bool EnsureCtPerRadiusVolume();

private:
	bool ComputeWideImages();

	CTSharedImage<short>* mpWideVolume = nullptr;
	CTSharedImage<short>* mpCtPerRadiusVolume = nullptr;

	int mnPixelsInImage = 1;

	int miFirst = -1;
	int miLast = -1;
	//int mStep;

	CDataCoordinates mRotationCenter;
	unsigned short mnSliceWidth = 1; // Number of consecutive input slices to average
};

