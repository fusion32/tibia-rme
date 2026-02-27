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

#include "main.h"
#include "const.h"
#include "main_toolbar.h"
#include "application.h"
#include "editor.h"
#include "settings.h"
#include "brush.h"
#include "pngfiles.h"
#include "artprovider.h"

#include <wx/artprov.h>
#include <wx/mstream.h>

const wxString MainToolBar::STANDARD_BAR_NAME = "standard_toolbar";
const wxString MainToolBar::BRUSHES_BAR_NAME = "brushes_toolbar";
const wxString MainToolBar::POSITION_BAR_NAME = "position_toolbar";
const wxString MainToolBar::SIZES_BAR_NAME = "sizes_toolbar";
const wxString MainToolBar::INDICATORS_BAR_NAME = "indicators_toolbar";

#define loadPNGFile(name) _wxGetBitmapFromMemory(name, sizeof(name))
inline wxBitmap* _wxGetBitmapFromMemory(const unsigned char* data, int length)
{
	wxMemoryInputStream is(data, length);
	wxImage img(is, "image/png");
	if(!img.IsOk()) return nullptr;
	return newd wxBitmap(img, -1);
}

MainToolBar::MainToolBar(wxWindow* parent, wxAuiManager* manager)
{
	wxSize icon_size = FROM_DIP(parent, wxSize(16, 16));
	wxBitmap new_bitmap = wxArtProvider::GetBitmap(wxART_NEW, wxART_TOOLBAR, icon_size);
	wxBitmap open_bitmap = wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_TOOLBAR, icon_size);
	wxBitmap save_bitmap = wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR, icon_size);
	wxBitmap saveas_bitmap = wxArtProvider::GetBitmap(wxART_FILE_SAVE_AS, wxART_TOOLBAR, icon_size);
	wxBitmap undo_bitmap = wxArtProvider::GetBitmap(wxART_UNDO, wxART_TOOLBAR, icon_size);
	wxBitmap redo_bitmap = wxArtProvider::GetBitmap(wxART_REDO, wxART_TOOLBAR, icon_size);
	wxBitmap cut_bitmap = wxArtProvider::GetBitmap(wxART_CUT, wxART_TOOLBAR, icon_size);
	wxBitmap copy_bitmap = wxArtProvider::GetBitmap(wxART_COPY, wxART_TOOLBAR, icon_size);
	wxBitmap paste_bitmap = wxArtProvider::GetBitmap(wxART_PASTE, wxART_TOOLBAR, icon_size);

	standard_toolbar = newd wxAuiToolBar(parent, TOOLBAR_STANDARD, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE);
	standard_toolbar->SetToolBitmapSize(icon_size);
	standard_toolbar->AddTool(wxID_NEW, wxEmptyString, new_bitmap, wxNullBitmap, wxITEM_NORMAL, "New Map", wxEmptyString, NULL);
	standard_toolbar->AddTool(wxID_OPEN, wxEmptyString, open_bitmap, wxNullBitmap, wxITEM_NORMAL, "Open Map", wxEmptyString, NULL);
	standard_toolbar->AddTool(wxID_SAVE, wxEmptyString, save_bitmap, wxNullBitmap, wxITEM_NORMAL, "Save Map", wxEmptyString, NULL);
	standard_toolbar->AddTool(wxID_SAVEAS, wxEmptyString, saveas_bitmap, wxNullBitmap, wxITEM_NORMAL, "Save Map As...", wxEmptyString, NULL);
	standard_toolbar->AddSeparator();
	standard_toolbar->AddTool(wxID_UNDO, wxEmptyString, undo_bitmap, wxNullBitmap, wxITEM_NORMAL, "Undo", wxEmptyString, NULL);
	standard_toolbar->AddTool(wxID_REDO, wxEmptyString, redo_bitmap, wxNullBitmap, wxITEM_NORMAL, "Redo", wxEmptyString, NULL);
	standard_toolbar->AddSeparator();
	standard_toolbar->AddTool(wxID_CUT, wxEmptyString, cut_bitmap, wxNullBitmap, wxITEM_NORMAL, "Cut", wxEmptyString, NULL);
	standard_toolbar->AddTool(wxID_COPY, wxEmptyString, copy_bitmap, wxNullBitmap, wxITEM_NORMAL, "Copy", wxEmptyString, NULL);
	standard_toolbar->AddTool(wxID_PASTE, wxEmptyString, paste_bitmap, wxNullBitmap, wxITEM_NORMAL, "Paste", wxEmptyString, NULL);
	standard_toolbar->Realize();

	wxBitmap* border_bitmap = loadPNGFile(optional_border_small_png);
	wxBitmap* eraser_bitmap = loadPNGFile(eraser_small_png);
	wxBitmap refresh_bitmap = wxArtProvider::GetBitmap(ART_REFRESH_BRUSH, wxART_TOOLBAR, icon_size);
	wxBitmap nologout_bitmap = wxArtProvider::GetBitmap(ART_NOLOGOUT_BRUSH, wxART_TOOLBAR, icon_size);
	wxBitmap pz_bitmap = wxArtProvider::GetBitmap(ART_PZ_BRUSH, wxART_TOOLBAR, icon_size);
	wxBitmap* normal_bitmap = loadPNGFile(door_normal_small_png);
	wxBitmap* locked_bitmap = loadPNGFile(door_locked_small_png);
	wxBitmap* magic_bitmap = loadPNGFile(door_magic_small_png);
	wxBitmap* quest_bitmap = loadPNGFile(door_quest_small_png);
	wxBitmap* hatch_bitmap = loadPNGFile(window_hatch_small_png);
	wxBitmap* window_bitmap = loadPNGFile(window_normal_small_png);

	brushes_toolbar = newd wxAuiToolBar(parent, TOOLBAR_BRUSHES, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE);
	brushes_toolbar->SetToolBitmapSize(icon_size);
	brushes_toolbar->AddTool(PALETTE_TERRAIN_OPTIONAL_BORDER_TOOL, wxEmptyString, *border_bitmap, wxNullBitmap, wxITEM_CHECK, "Border", wxEmptyString, NULL);
	brushes_toolbar->AddTool(PALETTE_TERRAIN_ERASER, wxEmptyString, *eraser_bitmap, wxNullBitmap, wxITEM_CHECK, "Eraser", wxEmptyString, NULL);
	brushes_toolbar->AddSeparator();
	brushes_toolbar->AddTool(PALETTE_TERRAIN_REFRESH_TOOL, wxEmptyString, refresh_bitmap, wxNullBitmap, wxITEM_CHECK, "Refresh Zone", wxEmptyString, NULL);
	brushes_toolbar->AddTool(PALETTE_TERRAIN_NOLOGOUT_TOOL, wxEmptyString, nologout_bitmap, wxNullBitmap, wxITEM_CHECK, "No Logout Zone", wxEmptyString, NULL);
	brushes_toolbar->AddTool(PALETTE_TERRAIN_PZ_TOOL, wxEmptyString, pz_bitmap, wxNullBitmap, wxITEM_CHECK, "Protected Zone", wxEmptyString, NULL);
	brushes_toolbar->AddSeparator();
	brushes_toolbar->AddTool(PALETTE_TERRAIN_NORMAL_DOOR, wxEmptyString, *normal_bitmap, wxNullBitmap, wxITEM_CHECK, "Normal Door", wxEmptyString, NULL);
	brushes_toolbar->AddTool(PALETTE_TERRAIN_LOCKED_DOOR, wxEmptyString, *locked_bitmap, wxNullBitmap, wxITEM_CHECK, "Locked Door", wxEmptyString, NULL);
	brushes_toolbar->AddTool(PALETTE_TERRAIN_MAGIC_DOOR, wxEmptyString, *magic_bitmap, wxNullBitmap, wxITEM_CHECK, "Magic Door", wxEmptyString, NULL);
	brushes_toolbar->AddTool(PALETTE_TERRAIN_QUEST_DOOR, wxEmptyString, *quest_bitmap, wxNullBitmap, wxITEM_CHECK, "Quest Door", wxEmptyString, NULL);
	brushes_toolbar->AddTool(PALETTE_TERRAIN_HATCH_DOOR, wxEmptyString, *hatch_bitmap, wxNullBitmap, wxITEM_CHECK, "Hatch Window", wxEmptyString, NULL);
	brushes_toolbar->AddTool(PALETTE_TERRAIN_WINDOW_DOOR, wxEmptyString, *window_bitmap, wxNullBitmap, wxITEM_CHECK, "Window", wxEmptyString, NULL);
	brushes_toolbar->Realize();

	wxBitmap go_bitmap = wxArtProvider::GetBitmap(ART_POSITION_GO, wxART_TOOLBAR, icon_size);

	position_toolbar = newd wxAuiToolBar(parent, TOOLBAR_POSITION, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORZ_TEXT);
	position_toolbar->SetToolBitmapSize(icon_size);
	x_control = newd NumberCtrl(position_toolbar, wxID_ANY, wxDefaultPosition, FROM_DIP(parent, wxSize(60, 20)), wxTE_PROCESS_ENTER, 0, rme::MapMaxWidth, 0, "X");
	x_control->SetToolTip("X Coordinate");
	y_control = newd NumberCtrl(position_toolbar, wxID_ANY, wxDefaultPosition, FROM_DIP(parent, wxSize(60, 20)), wxTE_PROCESS_ENTER, 0, rme::MapMaxHeight, 0, "Y");
	y_control->SetToolTip("Y Coordinate");
	z_control = newd NumberCtrl(position_toolbar, wxID_ANY, wxDefaultPosition, FROM_DIP(parent, wxSize(35, 20)), wxTE_PROCESS_ENTER, 0, rme::MapMaxLayer, rme::MapGroundLayer, "Z");
	z_control->SetToolTip("Z Coordinate");
	go_button = newd wxButton(position_toolbar, TOOLBAR_POSITION_GO, wxEmptyString, wxDefaultPosition, FROM_DIP(parent, wxSize(22, 20)));
	go_button->SetBitmap(go_bitmap);
	go_button->SetToolTip("Go To Position");
	position_toolbar->AddControl(x_control);
	position_toolbar->AddControl(y_control);
	position_toolbar->AddControl(z_control);
	position_toolbar->AddControl(go_button);
	position_toolbar->Realize();

	wxBitmap circular_bitmap = wxArtProvider::GetBitmap(ART_CIRCULAR, wxART_TOOLBAR, icon_size);
	wxBitmap rectangular_bitmap = wxArtProvider::GetBitmap(ART_RECTANGULAR, wxART_TOOLBAR, icon_size);
	wxBitmap size1_bitmap = wxArtProvider::GetBitmap(ART_RECTANGULAR_1, wxART_TOOLBAR, icon_size);
	wxBitmap size2_bitmap = wxArtProvider::GetBitmap(ART_RECTANGULAR_2, wxART_TOOLBAR, icon_size);
	wxBitmap size3_bitmap = wxArtProvider::GetBitmap(ART_RECTANGULAR_3, wxART_TOOLBAR, icon_size);
	wxBitmap size4_bitmap = wxArtProvider::GetBitmap(ART_RECTANGULAR_4, wxART_TOOLBAR, icon_size);
	wxBitmap size5_bitmap = wxArtProvider::GetBitmap(ART_RECTANGULAR_5, wxART_TOOLBAR, icon_size);
	wxBitmap size6_bitmap = wxArtProvider::GetBitmap(ART_RECTANGULAR_6, wxART_TOOLBAR, icon_size);
	wxBitmap size7_bitmap = wxArtProvider::GetBitmap(ART_RECTANGULAR_7, wxART_TOOLBAR, icon_size);

	sizes_toolbar = newd wxAuiToolBar(parent, TOOLBAR_SIZES, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE);
	sizes_toolbar->SetToolBitmapSize(icon_size);
	sizes_toolbar->AddTool(TOOLBAR_SIZES_RECTANGULAR, wxEmptyString, rectangular_bitmap, wxNullBitmap, wxITEM_CHECK, "Rectangular Brush", wxEmptyString, NULL);
	sizes_toolbar->AddTool(TOOLBAR_SIZES_CIRCULAR, wxEmptyString, circular_bitmap, wxNullBitmap, wxITEM_CHECK, "Circular Brush", wxEmptyString, NULL);
	sizes_toolbar->AddSeparator();
	sizes_toolbar->AddTool(TOOLBAR_SIZES_1, wxEmptyString, size1_bitmap, wxNullBitmap, wxITEM_CHECK, "Size 1", wxEmptyString, NULL);
	sizes_toolbar->AddTool(TOOLBAR_SIZES_2, wxEmptyString, size2_bitmap, wxNullBitmap, wxITEM_CHECK, "Size 2", wxEmptyString, NULL);
	sizes_toolbar->AddTool(TOOLBAR_SIZES_3, wxEmptyString, size3_bitmap, wxNullBitmap, wxITEM_CHECK, "Size 3", wxEmptyString, NULL);
	sizes_toolbar->AddTool(TOOLBAR_SIZES_4, wxEmptyString, size4_bitmap, wxNullBitmap, wxITEM_CHECK, "Size 4", wxEmptyString, NULL);
	sizes_toolbar->AddTool(TOOLBAR_SIZES_5, wxEmptyString, size5_bitmap, wxNullBitmap, wxITEM_CHECK, "Size 5", wxEmptyString, NULL);
	sizes_toolbar->AddTool(TOOLBAR_SIZES_6, wxEmptyString, size6_bitmap, wxNullBitmap, wxITEM_CHECK, "Size 6", wxEmptyString, NULL);
	sizes_toolbar->AddTool(TOOLBAR_SIZES_7, wxEmptyString, size7_bitmap, wxNullBitmap, wxITEM_CHECK, "Size 7", wxEmptyString, NULL);
	sizes_toolbar->Realize();
	sizes_toolbar->ToggleTool(TOOLBAR_SIZES_RECTANGULAR, true);
	sizes_toolbar->ToggleTool(TOOLBAR_SIZES_1, true);

	wxBitmap hooks_bitmap = wxArtProvider::GetBitmap(ART_HOOKS_TOOLBAR, wxART_TOOLBAR, icon_size);
	wxBitmap pickupables_bitmap = wxArtProvider::GetBitmap(ART_PICKUPABLE_TOOLBAR, wxART_TOOLBAR, icon_size);
	wxBitmap moveables_bitmap = wxArtProvider::GetBitmap(ART_MOVEABLE_TOOLBAR, wxART_TOOLBAR, icon_size);

	indicators_toolbar = newd wxAuiToolBar(parent, TOOLBAR_INDICATORS, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE);
	indicators_toolbar->SetToolBitmapSize(icon_size);
	indicators_toolbar->AddTool(TOOLBAR_HOOKS, wxEmptyString, hooks_bitmap, wxNullBitmap, wxITEM_CHECK, "Wall Hooks", wxEmptyString, NULL);
	indicators_toolbar->AddTool(TOOLBAR_PICKUPABLES, wxEmptyString, pickupables_bitmap, wxNullBitmap, wxITEM_CHECK, "Pickupables", wxEmptyString, NULL);
	indicators_toolbar->AddTool(TOOLBAR_MOVEABLES, wxEmptyString, moveables_bitmap, wxNullBitmap, wxITEM_CHECK, "Moveables", wxEmptyString, NULL);
	indicators_toolbar->Realize();
	indicators_toolbar->ToggleTool(TOOLBAR_HOOKS, g_settings.getBoolean(Config::SHOW_WALL_HOOKS));
	indicators_toolbar->ToggleTool(TOOLBAR_PICKUPABLES, g_settings.getBoolean(Config::SHOW_PICKUPABLES));
	indicators_toolbar->ToggleTool(TOOLBAR_MOVEABLES, g_settings.getBoolean(Config::SHOW_MOVEABLES));

	manager->AddPane(standard_toolbar, wxAuiPaneInfo().Name(STANDARD_BAR_NAME).ToolbarPane().Top().Row(1).Position(1).Floatable(false));
	manager->AddPane(brushes_toolbar, wxAuiPaneInfo().Name(BRUSHES_BAR_NAME).ToolbarPane().Top().Row(1).Position(2).Floatable(false));
	manager->AddPane(position_toolbar, wxAuiPaneInfo().Name(POSITION_BAR_NAME).ToolbarPane().Top().Row(1).Position(4).Floatable(false));
	manager->AddPane(sizes_toolbar, wxAuiPaneInfo().Name(SIZES_BAR_NAME).ToolbarPane().Top().Row(1).Position(3).Floatable(false));
	manager->AddPane(indicators_toolbar, wxAuiPaneInfo().Name(INDICATORS_BAR_NAME).ToolbarPane().Top().Row(1).Position(5).Floatable(false));

	standard_toolbar->Bind(wxEVT_COMMAND_MENU_SELECTED, &MainToolBar::OnStandardButtonClick, this);
	brushes_toolbar->Bind(wxEVT_COMMAND_MENU_SELECTED, &MainToolBar::OnBrushesButtonClick, this);
	x_control->Bind(wxEVT_TEXT_PASTE, &MainToolBar::OnPositionPasteText, this);
	x_control->Bind(wxEVT_KEY_UP, &MainToolBar::OnPositionKeyUp, this);
	y_control->Bind(wxEVT_TEXT_PASTE, &MainToolBar::OnPositionPasteText, this);
	y_control->Bind(wxEVT_KEY_UP, &MainToolBar::OnPositionKeyUp, this);
	z_control->Bind(wxEVT_TEXT_PASTE, &MainToolBar::OnPositionPasteText, this);
	z_control->Bind(wxEVT_KEY_UP, &MainToolBar::OnPositionKeyUp, this);
	go_button->Bind(wxEVT_BUTTON, &MainToolBar::OnPositionButtonClick, this);
	sizes_toolbar->Bind(wxEVT_COMMAND_MENU_SELECTED, &MainToolBar::OnSizesButtonClick, this);
	indicators_toolbar->Bind(wxEVT_COMMAND_MENU_SELECTED, &MainToolBar::OnIndicatorsButtonClick, this);

	HideAll();
}

