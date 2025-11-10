// CameraDlg.h
#pragma once

class CCameraDlg : public CDialogEx
{
public:
    CCameraDlg(CWnd* pParent = nullptr);
    virtual ~CCameraDlg();
    enum { IDD = IDD_CAMERA_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedBtnStart();
    afx_msg void OnBnClickedBtnDetect();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnDestroy();
    DECLARE_MESSAGE_MAP()

private:
    void InitializeCamera();
    void ReleaseCamera();
    void CaptureAndDisplay();
    void DisplayImage(const cv::Mat& img);
    void InitializeListControl(); 
    void AddHistoryItem(const CString& id, const CString& time, const CString& result, const CString& confidence, const CString& defectType);  // Ãß°¡

    Pylon::CInstantCamera m_camera;
    Pylon::CGrabResultPtr m_ptrGrabResult;
    bool m_bCameraRunning;
    CStatic m_staticCam;
    CEdit m_editResult;
    CEdit m_editDefectType;
    CButton m_btnStart;
    UINT_PTR m_nTimerID;
    CListCtrl m_listHistory;  
    CFont m_font;  
    int m_nHistoryID;  
};