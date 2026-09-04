#pragma once
#include "resource.h"
#include <vector>

// Optimize > Show Training Data Plot: a simple scatter plot of every labeled training case's
// main-area width (X axis - the case's own "data range") against one scorer's raw score (Y axis),
// colored by the human-assigned label (green Pass, red Fail). Reads the same
// TrainingSetReport_<type>.csv files "Score Training Data" already writes (see COptimizer) -
// doesn't rescore anything itself. Lets a scorer's raw signal be checked at a glance for whether
// it actually scales with data range the way the data-range correction (1000/width^2) assumes.
class CTrainingPlotDlg : public CDialog
{
public:
	CTrainingPlotDlg(CWnd* pParent = nullptr);

	enum { IDD = IDD_TRAINING_PLOT };

protected:
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnSelchangeComboPlotType();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()

	struct SPoint
	{
		float x = 0; // main area width
		float y = 0; // raw score
		bool bPass = true;
		CString sCaseName;

		// Where this point was last drawn on screen - set by DrawPlot(), used by
		// OnLButtonDown() to find which point (if any) was clicked.
		int screenX = 0;
		int screenY = 0;
	};

	// Reads TrainingSetReport_<currently selected type>.csv into mvPoints and updates the legend
	// line - clears mvPoints (with an explanatory legend) if that report doesn't exist yet.
	void LoadData();

	void DrawPlot(CDC* pDC, const CRect& plotRect);

	std::vector<SPoint> mvPoints;
};
