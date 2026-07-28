#include "stdafx.h"
#include "Config.h"
#include "..\..\yUtils\MyWindows.h"

CConfig gConfig;

CConfig::CConfig()
{
	CMyWindows::VerifyDirectory(msScoreGraphsDir.c_str());
}