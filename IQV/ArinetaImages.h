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

	// Full-volume dump files (CMyImage::Dump, under CMyImage::GetDumpDir() - d:\MyLog\Images by
	// default), written unconditionally once each volume is fully computed, purely so they're
	// always available for offline debugging. Also lets the viewer display from the file instead
	// of the live shared volume when gConfig.mbAvoidSharedMemory is on (see CIQVDlg::DisplayVolume) -
	// empty if the dump hasn't happened yet or failed (e.g. the dump directory doesn't exist).
	CString GetWideVolumeDumpName() const { return msWideDumpName; }
	CString GetCtPerRadiusDumpName() const { return msCtPerRadiusDumpName; }

	// Dumps the CT-per-radius volume to disk - unlike the wide volume (dumped once, internally,
	// right after ComputeWideImages() finishes computing it), this is filled incrementally one
	// image at a time by the scorer, so the caller must call this once scoring all images is done
	// (see CRingsScorer::ScoreAllImages). No-op if the volume was never populated.
	void DumpCtPerRadiusVolume();

private:
	bool ComputeWideImages();

	CTSharedImage<short>* mpWideVolume = nullptr;
	CTSharedImage<short>* mpCtPerRadiusVolume = nullptr;

	CString msWideDumpName;
	CString msCtPerRadiusDumpName;

	int mnPixelsInImage = 1;

	int miFirst = -1;
	int miLast = -1;
	//int mStep;

	CDataCoordinates mRotationCenter;
	unsigned short mnSliceWidth = 1; // Number of consecutive input slices to average
};

