#pragma once

#ifndef __AFXWIN_H__
	#error "PCH に対してこのファイルをインクルードする前に 'pch.h' をインクルードしてください"
#endif

class Application : public CWinApp
{
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};
