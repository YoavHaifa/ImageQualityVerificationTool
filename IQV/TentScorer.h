#pragma once
#include "ScorerBase.h"

// Shared "tent" logic: finds local min/max flex points in the ring-mean profile, walks
// each leg of the tent out to where it stops descending/ascending, then scores the flex
// point by combining the two leg heights - CombineLegs() is the only thing that differs
// between the Tent scorers (CTentScorer averages the legs, CTentMinScorer takes the min).
class CTentScorerBase : public CScorerBase
{
public:
	CTentScorerBase(const std::vector<float>& vRingMean, EScoreType eScoreType);

protected:
	void ComputeScore() override;

	virtual float CombineLegs(float leftHeight, float rightHeight) const = 0;

private:
	void ComputeLocalMinScore(int iRing);
	void ComputeLocalMaxScore(int iRing);
};

class CTentScorer : public CTentScorerBase
{
public:
	CTentScorer(const std::vector<float>& vRingMean);

protected:
	float CombineLegs(float leftHeight, float rightHeight) const override;
};

class CTentMinScorer : public CTentScorerBase
{
public:
	CTentMinScorer(const std::vector<float>& vRingMean);

protected:
	float CombineLegs(float leftHeight, float rightHeight) const override;
};
