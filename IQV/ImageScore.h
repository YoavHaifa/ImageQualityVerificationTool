#pragma once
#include "ScoreTypes.h"

class CImageScore
{
public:
	float mScore = 0; // Final score - after this scorer's weight is applied
	float mRawScore = 0; // Score before this scorer's weight is applied
	int miRing = -1; // Undefined ring is -1
	bool mbPeak = false; // Until peaks are identified
	int miPeak = 0;

	// Original DICOM slice number this row was scored from - set by CScoreTypeResults::AddScore,
	// lets CAllMaxScorer reconstruct itself from siblings' mResults on replay (see LoadSavedResults)
	int miOriginalImage = -1;

	// Which scorer this score actually came from - set only by CAllMaxScorer; N_SCORE_TYPES
	// ("none") for every other scorer's own scores
	EScoreType meSourceType = EScoreType::N_SCORE_TYPES;
};

