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

#ifndef RME_EDITOR_H_
#define RME_EDITOR_H_

#include "main.h"
#include "action.h"
#include "brush_enums.h"
#include "copybuffer.h"
#include "dcbutton.h"
#include "graphics.h"
#include "wxids.h"
#include "main_menubar.h"
#include "main_toolbar.h"
#include "map.h"
#include "palette_window.h"
#include "problems_window.h"
#include "position.h"
#include "selection.h"

class Brush;
class HouseBrush;
class HouseExitBrush;
class WaypointBrush;
class OptionalBorderBrush;
class EraserBrush;
class DoorBrush;
class FlagBrush;

class MainFrame;
class WelcomeDialog;
class MapWindow;
class MapCanvas;

class SearchResultWindow;
class DuplicatedItemsWindow;
class MinimapWindow;
class ActionsHistoryWindow;
class PaletteWindow;
class ItemPropertyWindow;
class CreaturePropertyWindow;
class EditTownsDialog;
class ItemButton;

enum ImportType
{
	IMPORT_DONT,
	IMPORT_MERGE,
	IMPORT_SMART_MERGE,
	IMPORT_INSERT,
};

class Hotkey
{
public:
	Hotkey(void)                   : type(NONE) {}
	Hotkey(Position pos_)          : type(POSITION), pos(pos_) {}
	Hotkey(Brush *brush)           : type(BRUSH), brushname(brush->getName()) {}
	Hotkey(std::string brushname_) : type(BRUSH), brushname(std::move(brushname_)) {}

	bool IsPosition() const { return type == POSITION; }
	bool IsBrush() const { return type == BRUSH; }
	Position GetPosition() const { ASSERT(IsPosition()); return pos; }
	std::string GetBrushname() const { ASSERT(IsBrush()); return brushname; }

private:
	enum {
		NONE,
		POSITION,
		BRUSH,
	} type = NONE;
	Position pos = {};
	std::string brushname = {};

	friend std::ostream& operator<<(std::ostream& os, const Hotkey& hotkey);
	friend std::istream& operator>>(std::istream& os, Hotkey& hotkey);
};

class Editor
{
public:
	Editor(void);
	~Editor(void);

	// non-copyable
	Editor(const Editor&) = delete;
	Editor& operator=(const Editor&) = delete;

	void LoadRecentFiles();
	void SaveRecentFiles();
	void AddRecentFile(const wxString &file);
	std::vector<wxString> GetRecentFiles();

	void SaveWindow();
	void LoadWindow();
	void SavePerspective();
	void LoadPerspective();

	void CreateLoadBar(wxString message, bool canCancel = false);
	void DestroyLoadBar();
	bool SetLoadDone(int done, const wxString& newMessage = "");
	void SetLoadScale(int from, int to);

	void ShowWelcomeDialog(const wxBitmap &icon);
	void FinishWelcomeDialog();
    bool IsWelcomeDialogShown();

	void UpdateMenubar();
	bool IsRenderingEnabled() const { return disabled_counter == 0; }

	// TODO(fusion): Use a counter here?
	void EnableHotkeys() { hotkeys_enabled = true; }
	void DisableHotkeys() { hotkeys_enabled = false; }
	bool AreHotkeysEnabled() const { return hotkeys_enabled; }

	void AddPendingMapEvent(wxEvent &event);
	void AddPendingCanvasEvent(wxEvent &event);

    void OnWelcomeDialogClosed(wxCloseEvent &event);
    void OnWelcomeDialogAction(wxCommandEvent &event);

protected:
	void DisableRendering() { ++disabled_counter; }
	void EnableRendering() { --disabled_counter; }

public:
	void SetTitle(wxString newtitle);
	void UpdateMenus();
	void UpdateActions();
	void RefreshActions();
	void ShowToolbar(ToolBarID id, bool show);
	void SetStatusText(wxString text);

	int PopupDialog(wxWindow* parent, wxString title, wxString text, long style);
	int PopupDialog(wxString title, wxString text, long style);

	void ListDialog(wxWindow* parent, wxString title, const wxArrayString& vec);
	void ListDialog(const wxString& title, const wxArrayString& vec) { ListDialog(nullptr, title, vec); }

	void ShowTextBox(wxWindow* parent, wxString title, wxString contents);
	void ShowTextBox(const wxString& title, const wxString& contents) { ShowTextBox(nullptr, title, contents); }

	// Get the current GL context
	// Param is required if the context is to be created.
	wxGLContext &GetGLContext(wxGLCanvas* win);

	// Search Results
	SearchResultWindow* ShowSearchWindow();
	void HideSearchWindow();

	DuplicatedItemsWindow* ShowDuplicatedItemsWindow();
	void HideDuplicatedItemsWindow();

	ActionsHistoryWindow* ShowActionsWindow();
	void HideActionsWindow();

	ProblemsWindow *ShowProblemsWindow();
	void ClearProblems(void);
	void Notice(wxString message, ProblemSource source = {});
	void Warning(wxString message, ProblemSource source = {});
	void Error(wxString message, ProblemSource source = {});

