#include "stdafx.h"
#include "TrainingPlotDlg.h"
#include "Config.h"
#include "ScoreTypes.h"
#include <cstdio>
#include <cfloat>
#include <format>

using namespace std;

BEGIN_MESSAGE_MAP(CTrainingPlotDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_CBN_SELCHANGE(IDC_COMBO_PLOT_TYPE, &CTrainingPlotDlg::OnSelchangeComboPlotType)
END_MESSAGE_MAP()

CTrainingPlotDlg::CTrainingPlotDlg(CWnd* pParent)
	: CDialog(IDD, pParent)
{
}
BOOL CTrainingPlotDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_PLOT_TYPE);
	for (int iType = 0; iType < (int)EScoreType::N_SCORE_TYPES; iType++)
		pCombo->AddString(ScoreTypeName((EScoreType)iType));
	pCombo->SetCurSel((int)gConfig.mScoreType);

	LoadData();

	return TRUE;
}
void CTrainingPlotDlg::OnSelchangeComboPlotType()
{
	LoadData();
	Invalidate();
}
void CTrainingPlotDlg::LoadData()
{
	mvPoints.clear();
	SetDlgItemText(IDC_STATIC_PLOT_SELECTED, "Click a point to identify its case");

	CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_PLOT_TYPE);
	CString sTypeName;
	pCombo->GetLBText(pCombo->GetCurSel(), sTypeName);

	string sfName(format("{}\\TrainingSetReport\\TrainingSetReport_{}.csv", gConfig.msLogRoot, (LPCTSTR)sTypeName));

	FILE* pf = nullptr;
	fopen_s(&pf, sfName.c_str(), "r");
	if (!pf)
	{
		SetDlgItemText(IDC_STATIC_PLOT_LEGEND, "No report found - run Optimize > Score Training Data first.");
		return;
	}

	char zLine[512];
	fgets(zLine, sizeof(zLine), pf); // header

	int nPass = 0, nFail = 0;
	while (fgets(zLine, sizeof(zLine), pf))
	{
		// Columns (see COptimizer::WriteReports): label, expected, case, main area width, image,
		// ring, critical scorer, critical raw score, score, verdict, gap, assessment. Tokenize on
		// "," only (not whitespace) since case names can contain spaces.
		CString sLine(zLine);
		int iPos = 0;
		CString sLabel = sLine.Tokenize(",", iPos); sLabel.Trim();
		CString sExpected = sLine.Tokenize(",", iPos); sExpected.Trim();
		CString sCase = sLine.Tokenize(",", iPos); sCase.Trim();
		CString sWidth = sLine.Tokenize(",", iPos); sWidth.Trim();
		CString sImage = sLine.Tokenize(",", iPos); // unused
		CString sRing = sLine.Tokenize(",", iPos); // unused
		CString sCriticalScorer = sLine.Tokenize(",", iPos); // unused
		CString sCriticalRaw = sLine.Tokenize(",", iPos); sCriticalRaw.Trim();

		if (sLabel.IsEmpty() || sCriticalRaw.IsEmpty())
			continue;

		SPoint pt;
		// Colored by what THIS scorer is actually expected to produce for this case (e.g. a
		// Fail_Ring case counts as green under the Center scorer, since Center isn't responsible
		// for ring-only problems - see COptimizer::IsExpectedToFail), not the case's raw label.
		pt.bPass = (sExpected.CompareNoCase("Pass") == 0);
		pt.x = (float)atof(sWidth);
		pt.y = (float)atof(sCriticalRaw);
		pt.sCaseName = sCase;
		pt.sLabel = sLabel;
		mvPoints.push_back(pt);

		if (pt.bPass) nPass++; else nFail++;
	}
	fclose(pf);

	CString sLegend;
	sLegend.Format("X: main area width   Y: raw score   Expected Pass: %d (green)   Expected Fail: %d (red)", nPass, nFail);
	SetDlgItemText(IDC_STATIC_PLOT_LEGEND, sLegend);
}
void CTrainingPlotDlg::OnPaint()
{
	CPaintDC dc(this);

	CRect clientRect;
	GetClientRect(&clientRect);

	// Generous margins on every side, purely for the axis-endpoint labels drawn just outside the
	// plot box itself - left/bottom need room for numbers, right/top just need to clear the edge.
	CRect plotRect(clientRect.left + 50, clientRect.top + 30, clientRect.right - 40, clientRect.bottom - 70);
	DrawPlot(&dc, plotRect);
}
void CTrainingPlotDlg::DrawPlot(CDC* pDC, const CRect& plotRect)
{
	pDC->FillSolidRect(plotRect, RGB(255, 255, 255));
	pDC->Draw3dRect(plotRect, RGB(0, 0, 0), RGB(0, 0, 0));

	if (mvPoints.empty())
		return;

	float minX = FLT_MAX, maxX = -FLT_MAX, minY = FLT_MAX, maxY = -FLT_MAX;
	for (const SPoint& pt : mvPoints)
	{
		minX = min(minX, pt.x);
		maxX = max(maxX, pt.x);
		minY = min(minY, pt.y);
		maxY = max(maxY, pt.y);
	}
	// A little padding so points don't sit exactly on the plot's border
	float padX = max((maxX - minX) * 0.05f, 1.0f);
	float padY = max((maxY - minY) * 0.05f, 1.0f);
	minX -= padX; maxX += padX;
	minY -= padY; maxY += padY;

	pDC->SetBkMode(TRANSPARENT);

	CString sMinX, sMaxX, sMinY, sMaxY;
	sMinX.Format("%.0f", minX);
	sMaxX.Format("%.0f", maxX);
	sMinY.Format("%.2f", minY);
	sMaxY.Format("%.2f", maxY);

	// Right/left-aligned within a rect ending/starting at the plot's own edge, rather than a
	// guessed fixed offset - so a wide number (e.g. 4+ digits) grows away from the plot instead
	// of overflowing past it and getting clipped.
	const int LABEL_H = 14;
	CRect minXRect(plotRect.left, plotRect.bottom + 2, plotRect.left + 60, plotRect.bottom + 2 + LABEL_H);
	pDC->DrawText(sMinX, minXRect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOCLIP);
	CRect maxXRect(plotRect.right - 60, plotRect.bottom + 2, plotRect.right, plotRect.bottom + 2 + LABEL_H);
	pDC->DrawText(sMaxX, maxXRect, DT_RIGHT | DT_TOP | DT_SINGLELINE | DT_NOCLIP);
	CRect minYRect(plotRect.left - 48, plotRect.bottom - LABEL_H, plotRect.left - 2, plotRect.bottom);
	pDC->DrawText(sMinY, minYRect, DT_RIGHT | DT_BOTTOM | DT_SINGLELINE | DT_NOCLIP);
	CRect maxYRect(plotRect.left - 48, plotRect.top, plotRect.left - 2, plotRect.top + LABEL_H);
	pDC->DrawText(sMaxY, maxYRect, DT_RIGHT | DT_TOP | DT_SINGLELINE | DT_NOCLIP);

	for (SPoint& pt : mvPoints)
	{
		int sx = plotRect.left + (int)((pt.x - minX) / (maxX - minX) * plotRect.Width());
		int sy = plotRect.bottom - (int)((pt.y - minY) / (maxY - minY) * plotRect.Height());
		pt.screenX = sx;
		pt.screenY = sy;

		CBrush brush(pt.bPass ? RGB(0, 170, 0) : RGB(200, 0, 0));
		CBrush* pOldBrush = pDC->SelectObject(&brush);
		CPen pen(PS_SOLID, 1, pt.bPass ? RGB(0, 100, 0) : RGB(120, 0, 0));
		CPen* pOldPen = pDC->SelectObject(&pen);

		pDC->Ellipse(sx - 4, sy - 4, sx + 4, sy + 4);

		pDC->SelectObject(pOldPen);
		pDC->SelectObject(pOldBrush);
	}
}
void CTrainingPlotDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	CDialog::OnLButtonDown(nFlags, point);

	int iBest = -1;
	int bestDistSq = 7 * 7; // must click within ~7 pixels of a point's center
	for (int i = 0; i < (int)mvPoints.size(); i++)
	{
		int dx = point.x - mvPoints[i].screenX;
		int dy = point.y - mvPoints[i].screenY;
		int distSq = dx * dx + dy * dy;
		if (distSq <= bestDistSq)
		{
			bestDistSq = distSq;
			iBest = i;
		}
	}
	if (iBest < 0)
		return;

	const SPoint& pt = mvPoints[iBest];
	CString sInfo;
	sInfo.Format("Selected: %s   (label=%s, expected=%s, width=%.0f, raw score=%.2f)",
		(LPCTSTR)pt.sCaseName, (LPCTSTR)pt.sLabel, pt.bPass ? "Pass" : "Fail", pt.x, pt.y);
	SetDlgItemText(IDC_STATIC_PLOT_SELECTED, sInfo);
}