MainToolBar::~MainToolBar()
{
	// no-op
}

void MainToolBar::UpdateButtons()
{
	bool has_map = g_editor.IsProjectOpen();
	bool is_host = has_map;

	standard_toolbar->EnableTool(wxID_UNDO, g_editor.canUndo());
	standard_toolbar->EnableTool(wxID_REDO, g_editor.canRedo());
	standard_toolbar->EnableTool(wxID_PASTE, g_editor.copybuffer.canPaste());

	standard_toolbar->EnableTool(wxID_SAVE, is_host);
	standard_toolbar->EnableTool(wxID_SAVEAS, is_host);
	standard_toolbar->EnableTool(wxID_CUT, has_map);
	standard_toolbar->EnableTool(wxID_COPY, has_map);
	standard_toolbar->Refresh();

	brushes_toolbar->EnableTool(PALETTE_TERRAIN_OPTIONAL_BORDER_TOOL, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_ERASER, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_REFRESH_TOOL, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_NOLOGOUT_TOOL, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_PZ_TOOL, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_NORMAL_DOOR, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_LOCKED_DOOR, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_MAGIC_DOOR, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_QUEST_DOOR, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_HATCH_DOOR, has_map);
	brushes_toolbar->EnableTool(PALETTE_TERRAIN_WINDOW_DOOR, has_map);
	brushes_toolbar->Refresh();

	position_toolbar->EnableTool(TOOLBAR_POSITION_GO, has_map);
	x_control->Enable(has_map);
	y_control->Enable(has_map);
	z_control->Enable(has_map);

	if(has_map) {
		Position minPos = g_editor.map.getMinPosition();
		Position maxPos = g_editor.map.getMaxPosition();
		x_control->SetRange(minPos.x, maxPos.x);
		y_control->SetRange(minPos.y, maxPos.y);
	}

	sizes_toolbar->EnableTool(TOOLBAR_SIZES_CIRCULAR, has_map);
	sizes_toolbar->EnableTool(TOOLBAR_SIZES_RECTANGULAR, has_map);
	sizes_toolbar->EnableTool(TOOLBAR_SIZES_1, has_map);
	sizes_toolbar->EnableTool(TOOLBAR_SIZES_2, has_map);
	sizes_toolbar->EnableTool(TOOLBAR_SIZES_3, has_map);
	sizes_toolbar->EnableTool(TOOLBAR_SIZES_4, has_map);
	sizes_toolbar->EnableTool(TOOLBAR_SIZES_5, has_map);
	sizes_toolbar->EnableTool(TOOLBAR_SIZES_6, has_map);
	sizes_toolbar->EnableTool(TOOLBAR_SIZES_7, has_map);
	sizes_toolbar->Refresh();
}

