#pragma once
#include "ImageScore.h"
#include "ScoreTypes.h"
#include "RingInfo.h"
#include "ScoreTypeResults.h"
#include "..\..\yUtils\TRange.h"
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
	CScorerBase(const std::vector<float>& vRingMean, EScoreType eScoreType);
	virtual ~CScorerBase() = default;

	// Clears state left over from whatever image was scored previously, then scores mvRingMean as it is now.
	// bEnoughData false (too few in-mask pixels to trust this image) scores it 0 without calling ComputeScore().
	void Score(int iImage, const std::vector<CRingInfo>& vRingsInfo, bool bEnoughData);

	// Called once all images have been scored and recorded - finalizes mResults' peaks
	void OnAllImagesScored()
	{
		mResults.OnAllImagesScored();
	}
	void ScaleScores(float factor)
	{
		mResults.ScaleScores(factor);
	}
	const char* Name() const { return ScoreTypeName(meScoreType); }

	// Writes this scorer's score/ring/peak data for every image scored so far to
	// <gConfig.msCaseLogDir>\ScoreAllImages_<Name>.csv, one row per image. iFirst/iStep
	// map internal image indices back to the original DICOM slice numbers.
	void LogAllImages(int iFirst, int iStep) const;

	// Reverse of LogAllImages: replays this scorer's score/ring data from
	// <zCaseDir>\ScoreAllImages_<Name>.csv into mResults, in place of an actual scoring pass.
	// Peak/peak_order columns aren't read back - OnAllImagesScored() recomputes them
	// identically from the replayed scores. Returns false if the CSV can't be opened.
	bool LoadSavedResults(const char* zCaseDir);

	CImageScore mScore;
	std::vector<float> mvRingScore; // score at every candidate ring this scorer considered, 0 elsewhere
	CScoreTypeResults mResults; // score+ring per image scored so far, for peak finding/navigation

protected:
	virtual void ComputeScore() = 0;
	void CorrectCenter(const std::vector<CRingInfo>& vRingsInfo);
	void FindMaxScorePerCurrentImage();

	STRange<int> ComputeDataRange(int iFrom, int n, const std::vector<CRingInfo>& vRingsInfo);

	const std::vector<float>& mvRingMean;
	const std::vector<CRingInfo>* mpRingsInfo = nullptr; // valid only during ComputeScore(), set by Score()
	int mnRings;
	EScoreType meScoreType;
};
