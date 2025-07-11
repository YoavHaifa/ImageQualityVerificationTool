// DemoAppDlg.h : header file
//

#pragma once
#include "..\..\ImageRLib\TSharedImage.h"

// CDemoAppDlg dialog
class CDemoAppDlg : public CDialog
{
// Construction
public:
	CDemoAppDlg(CWnd* pParent = NULL);	// standard constructor
	~CDemoAppDlg(void);	// standard constructor

// Dialog Data
	enum { IDD = IDD_DEMOAPP_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	virtual void OnOK();

	CTSharedImage<short> *mpSharedVolume;

public:
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
	FILE *mpfLog;
	void DisplayPos(void);
	afx_msg void OnBnClickedButtonUpPos();
	afx_msg void OnFileOpen32771();
	afx_msg void OnFileExit();
	afx_msg void OnProcessSmooth();

	bool mbDisplayReadyImages;
	class CArchivesImages *mpImages;
	class CDataFiles* mpDataFiles;
	CList<class CArchivesImages*,class CArchivesImages*> mImages;
	CTSharedImage<unsigned short> *mpSmoothed;
	CTSharedImage<COLORREF> *mpColors;

	class CSmoother *mpSmoother;
	void UpdateSmooth(void);
	void ProcessImageInVolume(int iImage);
	bool InitProcessVolume(void);
	float mLowFactor;
	float mHighFactor;
	CString msAppendToPatientName;

	afx_msg void OnGetWindow();
	afx_msg void OnSetWindow();
	afx_msg void OnSetAutoWindow();
	afx_msg void OnProcessSave();
	afx_msg void OnProcessSaveall();
	afx_msg void OnProcessSaveallwithnewname();
	afx_msg void OnSetStatustext();
	afx_msg void OnSetCursurbroadcast();
	afx_msg void OnBnClickedButtonAddColors();
	afx_msg void OnProcessProcessvolume();
	afx_msg void OnProcessCurrentinvolume();
	int mMyViewerOffsetX;
	int mMyViewerOffsetY;
	afx_msg void OnProcessSavewithnewmatrix();
	afx_msg void OnSetTitle();
	afx_msg void OnBnClickedButtonAddColorMap();
	afx_msg void OnSetTogglecolormap();
	bool mbColormapOn;
	afx_msg void OnFileOpencolorer();
	afx_msg void OnSetWindowrange();
	afx_msg void OnGetTest();
	afx_msg void OnFileOpenbinary();
};