void MainToolBar::UpdateBrushButtons()
{
	Brush* brush = g_editor.GetCurrentBrush();
	if(brush) {
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_OPTIONAL_BORDER_TOOL, brush == g_editor.optional_brush);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_ERASER, brush == g_editor.eraser);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_REFRESH_TOOL, brush == g_editor.refresh_brush);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_NOLOGOUT_TOOL, brush == g_editor.nolog_brush);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_PZ_TOOL, brush == g_editor.pz_brush);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_NORMAL_DOOR, brush == g_editor.normal_door_brush);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_LOCKED_DOOR, brush == g_editor.locked_door_brush);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_MAGIC_DOOR, brush == g_editor.magic_door_brush);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_QUEST_DOOR, brush == g_editor.quest_door_brush);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_HATCH_DOOR, brush == g_editor.hatch_door_brush);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_WINDOW_DOOR, brush == g_editor.window_door_brush);
	} else {
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_OPTIONAL_BORDER_TOOL, false);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_ERASER, false);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_REFRESH_TOOL, false);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_NOLOGOUT_TOOL, false);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_PZ_TOOL, false);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_NORMAL_DOOR, false);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_LOCKED_DOOR, false);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_MAGIC_DOOR, false);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_QUEST_DOOR, false);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_HATCH_DOOR, false);
		brushes_toolbar->ToggleTool(PALETTE_TERRAIN_WINDOW_DOOR, false);
	}
	g_editor.aui_manager->Update();
}

