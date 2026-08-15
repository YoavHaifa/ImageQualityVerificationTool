#pragma once

// Tracks the min/max of a series of T values, and how far a value deviates outside that range
template<typename T>
struct SRange
{
public:
	SRange();
	SRange(T value);
	SRange(T minVal, T maxVal);

	void Add(T value);
	void Add(T minVal, T maxVal);
	float AbsDeviation(float value);

	T mMin;
	T mMax;
};
