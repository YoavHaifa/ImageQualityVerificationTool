#pragma once
#include "ScorerBase.h"

class CTentScorer : public CScorerBase
{
public:
	CTentScorer(const std::vector<float>& vRingMean);
	const char* Name() const override { return "Tent"; }

protected:
	void ComputeScore() override;

private:
	void ComputeLocalMinScore(int iRing);
	void ComputeLocalMaxScore(int iRing);
};
