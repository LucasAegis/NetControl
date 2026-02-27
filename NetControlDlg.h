#pragma once
#include <vector>
#include <algorithm> 
#include <gdiplus.h>

#include <netfw.h>
#include <objbase.h>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

using namespace Gdiplus;

#define WM_DELAY_RESIZE (WM_USER + 101)
#define WM_TRAY_ICON    (WM_USER + 102)

#define IDC_EDIT_SEARCH 99993 
#define IDC_BTN_CLEAR   99994 

struct AppItem {
	CString name;
	CString path;
	int state;    // 1=允许, 2=禁止

	bool operator<(const AppItem& other) const {
		if (state != other.state) return state < other.state;
		return name.CompareNoCase(other.name) < 0;
	}
};

class CNetControlDlg : public CDialogEx
{
public:
	CNetControlDlg(CWnd* pParent = nullptr);

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_NETCONTROL_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	HICON m_hIcon;
	NOTIFYICONDATA m_nid;
	bool m_bTrayIconAdded;

	CListCtrl m_listApps;
	CImageList m_imgList;
	CEdit m_editSearch;
	CButton m_btnClear;
	CFont m_fontEdit;
	ULONG_PTR m_gdiplusToken;

	int m_nLanguage;
	std::vector<AppItem> m_Apps;

	void UpdateUILanguage();
	void ToggleNetStatus(int nItem, int nState);

	void SetFirewallRule(CString appName, CString appPath, bool bBlock);

	// 【新增】检查防火墙规则是否存在
	bool CheckFirewallRuleExists(CString appName);

	void RefreshAppList();
	void ReloadListFromData();

	void ToTray();
	void DeleteTrayIcon();

	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnDestroy();

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnDelayResize(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnTrayIconNotify(WPARAM wParam, LPARAM lParam);
	afx_msg void OnEnChangeEditSearch();
	afx_msg void OnBnClickedBtnClear();

	DECLARE_MESSAGE_MAP()

public:
	virtual void OnCancel();

	afx_msg void OnNMClickListApps(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMCustomdrawListApps(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedBtnScan();
	afx_msg void OnBnClickedBtnAdd();
	afx_msg void OnBnClickedBtnCn();
	afx_msg void OnBnClickedBtnEn();
};