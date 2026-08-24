#pragma once
#include "resource.h"

// Shown when a GUI-driven batch scoring run finishes, offering to jump straight into
// reviewing the just-scored results instead of just acknowledging and closing.
class CBatchCompleteDlg : public CDialog
{
public:
	CBatchCompleteDlg(const CString& sMessage, CWnd* pParent = nullptr);

	enum { IDD = IDD_BATCH_COMPLETE };

protected:
	virtual BOOL OnInitDialog();

	CString msMessage;
};
