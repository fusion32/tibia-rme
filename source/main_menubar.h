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

#ifndef RME_MAIN_BAR_H_
#define RME_MAIN_BAR_H_

#include <wx/docview.h>
#include <wx/menu.h>

class MainFrame;

struct MenuBarAction {
	int        id;
	wxItemKind kind;
};

class MainMenuBar : public wxMenuBar //public wxEvtHandler
{
public:
	MainMenuBar(MainFrame* frame);
	~MainMenuBar() override;

	void LoadDefault(void);
	bool Load(const wxString &projectDir);

	// Update
	// Turn on/off all buttons according to current editor state
	void Update() override;
	void UpdateFloorMenu(); // Only concerns the floor menu
	void UpdateIndicatorsMenu();

	// Interface
	void EnableItem(int id, bool enable);
	void CheckItem(int id, bool enable);
	bool IsItemChecked(int id) const;

	// Event handlers for all menu buttons
	// File Menu
	void OnNew(wxCommandEvent& event);
	void OnOpen(wxCommandEvent& event);
	void OnOpenRecent(wxCommandEvent& event);
	void OnSave(wxCommandEvent& event);
	void OnSaveAs(wxCommandEvent& event);
	void OnClose(wxCommandEvent& event);
	void OnPreferences(wxCommandEvent& event);
	void OnQuit(wxCommandEvent& event);

	// Import/Export Menu
	void OnExportPatch(wxCommandEvent &event);
	void OnImportMap(wxCommandEvent& event);
	void OnExportMinimap(wxCommandEvent& event);
	void OnReloadProject(wxCommandEvent& event);

	// Edit Menu
	void OnUndo(wxCommandEvent& event);
	void OnRedo(wxCommandEvent& event);
	void OnBorderizeSelection(wxCommandEvent& event);
	void OnBorderizeMap(wxCommandEvent& event);
	void OnRandomizeSelection(wxCommandEvent& event);
	void OnRandomizeMap(wxCommandEvent& event);
	void OnJumpToBrush(wxCommandEvent& event);
	void OnJumpToItemBrush(wxCommandEvent& event);
	void OnGotoPreviousPosition(wxCommandEvent& event);
	void OnGotoPosition(wxCommandEvent& event);
	void OnMapRemoveItems(wxCommandEvent& event);
	void OnMapRemoveCorpses(wxCommandEvent& event);
	void OnMapRemoveUnreachable(wxCommandEvent& event);
	void OnMapRemoveEmptySpawns(wxCommandEvent& event);
	void OnClearHouseTiles(wxCommandEvent& event);
	void OnToggleAutomagic(wxCommandEvent& event);
	void OnSelectionTypeChange(wxCommandEvent& event);
	void OnCut(wxCommandEvent& event);
	void OnCopy(wxCommandEvent& event);
	void OnPaste(wxCommandEvent& event);
	void OnSearchForItem(wxCommandEvent& event);
	void OnReplaceItems(wxCommandEvent& event);
	void OnSearchForStuffOnMap(wxCommandEvent& event);
	void OnSearchForContainerOnMap(wxCommandEvent& event);
	void OnSearchForWritableOnMap(wxCommandEvent& event);
	void OnSearchForDuplicatedItemsOnMap(wxCommandEvent& event);

	// Select menu
	void OnSearchForStuffOnSelection(wxCommandEvent& event);
	void OnSearchForContainerOnSelection(wxCommandEvent& event);
	void OnSearchForWritableOnSelection(wxCommandEvent& event);
	void OnSearchForItemOnSelection(wxCommandEvent& event);
	void OnSearchForDuplicatedItemsOnSelection(wxCommandEvent& event);
	void OnReplaceItemsOnSelection(wxCommandEvent& event);
	void OnRemoveItemOnSelection(wxCommandEvent& event);

	// Map menu
	void OnMapEditTowns(wxCommandEvent& event);
	void OnMapEditItems(wxCommandEvent& event);
	void OnMapEditMonsters(wxCommandEvent& event);
	void OnMapCleanHouseItems(wxCommandEvent& event);
	void OnMapCleanup(wxCommandEvent& event);
	void OnMapStatistics(wxCommandEvent& event);

	// View Menu
	void OnToolbars(wxCommandEvent& event);
	void OnToggleFullscreen(wxCommandEvent& event);
	void OnZoomIn(wxCommandEvent& event);
	void OnZoomOut(wxCommandEvent& event);
	void OnZoomNormal(wxCommandEvent& event);
	void OnChangeViewSettings(wxCommandEvent& event);

	// Window Menu
	void OnProblemsWindow(wxCommandEvent& event);
	void OnMinimapWindow(wxCommandEvent& event);
	void OnActionsHistoryWindow(wxCommandEvent& event);
	void OnNewPalette(wxCommandEvent& event);
	void OnTakeScreenshot(wxCommandEvent& event);
	void OnSelectTerrainPalette(wxCommandEvent& event);
	void OnSelectDoodadPalette(wxCommandEvent& event);
	void OnSelectItemPalette(wxCommandEvent& event);
	void OnSelectHousePalette(wxCommandEvent& event);
	void OnSelectCreaturePalette(wxCommandEvent& event);
	void OnSelectWaypointPalette(wxCommandEvent& event);
	void OnSelectRawPalette(wxCommandEvent& event);

	// Floor menu
	void OnChangeFloor(wxCommandEvent& event);

	// About Menu
	void OnDebugViewDat(wxCommandEvent& event);
	void OnGotoWebsite(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);

protected:
	bool LoadDocument(pugi::xml_document &doc, const wxString &name);
	// Load and returns a menu item, also sets accelerator
	wxObject* LoadItem(pugi::xml_node node, wxMenu* parent,
			std::vector<wxAcceleratorEntry> &accelerators);
	// Checks the items in the menus according to the settings (in config)
	void LoadValues();
	void SearchItems(bool container, bool writable, bool onSelection = false);
	void SearchDuplicatedItems(bool selection);

protected:
	MainFrame* frame;

	// Used so that calling Check on menu items don't trigger events (avoids infinite recursion)
	bool checking_programmaticly;

	std::unordered_map<std::string, MenuBarAction> actions;
	std::unordered_map<int, std::vector<wxMenuItem*>> menuItems;

	DECLARE_EVENT_TABLE()
};

#endif