void MainToolBar::UpdateBrushSize(BrushShape shape, int size)
{
	if(shape == BRUSHSHAPE_CIRCLE) {
		sizes_toolbar->ToggleTool(TOOLBAR_SIZES_CIRCULAR, true);
		sizes_toolbar->ToggleTool(TOOLBAR_SIZES_RECTANGULAR, false);

		wxSize icon_size = wxSize(16, 16);
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_1, wxArtProvider::GetBitmap(ART_CIRCULAR_1, wxART_TOOLBAR, icon_size));
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_2, wxArtProvider::GetBitmap(ART_CIRCULAR_2, wxART_TOOLBAR, icon_size));
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_3, wxArtProvider::GetBitmap(ART_CIRCULAR_3, wxART_TOOLBAR, icon_size));
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_4, wxArtProvider::GetBitmap(ART_CIRCULAR_4, wxART_TOOLBAR, icon_size));
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_5, wxArtProvider::GetBitmap(ART_CIRCULAR_5, wxART_TOOLBAR, icon_size));
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_6, wxArtProvider::GetBitmap(ART_CIRCULAR_6, wxART_TOOLBAR, icon_size));
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_7, wxArtProvider::GetBitmap(ART_CIRCULAR_7, wxART_TOOLBAR, icon_size));
	} else {
		sizes_toolbar->ToggleTool(TOOLBAR_SIZES_CIRCULAR, false);
		sizes_toolbar->ToggleTool(TOOLBAR_SIZES_RECTANGULAR, true);

		wxSize icon_size = wxSize(16, 16);
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_1, wxArtProvider::GetBitmap(ART_RECTANGULAR_1, wxART_TOOLBAR, icon_size));
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_2, wxArtProvider::GetBitmap(ART_RECTANGULAR_2, wxART_TOOLBAR, icon_size));
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_3, wxArtProvider::GetBitmap(ART_RECTANGULAR_3, wxART_TOOLBAR, icon_size));
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_4, wxArtProvider::GetBitmap(ART_RECTANGULAR_4, wxART_TOOLBAR, icon_size));
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_5, wxArtProvider::GetBitmap(ART_RECTANGULAR_5, wxART_TOOLBAR, icon_size));
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_6, wxArtProvider::GetBitmap(ART_RECTANGULAR_6, wxART_TOOLBAR, icon_size));
		sizes_toolbar->SetToolBitmap(TOOLBAR_SIZES_7, wxArtProvider::GetBitmap(ART_RECTANGULAR_7, wxART_TOOLBAR, icon_size));
	}

	sizes_toolbar->ToggleTool(TOOLBAR_SIZES_1, size == 0);
	sizes_toolbar->ToggleTool(TOOLBAR_SIZES_2, size == 1);
	sizes_toolbar->ToggleTool(TOOLBAR_SIZES_3, size == 2);
	sizes_toolbar->ToggleTool(TOOLBAR_SIZES_4, size == 4);
	sizes_toolbar->ToggleTool(TOOLBAR_SIZES_5, size == 6);
	sizes_toolbar->ToggleTool(TOOLBAR_SIZES_6, size == 8);
	sizes_toolbar->ToggleTool(TOOLBAR_SIZES_7, size == 11);

	g_editor.aui_manager->Update();
}

