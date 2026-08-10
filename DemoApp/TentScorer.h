#pragma once
#include "ImageScore.h"
#include <vector>

class CTentScorer
{
public:
	CTentScorer(const std::vector<float>& vRingMean);
	void Score();

	CImageScore mScore;
	std::vector<float> mvRingScore; // score at every tent candidate ring found, 0 elsewhere

private:
	void ComputeLocalMinScore(int iRing);
	void ComputeLocalMaxScore(int iRing);

	const std::vector<float>& mvRingMean;
	int mnRings;
};
