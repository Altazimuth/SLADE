
#ifndef __CC_PREFS_PANEL_H__
#define __CC_PREFS_PANEL_H__

#include "PrefsPanelBase.h"

class wxListBox;
class CCPrefsPanel : public PrefsPanelBase
{
private:
	wxTextCtrl*	text_ccpath;
	wxButton*	btn_browse_ccpath;
	wxButton*	btn_incpath_add;
	wxButton*	btn_incpath_remove;
	wxListBox*	list_inc_paths;

public:
	CCPrefsPanel(wxWindow* parent);
	~CCPrefsPanel();

	void	init();
	void	applyPreferences();

	// Events
	void	onBtnBrowseCCPath(wxCommandEvent& e);
	void	onBtnAddIncPath(wxCommandEvent& e);
	void	onBtnRemoveIncPath(wxCommandEvent& e);
};

#endif//__ACS_PREFS_PANEL_H__