void MainToolBar::UpdateIndicators()
{
	indicators_toolbar->ToggleTool(TOOLBAR_HOOKS, g_settings.getBoolean(Config::SHOW_WALL_HOOKS));
	indicators_toolbar->ToggleTool(TOOLBAR_PICKUPABLES, g_settings.getBoolean(Config::SHOW_PICKUPABLES));
	indicators_toolbar->ToggleTool(TOOLBAR_MOVEABLES, g_settings.getBoolean(Config::SHOW_MOVEABLES));

	g_editor.aui_manager->Update();
}

void MainToolBar::Show(ToolBarID id, bool show)
{
	ASSERT(g_editor.aui_manager != NULL);
	wxAuiPaneInfo &pane = GetPane(id);
	if(pane.IsOk()) {
		pane.Show(show);
		g_editor.aui_manager->Update();
	}
}

void MainToolBar::HideAll(bool update)
{
	ASSERT(g_editor.aui_manager != NULL);
	wxAuiPaneInfoArray &panes = g_editor.aui_manager->GetAllPanes();
	for(int i = 0, count = panes.GetCount(); i < count; ++i) {
		if(!panes.Item(i).IsToolbar())
			panes.Item(i).Hide();
	}

	if(update)
		g_editor.aui_manager->Update();
}

