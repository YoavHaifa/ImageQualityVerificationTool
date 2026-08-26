#pragma once
#include <vector>

// One case found under a batch-review root: its log directory, plus its worst score and
// severity rank for each scorer (index-aligned with CBatchReviewer's scorer name list).
// Mirrors CImageScore's mScore/miPeak, one level up - a case instead of an image.
class CCaseInfo
{
public:
	CString msCaseDir;
	std::vector<float> mvWorstScore; // one per scorer; 0 if that scorer had no saved results
	std::vector<int> mvOrder; // one per scorer; rank by severity (1 = worst), 0 = unranked
};
