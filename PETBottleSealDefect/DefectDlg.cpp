// DefectDlg.cpp
#include "pch.h"
#include "PETBottleSealDefect.h"
#include "DefectDlg.h"

BEGIN_MESSAGE_MAP(CDefectDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_RECHECK, &CDefectDlg::OnBnClickedBtnRecheck)
    ON_BN_CLICKED(IDC_BTN_APPROVE, &CDefectDlg::OnBnClickedBtnApprove)
    ON_WM_PAINT()
END_MESSAGE_MAP()

CDefectDlg::CDefectDlg(CWnd* pParent)
    : CDialogEx(IDD_DEFECT_DIALOG, pParent)
{
}

void CDefectDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_STATIC_DEFECT_IMAGE, m_staticDefectImage);
    DDX_Control(pDX, IDC_EDIT_MEMO, m_editMemo);
}

BOOL CDefectDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 폰트 생성
    m_font.CreatePointFont(140, _T("맑은 고딕"));

    // 모든 컨트롤에 폰트 적용
    CWnd* pWnd = GetWindow(GW_CHILD);
    while (pWnd)
    {
        pWnd->SetFont(&m_font);
        pWnd = pWnd->GetWindow(GW_HWNDNEXT);
    }

    // 시간 설정
    CTime now = CTime::GetCurrentTime();
    SetDlgItemText(IDC_STATIC_DEFECT_TIME, now.Format(_T("%Y-%m-%d %H:%M:%S")));

    // 불량 유형 설정
    SetDlgItemText(IDC_STATIC_DEFECT_REASON, m_defectType);

    // 신뢰도 설정
    if (!m_confidence.IsEmpty())
    {
        double conf = _ttof(m_confidence);
        CString confText;
        confText.Format(_T("%.1f%%"), conf * 100);
        SetDlgItemText(IDC_STATIC_CONFIDENCE, confText);
    }
    else
    {
        SetDlgItemText(IDC_STATIC_CONFIDENCE, _T("95.0%"));
    }

    // 이미지 표시
    if (!m_defectImage.empty())
    {
        DisplayImage();
    }

    return TRUE;
}

void CDefectDlg::DisplayImage()
{
    if (m_defectImage.empty())
        return;

    CRect rect;
    m_staticDefectImage.GetClientRect(&rect);

    if (rect.Width() <= 0 || rect.Height() <= 0)
        return;

    // BGR -> RGB 변환
    cv::Mat displayImage;
    cv::cvtColor(m_defectImage, displayImage, cv::COLOR_BGR2RGB);

    // 이미지 리사이즈
    cv::Mat resized;
    cv::resize(displayImage, resized, cv::Size(rect.Width(), rect.Height()));

    // 메모리 연속성 보장
    if (!resized.isContinuous())
    {
        resized = resized.clone();
    }

    // Windows DIB는 4바이트 정렬이 필요함
    int width = resized.cols;
    int height = resized.rows;
    int stride = ((width * 3 + 3) & ~3);  // 4바이트 정렬된 stride 계산

    // 정렬된 버퍼 생성
    std::vector<BYTE> buffer(stride * height);

    for (int y = 0; y < height; y++)
    {
        memcpy(&buffer[y * stride], resized.ptr(y), width * 3);
    }

    CDC* pDC = m_staticDefectImage.GetDC();
    if (pDC)
    {
        BITMAPINFO bmi;
        ZeroMemory(&bmi, sizeof(bmi));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -height;  // top-down DIB
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 24;
        bmi.bmiHeader.biCompression = BI_RGB;

        SetDIBitsToDevice(pDC->m_hDC, 0, 0, width, height,
            0, 0, 0, height, buffer.data(), &bmi, DIB_RGB_COLORS);

        m_staticDefectImage.ReleaseDC(pDC);
    }
}

void CDefectDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this);
    }
    else
    {
        CPaintDC dc(this);

        // 이미지가 있으면 다시 그리기
        if (!m_defectImage.empty())
        {
            DisplayImage();
        }
        else
        {
            // 이미지가 없을 때 메시지 표시
            CRect rect;
            m_staticDefectImage.GetClientRect(&rect);
            m_staticDefectImage.ClientToScreen(&rect);
            ScreenToClient(&rect);
            DrawNoImageText(&dc, rect, _T("이미지 없음"));
        }
    }

    CDialogEx::OnPaint();
}

void CDefectDlg::DrawNoImageText(CDC* pDC, CRect rect, CString text)
{
    if (!pDC)
        return;

    CBrush brush(RGB(240, 240, 240));
    pDC->FillRect(&rect, &brush);
    pDC->SetTextColor(RGB(128, 128, 128));
    pDC->SetBkMode(TRANSPARENT);
    pDC->DrawText(text, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void CDefectDlg::AdjustButtonPositions()
{
}

void CDefectDlg::OnBnClickedBtnRecheck()
{
    CString memo;
    m_editMemo.GetWindowText(memo);
    memo.Trim();

    if (memo.IsEmpty())
    {
        AfxMessageBox(_T("재검 사유를 메모에 작성해주세요."));
        m_editMemo.SetFocus();
        return;
    }

    CString msg;
    msg.Format(_T("재검 요청하시겠습니까?\n\n사유: %s"), memo);

    if (AfxMessageBox(msg, MB_YESNO | MB_ICONQUESTION) == IDYES)
    {
        AfxMessageBox(_T("재검 요청되었습니다."));
        EndDialog(IDRETRY);  // IDRETRY로 종료하여 재검 요청을 구분
    }
}

void CDefectDlg::OnBnClickedBtnApprove()
{
    if (AfxMessageBox(_T("불량을 승인하시겠습니까?"), MB_YESNO | MB_ICONQUESTION) == IDYES)
    {
        CString memo;
        m_editMemo.GetWindowText(memo);

        if (!memo.IsEmpty())
        {
            CString msg;
            msg.Format(_T("불량 승인되었습니다.\n메모: %s"), memo);
            AfxMessageBox(msg);
        }
        else
        {
            AfxMessageBox(_T("불량 승인되었습니다."));
        }

        EndDialog(IDOK);
    }
}