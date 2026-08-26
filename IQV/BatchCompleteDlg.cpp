#include "stdafx.h"
#include "BatchCompleteDlg.h"

CBatchCompleteDlg::CBatchCompleteDlg(const CString& sMessage, CWnd* pParent)
	: CDialog(IDD, pParent)
	, msMessage(sMessage)
{
}
BOOL CBatchCompleteDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetDlgItemText(IDC_STATIC_BATCH_COMPLETE_MSG, msMessage);

	return TRUE;
}
