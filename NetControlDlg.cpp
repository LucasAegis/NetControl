// NetControlDlg.cpp: 实现文件
#include "pch.h"
#include "framework.h"
#include "NetControl.h"
#include "NetControlDlg.h"
#include "afxdialogex.h"
#include <tlhelp32.h>
#include <psapi.h>
#include <shellapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifndef IDC_STATIC_TITLE
#define IDC_STATIC_TITLE 99991 
#endif
#ifndef IDC_STATIC_SUB
#define IDC_STATIC_SUB 99992
#endif

#define IDM_TRAY_RESTORE 1001
#define IDM_TRAY_EXIT    1002

#define STATE_GRAY   0
#define STATE_GREEN  1
#define STATE_RED    2

class CAboutDlg : public CDialogEx {
public:
	CAboutDlg() : CDialogEx(IDD_ABOUTBOX) {}
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif
protected:
	virtual void DoDataExchange(CDataExchange* pDX) { CDialogEx::DoDataExchange(pDX); }
protected:
	DECLARE_MESSAGE_MAP()
};
BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// ===========================================================================
// 主逻辑
// ===========================================================================

CNetControlDlg::CNetControlDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_NETCONTROL_DIALOG, pParent)
{
	m_hIcon = LoadIcon(NULL, IDI_SHIELD);
	m_nLanguage = 0;
	m_bTrayIconAdded = false;
	m_gdiplusToken = 0;
	memset(&m_nid, 0, sizeof(NOTIFYICONDATA));
}

void CNetControlDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_APPS, m_listApps);
}

BEGIN_MESSAGE_MAP(CNetControlDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_MESSAGE(WM_DELAY_RESIZE, &CNetControlDlg::OnDelayResize)
	ON_MESSAGE(WM_TRAY_ICON, &CNetControlDlg::OnTrayIconNotify)
	ON_EN_CHANGE(IDC_EDIT_SEARCH, &CNetControlDlg::OnEnChangeEditSearch)
	ON_BN_CLICKED(IDC_BTN_CLEAR, &CNetControlDlg::OnBnClickedBtnClear)
	ON_NOTIFY(NM_CLICK, IDC_LIST_APPS, &CNetControlDlg::OnNMClickListApps)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_APPS, &CNetControlDlg::OnNMCustomdrawListApps)
	ON_BN_CLICKED(IDC_BTN_SCAN, &CNetControlDlg::OnBnClickedBtnScan)
	ON_BN_CLICKED(IDC_BTN_ADD, &CNetControlDlg::OnBnClickedBtnAdd)
	ON_BN_CLICKED(IDC_BTN_CN, &CNetControlDlg::OnBnClickedBtnCn)
	ON_BN_CLICKED(IDC_BTN_EN, &CNetControlDlg::OnBnClickedBtnEn)
END_MESSAGE_MAP()

BOOL CNetControlDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 初始化 COM
	CoInitialize(NULL);

	ModifyStyle(0, WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	// 提权检测
	HANDLE hToken;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		TOKEN_PRIVILEGES tp;
		if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid)) {
			tp.PrivilegeCount = 1;
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, 0);
		}
		CloseHandle(hToken);
	}

	SetWindowTheme(m_listApps.GetSafeHwnd(), L"", L"");
	m_listApps.ModifyStyle(WS_BORDER, 0);
	m_listApps.ModifyStyleEx(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE, 0);
	m_listApps.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_listApps.SetBkColor(RGB(240, 240, 240));
	m_listApps.SetTextBkColor(RGB(240, 240, 240));

	m_imgList.Create(1, 45, ILC_COLOR, 1, 1);
	m_listApps.SetImageList(&m_imgList, LVSIL_SMALL);

	m_fontEdit.CreatePointFont(100, _T("Microsoft YaHei UI"));
	m_editSearch.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, CRect(0, 0, 0, 0), this, IDC_EDIT_SEARCH);
	m_editSearch.SetFont(&m_fontEdit);
	m_editSearch.SendMessage(0x1501, 0, (LPARAM)_T("🔍 Search..."));

	m_btnClear.Create(_T("❌"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT, CRect(0, 0, 0, 0), this, IDC_BTN_CLEAR);
	m_btnClear.SetFont(&m_fontEdit);

	UpdateUILanguage();

	CWnd* pChild = GetWindow(GW_CHILD);
	while (pChild) {
		int id = pChild->GetDlgCtrlID();
		if (id == IDC_STATIC || id == IDC_STATIC_TITLE || id == IDC_STATIC_SUB) {
			pChild->ShowWindow(SW_HIDE);
		}
		pChild = pChild->GetWindow(GW_HWNDNEXT);
	}

	RefreshAppList();
	return TRUE;
}

