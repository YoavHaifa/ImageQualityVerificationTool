#pragma once
#include <vector>

class CRingsScorer
{
public:
	CRingsScorer(class CArinetaImages* pImages);
	~CRingsScorer();

	class CArinetaImages* mpImages;
	class CRadiusImage* mpRadiusImage;


};

