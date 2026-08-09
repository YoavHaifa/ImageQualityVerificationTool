#pragma once

enum class EScoreType
{
	MinMax = 0,
	Tent = 1,
	N_SCORE_TYPES
};

inline const char* ScoreTypeName(EScoreType type)
{
	switch (type)
	{
	case EScoreType::MinMax: return "MinMax";
	case EScoreType::Tent: return "Tent";
	default: return "Unknown";
	}
}
