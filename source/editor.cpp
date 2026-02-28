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
#include "action.h"
#include "common.h"
#include "editor.h"

#include "actions_history_window.h"
#include "application.h"
#include "brush.h"
#include "common_windows.h"
#include "doodad_brush.h"
#include "duplicated_items_window.h"
#include "ground_brush.h"
#include "wxids.h"
#include "house_exit_brush.h"
#include "main_menubar.h"
#include "map.h"
#include "map_display.h"
#include "map_window.h"
#include "materials.h"
#include "minimap_window.h"
#include "palette_window.h"
#include "result_window.h"
#include "settings.h"
#include "waypoint_brush.h"
#include "welcome_dialog.h"

#include <wx/display.h>

#ifdef __WXOSX__
#	include <AGL/agl.h>
#endif

Editor g_editor;

Editor::Editor()
{
	doodad_buffer_map = newd Map();
}

Editor::~Editor()
{
	// NOTE(fusion): This is a "singleton" anyways, so this only gets executed
	// at exit. Let the OS reclaim any resources, any persistent data should
	// already have been saved to disk.
}

wxGLContext &Editor::GetGLContext(wxGLCanvas* win)
{
	if(OGLContext == nullptr) {
		OGLContext = newd wxGLContext(win);
    }

	return *OGLContext;
}

void Editor::LoadRecentFiles()
{
	recentFiles.Load(g_settings.getConfigObject());
}

void Editor::SaveRecentFiles()
{
	recentFiles.Save(g_settings.getConfigObject());
}

void Editor::AddRecentFile(const wxString &file)
{
	recentFiles.AddFileToHistory(file);
}

std::vector<wxString> Editor::GetRecentFiles()
{
    std::vector<wxString> files(recentFiles.GetCount());
    for(size_t i = 0; i < recentFiles.GetCount(); ++i) {
        files[i] = recentFiles.GetHistoryFile(i);
    }
    return files;
}

bool Editor::NewProject(void)
{
	// TODO(fusion): Create new project? Which doesn't make a lot of sense if
	// a project also includes objects, materials, etc... so we might want to
	// have some kind of project template?
	return OpenProject();
}