void CNetControlDlg::OnDestroy()
{
	DeleteTrayIcon();
	CoUninitialize();
	CDialogEx::OnDestroy();
	GdiplusShutdown(m_gdiplusToken);
}

void CNetControlDlg::OnCancel() { ToTray(); }

void CNetControlDlg::ToTray()
{
	if (!m_bTrayIconAdded) {
		memset(&m_nid, 0, sizeof(NOTIFYICONDATA));
		m_nid.cbSize = sizeof(NOTIFYICONDATA);
		m_nid.hWnd = m_hWnd;
		m_nid.uID = IDR_MAINFRAME;
		m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		m_nid.uCallbackMessage = WM_TRAY_ICON;
		m_nid.hIcon = m_hIcon;
		lstrcpy(m_nid.szTip, _T("NetControl Center"));
		Shell_NotifyIcon(NIM_ADD, &m_nid);
		m_bTrayIconAdded = true;
	}
	ShowWindow(SW_HIDE);
}

void CNetControlDlg::DeleteTrayIcon()
{
	if (m_bTrayIconAdded) {
		Shell_NotifyIcon(NIM_DELETE, &m_nid);
		m_bTrayIconAdded = false;
	}
}

LRESULT CNetControlDlg::OnTrayIconNotify(WPARAM wParam, LPARAM lParam)
{
	if (lParam == WM_LBUTTONUP || lParam == WM_LBUTTONDBLCLK) {
		ShowWindow(SW_SHOW); SetForegroundWindow(); DeleteTrayIcon();
	}
	else if (lParam == WM_RBUTTONUP) {
		CPoint pt; GetCursorPos(&pt);
		CMenu menu; menu.CreatePopupMenu();
		if (m_nLanguage == 0) {
			menu.AppendMenu(MF_STRING, IDM_TRAY_RESTORE, _T("显示主界面"));
			menu.AppendMenu(MF_SEPARATOR);
			menu.AppendMenu(MF_STRING, IDM_TRAY_EXIT, _T("退出程序"));
		}
		else {
			menu.AppendMenu(MF_STRING, IDM_TRAY_RESTORE, _T("Show Window"));
			menu.AppendMenu(MF_SEPARATOR);
			menu.AppendMenu(MF_STRING, IDM_TRAY_EXIT, _T("Exit"));
		}
		SetForegroundWindow();
		int cmd = menu.TrackPopupMenu(TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, this);
		if (cmd == IDM_TRAY_RESTORE) { ShowWindow(SW_SHOW); SetForegroundWindow(); DeleteTrayIcon(); }
		else if (cmd == IDM_TRAY_EXIT) { DeleteTrayIcon(); CDialogEx::OnCancel(); }
	}
	return 0;
}

// ---------------------------------------------------------------------------
// 防火墙操作
// ---------------------------------------------------------------------------
void CNetControlDlg::SetFirewallRule(CString appName, CString appPath, bool bBlock)
{
	CString ruleName = _T("NetControl_Block_") + appName;

	INetFwPolicy2* pNetFwPolicy2 = NULL;
	INetFwRules* pFwRules = NULL;
	INetFwRule* pFwRule = NULL;

	HRESULT hr = CoCreateInstance(__uuidof(NetFwPolicy2), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2), (void**)&pNetFwPolicy2);
	if (FAILED(hr)) return;

	hr = pNetFwPolicy2->get_Rules(&pFwRules);
	if (SUCCEEDED(hr)) {
		BSTR bstrRuleName = SysAllocString(ruleName);
		pFwRules->Remove(bstrRuleName); // 先删除旧的

		if (bBlock) {
			hr = CoCreateInstance(__uuidof(NetFwRule), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwRule), (void**)&pFwRule);
			if (SUCCEEDED(hr)) {
				pFwRule->put_Name(bstrRuleName);
				pFwRule->put_ApplicationName(SysAllocString(appPath));
				pFwRule->put_Action(NET_FW_ACTION_BLOCK);
				pFwRule->put_Direction(NET_FW_RULE_DIR_OUT);
				pFwRule->put_Enabled(VARIANT_TRUE);
				pFwRules->Add(pFwRule);
				pFwRule->Release();
			}
		}
		SysFreeString(bstrRuleName);
		pFwRules->Release();
	}
	pNetFwPolicy2->Release();
}

