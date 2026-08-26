#pragma once
#include "ScorerBase.h"

class CTentScorer : public CScorerBase
{
public:
	CTentScorer(const std::vector<float>& vRingMean);

protected:
	void ComputeScore() override;

private:
	void ComputeLocalMinScore(int iRing);
	void ComputeLocalMaxScore(int iRing);
};