bool Editor::OpenProject(void)
{
	bool result = false;
	wxDirDialog openDialog(root, "Select project directory...", wxEmptyString, wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
	if(openDialog.ShowModal() == wxID_OK){
		result = OpenProject(openDialog.GetPath());
	}
	return result;
}

bool Editor::OpenProject(const wxString &dir)
{
	FinishWelcomeDialog();

	if(!wxFileName::DirExists(dir)){
		PopupDialog("Error", "Project directory \"" + dir + "\" does not exist.", wxOK);
		return false;
	}

	if(IsProjectOpen() && !CloseProject()){
		return false;
	}

	{
		UnnamedRenderingLock();
		if(!LoadProject(dir)){
			return false;
		}

		AddRecentFile(projectDir);
		SetStatusText("");
		SetTitle(projectDir);
		LoadPerspective();
		RefreshPalettes();
		UpdateMenubar();
		root->Refresh();

		mapWindow->FitToMap();

		{ // NOTE(fusion): Load map position.
			Position mapPosition;
			std::istringstream ss(g_settings.getString(Config::MAP_POSITION));
			ss >> mapPosition;

			if(!PositionValid(mapPosition.x, mapPosition.y, mapPosition.z)){
				mapPosition = map.getCenterPosition();
			}

			mapWindow->SetScreenCenterPosition(mapPosition);
		}

		{ // NOTE(fusion): Load editor mode.
			EditorMode savedMode = (EditorMode)g_settings.getInteger(Config::EDITOR_MODE);
			if(savedMode != mode){
				if(savedMode == SELECTION_MODE){
					SetSelectionMode();
				}else if(savedMode == DRAWING_MODE){
					SetDrawingMode();
				}
			}
		}
	}

	return true;
}

bool Editor::CloseProject(void)
{
	if(!IsProjectOpen()){
		return false;
	}

	if(IsProjectDirty()){
		int ret = PopupDialog("Save changes",
				"Do you want to save your changes to \"" + projectDir + "\"?",
				wxYES | wxNO | wxCANCEL);
		if(ret == wxID_CANCEL) return false;
		if(ret == wxID_YES)    SaveProject();
	}

	{ // NOTE(fusion): Save map position.
		std::ostringstream ss;
		ss << mapWindow->GetScreenCenterPosition();
		g_settings.setString(Config::MAP_POSITION, ss.str());
	}

	{ // NOTE(fusion): Save editor mode.
		g_settings.setInteger(Config::EDITOR_MODE, mode);
	}

	SavePerspective();

	{
		UnnamedRenderingLock();
		DestroyPalettes();
		DestroyMinimap();
		UnloadProject();
	}

	return true;
}

void Editor::SaveProject(void)
{
	if(!IsProjectOpen()){
		return;
	}

	map.save(projectDir);
}

void Editor::SaveProjectAs(void)
{
	if(!IsProjectOpen()){
		return;
	}
}

bool Editor::ReloadProject(void)
{
	if(!IsProjectOpen()){
		return false;
	}

	wxString dir = projectDir; // make a copy to avoid aliasing issues
	return OpenProject(dir);
}

bool Editor::IsProjectOpen(void) const {
	return !projectDir.IsEmpty();
}

bool Editor::IsProjectDirty(void) const {
	// TODO(fusion): I'm not sure this is reliable since the action queue is
	// limited to a number of entries. While probably unrealistic, it could be
	// the case that real changes are pushed off by selects/deselects.
	return actionQueue.hasChanges();
}

bool Editor::ExportPatch(const wxString &filename, bool commit /*= false*/) {
	if(!IsProjectOpen()){
		return false;
	}

	ScopedLoadingBar loadingBar("Exporting patch...");
	return g_editor.map.exportPatch(projectDir, filename, commit);
}

bool Editor::LoadProject(wxString dir)
{
	ASSERT(!IsProjectOpen());

	dir = NormalizeDir(dir);

	ClearProblems();

	{
		ScopedLoadingBar loadingBar("Loading menu bar...");
		menubar->Load(dir);

		loadingBar.SetLoadDone(0, "Loading item types...");
		if(!LoadItemTypes(dir)){
			UnloadProject();
			return false;
		}

		loadingBar.SetLoadDone(20, "Loading creature types...");
		LoadCreatureTypes(dir);

		loadingBar.SetLoadDone(40, "Loading DAT...");
		if(!gfx.loadSpriteMetadata(dir)){
			UnloadProject();
			return false;
		}

		loadingBar.SetLoadDone(60, "Loading SPR...");
		if(!gfx.loadSpriteData(dir)){
			UnloadProject();
			return false;
		}

		loadingBar.SetLoadDone(80, "Loading materials.xml...");
		g_materials.loadMaterials(dir);
		g_brushes.init();
		g_materials.createOtherTileset();
	}

	{
		ScopedLoadingBar loadingBar("Loading map...");
		if(!map.load(dir)){
			UnloadProject();
			return false;
		}
	}

	if(true){ // g_settings.getBoolean(Config::CHECK_TILE_ITEMS)
		checkTileItems(true);
	}

	projectDir = std::move(dir);
	return true;
}

void Editor::UnloadProject(void)
{
	current_brush = NULL;
	previous_brush = NULL;
	house_brush = NULL;
	house_exit_brush = NULL;
	waypoint_brush = NULL;
	optional_brush = NULL;
	eraser = NULL;
	normal_door_brush = NULL;
	locked_door_brush = NULL;
	magic_door_brush = NULL;
	quest_door_brush = NULL;
	hatch_door_brush = NULL;
	window_door_brush = NULL;
	refresh_brush = NULL;
	nolog_brush = NULL;
	pz_brush = NULL;

	selection.clear(NULL);
	actionQueue.clear();
	map.clear();
	g_brushes.clear();
	g_materials.clear();
	ClearCreatureTypes();
	gfx.clear();
	ClearItemTypes();
	menubar->LoadDefault();
	projectDir.Clear();
}

double Editor::GetCurrentZoom()
{
	return mapWindow->GetCanvas()->GetZoom();
}

void Editor::SetCurrentZoom(double zoom)
{
	mapWindow->GetCanvas()->SetZoom(zoom);
}

void Editor::AddPendingMapEvent(wxEvent &event)
{
	mapWindow->GetEventHandler()->AddPendingEvent(event);
}

void Editor::AddPendingCanvasEvent(wxEvent &event)
{
	mapWindow->GetCanvas()->GetEventHandler()->AddPendingEvent(event);
}

static void DockPaneIfOffscreen(wxAuiPaneInfo &pane){
	if(pane.IsFloatable()) {
		bool offscreen = true;
		for(uint32_t index = 0; index < wxDisplay::GetCount(); ++index) {
			wxDisplay display(index);
			wxRect rect = display.GetClientArea();
			if(rect.Contains(pane.floating_pos)) {
				offscreen = false;
				break;
			}
		}

		if(offscreen) {
			pane.Dock();
		}
	}
}

void Editor::SaveWindow()
{
	// TODO(fusion): Perhaps there is a way to get the un-maximized size? Trying
	// to call root->Maximize(false) here doesn't work because the state change
	// isn't immediate.
	g_settings.setInteger(Config::WINDOW_WIDTH, root->GetSize().GetWidth());
	g_settings.setInteger(Config::WINDOW_HEIGHT, root->GetSize().GetHeight());
	g_settings.setInteger(Config::WINDOW_MAXIMIZED, root->IsMaximized());
}

void Editor::LoadWindow()
{
	if(g_settings.getBoolean(Config::WINDOW_MAXIMIZED)){
		root->Maximize();
	}else{
		root->SetSize(g_settings.getInteger(Config::WINDOW_WIDTH),
					  g_settings.getInteger(Config::WINDOW_HEIGHT));
	}
}

void Editor::LoadPerspective()
{
	std::vector<std::string> palette_list;
	{
		std::string tmp;
		std::string layout = g_settings.getString(Config::PALETTE_LAYOUT);
		for(char c : layout) {
			if(c == '|') {
				palette_list.push_back(tmp);
				tmp.clear();
			} else {
				tmp.push_back(c);
			}
		}

		if(!tmp.empty()) {
			palette_list.push_back(tmp);
		}
	}

	for(const std::string& name : palette_list) {
		PaletteWindow* palette = CreatePalette();
		wxAuiPaneInfo &pane = aui_manager->GetPane(palette);
		aui_manager->LoadPaneInfo(wxstr(name), pane);
		DockPaneIfOffscreen(pane);
	}

	if(g_settings.getInteger(Config::MINIMAP_VISIBLE)) {
		wxString layout = wxstr(g_settings.getString(Config::MINIMAP_LAYOUT));
		if(!minimap) {
			wxAuiPaneInfo pane;
			aui_manager->LoadPaneInfo(layout, pane);
			minimap = newd MinimapWindow(root);
			aui_manager->AddPane(minimap, pane);
			DockPaneIfOffscreen(pane);
		} else {
			wxAuiPaneInfo &pane = aui_manager->GetPane(minimap);
			aui_manager->LoadPaneInfo(layout, pane);
			DockPaneIfOffscreen(pane);
		}
	}

	if(g_settings.getInteger(Config::ACTIONS_HISTORY_VISIBLE)) {
		wxString layout = wxstr(g_settings.getString(Config::ACTIONS_HISTORY_LAYOUT));
		if(!actions_history_window) {
			wxAuiPaneInfo pane;
			aui_manager->LoadPaneInfo(layout, pane);
			actions_history_window = new ActionsHistoryWindow(root);
			aui_manager->AddPane(actions_history_window, pane);
			DockPaneIfOffscreen(pane);
		} else {
			wxAuiPaneInfo &pane = aui_manager->GetPane(actions_history_window);
			aui_manager->LoadPaneInfo(layout, pane);
			DockPaneIfOffscreen(pane);
		}
	}

	aui_manager->Update();
	UpdateMenubar();
	toolbar->LoadPerspective();
}

void Editor::SavePerspective()
{
	g_settings.setInteger(Config::MINIMAP_VISIBLE, minimap? 1: 0);
	g_settings.setInteger(Config::ACTIONS_HISTORY_VISIBLE, actions_history_window ? 1 : 0);

	{
		wxString layout;
		for(auto &palette : palettes) {
			if(aui_manager->GetPane(palette).IsShown())
				layout << aui_manager->SavePaneInfo(aui_manager->GetPane(palette)) << "|";
		}
		g_settings.setString(Config::PALETTE_LAYOUT, nstr(layout));
	}

	if(minimap) {
		wxString layout = aui_manager->SavePaneInfo(aui_manager->GetPane(minimap));
		g_settings.setString(Config::MINIMAP_LAYOUT, nstr(layout));
	}

	if(actions_history_window) {
		wxString layout = aui_manager->SavePaneInfo(aui_manager->GetPane(actions_history_window));
		g_settings.setString(Config::ACTIONS_HISTORY_LAYOUT, nstr(layout));
	}

	toolbar->SavePerspective();
}

void Editor::HideSearchWindow()
{
	if(search_result_window) {
		aui_manager->GetPane(search_result_window).Show(false);
		aui_manager->Update();
	}
}

SearchResultWindow* Editor::ShowSearchWindow()
{
	if(search_result_window == nullptr) {
		search_result_window = newd SearchResultWindow(root);
		aui_manager->AddPane(search_result_window, wxAuiPaneInfo().Caption("Search Results"));
	} else {
		aui_manager->GetPane(search_result_window).Show();
	}
	aui_manager->Update();
	return search_result_window;
}

DuplicatedItemsWindow* Editor::ShowDuplicatedItemsWindow()
{
	if(!duplicated_items_window) {
		duplicated_items_window = new DuplicatedItemsWindow(root);
		aui_manager->AddPane(duplicated_items_window, wxAuiPaneInfo().Caption("Duplicated Items"));
	} else {
		aui_manager->GetPane(duplicated_items_window).Show();
	}
	aui_manager->Update();
	return duplicated_items_window;
}

void Editor::HideDuplicatedItemsWindow()
{
	if(duplicated_items_window) {
		aui_manager->GetPane(duplicated_items_window).Show(false);
		aui_manager->Update();
	}
}

ActionsHistoryWindow* Editor::ShowActionsWindow()
{
	if(!actions_history_window) {
		actions_history_window = new ActionsHistoryWindow(root);
		aui_manager->AddPane(actions_history_window, wxAuiPaneInfo().Caption("Actions History"));
	} else {
		aui_manager->GetPane(actions_history_window).Show();
	}

	aui_manager->Update();
	actions_history_window->RefreshActions();
	return actions_history_window;
}

void Editor::HideActionsWindow()
{
	if(actions_history_window) {
		aui_manager->GetPane(actions_history_window).Show(false);
		aui_manager->Update();
	}
}

ProblemsWindow *Editor::ShowProblemsWindow()
{
	if(!problems_window){
		problems_window = new ProblemsWindow(root);
		aui_manager->AddPane(problems_window, wxAuiPaneInfo().Caption("Problems").Bottom().Floatable(false).Dock());
	}else{
		aui_manager->GetPane(problems_window).Show();
	}

	aui_manager->Update();
	return problems_window;
}

void Editor::ClearProblems(void){
	if(problems_window){
		problems_window->Clear();
	}
}

void Editor::Notice(wxString message, ProblemSource source /*= {}*/){
	ShowProblemsWindow()->Insert(PROBLEM_SEVERITY_NOTICE, source, std::move(message));
}

void Editor::Warning(wxString message, ProblemSource source /*= {}*/){
	ShowProblemsWindow()->Insert(PROBLEM_SEVERITY_WARNING, source, std::move(message));
}

void Editor::Error(wxString message, ProblemSource source /*= {}*/){
	ShowProblemsWindow()->Insert(PROBLEM_SEVERITY_ERROR, source, std::move(message));
}

//=============================================================================
// Palette Window Interface implementation

PaletteWindow* Editor::GetPalette()
{
	if(palettes.empty())
		return nullptr;
	return palettes.front();
}

PaletteWindow* Editor::NewPalette()
{
	return CreatePalette();
}

void Editor::RefreshPalettes()
{
	for(auto&palette : palettes) {
		palette->OnUpdate(&map);
	}
	SelectBrush();

	if(duplicated_items_window) {
		duplicated_items_window->UpdateButtons();
	}

	RefreshActions();
}

void Editor::RefreshOtherPalettes(PaletteWindow* p)
{
	for(auto &palette : palettes) {
		if(palette != p)
			palette->OnUpdate(&map);
	}
	SelectBrush();
}

PaletteWindow* Editor::CreatePalette()
{
	if(!IsProjectOpen())
		return nullptr;

	auto *palette = newd PaletteWindow(root, g_materials.tilesets);
	aui_manager->AddPane(palette, wxAuiPaneInfo().Caption("Palette").TopDockable(false).BottomDockable(false));
	aui_manager->Update();

	// Make us the active palette
	palettes.push_front(palette);
	// Select brush from this palette
	SelectBrushInternal(palette->GetSelectedBrush());

	return palette;
}

void Editor::ActivatePalette(PaletteWindow* p)
{
	palettes.erase(std::find(palettes.begin(), palettes.end(), p));
	palettes.push_front(p);
}

void Editor::DestroyPalettes()
{
	for(auto palette : palettes) {
		aui_manager->DetachPane(palette);
		palette->Destroy();
		palette = nullptr;
	}
	palettes.clear();
	aui_manager->Update();
}

void Editor::RebuildPalettes()
{
	// Palette lits might be modified due to active palette changes
	// Use a temporary list for iterating
	std::list<PaletteWindow*> tmp = palettes;
	for(auto &piter : tmp) {
		piter->ReloadSettings(&map);
	}
	aui_manager->Update();
}

void Editor::ShowPalette()
{
	if(palettes.empty())
		return;

	for(auto &palette : palettes) {
		if(aui_manager->GetPane(palette).IsShown())
			return;
	}

	aui_manager->GetPane(palettes.front()).Show(true);
	aui_manager->Update();
}

void Editor::SelectPalettePage(PaletteType pt)
{
	if(palettes.empty())
		CreatePalette();
	PaletteWindow* p = GetPalette();
	if(!p)
		return;

	ShowPalette();
	p->SelectPage(pt);
	aui_manager->Update();
	SelectBrushInternal(p->GetSelectedBrush());
}

//=============================================================================
// Minimap Window Interface Implementation

void Editor::CreateMinimap()
{
	if(!IsProjectOpen())
		return;

	if(minimap) {
		aui_manager->GetPane(minimap).Show(true);
	} else {
		minimap = newd MinimapWindow(root);
		minimap->Show(true);
		aui_manager->AddPane(minimap, wxAuiPaneInfo().Caption("Minimap"));
	}
	aui_manager->Update();
}

void Editor::HideMinimap()
{
	if(minimap) {
		aui_manager->GetPane(minimap).Show(false);
		aui_manager->Update();
	}
}

void Editor::DestroyMinimap()
{
	if(minimap) {
		aui_manager->DetachPane(minimap);
		aui_manager->Update();
		minimap->Destroy();
		minimap = nullptr;
	}
}

void Editor::UpdateMinimap(bool immediate)
{
	if(IsMinimapVisible()) {
		if(immediate) {
			minimap->Refresh();
		} else {
			minimap->DelayedUpdate();
		}
	}
}

bool Editor::IsMinimapVisible() const
{
	if(minimap) {
		const wxAuiPaneInfo& pi = aui_manager->GetPane(minimap);
		if(pi.IsShown()) {
			return true;
		}
	}
	return false;
}

//=============================================================================

void Editor::RefreshView()
{
	mapWindow->Refresh();
}

void Editor::CreateLoadBar(wxString message, bool canCancel /* = false */)
{
	DestroyLoadBar();
	progressText = message;
	progressFrom = 0;
	progressTo   = 100;
	progressBar  = newd wxGenericProgressDialog("Loading", progressText + " (0%)",
			100, root, wxPD_APP_MODAL | wxPD_SMOOTH | (canCancel ? wxPD_CAN_ABORT : 0));

	progressBar->SetSize(280, -1);
	progressBar->Show(true);
	progressBar->Raise();
	progressBar->Update(0);
}

void Editor::SetLoadScale(int from, int to)
{
	progressFrom = from;
	progressTo   = to;
}

bool Editor::SetLoadDone(int done, const wxString& newMessage)
{
	// NOTE(fusion): Make that there is a progress bar running and that we don't
	// update it too frequently. It seems that on Windows it could slow down things
	// significantly.
	if(!progressBar || progressTimer.Time() <= 1000){
		return true;
	}

	progressTimer.Start(0);
	if(!newMessage.empty()) {
		progressText = newMessage;
	}

	int progress = progressFrom + (int)((done / 100.0) * (progressTo - progressFrom));
	if(progress < 0)   progress = 0;
	if(progress > 100) progress = 100;
	return progressBar->Update(progress, (wxString() << progressText << " (" << progress << "%)"));
}

void Editor::DestroyLoadBar()
{
	if(progressBar) {
		progressBar->Show(false);
		progressBar->Destroy();
		progressBar = nullptr;

		if(root->IsActive()) {
			root->Raise();
		} else {
			root->RequestUserAttention();
		}
	}
}

void Editor::ShowWelcomeDialog(const wxBitmap &icon) {
    std::vector<wxString> recent_files = GetRecentFiles();
    welcomeDialog = newd WelcomeDialog(__RME_APPLICATION_NAME__,
			(wxString() << "Version " << __RME_VERSION__),
			FROM_DIP(root, wxSize(800, 480)), icon, recent_files);
    welcomeDialog->Bind(wxEVT_CLOSE_WINDOW, &Editor::OnWelcomeDialogClosed, this);
    welcomeDialog->Bind(WELCOME_DIALOG_ACTION, &Editor::OnWelcomeDialogAction, this);
    welcomeDialog->Show();
    UpdateMenubar();
}

void Editor::FinishWelcomeDialog() {
    if(welcomeDialog != nullptr) {
        welcomeDialog->Hide();
		root->Show();
        welcomeDialog->Destroy();
        welcomeDialog = nullptr;
		UpdateMenubar();
    }
}

bool Editor::IsWelcomeDialogShown() {
    return welcomeDialog != nullptr && welcomeDialog->IsShown();
}

void Editor::OnWelcomeDialogClosed(wxCloseEvent &event)
{
    welcomeDialog->Destroy();
    root->Close();
}

void Editor::OnWelcomeDialogAction(wxCommandEvent &event)
{
    if(event.GetId() == wxID_NEW) {
        NewProject();
    } else if(event.GetId() == wxID_OPEN) {
        OpenProject(event.GetString());
    }
}

void Editor::UpdateMenubar()
{
	menubar->Update();
	toolbar->UpdateButtons();
}

void Editor::SetScreenCenterPosition(const Position& position, bool showIndicator)
{
	mapWindow->SetScreenCenterPosition(position, showIndicator);
}

void Editor::DoCut()
{
	if(!IsSelectionMode())
		return;

	copybuffer.cut(GetCurrentFloor());
	RefreshView();
	UpdateMenubar();
}

void Editor::DoCopy()
{
	if(!IsSelectionMode())
		return;

	copybuffer.copy(GetCurrentFloor());
	RefreshView();
	UpdateMenubar();
}

void Editor::DoPaste()
{
	copybuffer.paste(mapWindow->GetCanvas()->GetCursorPosition());
}

void Editor::PreparePaste()
{
	SetSelectionMode();
	selection.clear(NULL);
	StartPasting();
	RefreshView();
}

void Editor::StartPasting()
{
	pasting = true;
	secondary_map = copybuffer.getBufferMap();
}

void Editor::EndPasting()
{
	if(pasting) {
		pasting = false;
		secondary_map = nullptr;
	}
}

void Editor::DoUndo(int numActions /*= 1*/)
{
	if(canUndo() && numActions > 0){
		undo(numActions);
		if(hasSelection())
			SetSelectionMode();
		SetStatusText("Undo action");
		UpdateMinimap();
		UpdateMenubar();
		root->Refresh();
	}
}

void Editor::DoRedo(int numActions /*= 1*/)
{
	if(canRedo() && numActions > 0) {
		redo(numActions);
		if(hasSelection())
			SetSelectionMode();
		SetStatusText("Redo action");
		UpdateMinimap();
		UpdateMenubar();
		root->Refresh();
	}
}

int Editor::GetCurrentFloor()
{
	return mapWindow->GetCanvas()->GetFloor();
}

void Editor::ChangeFloor(int new_floor)
{
	int old_floor = GetCurrentFloor();
	if(new_floor < rme::MapMinLayer || new_floor > rme::MapMaxLayer)
		return;

	if(old_floor != new_floor)
		mapWindow->GetCanvas()->ChangeFloor(new_floor);
}

void Editor::SetStatusText(wxString text)
{
	root->SetStatusText(text, 0);
}

void Editor::SetTitle(wxString title)
{
	if(root == nullptr)
		return;

	if(!title.empty()) {
		root->SetTitle(title << " - " << __RME_APPLICATION_NAME__);
	} else {
		root->SetTitle(__RME_APPLICATION_NAME__);
	}
}

void Editor::UpdateMenus()
{
	wxCommandEvent evt(EVT_UPDATE_MENUS);
	root->GetEventHandler()->AddPendingEvent(evt);
}

void Editor::UpdateActions()
{
	wxCommandEvent evt(EVT_UPDATE_ACTIONS);
	root->GetEventHandler()->AddPendingEvent(evt);
}

void Editor::RefreshActions()
{
	if(actions_history_window)
		actions_history_window->RefreshActions();
}

void Editor::ShowToolbar(ToolBarID id, bool show)
{
	if(toolbar) // ?
		toolbar->Show(id, show);
}

void Editor::SwitchMode()
{
	if(mode == DRAWING_MODE) {
		SetSelectionMode();
	} else {
		SetDrawingMode();
	}
}

void Editor::SetSelectionMode()
{
	if(mode == SELECTION_MODE)
		return;

	if(current_brush && current_brush->isDoodad()) {
		secondary_map = nullptr;
	}

	mapWindow->OnSwitchEditorMode(SELECTION_MODE);
	mode = SELECTION_MODE;
}

void Editor::SetDrawingMode()
{
	if(mode == DRAWING_MODE)
		return;

	if(current_brush && current_brush->isDoodad()) {
		secondary_map = doodad_buffer_map;
	} else {
		secondary_map = nullptr;
	}

	selection.clear(NULL);
	selection.updateSelectionCount();
	mapWindow->OnSwitchEditorMode(DRAWING_MODE);
	mode = DRAWING_MODE;
}

void Editor::SetBrushSizeInternal(int nz)
{
	if(nz != brush_size && current_brush && current_brush->isDoodad() && !current_brush->oneSizeFitsAll()) {
		brush_size = nz;
		FillDoodadPreviewBuffer();
		secondary_map = doodad_buffer_map;
	} else {
		brush_size = nz;
	}
}

void Editor::SetBrushSize(int nz)
{
	SetBrushSizeInternal(nz);

	for(auto &palette : palettes) {
		palette->OnUpdateBrushSize(brush_shape, brush_size);
	}

	toolbar->UpdateBrushSize(brush_shape, brush_size);
}

void Editor::SetBrushVariation(int nz)
{
	if(nz != brush_variation && current_brush && current_brush->isDoodad()) {
		// Monkey!
		brush_variation = nz;
		FillDoodadPreviewBuffer();
		secondary_map = doodad_buffer_map;
	}
}

void Editor::SetBrushShape(BrushShape bs)
{
	if(bs != brush_shape && current_brush && current_brush->isDoodad() && !current_brush->oneSizeFitsAll()) {
		// Donkey!
		brush_shape = bs;
		FillDoodadPreviewBuffer();
		secondary_map = doodad_buffer_map;
	}
	brush_shape = bs;

	for(auto &palette : palettes) {
		palette->OnUpdateBrushSize(brush_shape, brush_size);
	}

	toolbar->UpdateBrushSize(brush_shape, brush_size);
}

void Editor::SetBrushThickness(bool on, int x, int y)
{
	use_custom_thickness = on;

	if(x != -1 || y != -1) {
		custom_thickness_mod = std::max<float>(x, 1.f) / std::max<float>(y, 1.f);
	}

	if(current_brush && current_brush->isDoodad()) {
		FillDoodadPreviewBuffer();
	}

	RefreshView();
}

void Editor::SetBrushThickness(int low, int ceil)
{
	custom_thickness_mod = std::max<float>(low, 1.f) / std::max<float>(ceil, 1.f);

	if(use_custom_thickness && current_brush && current_brush->isDoodad()) {
		FillDoodadPreviewBuffer();
	}

	RefreshView();
}

void Editor::DecreaseBrushSize(bool wrap)
{
	switch(brush_size) {
		case 0: {
			if(wrap) {
				SetBrushSize(11);
			}
			break;
		}
		case 1: {
			SetBrushSize(0);
			break;
		}
		case 2:
		case 3: {
			SetBrushSize(1);
			break;
		}
		case 4:
		case 5: {
			SetBrushSize(2);
			break;
		}
		case 6:
		case 7: {
			SetBrushSize(4);
			break;
		}
		case 8:
		case 9:
		case 10: {
			SetBrushSize(6);
			break;
		}
		case 11:
		default: {
			SetBrushSize(8);
			break;
		}
	}
}

void Editor::IncreaseBrushSize(bool wrap)
{
	switch(brush_size) {
		case 0: {
			SetBrushSize(1);
			break;
		}
		case 1: {
			SetBrushSize(2);
			break;
		}
		case 2:
		case 3: {
			SetBrushSize(4);
			break;
		}
		case 4:
		case 5: {
			SetBrushSize(6);
			break;
		}
		case 6:
		case 7: {
			SetBrushSize(8);
			break;
		}
		case 8:
		case 9:
		case 10: {
			SetBrushSize(11);
			break;
		}
		case 11:
		default: {
			if(wrap) {
				SetBrushSize(0);
			}
			break;
		}
	}
}

Brush* Editor::GetCurrentBrush() const
{
	return current_brush;
}

BrushShape Editor::GetBrushShape() const
{
	return brush_shape;
}

int Editor::GetBrushSize() const
{
	return brush_size;
}

int Editor::GetBrushVariation() const
{
	return brush_variation;
}

void Editor::SelectBrush()
{
	if(palettes.empty())
		return;

	SelectBrushInternal(palettes.front()->GetSelectedBrush());

	RefreshView();
}

bool Editor::SelectBrush(const Brush* whatbrush, PaletteType primary)
{
	if(palettes.empty())
		if(!CreatePalette())
			return false;

	if(!palettes.front()->OnSelectBrush(whatbrush, primary))
		return false;

	SelectBrushInternal(const_cast<Brush*>(whatbrush));
	toolbar->UpdateBrushButtons();
	return true;
}

void Editor::SelectBrushInternal(Brush* brush)
{
	// Fear no evil don't you say no evil
	if(current_brush != brush && brush)
		previous_brush = current_brush;

	current_brush = brush;
	if(!current_brush)
		return;

	brush_variation = std::min(brush_variation, brush->getMaxVariation());
	FillDoodadPreviewBuffer();
	if(brush->isDoodad())
		secondary_map = doodad_buffer_map;

	SetDrawingMode();
	RefreshView();
}

void Editor::SelectPreviousBrush()
{
	if(previous_brush)
		SelectBrush(previous_brush);
}

void Editor::FillDoodadPreviewBuffer()
{
	if(!current_brush || !current_brush->isDoodad())
		return;

	doodad_buffer_map->clear();

	DoodadBrush* brush = current_brush->asDoodad();
	if(brush->isEmpty(GetBrushVariation()))
		return;

	int object_count = 0;
	int area;
	if(GetBrushShape() == BRUSHSHAPE_SQUARE) {
		area = 2*GetBrushSize();
		area = area*area + 1;
	} else {
		if(GetBrushSize() == 1) {
			// There is a huge deviation here with the other formula.
			area = 5;
		} else {
			area = int(0.5 + GetBrushSize() * GetBrushSize() * rme::PI);
		}
	}
	const int object_range = (use_custom_thickness ? int(area*custom_thickness_mod) : brush->getThickness() * area / std::max(1, brush->getThicknessCeiling()));
	const int final_object_count = std::max(1, object_range + random(object_range));

	Position center_pos(0x8000, 0x8000, 0x8);

	if(brush_size > 0 && !brush->oneSizeFitsAll()) {
		while(object_count < final_object_count) {
			int retries = 0;
			bool exit = false;

			// Try to place objects 5 times
			while(retries < 5 && !exit) {

				int pos_retries = 0;
				int xpos = 0, ypos = 0;
				bool found_pos = false;
				if(GetBrushShape() == BRUSHSHAPE_CIRCLE) {
					while(pos_retries < 5 && !found_pos) {
						xpos = random(-brush_size, brush_size);
						ypos = random(-brush_size, brush_size);
						float distance = sqrt(float(xpos*xpos) + float(ypos*ypos));
						if(distance < GetBrushSize() + 0.005) {
							found_pos = true;
						} else {
							++pos_retries;
						}
					}
				} else {
					found_pos = true;
					xpos = random(-brush_size, brush_size);
					ypos = random(-brush_size, brush_size);
				}

				if(!found_pos) {
					++retries;
					continue;
				}

				// Decide whether the zone should have a composite or several single objects.
				bool fail = false;
				if(random(brush->getTotalChance(GetBrushVariation())) <= brush->getCompositeChance(GetBrushVariation())) {
					// Composite
					const auto &composites = brush->getComposite(GetBrushVariation());

					// Figure out if the placement is valid
					for(const auto &composite: composites) {
						Position pos = center_pos + composite.offset + Position(xpos, ypos, 0);
						if(Tile *tile = doodad_buffer_map->getTile(pos)) {
							if(!tile->empty()) {
								fail = true;
								break;
							}
						}
					}
					if(fail) {
						++retries;
						break;
					}

					// Transfer items to the stack
					for(const auto &composite: composites) {
						Position pos = center_pos + composite.offset + Position(xpos, ypos, 0);
						Tile *tile = doodad_buffer_map->getOrCreateTile(pos);
						for(const Item *item = composite.first; item != NULL; item = item->next){
							tile->addItem(item->deepCopy());
						}
					}
					exit = true;
				} else if(brush->hasSingleObjects(GetBrushVariation())) {
					Position pos = center_pos + Position(xpos, ypos, 0);
					Tile *tile = doodad_buffer_map->getOrCreateTile(pos);
					if(!tile->empty()){
						fail = true;
						break;
					}

					int variation = GetBrushVariation();
					brush->draw(doodad_buffer_map, tile, &variation);
					//std::cout << "\tpos: " << tile->getPosition() << std::endl;
					exit = true;
				}
				if(fail) {
					++retries;
					break;
				}
			}
			++object_count;
		}
	} else {
		if(brush->hasCompositeObjects(GetBrushVariation()) &&
				random(brush->getTotalChance(GetBrushVariation())) <= brush->getCompositeChance(GetBrushVariation())) {
			// All placement is valid...
			// Transfer items to the buffer
			for(const auto &composite : brush->getComposite(GetBrushVariation())) {
				Position pos = center_pos + composite.offset;
				Tile *tile = doodad_buffer_map->getOrCreateTile(pos);
				for(const Item *item = composite.first; item != NULL; item = item->next){
					tile->addItem(item->deepCopy());
				}
			}
		} else if(brush->hasSingleObjects(GetBrushVariation())) {
			Tile *tile = doodad_buffer_map->getOrCreateTile(center_pos);
			int variation = GetBrushVariation();
			brush->draw(doodad_buffer_map, tile, &variation);
		}
	}
}

int Editor::PopupDialog(wxWindow* parent, wxString title, wxString text, long style)
{
	if(text.empty())
		return wxID_ANY;

	wxMessageDialog dlg(parent, text, title, style);
	return dlg.ShowModal();
}

int Editor::PopupDialog(wxString title, wxString text, long style)
{
	return PopupDialog(root, title, text, style);
}

void Editor::ListDialog(wxWindow* parent, wxString title, const wxArrayString& param_items)
{
	if(param_items.empty())
		return;

	wxArrayString list_items(param_items);

	// Create the window
	wxDialog* dlg = newd wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX);

	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
	wxListBox* item_list = newd wxListBox(dlg, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE);
	item_list->SetMinSize(wxSize(500, 300));

	for(size_t i = 0; i != list_items.GetCount();) {
		wxString str = list_items[i];
		size_t pos = str.find("\n");
		if(pos != wxString::npos) {
			// Split string!
			item_list->Append(str.substr(0, pos));
			list_items[i] = str.substr(pos+1);
			continue;
		}
		item_list->Append(list_items[i]);
		++i;
	}
	sizer->Add(item_list, 1, wxEXPAND);

	wxSizer* stdsizer = newd wxBoxSizer(wxHORIZONTAL);
	stdsizer->Add(newd wxButton(dlg, wxID_OK, "OK"), wxSizerFlags(1).Center());
	sizer->Add(stdsizer, wxSizerFlags(0).Center());

	dlg->SetSizerAndFit(sizer);

	// Show the window
	dlg->ShowModal();
	delete dlg;
}

