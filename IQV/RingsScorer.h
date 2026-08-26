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

	// Replays each named scorer's already-saved results (from zCaseDir) instead of rescoring.
	// vScorerNames is the set of scorer names found in the case's CaseInfo.yaml; a name with
	// no matching live scorer (or vice versa) is just left unscored. Returns the image with
	// the max score, same convention as ScoreAllImages().
	int LoadFromSavedResults(const char* zCaseDir, const std::vector<CString>& vScorerNames);

	// Navigation in peaks by severity order
	void DisplayMaxPeak();
	void DisplayNextPeak();
	void DisplayPrevPeak();

	// Jump to the max peak of the newly active score type (peaks for all types are already computed)
	void OnActiveScoreTypeChanged();

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

