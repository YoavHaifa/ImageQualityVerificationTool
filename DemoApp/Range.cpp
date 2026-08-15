#include "Range.h"

template<typename T>
SRange<T>::SRange()
	: mMin(0)
	, mMax(0)
{}
template<typename T>
SRange<T>::SRange(T value)
	: mMin(value)
	, mMax(value)
{}
template<typename T>
SRange<T>::SRange(T minVal, T maxVal)
	: mMin(minVal)
	, mMax(maxVal)
{}
template<typename T>
void SRange<T>::Add(T value)
{
	if (value < mMin)
		mMin = value;
	else if (value > mMax)
		mMax = value;
}
template<typename T>
void SRange<T>::Add(T minVal, T maxVal)
{
	if (minVal < mMin)
		mMin = minVal;
	if (maxVal > mMax)
		mMax = maxVal;
}
template<typename T>
float SRange<T>::AbsDeviation(float value)
{
	if (value < mMin)
		return mMin - value;
	if (value > mMax)
		return value - mMax;
	return 0;
}

template struct SRange<int>; // only instantiation currently used
