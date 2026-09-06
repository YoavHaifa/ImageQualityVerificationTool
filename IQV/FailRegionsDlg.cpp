#include "stdafx.h"
#include "FailRegionsDlg.h"

BEGIN_MESSAGE_MAP(CFailRegionsDlg, CDialog)
	ON_BN_CLICKED(IDC_CHECK_REGION_CENTER, &CFailRegionsDlg::OnRegionCheckClicked)
	ON_BN_CLICKED(IDC_CHECK_REGION_HR, &CFailRegionsDlg::OnRegionCheckClicked)
	ON_BN_CLICKED(IDC_CHECK_REGION_BORDER, &CFailRegionsDlg::OnRegionCheckClicked)
	ON_BN_CLICKED(IDC_CHECK_REGION_LR, &CFailRegionsDlg::OnRegionCheckClicked)
END_MESSAGE_MAP()

CFailRegionsDlg::CFailRegionsDlg(CWnd* pParent)
	: CDialog(IDD, pParent)
{
}
BOOL CFailRegionsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Nothing checked yet - OK stays disabled until the labeler picks at least one region.
	UpdateOkEnabled();

	return TRUE;
}
void CFailRegionsDlg::OnRegionCheckClicked()
{
	UpdateOkEnabled();
}
void CFailRegionsDlg::UpdateOkEnabled()
{
	bool bAnyChecked = IsDlgButtonChecked(IDC_CHECK_REGION_CENTER)
		|| IsDlgButtonChecked(IDC_CHECK_REGION_HR)
		|| IsDlgButtonChecked(IDC_CHECK_REGION_BORDER)
		|| IsDlgButtonChecked(IDC_CHECK_REGION_LR);

	if (CWnd* pOk = GetDlgItem(IDOK))
		pOk->EnableWindow(bAnyChecked);
}
void CFailRegionsDlg::OnOK()
{
	mbCenter = IsDlgButtonChecked(IDC_CHECK_REGION_CENTER) != 0;
	mbHR = IsDlgButtonChecked(IDC_CHECK_REGION_HR) != 0;
	mbBorder = IsDlgButtonChecked(IDC_CHECK_REGION_BORDER) != 0;
	mbLR = IsDlgButtonChecked(IDC_CHECK_REGION_LR) != 0;

	// Belt-and-suspenders: IDOK is disabled whenever none are checked, so this shouldn't be
	// reachable, but never close as if confirmed with nothing actually selected.
	if (!mbCenter && !mbHR && !mbBorder && !mbLR)
		return;

	CDialog::OnOK();
}
