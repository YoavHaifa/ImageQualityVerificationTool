#pragma once
#include "resource.h"

// Shown by the Label menu's "Save ... As Failed" entries before anything is saved: forces the
// labeler to say which of the 4 image regions (Center, High Resolution, Border, Low Resolution -
// see gConfig's region-boundary rings) actually show the problem. OK is only enabled once at
// least one box is checked; Cancel aborts with nothing saved.
class CFailRegionsDlg : public CDialog
{
public:
	CFailRegionsDlg(CWnd* pParent = nullptr);

	enum { IDD = IDD_FAIL_REGIONS };

	bool mbCenter = false;
	bool mbHR = false;
	bool mbBorder = false;
	bool mbLR = false;

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnRegionCheckClicked();
	DECLARE_MESSAGE_MAP()

	// Enables IDOK iff at least one of the 4 checkboxes is currently checked.
	void UpdateOkEnabled();
};
