#pragma once
#include "ScoreTypes.h"
#include <vector>
#include <array>

// One case found under a batch-review root: its log directory, plus its worst score and
// severity rank for each scorer (index-aligned with CBatchReviewer's scorer name list).
// Mirrors CImageScore's mScore/miPeak, one level up - a case instead of an image.
class CCaseInfo
{
public:
	CString msCaseDir;
	std::vector<float> mvWorstScore; // one per scorer; 0 if that scorer had no saved results
	std::vector<int> mvOrder; // one per scorer; rank by severity (1 = worst), 0 = unranked

	// A ring scorer's own 3 regions' worst scores (HighRes/Border/LowRes, index-aligned with
	// ERegion - see CConfig::IsRegionEnabled), read from CaseInfo.yaml's nested highres/border/
	// lowres keys under that scorer's own block. {0,0,0} for a non-ring scorer (Center/AllMax),
	// which has no per-region breakdown. Lets CBatchReviewer::ComputeOrder() rank cases by an
	// active-region-filtered severity, without reopening/rescoring every case whenever the
	// "Review Regions" checkboxes change.
	std::vector<std::array<float, 3>> mvRegionScore;
};
