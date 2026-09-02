#pragma once
#include "ScorerBase.h"
#include <memory>

// Aggregates this image's score as the max of every other scorer's (already-weighted) score.
// Must be appended last in CImageRingsScorer::CreateScorers(), since scorers are scored in list
// order and ComputeScore() here depends on every sibling already having scored this same image.
class CAllMaxScorer : public CScorerBase
{
public:
	CAllMaxScorer(const std::vector<float>& vRingMean, const std::vector<std::unique_ptr<CScorerBase>>* pAllScorers);

	// Overridden: ignores its own saved CSV and recomputes itself, image by image, as the max
	// of its siblings' mResults - which by then already reflect ScorerWeights.csv as it is now
	// (see CRingsScorer::LoadFromSavedResults, which loads scorers in registration order so
	// every sibling is loaded before this runs)
	bool LoadSavedResults(const char* zCaseDir, float dataRangeFactor) override;

protected:
	void ComputeScore() override;

private:
	const std::vector<std::unique_ptr<CScorerBase>>* mpAllScorers;
};