void MainToolBar::LoadPerspective()
{
	ASSERT(g_editor.aui_manager != NULL);

	if(g_settings.getBoolean(Config::SHOW_TOOLBAR_STANDARD)) {
		std::string layout = g_settings.getString(Config::TOOLBAR_STANDARD_LAYOUT);
		if(!layout.empty())
			g_editor.aui_manager->LoadPaneInfo(wxString(layout), GetPane(TOOLBAR_STANDARD));
		GetPane(TOOLBAR_STANDARD).Show();
	} else
		GetPane(TOOLBAR_STANDARD).Hide();

	if(g_settings.getBoolean(Config::SHOW_TOOLBAR_BRUSHES)) {
		std::string layout = g_settings.getString(Config::TOOLBAR_BRUSHES_LAYOUT);
		if(!layout.empty())
			g_editor.aui_manager->LoadPaneInfo(wxString(layout), GetPane(TOOLBAR_BRUSHES));
		GetPane(TOOLBAR_BRUSHES).Show();
	} else
		GetPane(TOOLBAR_BRUSHES).Hide();

	if(g_settings.getBoolean(Config::SHOW_TOOLBAR_POSITION)) {
		std::string layout = g_settings.getString(Config::TOOLBAR_POSITION_LAYOUT);
		if(!layout.empty())
			g_editor.aui_manager->LoadPaneInfo(wxString(layout), GetPane(TOOLBAR_POSITION));
		GetPane(TOOLBAR_POSITION).Show();
	} else
		GetPane(TOOLBAR_POSITION).Hide();

	if(g_settings.getBoolean(Config::SHOW_TOOLBAR_SIZES)) {
		std::string layout = g_settings.getString(Config::TOOLBAR_SIZES_LAYOUT);
		if(!layout.empty())
			g_editor.aui_manager->LoadPaneInfo(wxString(layout), GetPane(TOOLBAR_SIZES));
		GetPane(TOOLBAR_SIZES).Show();
	} else
		GetPane(TOOLBAR_SIZES).Hide();

	if(g_settings.getBoolean(Config::SHOW_TOOLBAR_INDICATORS)) {
		std::string layout = g_settings.getString(Config::TOOLBAR_INDICATORS_LAYOUT);
		if(!layout.empty())
			g_editor.aui_manager->LoadPaneInfo(wxString(layout), GetPane(TOOLBAR_INDICATORS));
		GetPane(TOOLBAR_INDICATORS).Show();
	} else
		GetPane(TOOLBAR_INDICATORS).Hide();

	g_editor.aui_manager->Update();
}

