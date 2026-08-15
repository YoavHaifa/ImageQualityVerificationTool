#pragma once
#include "ImageScore.h"
#include "ScoreTypes.h"
#include "ScoreTypeResults.h"
#include <vector>
#include <algorithm>

// Common interface for all per-image ring scorers (CMinMaxScorer, CTentScorer, ...).
// CImageRingsScorer holds a polymorphic list of these, created once and re-scored for
// every image - it creates them, but otherwise doesn't need to know which concrete
// types exist, how many there are, or how each scores.
// The set of concrete scorer types is fixed (not dynamically extended), and every
// scorer needs the same across-images bookkeeping, so that bookkeeping (mResults)
// lives here rather than in a separate per-type array kept elsewhere.
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
	// Files the score just computed by Score() into mResults, under iImage
	void RecordScore(int iImage)
	{
		mResults.AddScore(mScore.mScore, mScore.miRing, iImage);
	}
	// Called once all images have been scored and recorded - finalizes mResults' peaks
	void OnAllImagesScored()
	{
		mResults.OnAllImagesScored();
	}
	const char* Name() const { return ScoreTypeName(meScoreType); }

	CImageScore mScore;
	std::vector<float> mvRingScore; // score at every candidate ring this scorer considered, 0 elsewhere
	CScoreTypeResults mResults; // score+ring per image scored so far, for peak finding/navigation

protected:
	virtual void ComputeScore() = 0;

	const std::vector<float>& mvRingMean;
	int mnRings;
	EScoreType meScoreType;
};
