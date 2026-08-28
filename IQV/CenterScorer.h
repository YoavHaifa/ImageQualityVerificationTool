#pragma once
#include "ScorerBase.h"

// Scores a central artifact (a high or low bump in the mean-per-ring profile near the
// image center) by how far it - and its "shoulders" walking outward - diverge from the
// surrounding ring data. mnCentralRings only gates where a candidate local min/max is
// looked for; the scored area then grows outward ring by ring for as long as each ring
// is still out of range vs. its own surrounding, so its width isn't bounded by config.
class CCenterScorer : public CScorerBase
{
public:
	CCenterScorer(const std::vector<float>& vRingMean);

protected:
	void ComputeScore() override;

private:
	void ScoreFromCandidate(int iCandidate, bool bHigh);
};
