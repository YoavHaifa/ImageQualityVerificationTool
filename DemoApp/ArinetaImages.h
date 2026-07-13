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

private:
	CDataCoordinates mRotationCenter;

};

