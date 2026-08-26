#pragma once

class CRadiusImage
{
public:
	CRadiusImage(class CArinetaImages& images);
	~CRadiusImage();

	void Init(class CDataCoordinates& center);
	void Dump();
	float* GetData() { return mpData; }

	int mnLines = 0;
	int mnCols = 0;
	int mnPixels = 0;
	float* mpData = nullptr;
	float mMaxRadius = 0;
};


