// CameraDlg.cpp
#include "pch.h"
#include "PETBottleSealDefect.h"
#include "CameraDlg.h"
#include "DefectDlg.h"

extern bool SendImageToTcpServer(const cv::Mat& image, CString& result);

BEGIN_MESSAGE_MAP(CCameraDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_START, &CCameraDlg::OnBnClickedBtnStart)
    ON_BN_CLICKED(IDC_BTN_DETECT, &CCameraDlg::OnBnClickedBtnDetect)
    ON_WM_TIMER()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

CCameraDlg::CCameraDlg(CWnd* pParent)
    : CDialogEx(IDD_CAMERA_DIALOG, pParent)
    , m_bCameraRunning(false)
    , m_nTimerID(0)
    , m_nHistoryID(1)
{
    Pylon::PylonInitialize();
}

CCameraDlg::~CCameraDlg()
{
    ReleaseCamera();
    Pylon::PylonTerminate();
}

void CCameraDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_STATIC_CAM, m_staticCam);
    DDX_Control(pDX, IDC_EDIT_RESULT, m_editResult);
    DDX_Control(pDX, IDC_EDIT_DEFECT_TYPE, m_editDefectType);
    DDX_Control(pDX, IDC_BTN_START, m_btnStart);
    DDX_Control(pDX, IDC_LIST_HISTORY, m_listHistory);
}

BOOL CCameraDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    m_font.CreatePointFont(140, _T("맑은 고딕"));

    CWnd* pWnd = GetWindow(GW_CHILD);
    while (pWnd)
    {
        pWnd->SetFont(&m_font);
        pWnd = pWnd->GetWindow(GW_HWNDNEXT);
    }

    m_btnStart.SetWindowText(_T("켜기"));

    CWnd* pDetectBtn = GetDlgItem(IDC_BTN_DETECT);
    if (pDetectBtn)
    {
        pDetectBtn->SetWindowText(_T("캡처"));
    }

    InitializeListControl();

    return TRUE;
}

void CCameraDlg::InitializeListControl()
{
    m_listHistory.SetExtendedStyle(
        LVS_EX_FULLROWSELECT |
        LVS_EX_GRIDLINES
    );

    CRect rect;
    m_listHistory.GetClientRect(&rect);
    int totalWidth = rect.Width();

    int idWidth = (int)(totalWidth * 0.10);
    int timeWidth = (int)(totalWidth * 0.35);
    int resultWidth = (int)(totalWidth * 0.20);
    int confidenceWidth = (int)(totalWidth * 0.15);
    int defectWidth = (int)(totalWidth * 0.20);

    m_listHistory.InsertColumn(0, _T("ID"), LVCFMT_CENTER, idWidth);
    m_listHistory.InsertColumn(1, _T("시간"), LVCFMT_CENTER, timeWidth);
    m_listHistory.InsertColumn(2, _T("결과"), LVCFMT_CENTER, resultWidth);
    m_listHistory.InsertColumn(3, _T("신뢰도"), LVCFMT_CENTER, confidenceWidth);
    m_listHistory.InsertColumn(4, _T("불량유형"), LVCFMT_CENTER, defectWidth);

    m_listHistory.SetFont(&m_font);
}

void CCameraDlg::AddHistoryItem(const CString& id, const CString& time, const CString& result, const CString& confidence, const CString& defectType)
{
    int index = m_listHistory.GetItemCount();

    m_listHistory.InsertItem(index, id);
    m_listHistory.SetItemText(index, 1, time);
    m_listHistory.SetItemText(index, 2, result);
    m_listHistory.SetItemText(index, 3, confidence);
    m_listHistory.SetItemText(index, 4, defectType);

    m_listHistory.EnsureVisible(index, FALSE);
}

void CCameraDlg::InitializeCamera()
{
    try
    {
        m_camera.Attach(Pylon::CTlFactory::GetInstance().CreateFirstDevice());
        m_camera.Open();
        m_camera.StartGrabbing(Pylon::GrabStrategy_LatestImageOnly);
        m_bCameraRunning = true;
        m_nTimerID = SetTimer(1, 33, NULL);
    }
    catch (const Pylon::GenericException& e)
    {
        AfxMessageBox(CString(e.GetDescription()));
    }
}

void CCameraDlg::ReleaseCamera()
{
    if (m_nTimerID) KillTimer(m_nTimerID);
    if (m_camera.IsGrabbing()) m_camera.StopGrabbing();
    if (m_camera.IsOpen()) m_camera.Close();
    m_bCameraRunning = false;
}

void CCameraDlg::OnBnClickedBtnStart()
{
    if (!m_bCameraRunning)
    {
        InitializeCamera();
        m_btnStart.SetWindowText(_T("중지"));
    }
    else
    {
        ReleaseCamera();
        m_btnStart.SetWindowText(_T("켜기"));
    }
}

void CCameraDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1 && m_bCameraRunning) CaptureAndDisplay();
    CDialogEx::OnTimer(nIDEvent);
}

