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

#ifndef RME_MAP_WINDOW_H_
#define RME_MAP_WINDOW_H_

#include "position.h"
#include "replace_items_window.h"

class MapCanvas;
class MapScrollBar;
class DCButton;

// Map window, a window displaying a map, complete with scrollbars
// and everything. This is the window that's inside each tab in the
// editor. Does NOT control any map rendering or editing at all.
// MapCanvas does that. (mapdisplay.h)
class MapWindow : public wxPanel
{
public:
	MapWindow(wxWindow *parent);
	virtual ~MapWindow();

	// Event handlers
	void OnSize(wxSizeEvent& event);
	void OnScroll(wxScrollEvent& event);
	void OnScrollLineDown(wxScrollEvent& event);
	void OnScrollLineUp(wxScrollEvent& event);
	void OnScrollPageDown(wxScrollEvent& event);
	void OnScrollPageUp(wxScrollEvent& event);
	void OnGem(wxCommandEvent& event);

	double GetZoom(void);

	// GetViewSize returns the size of the containing canvas, in pixels
	void GetViewSize(int* x, int* y);

	// Returns the start of the camera on the map, in pixels
	void GetViewStart(int* x, int* y);

	// Scroll to the specified, absolute position (in pixels)
	void Scroll(int x, int y, bool center = false);

	// Scroll this many pixels in X/Y, relative to current position
	void ScrollRelative(int x, int y);

	// Adjust scroll bars to match the size of the currently loaded map.
	void FitToMap();

	// Screen position.
	Position GetScreenCenterPosition();
	void SetScreenCenterPosition(const Position& position, bool showIndicator = false);
	void GoToPreviousCenterPosition();

	// Return the containing canvas
	MapCanvas* GetCanvas() const noexcept { return canvas; }

	void ShowReplaceItemsDialog(bool selectionOnly);
	void CloseReplaceItemsDialog();
	void OnReplaceItemsDialogClose(wxCloseEvent& event);
	void UpdateDialogs(bool show);

	void OnSwitchEditorMode(EditorMode mode);

protected:
	MapCanvas *canvas = NULL;
	MapScrollBar *hScroll = NULL;
	MapScrollBar *vScroll = NULL;
	DCButton *gem = NULL;
	ReplaceItemsDialog *replaceItemsDialog = NULL;
	int currentMapWidth = 0;
	int currentMapHeight = 0;
	Position previousPosition = {};

	DECLARE_EVENT_TABLE()
};

// MapScrollbar, a special scrollbar that relays alot of events
// to the canvas, which allows scrolling when the scrollbar has
// focus (even though it also resents focus as hard as it can.
class MapScrollBar : public wxScrollBar
{
public:
	MapScrollBar(MapWindow* parent, wxWindowID id, long style, wxWindow* canvas) :
	  wxScrollBar(parent, id, wxDefaultPosition, wxDefaultSize, style), canvas(canvas) {}
	virtual ~MapScrollBar() {}

	void OnKey(wxKeyEvent& event) { canvas->GetEventHandler()->AddPendingEvent(event); }
	void OnWheel(wxMouseEvent& event) { canvas->GetEventHandler()->AddPendingEvent(event); }
	void OnFocus(wxFocusEvent& event) { canvas->SetFocus(); }

	wxWindow* canvas;
	DECLARE_EVENT_TABLE()
};

#endif
