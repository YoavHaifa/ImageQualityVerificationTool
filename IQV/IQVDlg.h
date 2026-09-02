#pragma once

// IQVDlg.h : header file
//

#include "..\..\ImageRLib\TSharedImage.h"
#include "..\..\yUtils\MyDialogEx.h"
#include "resource.h"		// main symbols

// CIQVDlg dialog
class CIQVDlg : public CMyDialogEx
{
// Construction
public:
	CIQVDlg(CWnd* pParent = NULL);	// standard constructor
	~CIQVDlg(void);	// standard constructor

// Dialog Data
	enum { IDD = IDD_IQV_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

// Implementation
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	virtual void OnOK();

	void DisplayScore();

	CTSharedImage<short> *mpSharedVolume;

public:
	void OnCurrentSelectedByScorer(int iPos);

	afx_msg void OnBnClickedButtonShared();
	class CImageRIF *mpImageRIF;
	void LoadViewer(void);
	bool LoadViewerWithImages(const char *zName);
	void InitSharedVolume(void);
	int mnImagesInRow;
	int mnImageRows;
	int mnImageLines;
	int mnImageCols;
	afx_msg void OnBnClickedButtonAddRoi();
	int miPos;
	int miPos2d;
	afx_msg void OnFileOpen32771();
	afx_msg void OnFileBatchscoring();
	afx_msg void OnFileOpencasescoring();
	afx_msg void OnFileOpenbatchscoring();
	afx_msg void OnTestFinddicomsets();
	afx_msg void OnUtilsDownloaddata();
	afx_msg void OnFileExit();

	// True (after showing why) if a case/batch is already open (mpImageRIF set) - the caller
	// should abort rather than reassign mpImages/mpRingsScorer/mpCaseReviewer/mpBatchReviewer out
	// from under the still-open viewer. Previously these callers silently did nothing instead.
	bool WarnIfAlreadyDisplaying();

	// Tears down whatever case/batch is currently open (viewer, manager, reviewers, the extra-
	// series image list) so a caller can start fresh - same cleanup the destructor does, minus
	// mpSharedVolume/mpColors (unrelated to case viewing). No-op if nothing is open.
	void CloseCurrentViewer();
	afx_msg void OnLabelSaveAllAsPassed();
	afx_msg void OnLabelSaveAllAsFailed();
	afx_msg void OnLabelSaveSectionAsPassed();
	afx_msg void OnLabelSaveSectionAsFailed();
	afx_msg void OnOptimizeScoretrainingdata();
	afx_msg void OnOptimizeScoreweights();

	// Copies the current case's DICOM files, as-is, to gConfig.msTrainingSetRoot\[Pass|Fail]\
	// <case-relative dir> - either the whole case (bWholeCase) or just gConfig.mSavedSectionLength
	// images centered on the one currently displayed, clipped to the case's own image range.
	void SaveLabeledData(bool bPass, bool bWholeCase);

	bool mbDisplayReadyImages;
	class CArinetaImages* mpImages;
	class CRingsScorer* mpRingsScorer;
	class CIQVManager* mpIQVManager;
	class CCaseReviewer* mpCaseReviewer;
	class CBatchReviewer* mpBatchReviewer;
	class CDataFiles* mpDataFiles;
	CList<class CArchivesImages*,class CArchivesImages*> mImages;
	CTSharedImage<COLORREF> *mpColors;

	afx_msg void OnBnClickedButtonAddColors();
	int mMyViewerOffsetX;
	int mMyViewerOffsetY;

	void DisplayCircle(CDataCoordinates& center, float radius);

	// Pass/fail indicator state for the currently displayed score - see
	// CConfig::IsPass()/ComputeCertaintyFraction(). mbHasScore is false until DisplayScore() has
	// actually shown a real score at least once (e.g. before any case is loaded), in which case
	// the indicator paints blank rather than a stale/default verdict. Updated in DisplayScore(),
	// painted in OnPaint().
	bool mbHasScore = false;
	bool mbScorePass = true;
	float mScoreCertaintyFraction = 0.0f;
	void PaintPassFailIndicator();

	// Shows/hides the "leaf between cases" row (only meaningful in batch review) - hidden by
	// default (OnInitDialog), shown once DisplayBatchCase() first runs.
	void ShowBatchCaseControls(bool bShow);

	// Displays pVolume (wide-images or CT-per-radius) in the viewer - normally shared live via
	// DisplayShared(), but if gConfig.mbAvoidSharedMemory is on (work-around for a machine where
	// shared memory shows blank), has ImageR open sDumpFileName (already dumped to disk by
	// CArinetaImages, unconditionally, once the volume was fully computed) instead.
	// No-op if pVolume is null (CT-per-radius isn't always populated).
	void DisplayVolume(CTSharedImage<short>* pVolume, const CString& sDumpFileName);
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedButtonMax();
	afx_msg void OnBnClickedButtonNext();
	afx_msg void OnBnClickedButtonPrev();
	afx_msg void OnCbnSelchangeComboScoreType();

	afx_msg void OnBnClickedButtonWorstCase();
	afx_msg void OnBnClickedButtonNextCase();
	afx_msg void OnBnClickedButtonPrevCase();
	void DisplayBatchCase();
};

extern CIQVDlg* gpDlg;