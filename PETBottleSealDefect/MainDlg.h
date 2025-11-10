// MainDlg.h
#pragma once

class CMainDlg : public CDialogEx
{
public:
    CMainDlg(CWnd* pParent = nullptr);
    enum { IDD = IDD_MAIN_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedBtnInspectStart();
    afx_msg void OnBnClickedBtnProgramExit();
    DECLARE_MESSAGE_MAP()

private:
    CFont m_font;
};