	// Minimap
	void CreateMinimap();
	void HideMinimap();
	void DestroyMinimap();
	void UpdateMinimap(bool immediate = false);
	bool IsMinimapVisible() const;

	int GetCurrentFloor();
	void ChangeFloor(int newfloor);

	double GetCurrentZoom();
	void SetCurrentZoom(double zoom);

	void SwitchMode();
	void SetSelectionMode();
	void SetDrawingMode();
	bool IsSelectionMode() const { return mode == SELECTION_MODE; }
	bool IsDrawingMode() const { return mode == DRAWING_MODE; }

	void SetHotkey(int index, Hotkey& hotkey);
	const Hotkey& GetHotkey(int index) const;
	void SaveHotkeys() const;
	void LoadHotkeys();

	// Brushes
	void FillDoodadPreviewBuffer();
	// Selects the currently seleceted brush in the active palette
	void SelectBrush();
	// Updates the palette AND selects the brush, second parameter is first palette to look in
	// Returns true if the brush was found and selected
	bool SelectBrush(const Brush* brush, PaletteType pt = TILESET_UNKNOWN);
	// Selects the brush selected before the current brush
	void SelectPreviousBrush();
	// Only selects the brush, doesn't update the palette
	void SelectBrushInternal(Brush* brush);
	// Get different brush parameters
	Brush* GetCurrentBrush() const;
	BrushShape GetBrushShape() const;
	int GetBrushSize() const;
	int GetBrushVariation() const;

	// Additional brush parameters
	void SetBrushSize(int nz);
	void SetBrushSizeInternal(int nz);
	void SetBrushShape(BrushShape bs);
	void SetBrushVariation(int nz);
	void SetBrushThickness(int low, int ceil);
	void SetBrushThickness(bool on, int low = -1, int ceil = -1);
	// Helper functions for size
	void DecreaseBrushSize(bool wrap = false);
	void IncreaseBrushSize(bool wrap = false);

	// Centers current view on position
	void SetScreenCenterPosition(const Position& position, bool showIndicator = true);
	// Refresh the view canvas
	void RefreshView();

	void DoCut();
	void DoCopy();
	void DoPaste();
	void PreparePaste();
	void StartPasting();
	void EndPasting();
	bool IsPasting() const { return pasting; }

	void DoUndo(int numActions = 1);
	void DoRedo(int numActions = 1);

	bool NewProject(void);
	bool OpenProject(void);
	bool OpenProject(const wxString &dir);
	bool CloseProject(void);
	void SaveProject(void);
	void SaveProjectAs(void);
	bool ReloadProject(void);
	bool IsProjectOpen(void) const;
	bool IsProjectDirty(void) const;
	bool ExportPatch(const wxString &filename, bool commit = false);

protected:
	bool LoadProject(wxString dir);
	void UnloadProject(void);

public:
	//=========================================================================
	// Palette Interface
	//=========================================================================
	// Spawn a newd palette
	PaletteWindow* NewPalette();
	// Bring this palette to the front (as the 'active' palette)
	void ActivatePalette(PaletteWindow* p);
	// Rebuild forces palette to reload the entire contents
	void RebuildPalettes();
	// Refresh only updates the content (such as house/waypoint list)
	void RefreshPalettes();
	// Won't refresh the palette in the parameter
	void RefreshOtherPalettes(PaletteWindow* p);
	// If no palette is shown, this displays the primary palette
	// else does nothing.
	void ShowPalette();
	// Select a particular page on the primary palette
	void SelectPalettePage(PaletteType pt);

	// Returns primary palette
	PaletteWindow* GetPalette();
	// Returns list of all palette, first in the list is primary
	const std::list<PaletteWindow*>& GetPalettes();

protected:
	// Hidden from public view (?)
	void DestroyPalettes();
	PaletteWindow* CreatePalette();

public:
	//=========================================================================
	// Drawing Interface
	//=========================================================================
	bool hasChanges() const;
	void clearChanges();

	bool canUndo() const;
	bool canRedo() const;
	void undo(int numActions = 1);
	void redo(int numActions = 1);
	void updateActions();
	void resetActionsTimer();
	void clearActions();

	// Selection
	Selection& getSelection() noexcept { return selection; }
	const Selection& getSelection() const noexcept { return selection; }
	bool hasSelection() const noexcept { return selection.size() != 0; }
	// Some simple actions that work on the map (these will work through the undo queue)
	// Moves the selected area by the offset
	void moveSelection(const Position& offset);
	// Deletes all selected items
	void destroySelection();
	// Borderizes the selected region
	void borderizeSelection();
	// Randomizes the ground in the selected region
	void randomizeSelection();

	// Same as above although it applies to the entire map
	// action queue is flushed when these functions are called
	// showdialog is whether a progress bar should be shown
	void borderizeMap(bool showDialog);
	void randomizeMap(bool showDialog);
	void clearInvalidHouseTiles(bool showDialog);
	void checkMap(bool showDialog);

	// Draw using the current brush to the target position
	// alt is whether the ALT key is pressed
	void draw(const Position& offset, bool alt){
		drawInternal(offset, alt, true);
	}

