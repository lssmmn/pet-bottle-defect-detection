// DefectDlg.h
#pragma once

class CDefectDlg : public CDialogEx
{
public:
    CDefectDlg(CWnd* pParent = nullptr);
    enum { IDD = IDD_DEFECT_DIALOG };

    cv::Mat m_defectImage;
    CString m_imagePath;
    CString m_defectType;
    CString m_confidence;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedBtnRecheck();
    afx_msg void OnBnClickedBtnApprove();
    afx_msg void OnPaint();

    void DisplayImage();
    void AdjustButtonPositions();
    void DrawNoImageText(CDC* pDC, CRect rect, CString text);

    DECLARE_MESSAGE_MAP()

private:
    CStatic m_staticDefectImage;
    CEdit m_editMemo;
    CFont m_font;
};