void MainToolBar::SavePerspective()
{
	ASSERT(g_editor.aui_manager != NULL);

	if(g_settings.getBoolean(Config::SHOW_TOOLBAR_STANDARD)) {
		wxString layout = g_editor.aui_manager->SavePaneInfo(GetPane(TOOLBAR_STANDARD));
		g_settings.setString(Config::TOOLBAR_STANDARD_LAYOUT, layout.ToStdString());
	}

	if(g_settings.getBoolean(Config::SHOW_TOOLBAR_BRUSHES)) {
		wxString layout = g_editor.aui_manager->SavePaneInfo(GetPane(TOOLBAR_BRUSHES));
		g_settings.setString(Config::TOOLBAR_BRUSHES_LAYOUT, layout.ToStdString());
	}

	if(g_settings.getBoolean(Config::SHOW_TOOLBAR_POSITION)) {
		wxString layout = g_editor.aui_manager->SavePaneInfo(GetPane(TOOLBAR_POSITION));
		g_settings.setString(Config::TOOLBAR_POSITION_LAYOUT, layout.ToStdString());
	}

	if(g_settings.getBoolean(Config::SHOW_TOOLBAR_SIZES)) {
		wxString layout = g_editor.aui_manager->SavePaneInfo(GetPane(TOOLBAR_SIZES));
		g_settings.setString(Config::TOOLBAR_SIZES_LAYOUT, layout.ToStdString());
	}

	if(g_settings.getBoolean(Config::SHOW_TOOLBAR_INDICATORS)) {
		wxString layout = g_editor.aui_manager->SavePaneInfo(GetPane(TOOLBAR_INDICATORS));
		g_settings.setString(Config::SHOW_TOOLBAR_INDICATORS, layout.ToStdString());
	}
}

void MainToolBar::OnStandardButtonClick(wxCommandEvent& event)
{
	switch (event.GetId()) {
		case wxID_NEW:
			g_editor.NewProject();
			break;
		case wxID_OPEN:
			g_editor.OpenProject();
			break;
		case wxID_SAVE:
			g_editor.SaveProject();
			break;
		case wxID_SAVEAS:
			g_editor.SaveProjectAs();
			break;
		case wxID_UNDO:
			g_editor.DoUndo();
			break;
		case wxID_REDO:
			g_editor.DoRedo();
			break;
		case wxID_CUT:
			g_editor.DoCut();
			break;
		case wxID_COPY:
			g_editor.DoCopy();
			break;
		case wxID_PASTE:
			g_editor.PreparePaste();
			break;
		default:
			break;
	}
}

void MainToolBar::OnBrushesButtonClick(wxCommandEvent& event)
{
	if(!g_editor.IsProjectOpen())
		return;

	switch (event.GetId()) {
		case PALETTE_TERRAIN_OPTIONAL_BORDER_TOOL:
			g_editor.SelectBrush(g_editor.optional_brush);
			break;
		case PALETTE_TERRAIN_ERASER:
			g_editor.SelectBrush(g_editor.eraser);
			break;
		case PALETTE_TERRAIN_REFRESH_TOOL:
			g_editor.SelectBrush(g_editor.refresh_brush);
			break;
		case PALETTE_TERRAIN_NOLOGOUT_TOOL:
			g_editor.SelectBrush(g_editor.nolog_brush);
			break;
		case PALETTE_TERRAIN_PZ_TOOL:
			g_editor.SelectBrush(g_editor.pz_brush);
			break;
		case PALETTE_TERRAIN_NORMAL_DOOR:
			g_editor.SelectBrush(g_editor.normal_door_brush);
			break;
		case PALETTE_TERRAIN_LOCKED_DOOR:
			g_editor.SelectBrush(g_editor.locked_door_brush);
			break;
		case PALETTE_TERRAIN_MAGIC_DOOR:
			g_editor.SelectBrush(g_editor.magic_door_brush);
			break;
		case PALETTE_TERRAIN_QUEST_DOOR:
			g_editor.SelectBrush(g_editor.quest_door_brush);
			break;
		case PALETTE_TERRAIN_HATCH_DOOR:
			g_editor.SelectBrush(g_editor.hatch_door_brush);
			break;
		case PALETTE_TERRAIN_WINDOW_DOOR:
			g_editor.SelectBrush(g_editor.window_door_brush);
			break;
		default:
			break;
	}
}

