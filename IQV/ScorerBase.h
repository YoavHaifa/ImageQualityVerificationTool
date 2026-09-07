#pragma once
#include "ImageScore.h"
#include "ScoreTypes.h"
#include "RingInfo.h"
#include "..\..\yUtils\TRange.h"
#include <vector>
#include <algorithm>

// Computes one scorer TYPE's raw per-ring scores (mvRingScore) for the current image - shared by
// the 3 "ring scorers" (CMinMaxScorer, CTentScorer, CTentMinScorer) and CCenterScorer. Turning
// those raw per-ring scores into actual weighted, recorded, region-scoped results is
// CImageRingsScorer's job (see CScorerResult) - this class only computes; it doesn't track
// per-image history, weight, or CSV logging/replay any more (all of that used to live here when
// there was exactly one result per scorer type - now a ring scorer has 3 independent region
// results, so that bookkeeping moved to CImageRingsScorer/CScorerResult, which don't care how
// many results a given type actually has).
class CScorerBase
{
public:
	CScorerBase(const std::vector<float>& vRingMean, EScoreType eScoreType);
	virtual ~CScorerBase() = default;

	// Fills mvRingScore for mvRingMean as it is right now - 0 everywhere if !bEnoughData (too few
	// in-mask pixels to trust this image).
	void ComputeRingScores(const std::vector<CRingInfo>& vRingsInfo, bool bEnoughData);

	const char* Name() const { return ScoreTypeName(meScoreType); }
	EScoreType GetScoreType() const { return meScoreType; }

	std::vector<float> mvRingScore; // score at every candidate ring this scorer considered, 0 elsewhere

protected:
	virtual void ComputeScore() = 0;
	void CorrectCenter(const std::vector<CRingInfo>& vRingsInfo);

	STRange<int> ComputeDataRange(int iFrom, int n, const std::vector<CRingInfo>& vRingsInfo);

	const std::vector<float>& mvRingMean;
	const std::vector<CRingInfo>* mpRingsInfo = nullptr; // valid only during ComputeScore(), set by ComputeRingScores()
	int mnRings;
	EScoreType meScoreType;
};