void Editor::ShowTextBox(wxWindow* parent, wxString title, wxString content)
{
	wxDialog* dlg = newd wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX);
	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);
	wxTextCtrl* text_field = newd wxTextCtrl(dlg, wxID_ANY, content, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
	text_field->SetMinSize(wxSize(400, 550));
	topsizer->Add(text_field, wxSizerFlags(5).Expand());

	wxSizer* choicesizer = newd wxBoxSizer(wxHORIZONTAL);
	choicesizer->Add(newd wxButton(dlg, wxID_CANCEL, "OK"), wxSizerFlags(1).Center());
	topsizer->Add(choicesizer, wxSizerFlags(0).Center());
	dlg->SetSizerAndFit(topsizer);

	dlg->ShowModal();
}

void Editor::SetHotkey(int index, Hotkey& hotkey)
{
	ASSERT(index >= 0 && index <= 9);
	hotkeys[index] = hotkey;
	SetStatusText("Set hotkey " + i2ws(index) + ".");
}

const Hotkey& Editor::GetHotkey(int index) const
{
	ASSERT(index >= 0 && index <= 9);
	return hotkeys[index];
}

void Editor::SaveHotkeys() const
{
	std::ostringstream os;
	for(const auto &hotkey : hotkeys) {
		os << hotkey << '\n';
	}
	g_settings.setString(Config::NUMERICAL_HOTKEYS, os.str());
}

