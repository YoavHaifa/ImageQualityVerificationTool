#include "stdafx.h"
#include "ScorerResult.h"
#include "Config.h"
#include <string>
#include <format>

using namespace std;

CScorerResult::CScorerResult(EScoreType eType, ERegion eRegion, const CString& sName)
	: meScoreType(eType)
	, meRegion(eRegion)
	, msName(sName)
{
}
bool CScorerResult::IsActive() const
{
	if (meScoreType == EScoreType::AllMax)
		return true; // the aggregator itself is always active - what feeds INTO it is filtered per-input instead

	// Both a ring scorer's region (HighRes/Border/LowRes) and Center's own region are gated the
	// same way - Center is not special here, it's just as toggleable as any other region.
	return gConfig.IsRegionEnabled(meRegion);
}
void CScorerResult::LogAllImages(int iFirst, int iStep) const
{
	string sfName(format("{}\\ScoreAllImages_{}.csv", gConfig.msCaseLogDir.c_str(), (LPCTSTR)msName));

	FILE* pfLog = nullptr;
	fopen_s(&pfLog, sfName.c_str(), "w");
	if (!pfLog)
		return;

	fprintf(pfLog, "image, raw_score, score, ring, peak, peak_order\n");
	for (int iImage = 0; iImage < mResults.NumImages(); iImage++)
	{
		int iOriginal = iFirst + iImage * iStep;
		const CImageScore& score = mResults[iImage];
		fprintf(pfLog, "%d, %.6f, %.6f, %d, %s, %d\n", iOriginal, score.mRawScore, score.mScore, score.miRing, score.mbPeak ? "Peak" : "-", score.miPeak);
	}
	fclose(pfLog);
}
bool CScorerResult::LoadSavedResults(const char* zCaseDir, float weight, float dataRangeFactor)
{
	string sfName(format("{}\\ScoreAllImages_{}.csv", zCaseDir, (LPCTSTR)msName));

	FILE* pf = nullptr;
	fopen_s(&pf, sfName.c_str(), "r");
	if (!pf)
		return false;

	char zLine[256];
	fgets(zLine, sizeof(zLine), pf); // header

	while (fgets(zLine, sizeof(zLine), pf))
	{
		int iOriginal, iRing;
		float rawScore, oldWeightedScore;
		if (sscanf_s(zLine, "%d, %f, %f, %d", &iOriginal, &rawScore, &oldWeightedScore, &iRing) == 4)
		{
			// Re-weight from the saved raw score with weight/dataRangeFactor as they are *now* -
			// lets a changed ScorerWeights.csv (or a changed data range correction) take effect on
			// replay without rescoring.
			CImageScore score;
			score.mRawScore = rawScore;
			score.mScore = rawScore * weight * dataRangeFactor;
			score.miRing = iRing;
			mResults.AddScore(score, iOriginal);
		}
	}
	fclose(pf);
	return true;
}
