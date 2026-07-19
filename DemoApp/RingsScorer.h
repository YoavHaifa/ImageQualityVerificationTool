#pragma once
#include <vector>

class CRingsScorer
{
public:
	CRingsScorer(class CArinetaImages* pImages);
	~CRingsScorer();

	float ScoreCurrentImage(int& oAtRing);

private:
	class CArinetaImages* mpImages = nullptr;
	class CRadiusImage* mpRadiusImage = nullptr;
	class CImageRingScorer* mpImageScorer = nullptr;
};