void Editor::LoadHotkeys()
{
	std::istringstream is;
	is.str(g_settings.getString(Config::NUMERICAL_HOTKEYS));

	std::string line;
	int index = 0;
	while(getline(is, line)) {
		std::istringstream line_is;
		line_is.str(line);
		line_is >> hotkeys[index];

		++index;
	}
}

bool Editor::canUndo() const
{
	return actionQueue.canUndo();
}

bool Editor::canRedo() const
{
	return actionQueue.canRedo();
}

void Editor::undo(int numActions /*= 1*/)
{
	if(numActions <= 0 || !actionQueue.canUndo())
		return;

	while(numActions > 0) {
		if(!actionQueue.undo())
			break;
		numActions -= 1;
	}

	UpdateActions();
	RefreshView();
}

void Editor::redo(int numActions /*= 1*/)
{
	if(numActions <= 0 || !actionQueue.canRedo())
		return;

	while(numActions > 0) {
		if(!actionQueue.redo())
			break;
		numActions -= 1;
	}

	UpdateActions();
	RefreshView();
}

void Editor::updateActions()
{
	UpdateMenus();
	UpdateActions();
}

void Editor::resetActionsTimer()
{
	actionQueue.resetTimer();
}

void Editor::clearActions()
{
	actionQueue.clear();
	UpdateActions();
}

