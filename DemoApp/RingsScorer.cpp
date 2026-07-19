#include "stdafx.h"
#include "RingsScorer.h"
#include "RadiusImage.h"
#include "ArinetaImages.h"
#include "ImageRingsScorer.h"

CRingsScorer::CRingsScorer(CArinetaImages* pImages)
	: mpImages(pImages)
{
	mpRadiusImage = new CRadiusImage(*pImages);
}
CRingsScorer::~CRingsScorer()
{
	delete mpRadiusImage;
}
float CRingsScorer::ScoreCurrentImage(int& oAtRing)
{
	CTImage<unsigned short>* pImage = mpImages->GetImage();
	
	CImageRingScorer scorer(pImage, mpRadiusImage);
	float score = scorer.Score();
	oAtRing = scorer.miRingOfScore;
	return score;

}