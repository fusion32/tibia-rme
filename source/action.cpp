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
#include "settings.h"
#include "map.h"
#include "editor.h"

// Action
//==============================================================================
size_t Action::memsize(void) const
{
	size_t result = sizeof(Action);
#ifdef __USE_EXACT_MEMSIZE__
	for(const Change &change: changes){
		if(ChangeTile *v = std::get_if<ChangeTile>(&change)){
			result += v->tile.memsize();
		}
	}
#else
	result += sizeof(Change) * changes.size();
#endif
	return result;
}

void Action::changeTile(Tile tile){
	// TODO(fusion): Since every tile change goes through this function, this is
	// hopefully enough (?) to determine whether a tile is dirty.
	if(type != ACTION_SELECT && type != ACTION_UNSELECT){
		tile.setTileFlag(TILE_FLAG_DIRTY);
	}

	changes.emplace_back(ChangeTile(std::move(tile)));
}

void Action::commit(void)
{
	if(commited){
		return;
	}

	Map &map = g_editor.map;
	for(Change &change: changes){
		if(ChangeTile *v = std::get_if<ChangeTile>(&change)){
			Tile *tile = map.swapTile(v->tile);
			if(tile->isSelected() != v->tile.isSelected()){
				if(tile->isSelected()){
					g_editor.selection.addInternal(tile);
				}else{
					g_editor.selection.removeInternal(tile);
				}
			}
		}
#if TODO
		else if(ChangeHouseExit *v = std::get_if<ChangeHouseExit>(&change)){
			// TODO(fusion): See how things move.
		}else if(ChangeWaypoint *v = std::get_if<ChangeWaypoint>(&change)){
			// TODO(fusion): See how things move.
		}
#endif
	}
	commited = true;
}

void Action::undo(void)
{
	if(!commited){
		return;
	}

	Map &map = g_editor.map;
	for(Change &change: changes){
		if(ChangeTile *v = std::get_if<ChangeTile>(&change)){
			Tile *tile = map.swapTile(v->tile);
			if(tile->isSelected() != v->tile.isSelected()){
				if(tile->isSelected()){
					g_editor.selection.addInternal(tile);
				}else{
					g_editor.selection.removeInternal(tile);
				}
			}
		}
#if TODO
		else if(ChangeHouseExit *v = std::get_if<ChangeHouseExit>(&change)){
			// TODO(fusion): See how things move.
		}else if(ChangeWaypoint *v = std::get_if<ChangeWaypoint>(&change)){
			// TODO(fusion): See how things move.
		}
#endif
	}
	commited = false;
}

void Action::setDirty(void)
{
	for(Change &change: changes){
		if(ChangeTile *v = std::get_if<ChangeTile>(&change)){
			v->tile.setTileFlag(TILE_FLAG_DIRTY);
		}
	}
}

// Action Group
//==============================================================================
const char *ActionGroup::getLabel(void) const
{
	switch(type){
		case ACTION_MOVE:				return "Move";
		case ACTION_SELECT:				return "Select";
		case ACTION_UNSELECT:			return "Unselect";
		case ACTION_DELETE_TILES:		return "Delete";
		case ACTION_CUT_TILES:			return "Cut";
		case ACTION_PASTE_TILES:		return "Paste";
		case ACTION_RANDOMIZE:			return "Randomize";
		case ACTION_BORDERIZE:			return "Borderize";
		case ACTION_DRAW:				return "Draw";
		case ACTION_ERASE:				return "Erase";
		case ACTION_SWITCHDOOR:			return "Switch Door";
		case ACTION_ROTATE_ITEM:		return "Rotate Item";
		case ACTION_REPLACE_ITEMS:		return "Replace";
		case ACTION_CHANGE_PROPERTIES:	return "Change Properties";
		default:						return "?";
	}
}

Action *ActionGroup::createAction(void)
{
	actions.emplace_back();
	actions.back().type = type;
	lastUsed = time(NULL);
	return &actions.back();
}

void ActionGroup::commit(void)
{
	for(Action &action: actions) {
		action.commit();
	}
}

void ActionGroup::undo()
{
	for(Action &action: std::views::reverse(actions)) {
		action.undo();
	}
}

void ActionGroup::setDirty(void){
	for(Action &action: actions){
		action.setDirty();
	}
}

// Action Queue
//==============================================================================
bool ActionQueue::hasChanges(void) const
{
	for(const ActionGroup &group: groups) {
		if(!group.empty() && group.type != ACTION_SELECT
				&& group.type != ACTION_UNSELECT){
			return true;
		}
	}
	return false;
}

// TODO(fusion): Rename this? It seems to be used to prevent grouping actions.
void ActionQueue::resetTimer(void)
{
	if(!groups.empty()){
		groups.back().lastUsed = 0;
	}
}


void ActionQueue::setDirty(void){
	for(ActionGroup &group: groups){
		group.setDirty();
	}
}

ActionGroup *ActionQueue::createGroup(ActionType type, int groupWindow /*= 0*/)
{
	ASSERT(cursor <= groups.size());

	while(groups.size() > cursor){
		groups.pop_back();
	}

	// TODO(fusion): We might want to review and re-enable this feature to limit
	// memory usage, although having a queue size should suffice on most cases
	// unless you're purpositely copying the whole map on every action. TBD.
#if TODO
	size_t megabyte = 1024 * 1024;
	size_t curMemory = memsize();
	size_t maxMemory = megabyte * g_settings.getInteger(Config::UNDO_MEM_SIZE);
	while(usedMemory > maxMemory && !groups.empty()){
		curMemory -= groups.front().memsize();
		groups.pop_front();
		if(cursor > 0){
			cursor -= 1;
		}
	}
#endif

	size_t maxQueueSize = g_settings.getInteger(Config::UNDO_SIZE);
	while(groups.size() > maxQueueSize){
		groups.pop_front();
		if(cursor > 0){
			cursor -= 1;
		}
	}

	time_t timeNow = time(NULL);
	if(!groups.empty()){
		ActionGroup *lastGroup = &groups.back();
		if(lastGroup->type == type && (lastGroup->lastUsed + groupWindow) >= timeNow){
			lastGroup->lastUsed = timeNow;
			return lastGroup;
		}
	}

	cursor += 1;
	groups.emplace_back();
	groups.back().type = type;
	groups.back().lastUsed = timeNow;
	return &groups.back();
}

Action *ActionQueue::createAction(ActionType type, int groupWindow /*= 0*/)
{
	ActionGroup *group = createGroup(type, groupWindow);
	return group->createAction();
}

bool ActionQueue::undo()
{
	if(cursor == 0){
		return false;
	}

	cursor -= 1;
	groups[cursor].undo();
	return true;
}

bool ActionQueue::redo()
{
	if(cursor >= groups.size()){
		return false;
	}

	groups[cursor].commit();
	cursor += 1;
	return true;
}

void ActionQueue::clear()
{
	cursor = 0;
	groups.clear();
}