void Editor::borderizeSelection()
{
	if(selection.empty()) {
		SetStatusText("No items selected. Can't borderize.");
		return;
	}

	Action *action = actionQueue.createAction(ACTION_BORDERIZE);
	for(const Tile *tile : selection) {
		Tile newTile; newTile.deepCopy(*tile);
		newTile.borderize(&map);
		newTile.select();
		action->changeTile(std::move(newTile));
	}
	action->commit();
	updateActions();
}

void Editor::borderizeMap(bool showDialog)
{
	if(showDialog) {
		CreateLoadBar("Borderizing map...");
	}

	double nextUpdate = 0.0;
	map.forEachTile(
		[&nextUpdate, this, showDialog](Tile *tile, double progress){
			if(showDialog && progress >= nextUpdate){
				SetLoadDone((int)(progress * 100.0));
				nextUpdate = progress + 0.01;
			}

			tile->borderize(&map);
		});

	if(showDialog) {
		DestroyLoadBar();
	}
}

void Editor::randomizeSelection()
{
	if(selection.empty()) {
		SetStatusText("No items selected. Can't randomize.");
		return;
	}

	Action *action = actionQueue.createAction(ACTION_RANDOMIZE);
	for(const Tile *tile: selection) {
		GroundBrush *brush = tile->getGroundBrush();
		if(brush && brush->isReRandomizable()){
			Tile newTile; newTile.deepCopy(*tile);
			brush->draw(&map, &newTile, NULL);
			newTile.select();
			action->changeTile(std::move(newTile));
		}
	}
	action->commit();
	updateActions();
}