// 【新增】检查防火墙状态：这个软件是否已经被禁止了？
bool CNetControlDlg::CheckFirewallRuleExists(CString appName)
{
	CString ruleName = _T("NetControl_Block_") + appName;
	INetFwPolicy2* pNetFwPolicy2 = NULL;
	INetFwRules* pFwRules = NULL;
	INetFwRule* pFwRule = NULL;
	bool bExists = false;

	HRESULT hr = CoCreateInstance(__uuidof(NetFwPolicy2), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2), (void**)&pNetFwPolicy2);
	if (SUCCEEDED(hr)) {
		hr = pNetFwPolicy2->get_Rules(&pFwRules);
		if (SUCCEEDED(hr)) {
			BSTR bstrRuleName = SysAllocString(ruleName);
			// 尝试获取这个名字的规则
			hr = pFwRules->Item(bstrRuleName, &pFwRule);
			if (SUCCEEDED(hr) && pFwRule != NULL) {
				bExists = true; // 找到了！说明它被禁止了
				pFwRule->Release();
			}
			SysFreeString(bstrRuleName);
			pFwRules->Release();
		}
		pNetFwPolicy2->Release();
	}
	return bExists;
}

// ---------------------------------------------------------------------------
// 扫描逻辑 (升级版：带记忆功能)
// ---------------------------------------------------------------------------
void CNetControlDlg::RefreshAppList()
{
	m_Apps.clear();
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot != INVALID_HANDLE_VALUE) {
		PROCESSENTRY32 pe;
		pe.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(hSnapshot, &pe)) {
			int i = 0;
			do {
				CString strExeName = pe.szExeFile;
				if (strExeName.Right(4).CompareNoCase(_T(".exe")) == 0) {
					if (strExeName.CompareNoCase(_T("System")) == 0) continue;
					if (strExeName.CompareNoCase(_T("Registry")) == 0) continue;
					if (strExeName.CompareNoCase(_T("Memory Compression")) == 0) continue;
					if (strExeName.CompareNoCase(_T("NetControl.exe")) == 0) continue;

					CString strFullPath = _T("");
					HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
					if (hProcess) {
						DWORD dwSize = MAX_PATH;
						TCHAR szPath[MAX_PATH];
						if (QueryFullProcessImageName(hProcess, 0, szPath, &dwSize)) strFullPath = szPath;
						CloseHandle(hProcess);
					}
					if (strFullPath.IsEmpty()) strFullPath = strExeName;

					AppItem item;
					item.name = strExeName;
					item.path = strFullPath;

					// 【核心记忆逻辑】
					// 不再默认设为 GREEN，而是去问问防火墙
					if (CheckFirewallRuleExists(strExeName)) {
						item.state = STATE_RED; // 防火墙里有记录，直接标红
					}
					else {
						item.state = STATE_GREEN;
					}

					m_Apps.push_back(item);
				}
			} while (Process32Next(hSnapshot, &pe));
		}
		CloseHandle(hSnapshot);
	}
	// 扫描完了，按 红/绿 排序 (红色会自动沉底)
	std::sort(m_Apps.begin(), m_Apps.end());
	ReloadListFromData();
}