void CCameraDlg::CaptureAndDisplay()
{
    try
    {
        if (m_camera.RetrieveResult(5000, m_ptrGrabResult, Pylon::TimeoutHandling_ThrowException))
        {
            if (m_ptrGrabResult->GrabSucceeded())
            {
                int w = m_ptrGrabResult->GetWidth();
                int h = m_ptrGrabResult->GetHeight();
                cv::Mat img(h, w, CV_8UC1, (uint8_t*)m_ptrGrabResult->GetBuffer());
                cv::Mat imgColor;
                cv::cvtColor(img, imgColor, cv::COLOR_GRAY2BGR);
                DisplayImage(imgColor);
            }
        }
    }
    catch (...) {}
}

void CCameraDlg::DisplayImage(const cv::Mat& img)
{
    CRect rect;
    m_staticCam.GetClientRect(&rect);

    cv::Mat resized;
    cv::resize(img, resized, cv::Size(rect.Width(), rect.Height()));

    if (!resized.isContinuous())
    {
        resized = resized.clone();
    }

    CDC* pDC = m_staticCam.GetDC();

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = resized.cols;
    bmi.bmiHeader.biHeight = -resized.rows;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    SetDIBitsToDevice(pDC->m_hDC, 0, 0, resized.cols, resized.rows,
        0, 0, 0, resized.rows, resized.data, &bmi, DIB_RGB_COLORS);

    m_staticCam.ReleaseDC(pDC);
}

void CCameraDlg::OnBnClickedBtnDetect()
{
    if (!m_bCameraRunning)
    {
        AfxMessageBox(_T("카메라를 먼저 켜주세요."));
        return;
    }

    try
    {
        if (m_camera.RetrieveResult(5000, m_ptrGrabResult, Pylon::TimeoutHandling_ThrowException))
        {
            if (m_ptrGrabResult->GrabSucceeded())
            {
                int w = m_ptrGrabResult->GetWidth();
                int h = m_ptrGrabResult->GetHeight();
                cv::Mat img(h, w, CV_8UC1, (uint8_t*)m_ptrGrabResult->GetBuffer());

                if (img.empty())
                {
                    AfxMessageBox(_T("이미지가 비어있습니다."));
                    return;
                }

                cv::Mat imgColor;
                cv::cvtColor(img, imgColor, cv::COLOR_GRAY2BGR);

                CString result;

                if (SendImageToTcpServer(imgColor, result))
                {
                    result.Trim();

                    int firstSep = result.Find(_T("|"));
                    int secondSep = result.Find(_T("|"), firstSep + 1);

                    CString defectTypeKorean;
                    CString defectTypeEnglish;
                    CString confidence;

                    if (firstSep != -1)
                    {
                        defectTypeKorean = result.Left(firstSep);

                        if (secondSep != -1)
                        {
                            defectTypeEnglish = result.Mid(firstSep + 1, secondSep - firstSep - 1);
                            confidence = result.Mid(secondSep + 1);
                        }
                        else
                        {
                            defectTypeEnglish = result.Mid(firstSep + 1);
                            confidence = _T("0.95");
                        }
                    }
                    else
                    {
                        defectTypeKorean = result;
                        defectTypeEnglish = result;
                        confidence = _T("0.95");
                    }

                    m_editResult.SetWindowText(defectTypeKorean);
                    m_editDefectType.SetWindowText(defectTypeKorean);

                    CTime now = CTime::GetCurrentTime();
                    CString timeStr = now.Format(_T("%Y-%m-%d %H:%M:%S"));

                    double conf = _ttof(confidence);
                    CString confText;
                    confText.Format(_T("%.1f%%"), conf * 100);

                    CString idStr;
                    idStr.Format(_T("%d"), m_nHistoryID++);

                    AddHistoryItem(idStr, timeStr, defectTypeKorean, confText, defectTypeKorean);

                    if (defectTypeKorean != _T("정상"))
                    {
                        CDefectDlg dlg;

                        cv::Mat rgbImage;
                        cv::cvtColor(imgColor, rgbImage, cv::COLOR_BGR2RGB);
                        dlg.m_defectImage = rgbImage.clone();

                        dlg.m_defectType = defectTypeKorean;
                        dlg.m_confidence = confidence;
                        dlg.DoModal();
                    }
                }
                else
                {
                    AfxMessageBox(_T("서버 통신 실패\n\n확인사항:\n1. C# 서버가 실행 중인가?\n2. 방화벽 설정 확인\n3. Python 서버가 실행 중인가?"));
                }
            }
        }
    }
    catch (const Pylon::GenericException& e)
    {
        CString msg;
        msg.Format(_T("카메라 오류: %s"), CString(e.GetDescription()));
        AfxMessageBox(msg);
    }
}

void CCameraDlg::OnDestroy()
{
    ReleaseCamera();
    CDialogEx::OnDestroy();
}