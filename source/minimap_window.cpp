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

#include "graphics.h"
#include "editor.h"
#include "map.h"
#include "settings.h"

#include "map_window.h"
#include "map_display.h"
#include "minimap_window.h"

BEGIN_EVENT_TABLE(MinimapWindow, wxPanel)
	EVT_LEFT_DOWN(MinimapWindow::OnMouseClick)
	EVT_SIZE(MinimapWindow::OnSize)
	EVT_PAINT(MinimapWindow::OnPaint)
	EVT_ERASE_BACKGROUND(MinimapWindow::OnEraseBackground)
	EVT_CLOSE(MinimapWindow::OnClose)
	EVT_TIMER(wxID_ANY, MinimapWindow::OnDelayedUpdate)
	EVT_KEY_DOWN(MinimapWindow::OnKey)
END_EVENT_TABLE()

MinimapWindow::MinimapWindow(wxWindow* parent) :
	wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(205, 130)),
	update_timer(this)
{
	for(int i = 0; i < 256; ++i) {
		pens[i] = new wxPen(colorFromEightBit(i));
	}
}

MinimapWindow::~MinimapWindow()
{
	for(int i = 0; i < 256; ++i) {
		delete pens[i];
	}
}

void MinimapWindow::OnSize(wxSizeEvent& event)
{
	Refresh();
}

void MinimapWindow::OnClose(wxCloseEvent&)
{
	g_editor.DestroyMinimap();
}

void MinimapWindow::DelayedUpdate()
{
	// We only updated the window AFTER actions have taken place, that
	// way we don't waste too much performance on updating this window
	update_timer.Start(g_settings.getInteger(Config::MINIMAP_UPDATE_DELAY), true);
}

void MinimapWindow::OnDelayedUpdate(wxTimerEvent& event)
{
	Refresh();
}

void MinimapWindow::OnPaint(wxPaintEvent& event)
{
	wxBufferedPaintDC pdc(this);
	pdc.SetBackground(*wxBLACK_BRUSH);
	pdc.Clear();

	if(!g_editor.IsProjectOpen()){
		return;
	}

	int windowWidth = GetSize().GetWidth();
	int windowHeight = GetSize().GetHeight();
	Position center = g_editor.mapWindow->GetScreenCenterPosition();
	int startX = center.x - windowWidth / 2;
	int startY = center.y - windowHeight / 2;
	int z = center.z;

	last_start_x = startX;
	last_start_y = startY;

	if(g_editor.IsRenderingEnabled()){
		uint8_t lastColor = 0;

		int minSectorX = startX                  / MAP_SECTOR_SIZE;
		int minSectorY = startY                  / MAP_SECTOR_SIZE;
		int maxSectorX = (startX + windowWidth)  / MAP_SECTOR_SIZE;
		int maxSectorY = (startY + windowHeight) / MAP_SECTOR_SIZE;
		int sectorZ    = z;

		for(int sectorX = minSectorX; sectorX <= maxSectorX; sectorX += 1)
		for(int sectorY = minSectorY; sectorY <= maxSectorY; sectorY += 1){
			MapSector *sector = g_editor.map.getSectorAt(
					sectorX * MAP_SECTOR_SIZE,
					sectorY * MAP_SECTOR_SIZE,
					sectorZ);
			if(!sector){
				continue;
			}

			for(const Tile &tile: sector->tiles){
				int windowX = tile.pos.x - startX;
				int windowY = tile.pos.y - startY;
				if(windowX < 0 || windowX > windowWidth
				|| windowY < 0 || windowY > windowHeight){
					continue;
				}

				uint8_t color = tile.getMiniMapColor();
				if(color == 0){
					continue;
				}

				if(color != lastColor){
					pdc.SetPen(*pens[color]);
					lastColor = color;
				}

				pdc.DrawPoint(windowX, windowY);
			}
		}

		if(g_settings.getInteger(Config::MINIMAP_VIEW_BOX)){
			pdc.SetPen(*wxWHITE_PEN);

			int viewWidth, viewHeight, viewStartX, viewStartY;
			g_editor.mapWindow->GetViewSize(&viewWidth, &viewHeight);
			g_editor.mapWindow->GetViewStart(&viewStartX, &viewStartY);
			double zoom = g_editor.mapWindow->GetZoom();

			int left = (viewStartX / rme::TileSize) - startX;
			int top = (viewStartY / rme::TileSize) - startY;
			int right = left + ((viewWidth * zoom) / rme::TileSize) + 1;
			int bottom = top + ((viewHeight * zoom) / rme::TileSize) + 1;

			pdc.DrawLine(left,  top,    left,  bottom);
			pdc.DrawLine(left,  bottom, right, bottom);
			pdc.DrawLine(right, bottom, right, top);
			pdc.DrawLine(right, top,    left,  top);
		}
	}
}

void MinimapWindow::OnMouseClick(wxMouseEvent& event)
{
	if(!g_editor.IsProjectOpen()) return;
	int new_map_x = last_start_x + event.GetX();
	int new_map_y = last_start_y + event.GetY();
	g_editor.SetScreenCenterPosition(Position(new_map_x, new_map_y, g_editor.GetCurrentFloor()));
	Refresh();
	g_editor.RefreshView();
}

void MinimapWindow::OnKey(wxKeyEvent& event)
{
	g_editor.AddPendingMapEvent(event);
}
