// PETBottleSealDefectDlg.h
#pragma once

class CPETBottleSealDefectDlg : public CDialogEx
{
public:
    CPETBottleSealDefectDlg(CWnd* pParent = nullptr);
    virtual ~CPETBottleSealDefectDlg();

    enum { IDD = IDD_PETBOTTLESEALDEFECT_DIALOG };

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
    void DisplayResult(const CString& result);

    Pylon::CInstantCamera m_camera;
    Pylon::CGrabResultPtr m_ptrGrabResult;
    bool m_bCameraRunning;
    CStatic m_staticCam;
    CEdit m_editResult;
    CEdit m_editDefectType;
    CButton m_btnStart;
    CButton m_btnDetect;
    UINT_PTR m_nTimerID;
};