void CNetControlDlg::ReloadListFromData()
{
	m_listApps.SetRedraw(FALSE);
	m_listApps.DeleteAllItems();

	CString strKeyword;
	m_editSearch.GetWindowText(strKeyword);
	strKeyword.MakeLower();

	int nDisplayIndex = 0;
	for (int i = 0; i < (int)m_Apps.size(); i++) {
		if (!strKeyword.IsEmpty()) {
			CString name = m_Apps[i].name; name.MakeLower();
			CString path = m_Apps[i].path; path.MakeLower();
			if (name.Find(strKeyword) == -1 && path.Find(strKeyword) == -1) continue;
		}

		int nItem = m_listApps.InsertItem(nDisplayIndex++, m_Apps[i].name);
		m_listApps.SetItemText(nItem, 1, m_Apps[i].path);
		m_listApps.SetItemText(nItem, 2, _T(""));
		m_listApps.SetItemText(nItem, 3, _T(""));
		m_listApps.SetItemData(nItem, i);
	}
	m_listApps.SetRedraw(TRUE);
}

void CNetControlDlg::OnEnChangeEditSearch() { ReloadListFromData(); }
void CNetControlDlg::OnBnClickedBtnClear() { m_editSearch.SetWindowText(_T("")); }

// ---------------------------------------------------------------------------
// 交互点击
// ---------------------------------------------------------------------------
void CNetControlDlg::OnNMClickListApps(NMHDR* pNMHDR, LRESULT* pResult) {
	LPNMITEMACTIVATE p = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	if (p->iItem != -1) {
		int nRealIndex = (int)m_listApps.GetItemData(p->iItem);
		if (p->iSubItem == 2) ToggleNetStatus(nRealIndex, STATE_GREEN);
		if (p->iSubItem == 3) ToggleNetStatus(nRealIndex, STATE_RED);
	}
	*pResult = 0;
}

void CNetControlDlg::ToggleNetStatus(int nItem, int nState) {
	if (nItem < 0 || nItem >= (int)m_Apps.size()) return;

	m_Apps[nItem].state = nState;
	SetFirewallRule(m_Apps[nItem].name, m_Apps[nItem].path, (nState == STATE_RED));

	std::sort(m_Apps.begin(), m_Apps.end());
	ReloadListFromData();
}

// ---------------------------------------------------------------------------
// 布局 & 渲染 & 辅助 (保持不变)
// ---------------------------------------------------------------------------
void CNetControlDlg::OnSize(UINT nType, int cx, int cy) { CDialogEx::OnSize(nType, cx, cy); PostMessage(WM_DELAY_RESIZE); }

LRESULT CNetControlDlg::OnDelayResize(WPARAM wParam, LPARAM lParam) {
	if (m_listApps.GetSafeHwnd() == NULL || !::IsWindow(m_listApps.m_hWnd)) return 0;
	CRect rectDlg; GetClientRect(&rectDlg);
	int cx = rectDlg.Width(); int cy = rectDlg.Height(); if (cx == 0) return 0;
	int margin = 20;
	int langBtnW = 60; int langBtnH = 25; int gap = 10; int funcBtnW = 100; int funcBtnH = 35;
	CWnd* pBtnEn = GetDlgItem(IDC_BTN_EN); if (pBtnEn) pBtnEn->MoveWindow(cx - margin - langBtnW, margin, langBtnW, langBtnH);
	CWnd* pBtnCn = GetDlgItem(IDC_BTN_CN); if (pBtnCn) pBtnCn->MoveWindow(cx - margin - langBtnW * 2 - gap, margin, langBtnW, langBtnH);
	int topRow2 = margin + langBtnH + 15;
	CWnd* pBtnAdd = GetDlgItem(IDC_BTN_ADD); if (pBtnAdd) pBtnAdd->MoveWindow(cx - margin - funcBtnW, topRow2, funcBtnW, funcBtnH);
	CWnd* pBtnScan = GetDlgItem(IDC_BTN_SCAN); if (pBtnScan) pBtnScan->MoveWindow(cx - margin - funcBtnW * 2 - gap, topRow2, funcBtnW, funcBtnH);
	if (m_editSearch.GetSafeHwnd()) { m_editSearch.MoveWindow(margin, 85, 250, 26); if (m_btnClear.GetSafeHwnd()) m_btnClear.MoveWindow(margin + 250 + 5, 85, 30, 26); }
	int listTop = topRow2 + funcBtnH + 20; int listH = cy - listTop - margin; if (listH < 10) listH = 10;
	m_listApps.MoveWindow(margin, listTop, cx - margin * 2, listH);
	CHeaderCtrl* pHeader = m_listApps.GetHeaderCtrl();
	if (pHeader && pHeader->GetItemCount() >= 4) {
		CRect rectList; m_listApps.GetClientRect(&rectList); int totalWidth = rectList.Width();
		if (totalWidth > 0) { int wName = 200; int wBtn = 80; int wPath = totalWidth - wName - (wBtn * 2); if (wPath < 50) wPath = 50; m_listApps.SetColumnWidth(0, wName); m_listApps.SetColumnWidth(1, wPath); m_listApps.SetColumnWidth(2, wBtn); m_listApps.SetColumnWidth(3, wBtn); }
	}
	Invalidate(); return 0;
}

