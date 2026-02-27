//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#ifndef RME_MAINTOOLBAR_H_
#define RME_MAINTOOLBAR_H_

#include <wx/wx.h>
#include <wx/aui/aui.h>
#include <wx/aui/auibar.h>

#include "wxids.h"
#include "numberctrl.h"

class MainToolBar : public wxEvtHandler
{
public:
	MainToolBar(wxWindow* parent, wxAuiManager* manager);
	~MainToolBar();

	wxAuiPaneInfo& GetPane(ToolBarID id);
	void UpdateButtons();
	void UpdateBrushButtons();
	void UpdateBrushSize(BrushShape shape, int size);
	void UpdateIndicators();
	void Show(ToolBarID id, bool show);
	void HideAll(bool update = true);
	void LoadPerspective();
	void SavePerspective();

	void OnStandardButtonClick(wxCommandEvent& event);
	void OnBrushesButtonClick(wxCommandEvent& event);
	void OnPositionButtonClick(wxCommandEvent& event);
	void OnPositionKeyUp(wxKeyEvent& event);
	void OnPositionPasteText(wxClipboardTextEvent& event);
	void OnSizesButtonClick(wxCommandEvent& event);
	void OnIndicatorsButtonClick(wxCommandEvent& event);

private:
	static const wxString STANDARD_BAR_NAME;
	static const wxString BRUSHES_BAR_NAME;
	static const wxString POSITION_BAR_NAME;
	static const wxString SIZES_BAR_NAME;
	static const wxString INDICATORS_BAR_NAME;

	wxAuiToolBar* standard_toolbar;
	wxAuiToolBar* brushes_toolbar;
	wxAuiToolBar* position_toolbar;
	NumberCtrl* x_control;
	NumberCtrl* y_control;
	NumberCtrl* z_control;
	wxButton* go_button;
	wxAuiToolBar* sizes_toolbar;
	wxAuiToolBar* indicators_toolbar;
};

#endif // RME_MAINTOOLBAR_H_
