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

#ifndef RME_TILE_H
#define RME_TILE_H

#include "position.h"
#include "item.h"
#include "wall_brush.h"

enum {
	TILE_FLAG_REFRESH         = 0x01,
	TILE_FLAG_NOLOGOUT        = 0x02,
	TILE_FLAG_PROTECTIONZONE  = 0x04,

	TILE_FLAG_OPTIONAL_BORDER = 0x40,
	TILE_FLAG_DIRTY           = 0x80,

	TILE_PERSISTENT_FLAGS     = (TILE_FLAG_REFRESH | TILE_FLAG_NOLOGOUT | TILE_FLAG_PROTECTIONZONE),
};

struct Tile {
	Item     *items    = NULL;
	Creature *creature = NULL;
	Position pos       = {};
	uint16_t houseId   = 0;
	uint8_t  flags     = 0;

	Tile(void) = default;
	~Tile(void) { clear(); }

	// non-copyable
	Tile(const Tile &tile) = delete;
	Tile &operator=(const Tile &other) = delete;

	// movable
	Tile(Tile &&other) { swap(other); }
	Tile &operator=(Tile &&other){
		swap(other);
		return *this;
	}

	void clear(void);
	void swap(Tile &other);
	void deepCopy(const Tile &other);
	Tile *deepCopy(void) const;
	void mergeCopy(const Tile &other);
	void merge(Tile &&other);
	uint32_t memsize(void) const;
	int countItems(void) const;
	bool empty(void) const;

	void select();
	void deselect();
	void selectGround();
	void deselectGround();
	Item *popSelectedItems();

	void addItem(Item *item);
	int addItems(Item *first);
	void clearCreature(void);
	void placeCreature(int raceId, int spawnRadius, int spawnAmount, int spawnInterval);
	int getIndexOf(Item *item) const;
	Item *getItemAt(int index) const;
	Item *getTopItem(void) const;
	bool getFlag(ObjectFlag flag) const;
	uint16_t getGroundSpeed(void) const noexcept;
	uint8_t getMiniMapColor(void) const;

	GroundBrush* getGroundBrush() const;
	void borderize(Map *parent);
	void wallize(Map *parent);
	void tableize(Map *parent);
	void carpetize(Map *parent);

	template<typename Pred>
	Item *getFirstItem(Pred &&predicate){
		for(Item *it = items; it != NULL; it = it->next){
			if(predicate(it)){
				return it;
			}
		}
		return NULL;
	}

	template<typename Pred>
	Item *getLastItem(Pred &&predicate){
		Item *result = NULL;
		for(Item *it = items; it != NULL; it = it->next){
			if(predicate(it)){
				result = it;
			}
		}
		return result;
	}

	Item *getFirstItem(ObjectFlag flag){
		return getFirstItem([flag](const Item *item) { return item->getFlag(flag); });
	}

	Item *getLastItem(ObjectFlag flag){
		return getLastItem([flag](const Item *item) { return item->getFlag(flag); });
	}

	Item *getWall(void){
		return getFirstItem([](const Item *item) { return item->isWall(); });
	}

	Item *getTable(void){
		return getFirstItem([](const Item *item) { return item->isTable(); });
	}

	Item *getCarpet(void){
		return getFirstItem([](const Item *item) { return item->isCarpet(); });
	}

	Item *getFirstSelectedItem(void){
		return getFirstItem([](const Item *item) { return item->isSelected(); });
	}

	Item *getLastSelectedItem(void){
		return getLastItem([](const Item *item) { return item->isSelected(); });
	}

	bool isSelected(void){
		return (creature && creature->isSelected())
			|| (getFirstSelectedItem() != NULL);
	}

	// TODO(fusion): Not exactly sure when we want to NOT delete removed items here.
	template<typename Pred>
	int removeItems(Pred &&predicate, bool del = true){
		int count = 0;
		Item **it = &items;
		while(*it != NULL){
			if(predicate(*it)){
				Item *next = (*it)->next;
				(*it)->next = NULL;
				if(del){
					delete *it;
				}
				*it = next;
				count += 1;
			}else{
				it = &(*it)->next;
			}
		}
		return count;
	}

	int removeBorders(void){
		return removeItems([](const Item *item) { return item->getFlag(CLIP); }, true);
	}

	int removeWalls(bool del = true){
		return removeItems([](const Item *item) { return item->isWall(); }, del);
	}

	int removeWalls(WallBrush *brush){
		int count = 0;
		if(brush){
			count = removeItems([brush](const Item *item){ return item->isWall() && brush->hasWall(item); });
		}
		return count;
	}

#if 0
	// TODO(fusion): We may want to do this?
	template<typename F>
	void forEachItem(F &&f){
		Item *item = items;
		while(item != NULL){
			Item *next = item->next;
			f(item);
			item = next;
		}
	}
#endif

	bool getTileFlag(uint8_t flag) const { return (flags & flag) != 0; }
	void setTileFlag(uint8_t flag) { flags |= flag; }
	void clearTileFlag(uint8_t flag) { flags &= ~flag; }

	// TODO(fusion): See if we want to keep these?

	bool hasOptionalBorder(void) const {
		return getTileFlag(TILE_FLAG_OPTIONAL_BORDER);
	}

	void setOptionalBorder(bool value) {
		if(value){
			setTileFlag(TILE_FLAG_OPTIONAL_BORDER);
		}else{
			clearTileFlag(TILE_FLAG_OPTIONAL_BORDER);
		}
	}
};

#endif
