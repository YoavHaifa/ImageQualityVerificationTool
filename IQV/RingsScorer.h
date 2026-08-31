#pragma once
#include "ScoreTypes.h"
#include <vector>

class CRingsScorer
{
public:
	CRingsScorer(class CArinetaImages* pImages);
	~CRingsScorer();

	const class CImageScore& ScoreCurrentImage(int iImage);

	// Compute score of all images
	// Return miPos of image with max score (to be displayed)
	int ScoreAllImages();

	// Replays every scorer's already-saved results (from zCaseDir) instead of rescoring. A
	// scorer with no matching saved CSV (e.g. added since this case was scored) is just left
	// unscored - LoadSavedResults() on it returns false harmlessly. Returns the image with the
	// max score, same convention as ScoreAllImages().
	int LoadFromSavedResults(const char* zCaseDir);

	// Navigation in peaks by severity order
	void DisplayMaxPeak();
	void DisplayNextPeak();
	void DisplayPrevPeak();

	// Jump to the max peak of the newly active score type (peaks for all types are already computed)
	void OnActiveScoreTypeChanged();

	// The uniform per-image scale applied on top of each scorer's own weight, derived from this
	// case's pixel-value histogram main area (see ScoreAllImages()) - read back from CaseInfo.yaml
	// on replay, since redoing the histogram would need rescoring. 1.0 if never computed/read.
	float GetDataRangeScoreFactor() const { return mDataRangeScoreFactor; }

private:
	static constexpr int N_SCORE_TYPES = (int)EScoreType::N_SCORE_TYPES;

	void Log();
	void LogPerScorer();
	void LogCaseInfo();

	bool LookForPeak(int iWantedPeak);

	int miFirst = 0;
	int miLast = 0;
	int mStep = 1;
	int mnImages = 0;

	int miCurrentPeak = 0;
	int miCurrentPeakImage = 0;

	// Width of the pixel-value histogram's main area, and the score-scaling factor derived from
	// it (1000 / width^2) - set once in ScoreAllImages(), reused (rather than recomputed) when
	// applying the scale and again when logging both to CaseInfo.yaml
	int miMainAreaWidth = 0;
	float mDataRangeScoreFactor = 1.0f;

	bool mbScoresComputed = false;

	class CArinetaImages* mpImages = nullptr;
	class CRadiusImage* mpRadiusImage = nullptr;
	class CImageRingsScorer* mpImageScorer = nullptr;
};

