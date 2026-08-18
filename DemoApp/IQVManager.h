#pragma once
#include <string>

// Loads a DICOM image set and runs the ring-quality scoring on it,
// independently of any UI. CDemoAppDlg is optional and only used to draw
// the rotation-center debug circle when a viewer dialog is present.
class CIQVManager
{
public:
	CIQVManager();
	~CIQVManager();

	bool LoadAndScore(const char* zImageFileName, class CDemoAppDlg* pDlg = nullptr);

	class CArinetaImages* GetImages(void) { return mpImages; }
	class CRingsScorer* GetRingsScorer(void) { return mpRingsScorer; }
	int GetScoredPosition(void) { return miScoredPosition; }

	std::string GetSetInfo(void);

private:
	class CArinetaImages* mpImages = nullptr;
	class CRingsScorer* mpRingsScorer = nullptr;
	int miScoredPosition = 0;
};
