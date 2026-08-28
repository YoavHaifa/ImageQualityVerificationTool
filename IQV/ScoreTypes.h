#pragma once

enum class EScoreType
{
	MinMax = 0,
	Tent = 1,
	TentMin = 2,
	N_SCORE_TYPES
};

inline const char* ScoreTypeName(EScoreType type)
{
	switch (type)
	{
	case EScoreType::MinMax: return "MinMax";
	case EScoreType::Tent: return "Tent";
	case EScoreType::TentMin: return "TentMin";
	default: return "Unknown";
	}
}
