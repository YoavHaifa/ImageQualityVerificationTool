#pragma once
#include "ImageScore.h"
#include "ScoreTypes.h"
#include <vector>
#include <algorithm>

// Common interface for all per-image ring scorers (CMinMaxScorer, CTentScorer, ...).
// CImageRingsScorer holds a polymorphic list of these, created once and re-scored for
// every image - it creates them, but otherwise doesn't need to know which concrete
// types exist, how many there are, or how each scores.
class CScorerBase
{
public:
	CScorerBase(const std::vector<float>& vRingMean, EScoreType eScoreType)
		: mvRingMean(vRingMean)
		, mnRings((int)vRingMean.size() - 1)
		, meScoreType(eScoreType)
	{
		mvRingScore.assign(vRingMean.size(), 0.0f);
	}
	virtual ~CScorerBase() = default;

	// Clears state left over from whatever image was scored previously, then scores mvRingMean as it is now
	void Score()
	{
		mScore = CImageScore();
		std::fill(mvRingScore.begin(), mvRingScore.end(), 0.0f);
		ComputeScore();
	}
	const char* Name() const { return ScoreTypeName(meScoreType); }

	CImageScore mScore;
	std::vector<float> mvRingScore; // score at every candidate ring this scorer considered, 0 elsewhere

protected:
	virtual void ComputeScore() = 0;

	const std::vector<float>& mvRingMean;
	int mnRings;
	EScoreType meScoreType;
};
