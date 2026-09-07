#pragma once

enum class EScoreType
{
	MinMax = 0,
	Tent = 1,
	TentMin = 2,
	Center = 3,
	AllMax = 4,
	N_SCORE_TYPES
};

inline const char* ScoreTypeName(EScoreType type)
{
	switch (type)
	{
	case EScoreType::MinMax: return "MinMax";
	case EScoreType::Tent: return "Tent";
	case EScoreType::TentMin: return "TentMin";
	case EScoreType::Center: return "Center";
	case EScoreType::AllMax: return "AllMax";
	default: return "Unknown";
	}
}

// True for the "ring scorers" (MinMax/Tent/TentMin) - the ones that get 3 independent
// per-region weights (see CConfig::GetScorerWeight(type, region)) rather than 1. Center is its
// own dedicated central-artifact detector, and AllMax is built from its siblings' already-weighted
// scores - neither owns a per-region weight of its own.
inline bool IsRingScorerType(EScoreType type)
{
	return type == EScoreType::MinMax || type == EScoreType::Tent || type == EScoreType::TentMin;
}

// The 4 image regions used for region-flagged labeling (CFailRegionsDlg/CaseLabelInfo.yaml) and,
// now, for per-region scoring weights and review filtering - see CConfig::ClassifyRing/
// IsRegionEnabled.
enum class ERegion
{
	Center = 0,
	HighRes = 1,
	Border = 2,
	LowRes = 3,
	N_REGIONS
};

inline const char* RegionName(ERegion region)
{
	switch (region)
	{
	case ERegion::Center: return "Center";
	case ERegion::HighRes: return "HighRes";
	case ERegion::Border: return "Border";
	case ERegion::LowRes: return "LowRes";
	default: return "Unknown";
	}
}
