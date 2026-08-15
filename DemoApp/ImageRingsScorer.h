#pragma once
#include "..\..\ImageRLib\TSharedImage.h"
#include "ScoreTypes.h"
#include <vector>
#include <memory>

// Compute the rings scores for a single image

class CScorerBase;
class CImageScore;

class CRingInfo
{
public:
	CRingInfo() {}
	void Add(int value)
	{
		mnPixelsInRange++;
		mSum += value;
		if (mnPixelsInRange == 1)
		{
			mMin = value;
			mMax = value;
		}
		else
		{
			if (value < mMin)
				mMin = value;
			if (value > mMax)
				mMax = value;
		}
	}

	int mnPixelsInRaster = 0;
	int mnPixelsInRange = 0;
	float mSum = 0;
	float mDiff = 0;
	int mMin = 0;
	int mMax = 0;
};

class CImageRingsScorer
{
public:
	CImageRingsScorer(class CArinetaImages* pImages, class CRadiusImage* pRadiusImage);
	~CImageRingsScorer();

	// Score the given image; safe to call repeatedly on the same instance, one image at a time
	const CImageScore& Score(int iImage);

	// Files the score each scorer just computed into its own mResults, under iImage
	void RecordScores(int iImage);

	// Call once, after all images have been scored and recorded, to finalize each scorer's peaks
	void OnAllImagesScored();

	// The score+ring already recorded for iImage, under the currently active score type (gConfig.mScoreType)
	const CImageScore& GetCurrentScore(int iImage) const;

	// The score+ring already recorded for iImage, under the given score type
	const CImageScore& GetScore(EScoreType eScoreType, int iImage) const;

	// The image index with the highest score, under the currently active score type
	int GetImageWithMaxScore() const;

	// The image index holding the given peak severity order under the currently active score type, or -1 if not found
	int FindImageIndexOfPeak(int iWantedPeak) const;

	//float mScore = 0;
	//int miRingOfScore = -1;

private:
	void CreateScorers();

	// The scorer of the given type; its mResults holds the score+ring history across all images scored so far
	class CScorerBase* GetScorer(EScoreType eScoreType) const { return mvScorers[(int)eScoreType].get(); }

	void CollectRingsInfo();
	void ErodeValidArea();

	void Log();

	class CArinetaImages* mpImages = nullptr;
	class CRadiusImage* mpRadiusImage;
	int miImage = -1;

	int mnRings = 0;
	int mnPixelsWithinThreshold = 0;

	std::vector<float> mvRingMean0;
	std::vector<float> mvRingMean;
	std::vector<CRingInfo> mvRingsInfo;
	std::vector<std::unique_ptr<CScorerBase>> mvScorers;

	bool mbLog = true;
};