void Editor::randomizeMap(bool showDialog)
{
	if(showDialog) {
		CreateLoadBar("Randomizing map...");
	}

	double nextUpdate = 0.0;
	map.forEachTile(
		[&nextUpdate, this, showDialog](Tile *tile, double progress){
			if(showDialog && progress >= nextUpdate){
				SetLoadDone((int)(progress * 100.0));
				nextUpdate = progress + 0.01;
			}

			GroundBrush *brush = tile->getGroundBrush();
			if(brush && brush->isReRandomizable()){
				brush->draw(&map, tile, NULL);
			}
		});

	if(showDialog) {
		DestroyLoadBar();
	}
}

void Editor::clearInvalidHouseTiles(bool showDialog)
{
	if(showDialog) {
		CreateLoadBar("Clearing invalid house tiles...");
	}

	// TODO(fusion): See what happens.

	if(showDialog) {
		DestroyLoadBar();
	}
}

void Editor::checkTileItems(bool showDialog)
{
	if(showDialog){
		CreateLoadBar("Checking tile items...");
	}

	map.forEachTile(
		[this](Tile *tile, double progress){
			int numBank   = 0;
			int numBottom = 0;
			int numTop    = 0;
			for(Item *item = tile->items; item != NULL; item = item->next){
				if(item->getFlag(BANK))   numBank += 1;
				if(item->getFlag(BOTTOM)) numBottom += 1;
				if(item->getFlag(TOP))    numTop += 1;
			}

			if(numBank > 1 || numBottom > 1 || numTop > 1){
				wxString warning;
				warning << "Multiple unique items detected: ";
				if(numBank > 1)   warning << " " << numBank   << " BANK";
				if(numBottom > 1) warning << " " << numBottom << " BOTTOM";
				if(numTop > 1)    warning << " " << numTop    << " TOP";
				g_editor.Warning(warning, ProblemSource::FromPosition(tile->pos));
			}

			SetLoadDone((int)(progress * 100.0));
		});

	if(showDialog){
		DestroyLoadBar();
	}
}

void Editor::moveSelection(const Position& offset)
{
	if(!hasSelection()){
		return;
	}

	bool borderize = false;
	int drag_threshold = g_settings.getInteger(Config::BORDERIZE_DRAG_THRESHOLD);
	bool create_borders = g_settings.getInteger(Config::USE_AUTOMAGIC)
		&& g_settings.getInteger(Config::BORDERIZE_DRAG);

	std::vector<Tile> buffer;
	ActionGroup *group = actionQueue.createGroup(ACTION_MOVE);
	{
		// NOTE(fusion): Something similar to CopyBuffer::cut.
		Action *action = group->createAction();
		for(Tile *tile: selection) {
			Tile copy; copy.pos = tile->pos;
			Tile newTile; newTile.deepCopy(*tile);

			copy.addItems(newTile.popSelectedItems());

			if(newTile.creature && newTile.creature->isSelected()) {
				copy.creature = newTile.creature;
				newTile.creature = NULL;
			}

			if(copy.getFlag(BANK)){
				copy.houseId    = newTile.houseId;
				copy.flags      = newTile.flags;
				newTile.houseId = 0;
				newTile.flags   = 0;
				borderize = true;
			}

			buffer.push_back(std::move(copy));
			action->changeTile(std::move(newTile));
		}
		action->commit();
	}

	// Remove old borders (and create some new?)
	if(create_borders && selection.size() < static_cast<size_t>(drag_threshold)){
		Action *action = group->createAction();
		std::vector<Tile*> tilesToBorderize;
		// Go through all modified (selected) tiles (might be slow)
		for(const Tile &copy: buffer){
			for(int offsetY = -1; offsetY <= 1; offsetY += 1)
			for(int offsetX = -1; offsetX <= 1; offsetX += 1){
				if(offsetX == 0 && offsetY == 0){
					continue;
				}

				if(Tile *neighbor = map.getTile(copy.pos.x + offsetX, copy.pos.y + offsetY, copy.pos.z)){
					if(!neighbor->isSelected()){
						tilesToBorderize.push_back(neighbor);
					}
				}
			}
		}

		VectorSortUnique(tilesToBorderize);

		// Create borders
		for(const Tile *tile: tilesToBorderize){
			Tile newTile; newTile.deepCopy(*tile);

			if(borderize) {
				newTile.borderize(&map);
			}
			newTile.wallize(&map);
			newTile.tableize(&map);
			newTile.carpetize(&map);

			if(Item *ground = newTile.getFirstItem(BANK)){
				if(ground->isSelected()){
					newTile.selectGround();
				}
			}

			action->changeTile(std::move(newTile));
		}
		action->commit();
	}

	{ // New action for adding the destination tiles
		Action *action = group->createAction();
		for(Tile &copy: buffer){
			// TODO(fusion): Why are we subtracting the offset?
			copy.pos = (copy.pos - offset);
			if(!copy.pos.isValid()){
				continue;
			}

			// NOTE(fusion): Something similar to CopyBuffer::paste.
			if(g_settings.getBoolean(Config::MERGE_MOVE) || !copy.getFlag(BANK)){
				if(Tile *tile = map.getTile(copy.pos)){
					Tile tmp = std::move(copy);
					copy.deepCopy(*tile);
					copy.merge(std::move(tmp));
				}
			}

			action->changeTile(std::move(copy));
		}
		action->commit();
	}

	if(create_borders && selection.size() < static_cast<size_t>(drag_threshold)) {
		Action *action = group->createAction();
		std::vector<Tile*> tilesToBorderize;
		// Go through all modified (selected) tiles (might be slow)
		for(Tile *tile: selection){
			bool addMe = false;
			for(int offsetY = -1; offsetY <= 1; offsetY += 1)
			for(int offsetX = -1; offsetX <= 1; offsetX += 1){
				if(offsetX == 0 && offsetY == 0){
					continue;
				}

				if(Tile *neighbor = map.getTile(tile->pos.x + offsetX, tile->pos.y + offsetY, tile->pos.z)){
					if(!neighbor->isSelected()){
						tilesToBorderize.push_back(neighbor);
						addMe = true;
					}
				}
			}

			if(addMe){
				tilesToBorderize.push_back(tile);
			}
		}

		VectorSortUnique(tilesToBorderize);

		// Create borders
		for(Tile *tile: tilesToBorderize) {
			if(!tile->getGroundBrush()){
				continue;
			}

			Tile newTile; newTile.deepCopy(*tile);
			if(borderize){
				newTile.borderize(&map);
			}
			newTile.wallize(&map);
			newTile.tableize(&map);
			newTile.carpetize(&map);

			if(Item *ground = tile->getFirstItem(BANK)){
				if(ground->isSelected()){
					newTile.selectGround();
				}
			}

			action->changeTile(std::move(newTile));
		}
		action->commit();
	}

	updateActions();
	selection.updateSelectionCount();
}

void Editor::destroySelection()
{
	if(selection.size() == 0) {
		SetStatusText("No selected items to delete.");
		return;
	}

	int numTiles = 0;
	int numItems = 0;
	std::vector<Position> tilesToBorderize;

	ActionGroup *group = actionQueue.createGroup(ACTION_DELETE_TILES);

	{
		Action *action = group->createAction();
		for(const Tile *tile: selection){
			Tile newTile; newTile.deepCopy(*tile);
			newTile.removeItems([](const Item *item){ return item->isSelected(); });

			if(newTile.creature && newTile.creature->isSelected()) {
				delete newTile.creature;
				newTile.creature = nullptr;
			}

			if(g_settings.getInteger(Config::USE_AUTOMAGIC)) {
				for(int offsetY = -1; offsetY <= 1; offsetY += 1)
				for(int offsetX = -1; offsetX <= 1; offsetX += 1){
					tilesToBorderize.push_back(Position(newTile.pos.x + offsetX, newTile.pos.y + offsetY, newTile.pos.z));
				}
			}

			action->changeTile(std::move(newTile));
			numTiles += 1;
		}
		action->commit();
	}

	if(g_settings.getInteger(Config::USE_AUTOMAGIC)) {
		VectorSortUnique(tilesToBorderize);

		Action *action = group->createAction();
		for(const Position &pos: tilesToBorderize){
			Tile newTile;
			if(Tile *tile = map.getTile(pos)){
				newTile.deepCopy(*tile);
				newTile.borderize(&map);
				newTile.wallize(&map);
				newTile.tableize(&map);
				newTile.carpetize(&map);
				action->changeTile(std::move(newTile));
			}else{
				newTile.pos = pos;
				newTile.borderize(&map);
				if(!newTile.empty()){
					action->changeTile(std::move(newTile));
				}
			}
		}
		action->commit();
	}

	updateActions();
	SetStatusText(wxString() << "Deleted " << numTiles << " tile"
			<< (numTiles == 1 ? "" : "s") <<  " (" << numItems << " item"
			<< (numItems == 1 ? "" : "s") << ")");
}

