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

#include "map_window.h"
#include "map_display.h"
#include "editor.h"

BEGIN_EVENT_TABLE(MapWindow, wxPanel)
	EVT_SIZE(MapWindow::OnSize)

	EVT_COMMAND_SCROLL_TOP(MAP_WINDOW_HSCROLL, MapWindow::OnScroll)
	EVT_COMMAND_SCROLL_BOTTOM(MAP_WINDOW_HSCROLL, MapWindow::OnScroll)
	EVT_COMMAND_SCROLL_THUMBTRACK(MAP_WINDOW_HSCROLL, MapWindow::OnScroll)
	EVT_COMMAND_SCROLL_LINEUP(MAP_WINDOW_HSCROLL, MapWindow::OnScrollLineUp)
	EVT_COMMAND_SCROLL_LINEDOWN(MAP_WINDOW_HSCROLL, MapWindow::OnScrollLineDown)
	EVT_COMMAND_SCROLL_PAGEUP(MAP_WINDOW_HSCROLL, MapWindow::OnScrollPageUp)
	EVT_COMMAND_SCROLL_PAGEDOWN(MAP_WINDOW_HSCROLL, MapWindow::OnScrollPageDown)

	EVT_COMMAND_SCROLL_TOP(MAP_WINDOW_VSCROLL, MapWindow::OnScroll)
	EVT_COMMAND_SCROLL_BOTTOM(MAP_WINDOW_VSCROLL, MapWindow::OnScroll)
	EVT_COMMAND_SCROLL_THUMBTRACK(MAP_WINDOW_VSCROLL, MapWindow::OnScroll)
	EVT_COMMAND_SCROLL_LINEUP(MAP_WINDOW_VSCROLL, MapWindow::OnScrollLineUp)
	EVT_COMMAND_SCROLL_LINEDOWN(MAP_WINDOW_VSCROLL, MapWindow::OnScrollLineDown)
	EVT_COMMAND_SCROLL_PAGEUP(MAP_WINDOW_VSCROLL, MapWindow::OnScrollPageUp)
	EVT_COMMAND_SCROLL_PAGEDOWN(MAP_WINDOW_VSCROLL, MapWindow::OnScrollPageDown)

	EVT_BUTTON(MAP_WINDOW_GEM, MapWindow::OnGem)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(MapScrollBar, wxScrollBar)
	EVT_KEY_DOWN(MapScrollBar::OnKey)
	EVT_KEY_UP(MapScrollBar::OnKey)
	EVT_CHAR(MapScrollBar::OnKey)
	EVT_SET_FOCUS(MapScrollBar::OnFocus)
	EVT_MOUSEWHEEL(MapScrollBar::OnWheel)
END_EVENT_TABLE()

MapWindow::MapWindow(wxWindow* parent) : wxPanel(parent, PANE_MAIN) {
	canvas = newd MapCanvas(this);
	vScroll = newd MapScrollBar(this, MAP_WINDOW_VSCROLL, wxVERTICAL, canvas);
	hScroll = newd MapScrollBar(this, MAP_WINDOW_HSCROLL, wxHORIZONTAL, canvas);
	gem = newd DCButton(this, MAP_WINDOW_GEM, wxDefaultPosition, DC_BTN_NORMAL, RENDER_SIZE_16x16, EDITOR_SPRITE_SELECTION_GEM);

	wxFlexGridSizer* topsizer = newd wxFlexGridSizer(2, 0, 0);
	topsizer->AddGrowableCol(0);
	topsizer->AddGrowableRow(0);
	topsizer->Add(canvas, wxSizerFlags(1).Expand());
	topsizer->Add(vScroll, wxSizerFlags(1).Expand());
	topsizer->Add(hScroll, wxSizerFlags(1).Expand());
	topsizer->Add(gem, wxSizerFlags(1));
	SetSizerAndFit(topsizer);
}

MapWindow::~MapWindow()
{
	////
}

void MapWindow::ShowReplaceItemsDialog(bool selectionOnly)
{
	if(replaceItemsDialog)
		return;

	replaceItemsDialog = new ReplaceItemsDialog(this, selectionOnly);
	replaceItemsDialog->Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(MapWindow::OnReplaceItemsDialogClose), NULL, this);
	replaceItemsDialog->Show();
}

void MapWindow::CloseReplaceItemsDialog()
{
	if(replaceItemsDialog)
		replaceItemsDialog->Close();
}

void MapWindow::OnReplaceItemsDialogClose(wxCloseEvent& event)
{
	if(replaceItemsDialog) {
		replaceItemsDialog->Disconnect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(MapWindow::OnReplaceItemsDialogClose), NULL, this);
		replaceItemsDialog->Destroy();
		replaceItemsDialog = nullptr;
	}
}

void MapWindow::UpdateDialogs(bool show)
{
	if(replaceItemsDialog)
		replaceItemsDialog->Show(show);
}

double MapWindow::GetZoom(void)
{
	return canvas->GetZoom();
}