void CNetControlDlg::OnPaint() {
	if (IsIconic()) { CPaintDC dc(this); SendMessage(WM_ICONERASEBKGND, (WPARAM)dc.GetSafeHdc(), 0); int cx = GetSystemMetrics(SM_CXICON); int cy = GetSystemMetrics(SM_CYICON); CRect rect; GetClientRect(&rect); int x = (rect.Width() - cx + 1) / 2; int y = (rect.Height() - cy + 1) / 2; dc.DrawIcon(x, y, m_hIcon); }
	else { CPaintDC dc(this); CFont fontTitle; fontTitle.CreatePointFont(160, _T("Microsoft YaHei UI"), &dc); CFont fontSub; fontSub.CreatePointFont(90, _T("Microsoft YaHei UI"), &dc); dc.SetBkMode(TRANSPARENT); CFont* pOldFont = dc.SelectObject(&fontTitle); dc.SetTextColor(RGB(0, 0, 0)); CString strTitle = (m_nLanguage == 0) ? _T("联网控制中心") : _T("NetControl Center"); dc.TextOut(20, 20, strTitle); dc.SelectObject(&fontSub); dc.SetTextColor(RGB(128, 128, 128)); CString strSub; if (m_nLanguage == 0) strSub = _T("\xD83D\xDEE1 纯本地运行 | 自身无联网代码"); else strSub = _T("\xD83D\xDEE1 Pure Local Run | No Network Code"); dc.TextOut(20, 55, strSub); dc.SelectObject(pOldFont); CDialogEx::OnPaint(); }
}

void CNetControlDlg::UpdateUILanguage() {
	SetWindowText(_T("NetControl")); m_listApps.SetRedraw(FALSE); while (m_listApps.DeleteColumn(0)); m_listApps.InsertColumn(0, _T(""), LVCFMT_LEFT, 200); m_listApps.InsertColumn(1, _T(""), LVCFMT_LEFT, 300); m_listApps.InsertColumn(2, _T(""), LVCFMT_CENTER, 80); m_listApps.InsertColumn(3, _T(""), LVCFMT_CENTER, 80); m_listApps.SetRedraw(TRUE);
	if (m_nLanguage == 0) { GetDlgItem(IDC_BTN_SCAN)->SetWindowText(_T("全盘扫描")); GetDlgItem(IDC_BTN_ADD)->SetWindowText(_T("手动选择")); GetDlgItem(IDC_BTN_CN)->SetWindowText(_T("中文")); GetDlgItem(IDC_BTN_EN)->SetWindowText(_T("English")); }
	else { GetDlgItem(IDC_BTN_SCAN)->SetWindowText(_T("Full Scan")); GetDlgItem(IDC_BTN_ADD)->SetWindowText(_T("Manual Select")); GetDlgItem(IDC_BTN_CN)->SetWindowText(_T("Chinese")); GetDlgItem(IDC_BTN_EN)->SetWindowText(_T("English")); }
	ReloadListFromData(); PostMessage(WM_DELAY_RESIZE); Invalidate();
}