// Macro to avoid useless code repetition
static void doSurroundingBorders(DoodadBrush *brush, std::vector<Position> &tilesToBorderize, Tile *bufferTile, Tile *newTile)
{
	if(g_settings.getInteger(Config::USE_AUTOMAGIC) && brush->doNewBorders()) {
		Position pos = newTile->pos;
		tilesToBorderize.push_back(pos);
		if(bufferTile->getFlag(BANK)) {
			for(int offsetY = -1; offsetY <= 1; offsetY += 1)
			for(int offsetX = -1; offsetX <= 1; offsetX += 1){
				tilesToBorderize.push_back(Position(pos.x + offsetX, pos.y + offsetY, pos.z));
			}
		} else if(bufferTile->getWall()) {
			tilesToBorderize.push_back(Position(pos.x    , pos.y - 1, pos.z));
			tilesToBorderize.push_back(Position(pos.x - 1, pos.y    , pos.z));
			tilesToBorderize.push_back(Position(pos.x + 1, pos.y    , pos.z));
			tilesToBorderize.push_back(Position(pos.x    , pos.y + 1, pos.z));
		}
	}
}

static void removeDuplicateWalls(Tile *bufferTile, Tile *newTile)
{
	if(!bufferTile || !bufferTile->items || !newTile || !newTile->items){
		return;
	}

	for(const Item *item = bufferTile->items; item != NULL; item = item->next){
		if(WallBrush *brush = item->getWallBrush()){
			newTile->removeWalls(brush);
		}
	}
}

void Editor::drawInternal(Position offset, bool alt, bool dodraw)
{
	Brush* brush = GetCurrentBrush();
	if(!brush) {
		return;
	}

	if(brush->isDoodad()) {
		ActionGroup *group = actionQueue.createGroup(ACTION_DRAW, 2);
		std::vector<Position> tilesToBorderize;
		{
			Action* action = group->createAction();
			DoodadBrush *doodadBrush = brush->asDoodad();
			Position deltaPos = offset - Position(0x8000, 0x8000, 0x8);
			doodad_buffer_map->forEachTile(
				[this, action, doodadBrush, deltaPos, &tilesToBorderize, alt](Tile *bufferTile, double progress){
					Position pos = (bufferTile->pos + deltaPos);
					if(!pos.isValid()){
						return;
					}

					// TODO(fusion): We could probably simplify this a little bit further.
					Tile *tile = map.getTile(pos);
					if(tile && (alt || doodadBrush->placeOnBlocking() || !tile->getFlag(UNPASS))){
						bool place = true;
						if(!doodadBrush->placeOnDuplicate() && !alt){
							for(const Item *item = tile->items; item != NULL; item = item->next){
								if(doodadBrush->ownsItem(item)){
									place = false;
									break;
								}
							}
						}

						if(place){
							Tile newTile; newTile.deepCopy(*tile);
							removeDuplicateWalls(bufferTile, &newTile);
							doSurroundingBorders(doodadBrush, tilesToBorderize, bufferTile, &newTile);
							newTile.mergeCopy(*bufferTile);
							action->changeTile(std::move(newTile));
						}
					}else if(!tile && (alt || doodadBrush->placeOnBlocking())){
						Tile newTile; newTile.pos = pos;
						removeDuplicateWalls(bufferTile, &newTile);
						doSurroundingBorders(doodadBrush, tilesToBorderize, bufferTile, &newTile);
						newTile.mergeCopy(*bufferTile);
						action->changeTile(std::move(newTile));
					}
				});
			action->commit();
		}

		if(!tilesToBorderize.empty()){
			Action* action = group->createAction();
			VectorSortUnique(tilesToBorderize);
			for(Position pos: tilesToBorderize){
				if(Tile* tile = map.getTile(pos)){
					Tile newTile; newTile.deepCopy(*tile);
					newTile.borderize(&map);
					newTile.wallize(&map);
					action->changeTile(std::move(newTile));
				}
			}
			action->commit();
		}

	} else if(brush->isHouseExit()) {
#if TODO
		HouseExitBrush* house_exit_brush = brush->asHouseExit();
		if(!house_exit_brush->canDraw(&map, offset))
			return;

		House* house = map.houses.getHouse(house_exit_brush->getHouseID());
		if(!house)
			return;

		BatchAction* batch = actionQueue->createBatch(ACTION_DRAW);
		Action* action = actionQueue->createAction(batch);
		action->addChange(Change::Create(house, offset));
		batch->addAndCommitAction(action);
		addBatch(batch, 2);
#endif
	} else if(brush->isWaypoint()) {
#if TODO
		WaypointBrush* waypoint_brush = brush->asWaypoint();
		if(!waypoint_brush->canDraw(&map, offset))
			return;

		Waypoint* waypoint = map.waypoints.getWaypoint(waypoint_brush->getWaypoint());
		if(!waypoint || waypoint->pos == offset)
			return;

		BatchAction* batch = actionQueue->createBatch(ACTION_DRAW);
		Action* action = actionQueue->createAction(batch);
		action->addChange(Change::Create(waypoint, offset));
		batch->addAndCommitAction(action);
		addBatch(batch, 2);
#endif
	} else if(brush->isWall()) {
		Action *action = actionQueue.createAction(dodraw ? ACTION_DRAW : ACTION_ERASE, 2);

		Tile newTile;
		if(Tile *tile = map.getTile(offset)){
			newTile.deepCopy(*tile);
		}else{
			newTile.pos = offset;
		}

		if(dodraw) {
			bool b = true;
			brush->asWall()->draw(&map, &newTile, &b);
		} else {
			brush->asWall()->undraw(&map, &newTile);
		}

		action->changeTile(std::move(newTile));
		action->commit();
	} else if(brush->isCreature()) {
		Action *action = actionQueue.createAction(dodraw ? ACTION_DRAW : ACTION_ERASE, 2);

		Tile newTile;
		if(Tile *tile = map.getTile(offset)){
			newTile.deepCopy(*tile);
		}else{
			newTile.pos = offset;
		}

		if(dodraw) {
			brush->draw(&map, &newTile);
		} else {
			brush->undraw(&map, &newTile);
		}

		action->changeTile(std::move(newTile));
		action->commit();
	}
}

void Editor::drawInternal(const std::vector<Position> &tilesToDraw, bool alt, bool dodraw)
{
	Brush* brush = GetCurrentBrush();
	if(!brush){
		return;
	}

#ifdef _DEBUG
	if(brush->isGround() || brush->isWall()){
		// Wrong function, end call
		return;
	}
#endif

	Action *action = actionQueue.createAction(dodraw ? ACTION_DRAW : ACTION_ERASE, 2);
	if(brush->isOptionalBorder()){
		for(Position pos: tilesToDraw){
			if(Tile *tile = map.getTile(pos)){
				Tile newTile; newTile.deepCopy(*tile);
				if(dodraw) {
					brush->draw(&map, &newTile);
				} else if(!dodraw && tile->hasOptionalBorder()) {
					brush->undraw(&map, &newTile);
				}
				newTile.borderize(&map);
				action->changeTile(std::move(newTile));
			}else if(dodraw){
				Tile newTile; newTile.pos = pos;
				brush->draw(&map, &newTile);
				newTile.borderize(&map);
				if(!newTile.empty()){
					action->changeTile(std::move(newTile));
				}
			}
		}
	} else {
		for(Position pos: tilesToDraw){
			if(Tile *tile = map.getTile(pos)) {
				Tile newTile; newTile.deepCopy(*tile);
				if(dodraw){
					brush->draw(&map, &newTile, &alt);
				}else{
					brush->undraw(&map, &newTile);
				}
				action->changeTile(std::move(newTile));
			}else if(dodraw){
				Tile newTile; newTile.pos = pos;
				brush->draw(&map, &newTile, &alt);
				if(!newTile.empty()){
					action->changeTile(std::move(newTile));
				}
			}
		}
	}
	action->commit();
}