void MapWindow::GetViewStart(int* x, int* y)
{
	Position minPos = g_editor.map.getMinPosition();
	*x = hScroll->GetThumbPosition() + minPos.x * rme::TileSize;
	*y = vScroll->GetThumbPosition() + minPos.y * rme::TileSize;
}

void MapWindow::GetViewSize(int* x, int* y)
{
	canvas->GetSize(x, y);
	*x *= canvas->GetContentScaleFactor();
	*y *= canvas->GetContentScaleFactor();
}

Position MapWindow::GetScreenCenterPosition()
{
	int x, y;
	canvas->GetScreenCenter(&x, &y);
	return Position(x, y, canvas->GetFloor());
}

void MapWindow::SetScreenCenterPosition(const Position& position, bool showIndicator)
{
	if(!position.isValid())
		return;

	int x = position.x * rme::TileSize;
	int y = position.y * rme::TileSize;
	int z = position.z;

	if(z < 8) {
		// Compensate for floor offset above ground
		x -= (rme::MapGroundLayer - z) * rme::TileSize;
		y -= (rme::MapGroundLayer - z) * rme::TileSize;
	}

	Position center = GetScreenCenterPosition();
	if(previousPosition != center) {
		previousPosition = center;
	}

	Scroll(x, y, true);
	canvas->ChangeFloor(z);

	if(showIndicator) {
		canvas->ShowPositionIndicator(position);
		Refresh();
	}
}

void MapWindow::GoToPreviousCenterPosition()
{
	SetScreenCenterPosition(previousPosition, true);
}

void MapWindow::Scroll(int x, int y, bool center)
{
	Position minPos = g_editor.map.getMinPosition();
	x -= minPos.x * rme::TileSize;
	y -= minPos.y * rme::TileSize;

	if(center) {
		int windowWidth, windowHeight;
		GetSize(&windowWidth, &windowHeight);
		x -= int((windowWidth * g_editor.GetCurrentZoom()) / 2.0);
		y -= int((windowHeight * g_editor.GetCurrentZoom()) / 2.0);
	}

	hScroll->SetThumbPosition(x);
	vScroll->SetThumbPosition(y);
	g_editor.UpdateMinimap();
}

void MapWindow::ScrollRelative(int x, int y)
{
	hScroll->SetThumbPosition(hScroll->GetThumbPosition()+x);
	vScroll->SetThumbPosition(vScroll->GetThumbPosition()+y);
	g_editor.UpdateMinimap();
}

void MapWindow::FitToMap(){
	int mapWidth = g_editor.map.getWidth() * rme::TileSize;
	int mapHeight = g_editor.map.getHeight() * rme::TileSize;
	if(mapWidth != currentMapWidth || mapHeight != currentMapHeight){
		currentMapWidth = mapWidth;
		currentMapHeight = mapHeight;
		hScroll->SetScrollbar(hScroll->GetThumbPosition(), rme::TileSize, mapWidth, rme::TileSize - 1);
		vScroll->SetScrollbar(vScroll->GetThumbPosition(), rme::TileSize, mapHeight, rme::TileSize - 1);
	}
}

void MapWindow::OnGem(wxCommandEvent& WXUNUSED(event))
{
	g_editor.SwitchMode();
}

void MapWindow::OnSize(wxSizeEvent &event)
{
	int newWidth = event.GetSize().GetWidth();
	int newHeight = event.GetSize().GetHeight();
	hScroll->SetScrollbar(hScroll->GetThumbPosition(), newWidth / std::max(1, hScroll->GetRange()), std::max(1, hScroll->GetRange()), 96);
	vScroll->SetScrollbar(vScroll->GetThumbPosition(), newHeight / std::max(1, vScroll->GetRange()), std::max(1, vScroll->GetRange()), 96);
	event.Skip();
}

void MapWindow::OnScroll(wxScrollEvent& event)
{
	Refresh();
}

void MapWindow::OnScrollLineDown(wxScrollEvent& event)
{
	if(event.GetOrientation() == wxHORIZONTAL)
		ScrollRelative(96,0);
	else
		ScrollRelative(0,96);
	Refresh();
}

void MapWindow::OnScrollLineUp(wxScrollEvent& event)
{
	if(event.GetOrientation() == wxHORIZONTAL)
		ScrollRelative(-96,0);
	else
		ScrollRelative(0,-96);
	Refresh();
}

void MapWindow::OnScrollPageDown(wxScrollEvent& event)
{
	if(event.GetOrientation() == wxHORIZONTAL)
		ScrollRelative(5*96,0);
	else
		ScrollRelative(0,5*96);
	Refresh();
}

void MapWindow::OnScrollPageUp(wxScrollEvent& event)
{
	if(event.GetOrientation() == wxHORIZONTAL)
		ScrollRelative(-5*96,0);
	else
		ScrollRelative(0,-5*96);
	Refresh();
}

void MapWindow::OnSwitchEditorMode(EditorMode mode)
{
	gem->SetSprite(mode == DRAWING_MODE ? EDITOR_SPRITE_DRAWING_GEM : EDITOR_SPRITE_SELECTION_GEM);
	if(mode == SELECTION_MODE)
		canvas->EnterSelectionMode();
	else
		canvas->EnterDrawingMode();
}