void MainToolBar::OnPositionButtonClick(wxCommandEvent& event)
{
	if(!g_editor.IsProjectOpen())
		return;

	if(event.GetId() == TOOLBAR_POSITION_GO) {
		Position pos(x_control->GetIntValue(), y_control->GetIntValue(), z_control->GetIntValue());
		if(pos.isValid())
			g_editor.SetScreenCenterPosition(pos);
	}
}

void MainToolBar::OnPositionKeyUp(wxKeyEvent& event)
{
	if(event.GetKeyCode() == WXK_TAB) {
		if(x_control->HasFocus()) {
			y_control->SelectAll();
			y_control->SetFocus();
		} else if(y_control->HasFocus()) {
			z_control->SelectAll();
			z_control->SetFocus();
		} else if(z_control->HasFocus()) {
			go_button->SetFocus();
		}
	} else if(event.GetKeyCode() == WXK_NUMPAD_ENTER || event.GetKeyCode() == WXK_RETURN) {
		Position pos(x_control->GetIntValue(), y_control->GetIntValue(), z_control->GetIntValue());
		if(pos.isValid())
			g_editor.SetScreenCenterPosition(pos);
	}
	event.Skip();
}

void MainToolBar::OnPositionPasteText(wxClipboardTextEvent& event)
{
	Position position;
	if(posFromClipboard(position.x, position.y, position.z)){
		x_control->SetIntValue(position.x);
		y_control->SetIntValue(position.y);
		z_control->SetIntValue(position.z);
	}else{
		event.Skip();
	}
}

void MainToolBar::OnSizesButtonClick(wxCommandEvent& event)
{
	if(!g_editor.IsProjectOpen())
		return;

	switch (event.GetId()) {
		case TOOLBAR_SIZES_CIRCULAR:
			g_editor.SetBrushShape(BRUSHSHAPE_CIRCLE);
			break;
		case TOOLBAR_SIZES_RECTANGULAR:
			g_editor.SetBrushShape(BRUSHSHAPE_SQUARE);
			break;
		case TOOLBAR_SIZES_1:
			g_editor.SetBrushSize(0);
			break;
		case TOOLBAR_SIZES_2:
			g_editor.SetBrushSize(1);
			break;
		case TOOLBAR_SIZES_3:
			g_editor.SetBrushSize(2);
			break;
		case TOOLBAR_SIZES_4:
			g_editor.SetBrushSize(4);
			break;
		case TOOLBAR_SIZES_5:
			g_editor.SetBrushSize(6);
			break;
		case TOOLBAR_SIZES_6:
			g_editor.SetBrushSize(8);
			break;
		case TOOLBAR_SIZES_7:
			g_editor.SetBrushSize(11);
			break;
		default:
			break;
	}
}

void MainToolBar::OnIndicatorsButtonClick(wxCommandEvent& event)
{
	bool toggled = indicators_toolbar->GetToolToggled(event.GetId());
	switch (event.GetId()) {
		case TOOLBAR_HOOKS:
			g_settings.setInteger(Config::SHOW_WALL_HOOKS, toggled);
			g_editor.root->UpdateIndicatorsMenu();
			g_editor.RefreshView();
			break;
		case TOOLBAR_PICKUPABLES:
			g_settings.setInteger(Config::SHOW_PICKUPABLES, toggled);
			g_editor.root->UpdateIndicatorsMenu();
			g_editor.RefreshView();
			break;
		case TOOLBAR_MOVEABLES:
			g_settings.setInteger(Config::SHOW_MOVEABLES, toggled);
			g_editor.root->UpdateIndicatorsMenu();
			g_editor.RefreshView();
			break;
		default:
			break;
	}
}

wxAuiPaneInfo& MainToolBar::GetPane(ToolBarID id)
{
	ASSERT(g_editor.aui_manager != NULL);
	switch (id) {
		case TOOLBAR_STANDARD:
			return g_editor.aui_manager->GetPane(STANDARD_BAR_NAME);
		case TOOLBAR_BRUSHES:
			return g_editor.aui_manager->GetPane(BRUSHES_BAR_NAME);
		case TOOLBAR_POSITION:
			return g_editor.aui_manager->GetPane(POSITION_BAR_NAME);
		case TOOLBAR_SIZES:
			return g_editor.aui_manager->GetPane(SIZES_BAR_NAME);
		case TOOLBAR_INDICATORS:
			return g_editor.aui_manager->GetPane(INDICATORS_BAR_NAME);
		default:
			return wxAuiNullPaneInfo;
	}
}
