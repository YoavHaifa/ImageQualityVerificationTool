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
	void GetPos1(void);
	afx_msg void OnBnClickedButtonAddRoi();
	int miPos;
	int miPos2d;
	afx_msg void OnBnClickedButtonUpPos();
	afx_msg void OnFileOpen32771();
	afx_msg void OnFileBatchscoring();
	afx_msg void OnFileOpencasescoring();
	afx_msg void OnFileOpenbatchscoring();
	afx_msg void OnTestFinddicomsets();
	afx_msg void OnUtilsDownloaddata();
	afx_msg void OnFileExit();

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