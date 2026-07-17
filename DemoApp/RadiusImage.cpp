#include "stdafx.h"
#include "RadiusImage.h"
#include "ArinetaImages.h"
#include "..\..\ImageRLib\DataCoordinates.h"
#include <string>
#include <format>

using namespace std;

CRadiusImage::CRadiusImage(class CArinetaImages& images)
	: mnCols(images.GetNCols())
	, mnLines(images.GetNLines())
	, mnPixels(images.GetNPixelsInPage())
{
	Init(images.GetRotationCenter());
	Dump();
}
CRadiusImage::~CRadiusImage()
{
	delete[] mpData;
}

void CRadiusImage::Init(CDataCoordinates& center)
{
	mpData = new float[mnPixels];

	float reconFOV = 250;
	float pixelSize = reconFOV / mnCols;		// assuming (imageW == imageH)

	float* pSave = mpData;
	for (int iy = 0; iy < mnLines; iy++)
	{
		//const float y = -(iy - (float)(mnLines - 1) / 2.0f) * pixelSize + reconFOVYOffset;	// in mm
		const float y = iy - center.fy;	// in pixels

		for (int ix = 0; ix < mnCols; ix++)
		{
			//const float x = (ix - (float)(mnCols - 1) / 2.0f) * pixelSize + reconFOVXOffset;	// in mm
			const float x = ix - center.fx;	// in pixels
			float radius2 = x * x + y * y;
			float radius = sqrtf(radius2);
			*pSave++ = radius;
			if (radius > mMaxRadius)
				mMaxRadius = radius;
		}
	}
}
void CRadiusImage::Dump()
{
	string sfName(format("d:\\Dump\\RadiusImage_width{}_height{}.float.dat", mnCols, mnLines));
	FILE* pf = NULL;

	fopen_s(&pf, sfName.c_str(), "wb");
	if (pf)
	{
		fwrite(mpData, sizeof(mpData[0]), mnPixels, pf);
		fclose(pf);
	}
}
