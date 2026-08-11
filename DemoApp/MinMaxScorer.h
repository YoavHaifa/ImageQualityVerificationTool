#pragma once
#include "ScorerBase.h"

class CMinMaxScorer : public CScorerBase
{
public:
	CMinMaxScorer(const std::vector<float>& vRingMean);
	const char* Name() const override { return "MinMax"; }

protected:
	void ComputeScore() override;
};