	void undraw(const Position& offset, bool alt){
		drawInternal(offset, alt, false);
	}

	void draw(const std::vector<Position> &tilesToDraw, bool alt){
		drawInternal(tilesToDraw, alt, true);
	}

	void draw(const std::vector<Position> &tilesToDraw,
			std::vector<Position> &tilesToBorder, bool alt){
		drawInternal(tilesToDraw, tilesToBorder, alt, true);
	}

	void undraw(const std::vector<Position> &tilesToDraw, bool alt){
		drawInternal(tilesToDraw, alt, false);
	}

	void undraw(const std::vector<Position> &tilesToDraw,
			std::vector<Position> &tilesToBorder, bool alt){
		drawInternal(tilesToDraw, tilesToBorder, alt, false);
	}

protected:
	void drawInternal(Position offset, bool alt, bool dodraw);
	void drawInternal(const std::vector<Position> &tilesToDraw, bool alt, bool dodraw);
	void drawInternal(const std::vector<Position> &tilesToDraw,
			std::vector<Position> &tilesToBorder, bool alt, bool dodraw);

public:
	//=========================================================================
	// Public members
	//=========================================================================
	wxString projectDir = {};
	wxFileHistory recentFiles = {};

	Map map = {};
	GraphicManager gfx = {};
	CopyBuffer copybuffer = {};
	ActionQueue actionQueue = {};
	Selection selection = {};

	WelcomeDialog *welcomeDialog = NULL;
	MainFrame *root = NULL;
	wxAuiManager *aui_manager = NULL;
	MainMenuBar *menubar = NULL;
	MainToolBar *toolbar = NULL;
	MapWindow *mapWindow = NULL;
	MinimapWindow *minimap = NULL;
	SearchResultWindow *search_result_window = NULL;
	DuplicatedItemsWindow *duplicated_items_window = NULL;
	ActionsHistoryWindow *actions_history_window = NULL;
	ProblemsWindow *problems_window = NULL;

	Map *secondary_map = NULL; // A buffer map
	Map *doodad_buffer_map = NULL; // The map in which doodads are temporarily stored

	//=========================================================================
	// Brush references
	//=========================================================================
	HouseBrush *house_brush = NULL;
	HouseExitBrush *house_exit_brush = NULL;
	WaypointBrush *waypoint_brush = NULL;
	OptionalBorderBrush *optional_brush = NULL;
	EraserBrush *eraser = NULL;
	DoorBrush *normal_door_brush = NULL;
	DoorBrush *locked_door_brush = NULL;
	DoorBrush *magic_door_brush = NULL;
	DoorBrush *quest_door_brush = NULL;
	DoorBrush *hatch_door_brush = NULL;
	DoorBrush *window_door_brush = NULL;
	FlagBrush *refresh_brush = NULL;
	FlagBrush *nolog_brush = NULL;
	FlagBrush *pz_brush = NULL;

protected:
	//=========================================================================
	// Global editor state (?)
	//=========================================================================
	wxGLContext *OGLContext = NULL;
	EditorMode mode = SELECTION_MODE;
	bool pasting = false;
	bool hotkeys_enabled = false;
	Hotkey hotkeys[10] = {};
	std::list<PaletteWindow*> palettes;

	//=========================================================================
	// Internal brush data
	//=========================================================================
	Brush *current_brush = NULL;
	Brush *previous_brush = NULL;
	BrushShape brush_shape = BRUSHSHAPE_SQUARE;
	int brush_size = 0;
	int brush_variation = 0;
	bool use_custom_thickness = false;
	float custom_thickness_mod = 0.0f;

	//=========================================================================
	// Progress bar tracking
	//=========================================================================
	wxStopWatch progressTimer = {};
	wxString progressText = {};
	int progressFrom = 0;
	int progressTo = 0;
	wxGenericProgressDialog *progressBar = NULL;

	// ??
	wxWindowDisabler* winDisabler = NULL;
	int disabled_counter = 0;

	friend class RenderingLock;
};

extern Editor g_editor;

class RenderingLock
{
public:
	RenderingLock(void)  { g_editor.DisableRendering(); }
	~RenderingLock(void) { g_editor.EnableRendering(); }
};

class ScopedLoadingBar
{
public:
	ScopedLoadingBar(wxString message, bool canCancel = false)
	{
		g_editor.CreateLoadBar(message, canCancel);
	}
	~ScopedLoadingBar()
	{
		g_editor.DestroyLoadBar();
	}

	void SetLoadDone(int32_t done, const wxString& newmessage = wxEmptyString)
	{
		g_editor.SetLoadDone(done, newmessage);
	}

	void SetLoadScale(int32_t from, int32_t to)
	{
		g_editor.SetLoadScale(from, to);
	}
};

#define UnnamedRenderingLock() RenderingLock __unnamed_rendering_lock_##__LINE__

std::ostream& operator<<(std::ostream& os, const Hotkey& hotkey);
std::istream& operator>>(std::istream& os, Hotkey& hotkey);


#endif // RME_EDITOR_H_
