#include "stdafx.h"
#include "RingsScorer.h"
#include "RadiusImage.h"

CRingsScorer::CRingsScorer(CArinetaImages* pImages)
	: mpImages(pImages)
{
	mpRadiusImage = new CRadiusImage(*pImages);
}
CRingsScorer::~CRingsScorer()
{
	delete mpRadiusImage;
}
