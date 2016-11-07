
/*******************************************************************
* SLADE - It's a Doom Editor
* Copyright (C) 2008-2016 Simon Judd
*
* Email:       sirjuddington@gmail.com
* Web:         http://slade.mancubus.net
* Filename:    CPrefsPanel.cpp
* Description: Panel containing ACS script preference controls
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*******************************************************************/


/*******************************************************************
* INCLUDES
*******************************************************************/
#include "Main.h"
#include "CCPrefsPanel.h"
#include "Utility/SFileDialog.h"



/*******************************************************************
* EXTERNAL VARIABLES
*******************************************************************/
EXTERN_CVAR(String, path_gdcc_cc)
EXTERN_CVAR(String, path_gdcc_cc_libs)
EXTERN_CVAR(String, path_gdcc_cc_incs)


/*******************************************************************
* CPREFSPANEL CLASS FUNCTIONS
*******************************************************************/

/* CPrefsPanel::CPrefsPanel
* CPrefsPanel class constructor
*******************************************************************/
CCPrefsPanel::CCPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	wxBoxSizer* psizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(psizer);

	// Create frame+sizer
	wxStaticBox* frame = new wxStaticBox(this, -1, "ACS Preferences");
	wxStaticBoxSizer* sizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	psizer->Add(sizer, 1, wxEXPAND | wxALL, 4);

	// GDCC-CC.exe path
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(new wxStaticText(this, -1, "Location of gdcc-cc executable:"), 0, wxALL, 4);
	text_ccpath = new wxTextCtrl(this, -1, wxString(path_gdcc_cc));
	hbox->Add(text_ccpath, 1, wxEXPAND | wxRIGHT, 4);
	btn_browse_ccpath = new wxButton(this, -1, "Browse");
	hbox->Add(btn_browse_ccpath, 0, wxEXPAND);
	sizer->Add(hbox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);

	// Library paths
	sizer->Add(new wxStaticText(this, -1, "Library Paths:"), 0, wxEXPAND | wxLEFT | wxRIGHT, 4);
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);
	list_lib_paths = new wxListBox(this, -1, wxDefaultPosition, wxSize(-1, 200));
	hbox->Add(list_lib_paths, 1, wxEXPAND | wxRIGHT, 4);

	// Add library path
	btn_libpath_add = new wxButton(this, -1, "Add");
	wxBoxSizer* vbox_lib = new wxBoxSizer(wxVERTICAL);
	hbox->Add(vbox_lib, 0, wxEXPAND);
	vbox_lib->Add(btn_libpath_add, 0, wxEXPAND | wxBOTTOM, 4);

	// Remove library path
	btn_libpath_remove = new wxButton(this, -1, "Remove");
	vbox_lib->Add(btn_libpath_remove, 0, wxEXPAND | wxBOTTOM, 4);

	// Populate library paths list
	list_lib_paths->Append(wxSplit(path_gdcc_cc_libs, ';'));

	// Include paths
	sizer->Add(new wxStaticText(this, -1, "Include Paths:"), 0, wxEXPAND | wxLEFT | wxRIGHT, 4);
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);
	list_inc_paths = new wxListBox(this, -1, wxDefaultPosition, wxSize(-1, 200));
	hbox->Add(list_inc_paths, 1, wxEXPAND | wxRIGHT, 4);

	// Add include path
	btn_incpath_add = new wxButton(this, -1, "Add");
	wxBoxSizer* vbox_inc = new wxBoxSizer(wxVERTICAL);
	hbox->Add(vbox_inc, 0, wxEXPAND);
	vbox_inc->Add(btn_incpath_add, 0, wxEXPAND | wxBOTTOM, 4);

	// Remove include path
	btn_incpath_remove = new wxButton(this, -1, "Remove");
	vbox_inc->Add(btn_incpath_remove, 0, wxEXPAND | wxBOTTOM, 4);

	// Populate include paths list
	// list_lib_paths->Append(wxSplit(path_gdcc_cc_libs, ';'));
	list_inc_paths->Append(wxSplit(path_gdcc_cc_incs, ';'));

	// Bind events
	btn_browse_ccpath->Bind(wxEVT_BUTTON, &CCPrefsPanel::onBtnBrowseCCPath, this);
	btn_libpath_add->Bind(wxEVT_BUTTON, &CCPrefsPanel::onBtnAddLibPath, this);
	btn_libpath_remove->Bind(wxEVT_BUTTON, &CCPrefsPanel::onBtnRemoveLibPath, this);
	btn_incpath_add->Bind(wxEVT_BUTTON, &CCPrefsPanel::onBtnAddIncPath, this);
	btn_incpath_remove->Bind(wxEVT_BUTTON, &CCPrefsPanel::onBtnRemoveIncPath, this);
}

