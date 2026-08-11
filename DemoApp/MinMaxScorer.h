#pragma once
#include "ScorerBase.h"

class CMinMaxScorer : public CScorerBase
{
public:
	CMinMaxScorer(const std::vector<float>& vRingMean);

protected:
	void ComputeScore() override;
};