void Editor::drawInternal(const std::vector<Position> &tilesToDraw,
		std::vector<Position> &tilesToBorderize, bool alt, bool dodraw)
{
	Brush *brush = GetCurrentBrush();
	if(!brush){
		return;
	}

	if(brush->isGround() || brush->isEraser()){
		ActionType type = (dodraw && !brush->isEraser()) ? ACTION_DRAW : ACTION_ERASE;
		ActionGroup *group = actionQueue.createGroup(type, 2);

		{
			Action *action = group->createAction();
			for(Position pos: tilesToDraw){
				if(Tile *tile = map.getTile(pos)){
					Tile newTile; newTile.deepCopy(*tile);

					if(g_settings.getInteger(Config::USE_AUTOMAGIC)) {
						newTile.removeBorders();
					}

					if (dodraw) {
						brush->draw(&map, &newTile, nullptr);
					} else {
						brush->undraw(&map, &newTile);
						tilesToBorderize.push_back(pos);
					}

					action->changeTile(std::move(newTile));
				} else if(dodraw) {
					Tile newTile; newTile.pos = pos;
					brush->draw(&map, &newTile, nullptr);
					if(!newTile.empty()){
						action->changeTile(std::move(newTile));
					}
				}
			}
			action->commit();
		}

		if(g_settings.getInteger(Config::USE_AUTOMAGIC)){
			Action *action = group->createAction();
			for(Position pos: tilesToBorderize){
				if(Tile *tile = map.getTile(pos)){
					Tile newTile; newTile.deepCopy(*tile);
					if(brush->isEraser()){
						newTile.wallize(&map);
						newTile.tableize(&map);
						newTile.carpetize(&map);
					}
					newTile.borderize(&map);
					action->changeTile(std::move(newTile));
				}else{
					Tile newTile; newTile.pos = pos;
					newTile.borderize(&map);
					if(!newTile.empty()){
						action->changeTile(std::move(newTile));
					}
				}
			}
			action->commit();
		}
	}else if(brush->isTable() || brush->isCarpet()){
		ActionGroup *group = actionQueue.createGroup(ACTION_DRAW, 2);

		{
			Action *action = group->createAction();
			for(Position pos: tilesToDraw){
				if(Tile *tile = map.getTile(pos)){
					Tile newTile; newTile.deepCopy(*tile);
					if(dodraw){
						brush->draw(&map, &newTile, nullptr);
					}else{
						brush->undraw(&map, &newTile);
					}
					action->changeTile(std::move(newTile));
				}else if(dodraw){
					Tile newTile; newTile.pos = pos;
					brush->draw(&map, &newTile, nullptr);
					if(!newTile.empty()){
						action->changeTile(std::move(newTile));
					}
				}
			}
			action->commit();
		}

		{
			Action *action = group->createAction();
			for(Position pos: tilesToBorderize){
				if(Tile *tile = map.getTile(pos)){
					if(brush->isTable() && tile->getTable()){
						Tile newTile; newTile.deepCopy(*tile);
						newTile.tableize(&map);
						action->changeTile(std::move(newTile));
					}else if(brush->isCarpet() && tile->getCarpet()){
						Tile newTile; newTile.deepCopy(*tile);
						newTile.carpetize(&map);
						action->changeTile(std::move(newTile));
					}
				}
			}
			action->commit();
		}
	}else if(brush->isWall()){
		ActionGroup *group = actionQueue.createGroup(ACTION_DRAW, 2);
		if(alt && dodraw){
			// NOTE(fusion): This branch is almost the same as the one below, except
			// that we're using a map buffer, probably to avoid external walls from
			// from altering the result of Tile::wallize.
			Map *buffer = doodad_buffer_map;
			buffer->clear();

			for(Position pos: tilesToDraw){
				Tile *bufferTile = buffer->getOrCreateTile(pos);
				if(Tile *tile = map.getTile(pos)){
					bufferTile->deepCopy(*tile);
					bufferTile->removeWalls();
				}
				brush->draw(buffer, bufferTile);
			}

			{
				Action *action = group->createAction();
				for(Position pos: tilesToDraw){
					if(Tile *bufferTile = buffer->getTile(pos)){
						bufferTile->wallize(buffer);

						// TODO(fusion): We could probably move the tile directly
						// here but then it would probably mess with the wallization
						// of the other tiles...
						Tile newTile; newTile.deepCopy(*bufferTile);
						action->changeTile(std::move(newTile));
					}
				}
				action->commit();
			}

			buffer->clear();
		}else{
			{
				Action *action = group->createAction();
				for(Position pos: tilesToDraw){
					if(Tile *tile = map.getTile(pos)){
						Tile newTile; newTile.deepCopy(*tile);
						newTile.removeWalls();
						if(dodraw){
							brush->draw(&map, &newTile);
						}else{
							brush->undraw(&map, &newTile);
						}
						action->changeTile(std::move(newTile));
					}else if(dodraw){
						Tile newTile; newTile.pos = pos;
						brush->draw(&map, &newTile);
						if(!newTile.empty()){
							action->changeTile(std::move(newTile));
						}
					}
				}
				action->commit();
			}

			if(g_settings.getInteger(Config::USE_AUTOMAGIC)){
				Action *action = group->createAction();
				for(Position pos: tilesToBorderize){
					if(Tile *tile = map.getTile(pos)){
						Tile newTile; newTile.deepCopy(*tile);
						newTile.wallize(&map);
						action->changeTile(std::move(newTile));
					}
				}
				action->commit();
			}
		}
	}else if(brush->isDoor()){
		ActionGroup *group = actionQueue.createGroup(ACTION_DRAW, 2);

		{
			Action *action = group->createAction();
			for(Position pos: tilesToDraw){
				if(Tile *tile = map.getTile(pos)){
					Tile newTile; newTile.deepCopy(*tile);
					if(dodraw){
						brush->draw(&map, &newTile, &alt);
					}else{
						brush->undraw(&map, &newTile);
					}
					action->changeTile(std::move(newTile));
				}else if(dodraw){
					Tile newTile; newTile.pos = pos;
					brush->draw(&map, &newTile, &alt);
					if(!newTile.empty()){
						action->changeTile(std::move(newTile));
					}
				}
			}
			action->commit();
		}

		if(g_settings.getInteger(Config::USE_AUTOMAGIC)) {
			Action *action = group->createAction();
			for(Position pos: tilesToBorderize){
				if(Tile *tile = map.getTile(pos)) {
					Tile newTile; newTile.deepCopy(*tile);
					newTile.wallize(&map);
					action->changeTile(std::move(newTile));
				}
			}
			action->commit();
		}
	}else{
		Action *action = actionQueue.createAction(ACTION_DRAW, 2);
		for(Position pos: tilesToDraw){
			if(Tile *tile = map.getTile(pos)) {
				Tile newTile; newTile.deepCopy(*tile);
				if(dodraw){
					brush->draw(&map, &newTile);
				}else{
					brush->undraw(&map, &newTile);
				}
				action->changeTile(std::move(newTile));
			}else if(dodraw){
				Tile newTile; newTile.pos = pos;
				brush->draw(&map, &newTile);
				if(!newTile.empty()){
					action->changeTile(std::move(newTile));
				}
			}
		}
		action->commit();
	}
}

std::ostream& operator<<(std::ostream& os, const Hotkey& hotkey)
{
	switch(hotkey.type) {
		case Hotkey::POSITION: {
			os << "pos:{" << hotkey.pos << "}";
		} break;
		case Hotkey::BRUSH: {
			if(hotkey.brushname.find('{') != std::string::npos ||
					hotkey.brushname.find('}') != std::string::npos) {
				break;
			}
			os << "brush:{" << hotkey.brushname << "}";
		} break;
		default: {
			os << "none:{}";
		} break;
	}
	return os;
}

std::istream& operator>>(std::istream& is, Hotkey& hotkey)
{
	std::string type;
	getline(is, type, ':');
	if(type == "none") {
		is.ignore(2); // ignore "{}"
	} else if(type == "pos") {
		is.ignore(1); // ignore "{"
		Position pos;
		is >> pos;
		hotkey = Hotkey(pos);
		is.ignore(1); // ignore "}"
	} else if(type == "brush") {
		is.ignore(1); // ignore "{"
		std::string brushname;
		getline(is, brushname, '}');
		hotkey = Hotkey(brushname);
	} else {
		// Do nothing...
	}

	return is;
}