/* CPrefsPanel::~CPrefsPanel
* CPrefsPanel class destructor
*******************************************************************/
CCPrefsPanel::~CCPrefsPanel()
{
}

/* CPrefsPanel::init
* Initialises panel controls
*******************************************************************/
void CCPrefsPanel::init()
{
	text_ccpath->SetValue(wxString(path_gdcc_cc));
}

/* CPrefsPanel::applyPreferences
* Applies preferences from the panel controls
*******************************************************************/
void CCPrefsPanel::applyPreferences()
{
	path_gdcc_cc = text_ccpath->GetValue();

	// Build include paths string
	string lib_paths_string, inc_paths_string;
	wxArrayString lib_paths = list_lib_paths->GetStrings();
	wxArrayString inc_paths = list_inc_paths->GetStrings();
	for(unsigned a = 0; a < lib_paths.size(); a++)
		lib_paths_string += lib_paths[a] + ";";
	if(lib_paths_string.EndsWith(";"))
		lib_paths_string.RemoveLast(1);

	path_gdcc_cc_libs = lib_paths_string;


	for(unsigned a = 0; a < inc_paths.size(); a++)
		inc_paths_string += inc_paths[a] + ";";
	if(inc_paths_string.EndsWith(";"))
		inc_paths_string.RemoveLast(1);

	path_gdcc_cc_incs = inc_paths_string;
}


/*******************************************************************
* CPREFSPANEL CLASS EVENTS
*******************************************************************/

/* CPrefsPanel::onBtnBrowseCCPath
* Called when the 'Browse' for GDCC-CC path button is clicked
*******************************************************************/
void CCPrefsPanel::onBtnBrowseCCPath(wxCommandEvent& e)
{
	// Setup cc executable file string
	string gdcc_cc_exe = "gdcc-cc";
#ifdef WIN32
	gdcc_cc_exe += ".exe";	// exe extension in windows
#elif __WXOSX__
	gdcc_cc_exe = "";
#endif

	// Open file dialog
	wxFileDialog fd(this, "Browse for GDCC-CC Executable", wxEmptyString, gdcc_cc_exe, gdcc_cc_exe);
	if(fd.ShowModal() == wxID_OK)
		text_ccpath->SetValue(fd.GetPath());
}


void CCPrefsPanel::onBtnAddIncPath(wxCommandEvent& e)
{
	wxDirDialog dlg(this, "Browse for GDCC-CC Include Path");
	if(dlg.ShowModal() == wxID_OK)
	{
		list_inc_paths->Append(dlg.GetPath());
	}
}

void CCPrefsPanel::onBtnRemoveIncPath(wxCommandEvent& e)
{
	if(list_inc_paths->GetSelection() >= 0)
		list_inc_paths->Delete(list_inc_paths->GetSelection());
}

void CCPrefsPanel::onBtnAddLibPath(wxCommandEvent& e)
{
	wxDirDialog dlg(this, "Browse for GDCC-CC Library Path");
	if(dlg.ShowModal() == wxID_OK)
	{
		list_lib_paths->Append(dlg.GetPath());
	}
}

void CCPrefsPanel::onBtnRemoveLibPath(wxCommandEvent& e)
{
	if(list_lib_paths->GetSelection() >= 0)
		list_lib_paths->Delete(list_lib_paths->GetSelection());
}
