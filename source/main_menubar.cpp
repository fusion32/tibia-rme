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

#include "main_menubar.h"
#include "application.h"
#include "preferences.h"
#include "about_window.h"
#include "minimap_window.h"
#include "dat_debug_view.h"
#include "result_window.h"
#include "find_item_window.h"
#include "duplicated_items_window.h"
#include "settings.h"
#include "items.h"
#include "editor.h"
#include "materials.h"
#include "map_display.h"
#include "map_window.h"

BEGIN_EVENT_TABLE(MainMenuBar, wxEvtHandler)
END_EVENT_TABLE()

MainMenuBar::MainMenuBar(MainFrame *frame) :
	frame(frame),
	checking_programmaticly(false)
{
	frame->SetMenuBar(this);

#define MAKE_ACTION(id, kind, handler)										\
	do{																		\
		actions[#id] = MenuBarAction{MENUBAR_ ## id, kind};					\
		frame->Connect(MENUBAR_ ## id, wxEVT_COMMAND_MENU_SELECTED,			\
				wxCommandEventHandler(MainMenuBar::handler), NULL, this);	\
	}while(0)

	MAKE_ACTION(NEW, wxITEM_NORMAL, OnNew);
	MAKE_ACTION(OPEN, wxITEM_NORMAL, OnOpen);
	MAKE_ACTION(SAVE, wxITEM_NORMAL, OnSave);
	MAKE_ACTION(SAVE_AS, wxITEM_NORMAL, OnSaveAs);
	MAKE_ACTION(CLOSE, wxITEM_NORMAL, OnClose);

	MAKE_ACTION(EXPORT_PATCH, wxITEM_NORMAL, OnExportPatch);
	MAKE_ACTION(IMPORT_MAP, wxITEM_NORMAL, OnImportMap);
	MAKE_ACTION(EXPORT_MINIMAP, wxITEM_NORMAL, OnExportMinimap);

	MAKE_ACTION(RELOAD_PROJECT, wxITEM_NORMAL, OnReloadProject);
	MAKE_ACTION(PREFERENCES, wxITEM_NORMAL, OnPreferences);
	MAKE_ACTION(EXIT, wxITEM_NORMAL, OnQuit);

	MAKE_ACTION(UNDO, wxITEM_NORMAL, OnUndo);
	MAKE_ACTION(REDO, wxITEM_NORMAL, OnRedo);

	MAKE_ACTION(FIND_ITEM, wxITEM_NORMAL, OnSearchForItem);
	MAKE_ACTION(REPLACE_ITEMS, wxITEM_NORMAL, OnReplaceItems);
	MAKE_ACTION(SEARCH_ON_MAP_EVERYTHING, wxITEM_NORMAL, OnSearchForStuffOnMap);
	MAKE_ACTION(SEARCH_ON_MAP_CONTAINER, wxITEM_NORMAL, OnSearchForContainerOnMap);
	MAKE_ACTION(SEARCH_ON_MAP_WRITEABLE, wxITEM_NORMAL, OnSearchForWritableOnMap);
	MAKE_ACTION(SEARCH_ON_MAP_DUPLICATED_ITEMS, wxITEM_NORMAL, OnSearchForDuplicatedItemsOnMap);
	MAKE_ACTION(SEARCH_ON_SELECTION_EVERYTHING, wxITEM_NORMAL, OnSearchForStuffOnSelection);
	MAKE_ACTION(SEARCH_ON_SELECTION_CONTAINER, wxITEM_NORMAL, OnSearchForContainerOnSelection);
	MAKE_ACTION(SEARCH_ON_SELECTION_WRITEABLE, wxITEM_NORMAL, OnSearchForWritableOnSelection);
	MAKE_ACTION(SEARCH_ON_SELECTION_ITEM, wxITEM_NORMAL, OnSearchForItemOnSelection);
	MAKE_ACTION(SEARCH_ON_SELECTION_DUPLICATED_ITEMS, wxITEM_NORMAL, OnSearchForDuplicatedItemsOnSelection);
	MAKE_ACTION(REPLACE_ON_SELECTION_ITEMS, wxITEM_NORMAL, OnReplaceItemsOnSelection);
	MAKE_ACTION(REMOVE_ON_SELECTION_ITEM, wxITEM_NORMAL, OnRemoveItemOnSelection);
	MAKE_ACTION(SELECT_MODE_COMPENSATE, wxITEM_RADIO, OnSelectionTypeChange);
	MAKE_ACTION(SELECT_MODE_LOWER, wxITEM_RADIO, OnSelectionTypeChange);
	MAKE_ACTION(SELECT_MODE_CURRENT, wxITEM_RADIO, OnSelectionTypeChange);
	MAKE_ACTION(SELECT_MODE_VISIBLE, wxITEM_RADIO, OnSelectionTypeChange);

	MAKE_ACTION(AUTOMAGIC, wxITEM_CHECK, OnToggleAutomagic);
	MAKE_ACTION(BORDERIZE_SELECTION, wxITEM_NORMAL, OnBorderizeSelection);
	MAKE_ACTION(BORDERIZE_MAP, wxITEM_NORMAL, OnBorderizeMap);
	MAKE_ACTION(RANDOMIZE_SELECTION, wxITEM_NORMAL, OnRandomizeSelection);
	MAKE_ACTION(RANDOMIZE_MAP, wxITEM_NORMAL, OnRandomizeMap);
	MAKE_ACTION(GOTO_PREVIOUS_POSITION, wxITEM_NORMAL, OnGotoPreviousPosition);
	MAKE_ACTION(GOTO_POSITION, wxITEM_NORMAL, OnGotoPosition);
	MAKE_ACTION(JUMP_TO_BRUSH, wxITEM_NORMAL, OnJumpToBrush);
	MAKE_ACTION(JUMP_TO_ITEM_BRUSH, wxITEM_NORMAL, OnJumpToItemBrush);

	MAKE_ACTION(CUT, wxITEM_NORMAL, OnCut);
	MAKE_ACTION(COPY, wxITEM_NORMAL, OnCopy);
	MAKE_ACTION(PASTE, wxITEM_NORMAL, OnPaste);

	MAKE_ACTION(EDIT_TOWNS, wxITEM_NORMAL, OnMapEditTowns);
	MAKE_ACTION(EDIT_ITEMS, wxITEM_NORMAL, OnMapEditItems);
	MAKE_ACTION(EDIT_MONSTERS, wxITEM_NORMAL, OnMapEditMonsters);

	MAKE_ACTION(CLEAR_INVALID_HOUSES, wxITEM_NORMAL, OnClearHouseTiles);
	MAKE_ACTION(MAP_REMOVE_ITEMS, wxITEM_NORMAL, OnMapRemoveItems);
	MAKE_ACTION(MAP_REMOVE_CORPSES, wxITEM_NORMAL, OnMapRemoveCorpses);
	MAKE_ACTION(MAP_REMOVE_UNREACHABLE_TILES, wxITEM_NORMAL, OnMapRemoveUnreachable);
	MAKE_ACTION(MAP_CLEANUP, wxITEM_NORMAL, OnMapCleanup);
	MAKE_ACTION(MAP_CLEAN_HOUSE_ITEMS, wxITEM_NORMAL, OnMapCleanHouseItems);
	MAKE_ACTION(MAP_STATISTICS, wxITEM_NORMAL, OnMapStatistics);

	MAKE_ACTION(VIEW_TOOLBARS_BRUSHES, wxITEM_CHECK, OnToolbars);
	MAKE_ACTION(VIEW_TOOLBARS_POSITION, wxITEM_CHECK, OnToolbars);
	MAKE_ACTION(VIEW_TOOLBARS_SIZES, wxITEM_CHECK, OnToolbars);
	MAKE_ACTION(VIEW_TOOLBARS_INDICATORS, wxITEM_CHECK, OnToolbars);
	MAKE_ACTION(VIEW_TOOLBARS_STANDARD, wxITEM_CHECK, OnToolbars);
	MAKE_ACTION(TOGGLE_FULLSCREEN, wxITEM_NORMAL, OnToggleFullscreen);

	MAKE_ACTION(ZOOM_IN, wxITEM_NORMAL, OnZoomIn);
	MAKE_ACTION(ZOOM_OUT, wxITEM_NORMAL, OnZoomOut);
	MAKE_ACTION(ZOOM_NORMAL, wxITEM_NORMAL, OnZoomNormal);

	MAKE_ACTION(SHOW_SHADE, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_ALL_FLOORS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(GHOST_ITEMS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(GHOST_HIGHER_FLOORS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(HIGHLIGHT_ITEMS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_EXTRA, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_INGAME_BOX, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_LIGHTS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_GRID, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_CREATURES, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_SPECIAL, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_AS_MINIMAP, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_ONLY_COLORS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_ONLY_MODIFIED, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_HOUSES, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_PATHING, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_TOOLTIPS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_PREVIEW, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_WALL_HOOKS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_PICKUPABLES, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_MOVEABLES, wxITEM_CHECK, OnChangeViewSettings);

	MAKE_ACTION(WIN_PROBLEMS, wxITEM_NORMAL, OnProblemsWindow);
	MAKE_ACTION(WIN_MINIMAP, wxITEM_NORMAL, OnMinimapWindow);
	MAKE_ACTION(WIN_ACTIONS_HISTORY, wxITEM_NORMAL, OnActionsHistoryWindow);
	MAKE_ACTION(NEW_PALETTE, wxITEM_NORMAL, OnNewPalette);
	MAKE_ACTION(TAKE_SCREENSHOT, wxITEM_NORMAL, OnTakeScreenshot);

	MAKE_ACTION(SELECT_TERRAIN, wxITEM_NORMAL, OnSelectTerrainPalette);
	MAKE_ACTION(SELECT_DOODAD, wxITEM_NORMAL, OnSelectDoodadPalette);
	MAKE_ACTION(SELECT_ITEM, wxITEM_NORMAL, OnSelectItemPalette);
	MAKE_ACTION(SELECT_CREATURE, wxITEM_NORMAL, OnSelectCreaturePalette);
	MAKE_ACTION(SELECT_HOUSE, wxITEM_NORMAL, OnSelectHousePalette);
	MAKE_ACTION(SELECT_WAYPOINT, wxITEM_NORMAL, OnSelectWaypointPalette);
	MAKE_ACTION(SELECT_RAW, wxITEM_NORMAL, OnSelectRawPalette);

	MAKE_ACTION(FLOOR_0, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_1, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_2, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_3, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_4, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_5, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_6, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_7, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_8, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_9, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_10, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_11, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_12, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_13, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_14, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_15, wxITEM_RADIO, OnChangeFloor);

	MAKE_ACTION(DEBUG_VIEW_DAT, wxITEM_NORMAL, OnDebugViewDat);
	MAKE_ACTION(GOTO_WEBSITE, wxITEM_NORMAL, OnGotoWebsite);
	MAKE_ACTION(ABOUT, wxITEM_NORMAL, OnAbout);

#undef MAKE_ACTION

	for(size_t i = 0; i < 10; ++i) {
		frame->Connect(g_editor.recentFiles.GetBaseId() + i, wxEVT_COMMAND_MENU_SELECTED,
				wxCommandEventHandler(MainMenuBar::OnOpenRecent), nullptr, this);
	}
}

MainMenuBar::~MainMenuBar()
{
	// no-op
}

namespace OnMapRemoveItems
{
	struct RemoveItemCondition
	{
		double nextUpdate = 0.0;
		int itemId = 0;
		bool selectedOnly = false;

		bool operator()(const Item* item, double progress) {
			if(progress >= nextUpdate){
				g_editor.SetLoadDone((int)(progress * 100.0));
				nextUpdate = progress + 0.01;
			}

			return item->getID() == itemId
				&& (!selectedOnly || item->isSelected());
		}
	};
}

void MainMenuBar::EnableItem(int id, bool enable)
{
	auto it = menuItems.find(id);
	if(it != menuItems.end()){
		for(wxMenuItem *menuItem: it->second){
			menuItem->Enable(enable);
		}
	}
}

void MainMenuBar::CheckItem(int id, bool enable)
{
	auto it = menuItems.find(id);
	if(it != menuItems.end()){
		checking_programmaticly = true;
		for(wxMenuItem *menuItem: it->second){
			menuItem->Check(enable);
		}
		checking_programmaticly = false;
	}
}

bool MainMenuBar::IsItemChecked(int id) const
{
	auto it = menuItems.find(id);
	if(it != menuItems.end()){
		for(wxMenuItem *menuItem: it->second){
			if(menuItem->IsChecked()){
				return true;
			}
		}
	}

	return false;
}

void MainMenuBar::Update()
{
	// This updates all buttons and sets them to proper enabled/disabled state
	bool enable = !g_editor.IsWelcomeDialogShown();
	Enable(enable);
    if(!enable) {
        return;
	}

	bool loaded = g_editor.IsProjectOpen();
	bool dirty  = g_editor.IsProjectDirty();
	bool has_selection = g_editor.hasSelection();

	EnableItem(MENUBAR_UNDO, g_editor.canUndo());
	EnableItem(MENUBAR_REDO, g_editor.canRedo());
	EnableItem(MENUBAR_PASTE, g_editor.copybuffer.canPaste());

	EnableItem(MENUBAR_CLOSE, loaded);
	EnableItem(MENUBAR_SAVE, loaded && dirty);
	EnableItem(MENUBAR_SAVE_AS, loaded && dirty);

	EnableItem(MENUBAR_EXPORT_PATCH, loaded);
	EnableItem(MENUBAR_IMPORT_MAP, loaded);
	EnableItem(MENUBAR_EXPORT_MINIMAP, loaded);

	EnableItem(MENUBAR_FIND_ITEM, loaded);
	EnableItem(MENUBAR_REPLACE_ITEMS, loaded);
	EnableItem(MENUBAR_SEARCH_ON_MAP_EVERYTHING, loaded);
	EnableItem(MENUBAR_SEARCH_ON_MAP_CONTAINER, loaded);
	EnableItem(MENUBAR_SEARCH_ON_MAP_WRITEABLE, loaded);
	EnableItem(MENUBAR_SEARCH_ON_MAP_DUPLICATED_ITEMS, loaded);
	EnableItem(MENUBAR_SEARCH_ON_SELECTION_EVERYTHING, has_selection && loaded);
	EnableItem(MENUBAR_SEARCH_ON_SELECTION_CONTAINER, has_selection && loaded);
	EnableItem(MENUBAR_SEARCH_ON_SELECTION_WRITEABLE, has_selection && loaded);
	EnableItem(MENUBAR_SEARCH_ON_SELECTION_DUPLICATED_ITEMS, has_selection && loaded);
	EnableItem(MENUBAR_SEARCH_ON_SELECTION_ITEM, has_selection && loaded);
	EnableItem(MENUBAR_REPLACE_ON_SELECTION_ITEMS, has_selection && loaded);
	EnableItem(MENUBAR_REMOVE_ON_SELECTION_ITEM, has_selection && loaded);

	EnableItem(MENUBAR_CUT, loaded);
	EnableItem(MENUBAR_COPY, loaded);

	EnableItem(MENUBAR_BORDERIZE_SELECTION, loaded && has_selection);
	EnableItem(MENUBAR_BORDERIZE_MAP, loaded);
	EnableItem(MENUBAR_RANDOMIZE_SELECTION, loaded && has_selection);
	EnableItem(MENUBAR_RANDOMIZE_MAP, loaded);

	EnableItem(MENUBAR_GOTO_PREVIOUS_POSITION, loaded);
	EnableItem(MENUBAR_GOTO_POSITION, loaded);
	EnableItem(MENUBAR_JUMP_TO_BRUSH, loaded);
	EnableItem(MENUBAR_JUMP_TO_ITEM_BRUSH, loaded);

	EnableItem(MENUBAR_MAP_REMOVE_ITEMS, loaded);
	EnableItem(MENUBAR_MAP_REMOVE_CORPSES, loaded);
	EnableItem(MENUBAR_MAP_REMOVE_UNREACHABLE_TILES, loaded);
	EnableItem(MENUBAR_CLEAR_INVALID_HOUSES, loaded);
	EnableItem(MENUBAR_CLEAR_MODIFIED_STATE, loaded);

	EnableItem(MENUBAR_EDIT_TOWNS, loaded);
	EnableItem(MENUBAR_EDIT_ITEMS, false);
	EnableItem(MENUBAR_EDIT_MONSTERS, false);

	EnableItem(MENUBAR_MAP_CLEANUP, loaded);
	EnableItem(MENUBAR_MAP_STATISTICS, loaded);

	EnableItem(MENUBAR_ZOOM_IN, loaded);
	EnableItem(MENUBAR_ZOOM_OUT, loaded);
	EnableItem(MENUBAR_ZOOM_NORMAL, loaded);

	EnableItem(MENUBAR_WIN_PROBLEMS, loaded);
	EnableItem(MENUBAR_WIN_MINIMAP, loaded);
	EnableItem(MENUBAR_NEW_PALETTE, loaded);
	EnableItem(MENUBAR_SELECT_TERRAIN, loaded);
	EnableItem(MENUBAR_SELECT_DOODAD, loaded);
	EnableItem(MENUBAR_SELECT_ITEM, loaded);
	EnableItem(MENUBAR_SELECT_HOUSE, loaded);
	EnableItem(MENUBAR_SELECT_CREATURE, loaded);
	EnableItem(MENUBAR_SELECT_WAYPOINT, loaded);
	EnableItem(MENUBAR_SELECT_RAW, loaded);

	EnableItem(MENUBAR_DEBUG_VIEW_DAT, loaded);

	UpdateFloorMenu();
	UpdateIndicatorsMenu();
}

void MainMenuBar::LoadValues()
{
	CheckItem(MENUBAR_VIEW_TOOLBARS_BRUSHES, g_settings.getBoolean(Config::SHOW_TOOLBAR_BRUSHES));
	CheckItem(MENUBAR_VIEW_TOOLBARS_POSITION, g_settings.getBoolean(Config::SHOW_TOOLBAR_POSITION));
	CheckItem(MENUBAR_VIEW_TOOLBARS_SIZES, g_settings.getBoolean(Config::SHOW_TOOLBAR_SIZES));
	CheckItem(MENUBAR_VIEW_TOOLBARS_INDICATORS, g_settings.getBoolean(Config::SHOW_TOOLBAR_INDICATORS));
	CheckItem(MENUBAR_VIEW_TOOLBARS_STANDARD, g_settings.getBoolean(Config::SHOW_TOOLBAR_STANDARD));

	CheckItem(MENUBAR_SELECT_MODE_COMPENSATE, g_settings.getBoolean(Config::COMPENSATED_SELECT));

	if(IsItemChecked(MENUBAR_SELECT_MODE_CURRENT)){
		g_settings.setInteger(Config::SELECTION_TYPE, SELECT_CURRENT_FLOOR);
	}else if(IsItemChecked(MENUBAR_SELECT_MODE_LOWER)){
		g_settings.setInteger(Config::SELECTION_TYPE, SELECT_ALL_FLOORS);
	}else if(IsItemChecked(MENUBAR_SELECT_MODE_VISIBLE)){
		g_settings.setInteger(Config::SELECTION_TYPE, SELECT_VISIBLE_FLOORS);
	}

	switch(g_settings.getInteger(Config::SELECTION_TYPE)) {
		case SELECT_CURRENT_FLOOR:
			CheckItem(MENUBAR_SELECT_MODE_CURRENT, true);
			break;
		case SELECT_ALL_FLOORS:
			CheckItem(MENUBAR_SELECT_MODE_LOWER, true);
			break;
		default:
		case SELECT_VISIBLE_FLOORS:
			CheckItem(MENUBAR_SELECT_MODE_VISIBLE, true);
			break;
	}

	CheckItem(MENUBAR_AUTOMAGIC, g_settings.getBoolean(Config::USE_AUTOMAGIC));

	CheckItem(MENUBAR_SHOW_SHADE, g_settings.getBoolean(Config::SHOW_SHADE));
	CheckItem(MENUBAR_SHOW_INGAME_BOX, g_settings.getBoolean(Config::SHOW_INGAME_BOX));
	CheckItem(MENUBAR_SHOW_LIGHTS, g_settings.getBoolean(Config::SHOW_LIGHTS));
	CheckItem(MENUBAR_SHOW_ALL_FLOORS, g_settings.getBoolean(Config::SHOW_ALL_FLOORS));
	CheckItem(MENUBAR_GHOST_ITEMS, g_settings.getBoolean(Config::TRANSPARENT_ITEMS));
	CheckItem(MENUBAR_GHOST_HIGHER_FLOORS, g_settings.getBoolean(Config::TRANSPARENT_FLOORS));
	CheckItem(MENUBAR_SHOW_EXTRA, !g_settings.getBoolean(Config::SHOW_EXTRA));
	CheckItem(MENUBAR_SHOW_GRID, g_settings.getBoolean(Config::SHOW_GRID));
	CheckItem(MENUBAR_HIGHLIGHT_ITEMS, g_settings.getBoolean(Config::HIGHLIGHT_ITEMS));
	CheckItem(MENUBAR_SHOW_CREATURES, g_settings.getBoolean(Config::SHOW_CREATURES));
	CheckItem(MENUBAR_SHOW_SPECIAL, g_settings.getBoolean(Config::SHOW_SPECIAL_TILES));
	CheckItem(MENUBAR_SHOW_AS_MINIMAP, g_settings.getBoolean(Config::SHOW_AS_MINIMAP));
	CheckItem(MENUBAR_SHOW_ONLY_COLORS, g_settings.getBoolean(Config::SHOW_ONLY_TILEFLAGS));
	CheckItem(MENUBAR_SHOW_ONLY_MODIFIED, g_settings.getBoolean(Config::SHOW_ONLY_MODIFIED_TILES));
	CheckItem(MENUBAR_SHOW_HOUSES, g_settings.getBoolean(Config::SHOW_HOUSES));
	CheckItem(MENUBAR_SHOW_PATHING, g_settings.getBoolean(Config::SHOW_BLOCKING));
	CheckItem(MENUBAR_SHOW_TOOLTIPS, g_settings.getBoolean(Config::SHOW_TOOLTIPS));
	CheckItem(MENUBAR_SHOW_PREVIEW, g_settings.getBoolean(Config::SHOW_PREVIEW));
	CheckItem(MENUBAR_SHOW_WALL_HOOKS, g_settings.getBoolean(Config::SHOW_WALL_HOOKS));
	CheckItem(MENUBAR_SHOW_PICKUPABLES, g_settings.getBoolean(Config::SHOW_PICKUPABLES));
	CheckItem(MENUBAR_SHOW_MOVEABLES, g_settings.getBoolean(Config::SHOW_MOVEABLES));
}

void MainMenuBar::UpdateFloorMenu()
{
	if(!g_editor.IsProjectOpen()) {
		return;
	}

	for(int i = 0; i < rme::MapLayers; ++i)
		CheckItem(MENUBAR_FLOOR_0 + i, false);

	CheckItem(MENUBAR_FLOOR_0 + g_editor.GetCurrentFloor(), true);
}

void MainMenuBar::UpdateIndicatorsMenu()
{
	if(!g_editor.IsProjectOpen()) {
		return;
	}

	CheckItem(MENUBAR_SHOW_WALL_HOOKS, g_settings.getBoolean(Config::SHOW_WALL_HOOKS));
	CheckItem(MENUBAR_SHOW_PICKUPABLES, g_settings.getBoolean(Config::SHOW_PICKUPABLES));
	CheckItem(MENUBAR_SHOW_MOVEABLES, g_settings.getBoolean(Config::SHOW_MOVEABLES));
}

void MainMenuBar::LoadDefault(void){
	while(GetMenuCount() > 0){
		Remove(0);
	}

	wxMenu *recentFiles = newd wxMenu;
	g_editor.recentFiles.UseMenu(recentFiles);

	wxMenu *fileMenu = newd wxMenu("File");
	fileMenu->Append(MENUBAR_OPEN,        "Open\tCtrl+O",  "Open project.");
	fileMenu->Append(MENUBAR_SAVE,        "Save\tCtrl+S",  "Save project.");
	fileMenu->Append(MENUBAR_CLOSE,       "Close\tCtrl+Q", "Close project.");
	fileMenu->AppendSeparator();
	fileMenu->AppendSubMenu(recentFiles,                   "Recent Files",  "");
	fileMenu->Append(MENUBAR_PREFERENCES, "Preferences",   "Configure editor.");
	fileMenu->Append(MENUBAR_CLOSE,       "Exit",          "Close editor.");

	Append(fileMenu, "File");

	wxAcceleratorEntry accelerators[] = {
		wxAcceleratorEntry(wxACCEL_CTRL, 'O', MENUBAR_OPEN),
		wxAcceleratorEntry(wxACCEL_CTRL, 'S', MENUBAR_SAVE),
		wxAcceleratorEntry(wxACCEL_CTRL, 'Q', MENUBAR_CLOSE),
	};

	frame->SetAcceleratorTable(wxAcceleratorTable(
			NARRAY(accelerators), accelerators));

	g_editor.recentFiles.AddFilesToMenu();
	Update();
	LoadValues();
}

bool MainMenuBar::Load(const wxString &projectDir)
{
	wxString filename = ConcatPath(projectDir, "editor", "menubar.xml");
	if(!wxFileName::Exists(filename)){
		g_editor.Error("Unable to locate menubar.xml");
		return false;
	}

	// Open the XML file
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(filename.mb_str());
	if(!result) {
		g_editor.Error(wxString() << "Unable to open " << filename << " for reading.");
		return false;
	}

	pugi::xml_node node = doc.child("menubar");
	if(!node) {
		g_editor.Error(wxString() << "Menu file " << filename << "is missing top-level menubar node.");
		return false;
	}

	// Clear the menu
	while(GetMenuCount() > 0){
		Remove(0);
	}

	// Load succeded
	std::vector<wxAcceleratorEntry> accelerators;
	for(pugi::xml_node menuNode: node.children()){
		// For each child node, load it
		wxObject *i = LoadItem(menuNode, nullptr, accelerators);
		if (wxMenu *m = dynamic_cast<wxMenu*>(i)) {
			Append(m, m->GetTitle());
#ifdef __APPLE__
			m->SetTitle(m->GetTitle());
#else
			m->SetTitle("");
#endif
		}else if(i){
			delete i;
			g_editor.Warning(wxString() << filename << ": Only menus can be subitems of main menu");
		}
	}

	frame->SetAcceleratorTable(wxAcceleratorTable(
			(int)accelerators.size(), accelerators.data()));

	g_editor.recentFiles.AddFilesToMenu();
	Update();
	LoadValues();
	return true;
}

wxObject *MainMenuBar::LoadItem(pugi::xml_node node, wxMenu *parent,
		std::vector<wxAcceleratorEntry> &accelerators){
	std::string_view nodeTag = node.name();
	if(nodeTag == "menu") {
		if(!node.attribute("name")){
			return nullptr;
		}

		std::string name = node.attribute("name").as_string();
		std::replace(name.begin(), name.end(), '$', '&');

		wxMenu* menu = newd wxMenu;
		if(std::string_view(node.attribute("special").as_string()) == "RECENT_FILES") {
			g_editor.recentFiles.UseMenu(menu);
		} else {
			for(pugi::xml_node menuNode: node.children()){
				LoadItem(menuNode, menu, accelerators);
			}
		}

		// If we have a parent, add ourselves.
		// If not, we just return the item and the parent function
		// is responsible for adding us to wherever
		if(parent) {
			parent->AppendSubMenu(menu, wxstr(name));
		} else {
			menu->SetTitle(name);
		}

		return menu;
	} else if(nodeTag == "item") {
		// We must have a parent when loading items
		if(!parent || !node.attribute("name")) {
			return nullptr;
		}

		std::string name = node.attribute("name").as_string();
		std::replace(name.begin(), name.end(), '$', '&');

		std::string action = node.attribute("action").as_string();
		if(action.empty()){
			return nullptr;
		}

		auto it = actions.find(action);
		if(it == actions.end()) {
			g_editor.Warning(wxString() << "Invalid action type '" << action << "'.");
			return nullptr;
		}

		std::string_view hotkey = node.attribute("hotkey").as_string();
		if(!hotkey.empty()){
			name += "\t";
			name += hotkey;

			wxAcceleratorEntry entry;
			if(entry.FromString(name)){
				accelerators.push_back(wxAcceleratorEntry(
						entry.GetFlags(), entry.GetKeyCode(),
						it->second.id));
			}else{
				g_editor.Warning(wxString() << "Invalid hotkey for item '" << name << "'.");
			}
		}

		wxMenuItem *menuItem = parent->Append(
			it->second.id,						// ID
			name,								// Title of button
			node.attribute("help").as_string(), // Help text
			it->second.kind						// Kind of item
		);

		menuItems[it->second.id].push_back(menuItem);
		return menuItem;
	} else if(nodeTag == "separator") {
		// We must have a parent when loading items
		if(!parent) {
			return nullptr;
		}
		return parent->AppendSeparator();
	}
	return nullptr;
}

void MainMenuBar::OnNew(wxCommandEvent& WXUNUSED(event))
{
	g_editor.NewProject();
}

void MainMenuBar::OnOpenRecent(wxCommandEvent& event)
{
	const wxFileHistory &recentFiles = g_editor.recentFiles;
	g_editor.OpenProject(recentFiles.GetHistoryFile(event.GetId() - recentFiles.GetBaseId()));
}

void MainMenuBar::OnOpen(wxCommandEvent& WXUNUSED(event))
{
	g_editor.OpenProject();
}

void MainMenuBar::OnClose(wxCommandEvent& WXUNUSED(event))
{
	g_editor.CloseProject();
}

void MainMenuBar::OnSave(wxCommandEvent& WXUNUSED(event))
{
	g_editor.SaveProject();
}

void MainMenuBar::OnSaveAs(wxCommandEvent& WXUNUSED(event))
{
	g_editor.SaveProjectAs();
}

void MainMenuBar::OnPreferences(wxCommandEvent& WXUNUSED(event))
{
	PreferencesWindow dialog(frame);
	dialog.ShowModal();
	dialog.Destroy();
}

void MainMenuBar::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	g_editor.root->Close();
}

void MainMenuBar::OnExportPatch(wxCommandEvent &WXUNUSED(event))
{
	if(!g_editor.IsProjectOpen()){
		return;
	}

	wxString defaultPath = ConcatPath(g_editor.projectDir,
			wxDateTime().Now().Format("patch-%Y-%m-%d-%H%M%S.zip"));
	ExportPatchDialog dialog(frame, defaultPath);
	if(dialog.ShowModal() == wxID_OK){
		wxFileName filename = dialog.GetFileName();
		bool       commit   = dialog.GetCommitPatch();
		if(!g_editor.ExportPatch(filename.GetFullPath(), commit)){
			// TODO(fusion): We could add a "retry" loop here but is it even a good idea?
			g_editor.PopupDialog("Error", "There was an error while exporting patch.", wxOK);
		}
	}
}

void MainMenuBar::OnImportMap(wxCommandEvent& WXUNUSED(event))
{
	if(!g_editor.IsProjectOpen()){
		return;
	}

	ImportMapWindow dialog(frame);
	dialog.ShowModal();
}

void MainMenuBar::OnExportMinimap(wxCommandEvent& WXUNUSED(event))
{
	if(!g_editor.IsProjectOpen()) {
		return;
	}

	ExportMiniMapWindow dialog(frame);
	dialog.ShowModal();
}

void MainMenuBar::OnDebugViewDat(wxCommandEvent& WXUNUSED(event))
{
	wxDialog dlg(frame, wxID_ANY, "Debug .dat file", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
	new DatDebugView(&dlg);
	dlg.ShowModal();
}

void MainMenuBar::OnReloadProject(wxCommandEvent& WXUNUSED(event))
{
	g_editor.ReloadProject();
}

void MainMenuBar::OnGotoWebsite(wxCommandEvent& WXUNUSED(event))
{
	::wxLaunchDefaultBrowser(__RME_WEBSITE_URL__,  wxBROWSER_NEW_WINDOW);
}

void MainMenuBar::OnAbout(wxCommandEvent& WXUNUSED(event))
{
	AboutWindow about(frame);
	about.ShowModal();
}

void MainMenuBar::OnUndo(wxCommandEvent& WXUNUSED(event))
{
	g_editor.DoUndo();
}

void MainMenuBar::OnRedo(wxCommandEvent& WXUNUSED(event))
{
	g_editor.DoRedo();
}

namespace OnSearchForItem
{
	struct Finder
	{
		double nextUpdate = 0.0;
		int itemId = 0;
		int maxCount = 0;
		bool selectedOnly = false;
		std::vector<std::pair<Tile*, Item*>> results;

		bool limitReached() const { return results.size() >= (size_t)maxCount; }
		bool operator()(Tile* tile, Item* item, double progress)
		{
			if(progress >= nextUpdate){
				g_editor.SetLoadDone((int)(progress * 100.0));
				nextUpdate = progress + 0.01;
			}

			if(results.size() >= (size_t)maxCount)
				return false;

			if(selectedOnly && !item->isSelected()){
				return false;
			}

			if(item->getID() == itemId)
				results.push_back(std::make_pair(tile, item));

			return true;
		}
	};
}

void MainMenuBar::OnSearchForItem(wxCommandEvent& WXUNUSED(event))
{
	if(!g_editor.IsProjectOpen())
		return;

	FindItemDialog dialog(frame, "Search for Item");
	dialog.setSearchMode((FindItemDialog::SearchMode)g_settings.getInteger(Config::FIND_ITEM_MODE));
	if(dialog.ShowModal() == wxID_OK) {
		OnSearchForItem::Finder finder;
		finder.itemId = dialog.getResultID();
		finder.maxCount = g_settings.getInteger(Config::REPLACE_SIZE);
		finder.selectedOnly = false;

		g_editor.CreateLoadBar("Searching map...");
		g_editor.map.forEachItem(finder);
		g_editor.DestroyLoadBar();

		if(finder.limitReached()) {
			wxString msg;
			msg << "The configured limit has been reached. Only " << finder.maxCount << " results will be displayed.";
			g_editor.PopupDialog("Notice", msg, wxOK);
		}

		SearchResultWindow* window = g_editor.ShowSearchWindow();
		window->Clear();
		for(const auto &[tile, item]: finder.results){
			window->AddPosition(wxstr(item->getName()), tile->pos);
		}

		g_settings.setInteger(Config::FIND_ITEM_MODE, (int)dialog.getSearchMode());
	}
	dialog.Destroy();
}

void MainMenuBar::OnReplaceItems(wxCommandEvent& WXUNUSED(event))
{
	if(!g_editor.IsProjectOpen())
		return;

	g_editor.mapWindow->ShowReplaceItemsDialog(false);
}

namespace OnSearchForStuff
{
	struct Searcher
	{
		// TODO(fusion): Relevant srv flags/attributes instead?
		double nextUpdate = 0.0;
		bool searchContainer = false;
		bool searchWritable = false;
		bool selectedOnly = false;
		std::vector<std::pair<Tile*, Item*>> results;

		bool operator()(Tile *tile, Item *item, double progress)
		{
			if(progress >= nextUpdate){
				g_editor.SetLoadDone((int)(progress * 100.0));
				nextUpdate = progress + 0.01;
			}

			if(selectedOnly && !item->isSelected()){
				return false;
			}

			bool add = false;
			if(!add && searchContainer){
				if((item->getFlag(CONTAINER) || item->getFlag(CHEST)) && item->content != NULL){
					add = true;
				}
			}

			if(!add && searchWritable){
				if(item->getFlag(TEXT)){
					const char *text = item->getTextAttribute(TEXTSTRING);
					if(text && strlen(text) > 0){
						add = true;
					}
				}
			}

			if(add){
				results.push_back(std::make_pair(tile, item));
			}

			return true;
		}

		wxString desc(Item* item)
		{
			wxString label = wxstr(item->getName());

			if(item->getFlag(CONTAINER) || item->getFlag(CHEST)){
				label << " (Container) ";
			}

			if(item->getFlag(TEXT)){
				const char *text = item->getTextAttribute(TEXTSTRING);
				if(text && strlen(text) > 0){
					label << " (Text: " << text << ") ";
				}
			}

			return label;
		}

		void sort()
		{
			// TODO ?
		}
	};
}

void MainMenuBar::OnSearchForStuffOnMap(wxCommandEvent& WXUNUSED(event))
{
	SearchItems(true, true);
}

void MainMenuBar::OnSearchForContainerOnMap(wxCommandEvent& WXUNUSED(event))
{
	SearchItems(true, false);
}

void MainMenuBar::OnSearchForWritableOnMap(wxCommandEvent& WXUNUSED(event))
{
	SearchItems(false, true);
}

void MainMenuBar::OnSearchForDuplicatedItemsOnMap(wxCommandEvent& WXUNUSED(event))
{
	SearchDuplicatedItems(false);
}

void MainMenuBar::OnSearchForStuffOnSelection(wxCommandEvent& WXUNUSED(event))
{
	SearchItems(true, true, true);
}

void MainMenuBar::OnSearchForContainerOnSelection(wxCommandEvent& WXUNUSED(event))
{
	SearchItems(true, false, true);
}

void MainMenuBar::OnSearchForWritableOnSelection(wxCommandEvent& WXUNUSED(event))
{
	SearchItems(false, true, true);
}

void MainMenuBar::OnSearchForItemOnSelection(wxCommandEvent& WXUNUSED(event))
{
	if(!g_editor.IsProjectOpen())
		return;

	FindItemDialog dialog(frame, "Search on Selection");
	dialog.setSearchMode((FindItemDialog::SearchMode)g_settings.getInteger(Config::FIND_ITEM_MODE));
	if(dialog.ShowModal() == wxID_OK) {
		OnSearchForItem::Finder finder;
		finder.itemId = dialog.getResultID();
		finder.maxCount = g_settings.getInteger(Config::REPLACE_SIZE);
		finder.selectedOnly = true;

		g_editor.CreateLoadBar("Searching on selected area...");
		g_editor.map.forEachItem(finder);
		g_editor.DestroyLoadBar();

		if(finder.limitReached()) {
			wxString msg;
			msg << "The configured limit has been reached. Only " << finder.maxCount << " results will be displayed.";
			g_editor.PopupDialog("Notice", msg, wxOK);
		}

		SearchResultWindow* window = g_editor.ShowSearchWindow();
		window->Clear();
		for(const auto &[tile, item]: finder.results){
			window->AddPosition(wxstr(item->getName()), tile->pos);
		}

		g_settings.setInteger(Config::FIND_ITEM_MODE, (int)dialog.getSearchMode());
	}

	dialog.Destroy();
}

void MainMenuBar::OnSearchForDuplicatedItemsOnSelection(wxCommandEvent& WXUNUSED(event))
{
	SearchDuplicatedItems(true);
}

void MainMenuBar::OnReplaceItemsOnSelection(wxCommandEvent& WXUNUSED(event))
{
	if(!g_editor.IsProjectOpen())
		return;

	g_editor.mapWindow->ShowReplaceItemsDialog(true);
}

void MainMenuBar::OnRemoveItemOnSelection(wxCommandEvent& WXUNUSED(event))
{
	if(!g_editor.IsProjectOpen())
		return;

	FindItemDialog dialog(frame, "Remove Item on Selection");
	if(dialog.ShowModal() == wxID_OK) {
		OnMapRemoveItems::RemoveItemCondition condition;
		condition.itemId = dialog.getResultID();
		condition.selectedOnly = true;

		g_editor.CreateLoadBar("Searching item on selection to remove...");
		int count = g_editor.map.removeItems(condition);
		g_editor.DestroyLoadBar();

		wxString msg;
		msg << count << " items removed.";
		g_editor.PopupDialog("Remove Item", msg, wxOK);
		g_editor.RefreshView();
	}
	dialog.Destroy();
}

void MainMenuBar::OnSelectionTypeChange(wxCommandEvent& WXUNUSED(event))
{
	g_settings.setInteger(Config::COMPENSATED_SELECT, IsItemChecked(MENUBAR_SELECT_MODE_COMPENSATE));

	if(IsItemChecked(MENUBAR_SELECT_MODE_CURRENT)){
		g_settings.setInteger(Config::SELECTION_TYPE, SELECT_CURRENT_FLOOR);
	}else if(IsItemChecked(MENUBAR_SELECT_MODE_LOWER)){
		g_settings.setInteger(Config::SELECTION_TYPE, SELECT_ALL_FLOORS);
	}else if(IsItemChecked(MENUBAR_SELECT_MODE_VISIBLE)){
		g_settings.setInteger(Config::SELECTION_TYPE, SELECT_VISIBLE_FLOORS);
	}
}

void MainMenuBar::OnCopy(wxCommandEvent& WXUNUSED(event))
{
	g_editor.DoCopy();
}

void MainMenuBar::OnCut(wxCommandEvent& WXUNUSED(event))
{
	g_editor.DoCut();
}

void MainMenuBar::OnPaste(wxCommandEvent& WXUNUSED(event))
{
	g_editor.PreparePaste();
}

void MainMenuBar::OnToggleAutomagic(wxCommandEvent& WXUNUSED(event))
{
	g_settings.setInteger(Config::USE_AUTOMAGIC, IsItemChecked(MENUBAR_AUTOMAGIC));
	g_settings.setInteger(Config::BORDER_IS_GROUND, IsItemChecked(MENUBAR_AUTOMAGIC));
	if(g_settings.getInteger(Config::USE_AUTOMAGIC))
		g_editor.SetStatusText("Automagic enabled.");
	else
		g_editor.SetStatusText("Automagic disabled.");
}

void MainMenuBar::OnBorderizeSelection(wxCommandEvent& WXUNUSED(event))
{
	if(!g_editor.IsProjectOpen())
		return;

	g_editor.borderizeSelection();
	g_editor.RefreshView();
}

void MainMenuBar::OnBorderizeMap(wxCommandEvent& WXUNUSED(event))
{
	if(!g_editor.IsProjectOpen())
		return;

	int ret = g_editor.PopupDialog("Borderize Map", "Are you sure you want to borderize the entire map (this action cannot be undone)?", wxYES | wxNO);
	if(ret == wxID_YES)
		g_editor.borderizeMap(true);

	g_editor.RefreshView();
}

void MainMenuBar::OnRandomizeSelection(wxCommandEvent& WXUNUSED(event))
{
	if(!g_editor.IsProjectOpen())
		return;

	g_editor.randomizeSelection();
	g_editor.RefreshView();
}

void MainMenuBar::OnRandomizeMap(wxCommandEvent& WXUNUSED(event))
{
	if(!g_editor.IsProjectOpen())
		return;

	int ret = g_editor.PopupDialog("Randomize Map", "Are you sure you want to randomize the entire map (this action cannot be undone)?", wxYES | wxNO);
	if(ret == wxID_YES)
		g_editor.randomizeMap(true);

	g_editor.RefreshView();
}

void MainMenuBar::OnJumpToBrush(wxCommandEvent& WXUNUSED(event))
{
	if(!g_editor.IsProjectOpen())
		return;

	FindBrushDialog dlg(frame);
	dlg.ShowModal();

	const Brush* brush = dlg.getResult();
	if(brush) {
		g_editor.SelectBrush(brush, TILESET_UNKNOWN);
	}
}

void MainMenuBar::OnJumpToItemBrush(wxCommandEvent& WXUNUSED(event))
{
	if(!g_editor.IsProjectOpen())
		return;

	FindItemDialog dialog(frame, "Jump to Item");
	dialog.setSearchMode((FindItemDialog::SearchMode)g_settings.getInteger(Config::JUMP_TO_ITEM_MODE));
	if(dialog.ShowModal() == wxID_OK) {
		// Retrieve result, if null user canceled
		const Brush* brush = dialog.getResult();
		if(brush)
			g_editor.SelectBrush(brush, TILESET_RAW);
		g_settings.setInteger(Config::JUMP_TO_ITEM_MODE, (int)dialog.getSearchMode());
	}
}

void MainMenuBar::OnGotoPreviousPosition(wxCommandEvent& WXUNUSED(event))
{
	g_editor.mapWindow->GoToPreviousCenterPosition();
}

void MainMenuBar::OnGotoPosition(wxCommandEvent& WXUNUSED(event))
{
	if(!g_editor.IsProjectOpen())
		return;

	GotoPositionDialog dlg(frame);
	dlg.ShowModal();
}

void MainMenuBar::OnMapRemoveItems(wxCommandEvent& WXUNUSED(event))
{
	if(!g_editor.IsProjectOpen())
		return;

	FindItemDialog dialog(frame, "Item Type to Remove");
	if(dialog.ShowModal() == wxID_OK) {
		OnMapRemoveItems::RemoveItemCondition condition;
		condition.itemId = dialog.getResultID();
		condition.selectedOnly = false;

		g_editor.CreateLoadBar("Searching map for items to remove...");
		int count = g_editor.map.removeItems(condition);
		g_editor.DestroyLoadBar();

		wxString msg;
		msg << count << " items deleted.";

		g_editor.PopupDialog("Search completed", msg, wxOK);
		g_editor.RefreshView();
	}
}

void MainMenuBar::OnMapRemoveCorpses(wxCommandEvent& WXUNUSED(event))
{
	if(!g_editor.IsProjectOpen())
		return;

	int ok = g_editor.PopupDialog("Remove Corpses", "Do you want to remove all corpses from the map?", wxYES | wxNO);

	if(ok == wxID_YES) {
		g_editor.CreateLoadBar("Searching map for items to remove...");
		int count = g_editor.map.removeItems(
			[nextUpdate = 0.0](const Item *item, double progress) mutable {
				if(progress >= nextUpdate){
					g_editor.SetLoadDone((int)(progress * 100.0));
					nextUpdate = progress + 0.01;
				}

				return item->getFlag(CORPSE);
			});
		g_editor.DestroyLoadBar();

		wxString msg;
		msg << count << " items deleted.";
		g_editor.PopupDialog("Search completed", msg, wxOK);
	}
}

void MainMenuBar::OnMapRemoveUnreachable(wxCommandEvent& WXUNUSED(event))
{
	if(!g_editor.IsProjectOpen())
		return;

	int ok = g_editor.PopupDialog("Remove Unreachable Tiles", "Do you want to remove all unreachable items from the map?", wxYES | wxNO);

	if(ok == wxID_YES) {
		g_editor.CreateLoadBar("Searching map for tiles to remove...");
		int removed = g_editor.map.clearTiles(
			[nextUpdate = 0.0](const Tile *tile, double progress) mutable {
				if(progress >= nextUpdate){
					g_editor.SetLoadDone((int)(progress * 100.0));
					nextUpdate = progress + 0.01;
				}

				Position pos = tile->pos;
				int minX = std::max<int>(pos.x - 10, 0);
				int maxX = std::min<int>(pos.x + 10, rme::MapMaxWidth);
				int minY = std::max<int>(pos.y - 8,  0);
				int maxY = std::min<int>(pos.y + 8,  rme::MapMaxHeight);
				int minZ = 0;
				int maxZ = 9;

				if(pos.x > 7){ // underground
					minZ = std::max<int>(pos.z - 2, rme::MapGroundLayer);
					maxZ = std::min<int>(pos.z + 2, rme::MapMaxLayer);
				}

				for(int z = minZ; z <= maxZ; z += 1)
				for(int y = minY; y <= maxY; y += 1)
				for(int x = minX; x <= maxX; x += 1){
					Tile *other = g_editor.map.getTile(x, y, z);
					if(other && !other->getFlag(UNPASS)){
						return false;
					}
				}

				return true;
			});
		g_editor.DestroyLoadBar();

		wxString msg;
		msg << removed << " tiles deleted.";
		g_editor.PopupDialog("Search completed", msg, wxOK);
	}
}

void MainMenuBar::OnClearHouseTiles(wxCommandEvent& WXUNUSED(event))
{
	int ret = g_editor.PopupDialog(
		"Clear Invalid House Tiles",
		"Are you sure you want to remove all house tiles that do not belong to a house (this action cannot be undone)?",
		wxYES | wxNO
	);

	if(ret == wxID_YES) {
		// Editor will do the work
		g_editor.clearInvalidHouseTiles(true);
	}

	g_editor.RefreshView();
}

void MainMenuBar::OnMapCleanHouseItems(wxCommandEvent& WXUNUSED(event))
{
	int ret = g_editor.PopupDialog(
		"Clear Moveable House Items",
		"Are you sure you want to remove all items inside houses that can be moved (this action cannot be undone)?",
		wxYES | wxNO
	);

	if(ret == wxID_YES) {
		// Editor will do the work
		//g_editor.removeHouseItems(true);
	}

	g_editor.RefreshView();
}

void MainMenuBar::OnMapEditTowns(wxCommandEvent& WXUNUSED(event))
{
	wxDialog* town_dialog = newd EditTownsDialog(frame);
	town_dialog->ShowModal();
	town_dialog->Destroy();
}

void MainMenuBar::OnMapEditItems(wxCommandEvent& WXUNUSED(event))
{
	// no-op
}

void MainMenuBar::OnMapEditMonsters(wxCommandEvent& WXUNUSED(event))
{
	// no-op
}

void MainMenuBar::OnMapStatistics(wxCommandEvent& WXUNUSED(event))
{
	if(!g_editor.IsProjectOpen())
		return;

#if TODO
	g_editor.CreateLoadBar("Collecting data...");

	Map &map = g_editor.map;

	int load_counter = 0;

	uint64_t tile_count = 0;
	uint64_t detailed_tile_count = 0;
	uint64_t blocking_tile_count = 0;
	uint64_t walkable_tile_count = 0;
	double percent_pathable = 0.0;
	double percent_detailed = 0.0;
	uint64_t spawn_count = 0;
	uint64_t creature_count = 0;
	double creatures_per_spawn = 0.0;

	uint64_t item_count = 0;
	uint64_t loose_item_count = 0;
	uint64_t depot_count = 0;
	uint64_t action_item_count = 0;
	uint64_t unique_item_count = 0;
	uint64_t container_count = 0; // Only includes containers containing more than 1 item

	int town_count = map.towns.count();
	int house_count = map.houses.count();
	std::map<uint32_t, uint32_t> town_sqm_count;
	const Town* largest_town = nullptr;
	uint64_t largest_town_size = 0;
	uint64_t total_house_sqm = 0;
	const House* largest_house = nullptr;
	uint64_t largest_house_size = 0;
	double houses_per_town = 0.0;
	double sqm_per_house = 0.0;
	double sqm_per_town = 0.0;

	for(MapIterator mit = map.begin(); mit != map.end(); ++mit) {
		Tile* tile = (*mit)->get();
		if(load_counter % 8192 == 0) {
			g_editor.SetLoadDone((unsigned int)(int64_t(load_counter) * 95ll / int64_t(map.getTileCount())));
		}

		if(tile->empty())
			continue;

		tile_count += 1;

		// TODO(fusion): Relevant srv flags/attributes instead?
		bool is_detailed = false;
		for(const Item *item = tile->items; item != NULL; item = item->next){
			item_count += 1;
			if(!item->getFlag(BANK) && !item->getFlag(CLIP)) {
				is_detailed = true;
				if(!item->getFlag(UNMOVE)) {
					loose_item_count += 1;
				}
				if(item->getFlag(CONTAINER) || item->getFlag(CHEST)){
					if(item->content != NULL) {
						container_count += 1;
					}
				}
			}
		}

		if(tile->creature)
			creature_count += 1;

		if(tile->getFlag(UNPASS))
			blocking_tile_count += 1;
		else
			walkable_tile_count += 1;

		if(is_detailed)
			detailed_tile_count += 1;

		load_counter += 1;
	}

	creatures_per_spawn = (spawn_count != 0 ? double(creature_count) / double(spawn_count) : -1.0);
	percent_pathable = 100.0*(tile_count != 0 ? double(walkable_tile_count) / double(tile_count) : -1.0);
	percent_detailed = 100.0*(tile_count != 0 ? double(detailed_tile_count) / double(tile_count) : -1.0);

	load_counter = 0;
	Houses& houses = map.houses;
	for(HouseMap::const_iterator hit = houses.begin(); hit != houses.end(); ++hit) {
		const House* house = hit->second;

		if(load_counter % 64)
			g_editor.SetLoadDone((unsigned int)(95ll + int64_t(load_counter) * 5ll / int64_t(house_count)));

		if(house->size() > largest_house_size) {
			largest_house = house;
			largest_house_size = house->size();
		}
		total_house_sqm += house->size();
		town_sqm_count[house->townid] += house->size();
	}

	houses_per_town = (town_count != 0?  double(house_count) /     double(town_count)  : -1.0);
	sqm_per_house   = (house_count != 0? double(total_house_sqm) / double(house_count) : -1.0);
	sqm_per_town    = (town_count != 0?  double(total_house_sqm) / double(town_count)  : -1.0);

	Towns& towns = map.towns;
	for(std::map<uint32_t, uint32_t>::iterator town_iter = town_sqm_count.begin();
			town_iter != town_sqm_count.end();
			++town_iter)
	{
		// No load bar for this, load is non-existant
		uint32_t town_id = town_iter->first;
		uint32_t town_sqm = town_iter->second;
		Town* town = towns.getTown(town_id);
		if(town && town_sqm > largest_town_size) {
			largest_town = town;
			largest_town_size = town_sqm;
		} else {
			// Non-existant town!
		}
	}

	g_editor.DestroyLoadBar();

	std::ostringstream os;
	os.setf(std::ios::fixed, std::ios::floatfield);
	os.precision(2);
	os << "Map statistics for the map \"" << map.getMapDescription() << "\"\n";
	os << "\tTile data:\n";
	os << "\t\tTotal number of tiles: " << tile_count << "\n";
	os << "\t\tNumber of pathable tiles: " << walkable_tile_count << "\n";
	os << "\t\tNumber of unpathable tiles: " << blocking_tile_count << "\n";
	if(percent_pathable >= 0.0)
		os << "\t\tPercent walkable tiles: " << percent_pathable << "%\n";
	os << "\t\tDetailed tiles: " << detailed_tile_count << "\n";
	if(percent_detailed >= 0.0)
		os << "\t\tPercent detailed tiles: " << percent_detailed << "%\n";

	os << "\tItem data:\n";
	os << "\t\tTotal number of items: " << item_count << "\n";
	os << "\t\tNumber of moveable tiles: " << loose_item_count << "\n";
	os << "\t\tNumber of depots: " << depot_count << "\n";
	os << "\t\tNumber of containers: " << container_count << "\n";
	os << "\t\tNumber of items with Action ID: " << action_item_count << "\n";
	os << "\t\tNumber of items with Unique ID: " << unique_item_count << "\n";

	os << "\tCreature data:\n";
	os << "\t\tTotal creature count: " << creature_count << "\n";
	os << "\t\tTotal spawn count: " << spawn_count << "\n";
	if(creatures_per_spawn >= 0)
		os << "\t\tMean creatures per spawn: " << creatures_per_spawn << "\n";

	os << "\tTown/House data:\n";
	os << "\t\tTotal number of towns: " << town_count << "\n";
	os << "\t\tTotal number of houses: " << house_count << "\n";
	if(houses_per_town >= 0)
		os << "\t\tMean houses per town: " << houses_per_town << "\n";
	os << "\t\tTotal amount of housetiles: " << total_house_sqm << "\n";
	if(sqm_per_house >= 0)
		os << "\t\tMean tiles per house: " << sqm_per_house << "\n";
	if(sqm_per_town >= 0)
		os << "\t\tMean tiles per town: " << sqm_per_town << "\n";

	if(largest_town)
		os << "\t\tLargest Town: \"" << largest_town->getName() << "\" (" << largest_town_size << " sqm)\n";
	if(largest_house)
		os << "\t\tLargest House: \"" << largest_house->name << "\" (" << largest_house_size << " sqm)\n";

	os << "\n";
	os << "Generated by " << __RME_APPLICATION_NAME__ << " version " << __RME_VERSION__ << "\n";


    wxDialog* dg = newd wxDialog(frame, wxID_ANY, "Map Statistics", wxDefaultPosition, wxDefaultSize, wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX);
	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);
	wxTextCtrl* text_field = newd wxTextCtrl(dg, wxID_ANY, wxstr(os.str()), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
	text_field->SetMinSize(wxSize(400, 300));
	topsizer->Add(text_field, wxSizerFlags(5).Expand());

	wxSizer* choicesizer = newd wxBoxSizer(wxHORIZONTAL);
	wxButton* export_button = newd wxButton(dg, wxID_OK, "Export as XML");
	choicesizer->Add(export_button, wxSizerFlags(1).Center());
	export_button->Enable(false);
	choicesizer->Add(newd wxButton(dg, wxID_CANCEL, "OK"), wxSizerFlags(1).Center());
	topsizer->Add(choicesizer, wxSizerFlags(1).Center());
	dg->SetSizerAndFit(topsizer);
	dg->Centre(wxBOTH);

	int ret = dg->ShowModal();

	if(ret == wxID_OK) {
		//std::cout << "XML EXPORT";
	} else if(ret == wxID_CANCEL) {
		//std::cout << "OK";
	}
#endif
}

void MainMenuBar::OnMapCleanup(wxCommandEvent& WXUNUSED(event))
{
	int ok = g_editor.PopupDialog("Clean map", "Do you want to remove all invalid items from the map?", wxYES | wxNO);

	if(ok == wxID_YES)
		g_editor.map.cleanInvalidTiles(true);
}

void MainMenuBar::OnToolbars(wxCommandEvent& event)
{
	switch(event.GetId()){
		case MENUBAR_VIEW_TOOLBARS_BRUSHES:
			g_editor.ShowToolbar(TOOLBAR_BRUSHES, event.IsChecked());
			g_settings.setInteger(Config::SHOW_TOOLBAR_BRUSHES, event.IsChecked());
			break;
		case MENUBAR_VIEW_TOOLBARS_POSITION:
			g_editor.ShowToolbar(TOOLBAR_POSITION, event.IsChecked());
			g_settings.setInteger(Config::SHOW_TOOLBAR_POSITION, event.IsChecked());
			break;
		case MENUBAR_VIEW_TOOLBARS_SIZES:
			g_editor.ShowToolbar(TOOLBAR_SIZES, event.IsChecked());
			g_settings.setInteger(Config::SHOW_TOOLBAR_SIZES, event.IsChecked());
			break;
		case MENUBAR_VIEW_TOOLBARS_INDICATORS:
			g_editor.ShowToolbar(TOOLBAR_INDICATORS, event.IsChecked());
			g_settings.setInteger(Config::SHOW_TOOLBAR_INDICATORS, event.IsChecked());
			break;
		case MENUBAR_VIEW_TOOLBARS_STANDARD:
			g_editor.ShowToolbar(TOOLBAR_STANDARD, event.IsChecked());
			g_settings.setInteger(Config::SHOW_TOOLBAR_STANDARD, event.IsChecked());
			break;
	}
}

void MainMenuBar::OnToggleFullscreen(wxCommandEvent& WXUNUSED(event))
{
	if(frame->IsFullScreen())
		frame->ShowFullScreen(false);
	else
		frame->ShowFullScreen(true, wxFULLSCREEN_NOBORDER | wxFULLSCREEN_NOCAPTION);
}

void MainMenuBar::OnTakeScreenshot(wxCommandEvent& WXUNUSED(event))
{
	wxFileName path(g_settings.getString(Config::SCREENSHOT_DIRECTORY), "");
	g_editor.mapWindow->GetCanvas()->TakeScreenshot(
		path, wxstr(g_settings.getString(Config::SCREENSHOT_FORMAT)));
}

void MainMenuBar::OnZoomIn(wxCommandEvent& event)
{
	double zoom = g_editor.GetCurrentZoom();
	g_editor.SetCurrentZoom(zoom - 0.1);
}

void MainMenuBar::OnZoomOut(wxCommandEvent& event)
{
	double zoom = g_editor.GetCurrentZoom();
	g_editor.SetCurrentZoom(zoom + 0.1);
}

void MainMenuBar::OnZoomNormal(wxCommandEvent& event)
{
	g_editor.SetCurrentZoom(1.0);
}

void MainMenuBar::OnChangeViewSettings(wxCommandEvent& event)
{
	g_settings.setInteger(Config::SHOW_ALL_FLOORS, IsItemChecked(MENUBAR_SHOW_ALL_FLOORS));
	if(IsItemChecked(MENUBAR_SHOW_ALL_FLOORS)) {
		EnableItem(MENUBAR_SELECT_MODE_VISIBLE, true);
		EnableItem(MENUBAR_SELECT_MODE_LOWER, true);
	} else {
		EnableItem(MENUBAR_SELECT_MODE_VISIBLE, false);
		EnableItem(MENUBAR_SELECT_MODE_LOWER, false);
		CheckItem(MENUBAR_SELECT_MODE_CURRENT, true);
		g_settings.setInteger(Config::SELECTION_TYPE, SELECT_CURRENT_FLOOR);
	}
	g_settings.setInteger(Config::TRANSPARENT_FLOORS, IsItemChecked(MENUBAR_GHOST_HIGHER_FLOORS));
	g_settings.setInteger(Config::TRANSPARENT_ITEMS, IsItemChecked(MENUBAR_GHOST_ITEMS));
	g_settings.setInteger(Config::SHOW_INGAME_BOX, IsItemChecked(MENUBAR_SHOW_INGAME_BOX));
	g_settings.setInteger(Config::SHOW_LIGHTS, IsItemChecked(MENUBAR_SHOW_LIGHTS));
	g_settings.setInteger(Config::SHOW_GRID, IsItemChecked(MENUBAR_SHOW_GRID));
	g_settings.setInteger(Config::SHOW_EXTRA, !IsItemChecked(MENUBAR_SHOW_EXTRA));

	g_settings.setInteger(Config::SHOW_SHADE, IsItemChecked(MENUBAR_SHOW_SHADE));
	g_settings.setInteger(Config::SHOW_SPECIAL_TILES, IsItemChecked(MENUBAR_SHOW_SPECIAL));
	g_settings.setInteger(Config::SHOW_AS_MINIMAP, IsItemChecked(MENUBAR_SHOW_AS_MINIMAP));
	g_settings.setInteger(Config::SHOW_ONLY_TILEFLAGS, IsItemChecked(MENUBAR_SHOW_ONLY_COLORS));
	g_settings.setInteger(Config::SHOW_ONLY_MODIFIED_TILES, IsItemChecked(MENUBAR_SHOW_ONLY_MODIFIED));
	g_settings.setInteger(Config::SHOW_CREATURES, IsItemChecked(MENUBAR_SHOW_CREATURES));
	g_settings.setInteger(Config::SHOW_HOUSES, IsItemChecked(MENUBAR_SHOW_HOUSES));
	g_settings.setInteger(Config::HIGHLIGHT_ITEMS, IsItemChecked(MENUBAR_HIGHLIGHT_ITEMS));
	g_settings.setInteger(Config::SHOW_BLOCKING, IsItemChecked(MENUBAR_SHOW_PATHING));
	g_settings.setInteger(Config::SHOW_TOOLTIPS, IsItemChecked(MENUBAR_SHOW_TOOLTIPS));
	g_settings.setInteger(Config::SHOW_PREVIEW, IsItemChecked(MENUBAR_SHOW_PREVIEW));
	g_settings.setInteger(Config::SHOW_WALL_HOOKS, IsItemChecked(MENUBAR_SHOW_WALL_HOOKS));
	g_settings.setInteger(Config::SHOW_PICKUPABLES, IsItemChecked(MENUBAR_SHOW_PICKUPABLES));
	g_settings.setInteger(Config::SHOW_MOVEABLES, IsItemChecked(MENUBAR_SHOW_MOVEABLES));

	g_editor.RefreshView();
	g_editor.toolbar->UpdateIndicators();
}

void MainMenuBar::OnChangeFloor(wxCommandEvent& event)
{
	// Workaround to stop events from looping
	if(checking_programmaticly)
		return;

	for(int i = 0; i < 16; ++i) {
		if(IsItemChecked(MENUBAR_FLOOR_0 + i)) {
			g_editor.ChangeFloor(i);
		}
	}
}

void MainMenuBar::OnProblemsWindow(wxCommandEvent& event)
{
	g_editor.ShowProblemsWindow();
}

void MainMenuBar::OnMinimapWindow(wxCommandEvent& event)
{
	g_editor.CreateMinimap();
}

void MainMenuBar::OnActionsHistoryWindow(wxCommandEvent& WXUNUSED(event))
{
	g_editor.ShowActionsWindow();
}

void MainMenuBar::OnNewPalette(wxCommandEvent& event)
{
	g_editor.NewPalette();
}

void MainMenuBar::OnSelectTerrainPalette(wxCommandEvent& WXUNUSED(event))
{
	g_editor.SelectPalettePage(TILESET_TERRAIN);
}

void MainMenuBar::OnSelectDoodadPalette(wxCommandEvent& WXUNUSED(event))
{
	g_editor.SelectPalettePage(TILESET_DOODAD);
}

void MainMenuBar::OnSelectItemPalette(wxCommandEvent& WXUNUSED(event))
{
	g_editor.SelectPalettePage(TILESET_ITEM);
}

void MainMenuBar::OnSelectHousePalette(wxCommandEvent& WXUNUSED(event))
{
	g_editor.SelectPalettePage(TILESET_HOUSE);
}

void MainMenuBar::OnSelectCreaturePalette(wxCommandEvent& WXUNUSED(event))
{
	g_editor.SelectPalettePage(TILESET_CREATURE);
}

void MainMenuBar::OnSelectWaypointPalette(wxCommandEvent& WXUNUSED(event))
{
	g_editor.SelectPalettePage(TILESET_WAYPOINT);
}

void MainMenuBar::OnSelectRawPalette(wxCommandEvent& WXUNUSED(event))
{
	g_editor.SelectPalettePage(TILESET_RAW);
}

void MainMenuBar::SearchItems(bool container, bool writable, bool onSelection/* = false*/)
{
	if(!container && !writable)
		return;

	if(!g_editor.IsProjectOpen())
		return;

	if(onSelection)
		g_editor.CreateLoadBar("Searching on selected area...");
	else
		g_editor.CreateLoadBar("Searching on map...");

	OnSearchForStuff::Searcher searcher;
	searcher.searchContainer = container;
	searcher.searchWritable = writable;
	searcher.selectedOnly = onSelection;
	g_editor.map.forEachItem(searcher);
	searcher.sort();

	g_editor.DestroyLoadBar();

	SearchResultWindow* result = g_editor.ShowSearchWindow();
	result->Clear();

	for(const auto &[tile, item]: searcher.results){
		result->AddPosition(searcher.desc(item), tile->pos);
	}
}

void MainMenuBar::SearchDuplicatedItems(bool selection)
{
	if(!g_editor.IsProjectOpen()) {
		return;
	}

	auto dialog = g_editor.ShowDuplicatedItemsWindow();
	dialog->StartSearch(selection);
}