void CNetControlDlg::OnBnClickedBtnScan() { RefreshAppList(); }
void CNetControlDlg::OnBnClickedBtnAdd() {
	CFileDialog d(TRUE, _T("exe"), NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Exes (*.exe)|*.exe||"));
	if (d.DoModal() == IDOK) { AppItem item; item.name = d.GetFileName(); item.path = d.GetPathName(); item.state = STATE_GREEN; m_Apps.push_back(item); std::sort(m_Apps.begin(), m_Apps.end()); ReloadListFromData(); }
}
void CNetControlDlg::OnBnClickedBtnCn() { m_nLanguage = 0; UpdateUILanguage(); }
void CNetControlDlg::OnBnClickedBtnEn() { m_nLanguage = 1; UpdateUILanguage(); }
void CNetControlDlg::OnSysCommand(UINT nID, LPARAM lParam) { if ((nID & 0xFFF0) == IDM_ABOUTBOX) { CAboutDlg d; d.DoModal(); } else CDialogEx::OnSysCommand(nID, lParam); }
HCURSOR CNetControlDlg::OnQueryDragIcon() { return (HCURSOR)m_hIcon; }
void DrawVectorSwitch(Graphics& graphics, CRect rect, Color colorBg) { rect.DeflateRect(15, 10); graphics.SetSmoothingMode(SmoothingModeHighQuality); SolidBrush brushBg(colorBg); RectF rectF((REAL)rect.left, (REAL)rect.top, (REAL)rect.Width(), (REAL)rect.Height()); REAL r = rectF.Height; Pen penBg(colorBg, r); penBg.SetStartCap(LineCapRound); penBg.SetEndCap(LineCapRound); graphics.DrawLine(&penBg, rectF.X + r / 2, rectF.Y + r / 2, rectF.GetRight() - r / 2, rectF.Y + r / 2); SolidBrush brushWhite(Color::White); REAL dotSize = r - 6; REAL dotY = rectF.Y + 3; REAL dotX = rectF.GetRight() - dotSize - 3; graphics.FillEllipse(&brushWhite, dotX, dotY, dotSize, dotSize); }
void CNetControlDlg::OnNMCustomdrawListApps(NMHDR* pNMHDR, LRESULT* pResult) { NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR); *pResult = CDRF_DODEFAULT; if (pLVCD->nmcd.dwDrawStage == CDDS_PREPAINT) *pResult = CDRF_NOTIFYITEMDRAW; else if (pLVCD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) *pResult = CDRF_NOTIFYSUBITEMDRAW; else if (pLVCD->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM)) { int R = (int)pLVCD->nmcd.dwItemSpec; CDC* DC = CDC::FromHandle(pLVCD->nmcd.hdc); CRect rcRow; m_listApps.GetItemRect(R, rcRow, LVIR_BOUNDS); rcRow.DeflateRect(0, 2, 0, 2); int nRealIndex = (int)m_listApps.GetItemData(R); COLORREF bg = RGB(255, 255, 255); if (nRealIndex >= 0 && nRealIndex < (int)m_Apps.size()) { if (m_Apps[nRealIndex].state == STATE_GREEN) bg = RGB(220, 255, 220); if (m_Apps[nRealIndex].state == STATE_RED) bg = RGB(255, 220, 220); } DC->FillSolidRect(rcRow, bg); DC->SetBkMode(TRANSPARENT); CRect rc0; m_listApps.GetSubItemRect(R, 0, LVIR_LABEL, rc0); rc0.top = rcRow.top; rc0.bottom = rcRow.bottom; rc0.left += 10; DC->SetTextColor(RGB(0, 0, 0)); DC->DrawText(m_listApps.GetItemText(R, 0), rc0, DT_LEFT | DT_VCENTER | DT_SINGLELINE); CRect rc1; m_listApps.GetSubItemRect(R, 1, LVIR_LABEL, rc1); rc1.top = rcRow.top; rc1.bottom = rcRow.bottom; rc1.left += 5; DC->SetTextColor(RGB(128, 128, 128)); DC->DrawText(m_listApps.GetItemText(R, 1), rc1, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_PATH_ELLIPSIS); Graphics graphics(DC->GetSafeHdc()); CRect rc2; m_listApps.GetSubItemRect(R, 2, LVIR_BOUNDS, rc2); rc2.top = rcRow.top; rc2.bottom = rcRow.bottom; DrawVectorSwitch(graphics, rc2, Color(255, 34, 197, 94)); CRect rc3; m_listApps.GetSubItemRect(R, 3, LVIR_BOUNDS, rc3); rc3.top = rcRow.top; rc3.bottom = rcRow.bottom; DrawVectorSwitch(graphics, rc3, Color(255, 239, 68, 68)); *pResult = CDRF_SKIPDEFAULT; } }