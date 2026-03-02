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

#ifndef RME_MAP_H_
#define RME_MAP_H_

#include "const.h"
#include "forward.h"
#include "tile.h"
#include "town.h"
#include "house.h"

#define MAP_SECTOR_SIZE 32
#define MAP_SECTOR_MASK (MAP_SECTOR_SIZE - 1)
STATIC_ASSERT(MAP_SECTOR_SIZE >= 16);
STATIC_ASSERT(ISPOW2(MAP_SECTOR_SIZE));

enum SectorType {
	SECTOR_BASELINE,
	SECTOR_PATCH,
	SECTOR_FULL_PATCH,
};

inline bool PositionValid(int x, int y, int z){
	return x >= 0 && x <= UINT16_MAX
		&& y >= 0 && y <= UINT16_MAX
		&& z >= 0 && z <= UINT8_MAX;
}

inline bool SectorValid(int sectorX, int sectorY, int sectorZ){
	return PositionValid(
			sectorX * MAP_SECTOR_SIZE,
			sectorY * MAP_SECTOR_SIZE,
			sectorZ);
}

inline uint32_t GetMapSectorId(int x, int y, int z){
	ASSERT(PositionValid(x, y, z));

	// IMPORTANT(fusion): Each position has 40 bits of information so if we trim
	// 4 bits from both the x and y coordinates, we can obtain an unique identifier
	// for each sector.
	return ((uint32_t)z << 24)
		| (((uint32_t)y & ~MAP_SECTOR_MASK) << 8)
		| (((uint32_t)x & ~MAP_SECTOR_MASK) >> 4);
}

// IMPORTANT(fusion): We're storing tiles in column major order. This shouldn't
// affect performance, but it's the way they're are stored in sector and patch
// files so it simplifies the saving loop (only marginally, but iterating items
// in linear order will always be more optimal than not, so there is that).

inline int GetTileIndex(int offsetX, int offsetY){
	return offsetX * MAP_SECTOR_SIZE + offsetY;
}

struct MapSector{
	MapSector(void) = default;

	void setTilePositions(int baseX, int baseY, int baseZ){
		for(int offsetX = 0; offsetX < MAP_SECTOR_SIZE; offsetX += 1)
		for(int offsetY = 0; offsetY < MAP_SECTOR_SIZE; offsetY += 1){
			Position tilePos = {
				baseX + offsetX,
				baseY + offsetY,
				baseZ,
			};

			tiles[GetTileIndex(offsetX, offsetY)].pos = tilePos;
		}
	}

	Tile *getTile(int offsetX, int offsetY){
		Tile *tile = NULL;
		int index = GetTileIndex(offsetX, offsetY);
		if(index >= 0 && index < NARRAY(tiles)){
			tile = &tiles[index];
		}
		return tile;
	}

	Tile tiles[MAP_SECTOR_SIZE * MAP_SECTOR_SIZE];
};

struct Map {
	int minSectorX = 0;
	int minSectorY = 0;
	int minSectorZ = 0;
	int maxSectorX = 0;
	int maxSectorY = 0;
	int maxSectorZ = 0;

	// IMPORTANT(fusion): `std::unordered_map` is node based so sector and tile
	// pointers should remain stable and valid while their sectors are still in
	// this map.
	std::unordered_map<uint32_t, MapSector> sectors;

	//std::vector<House> houses;
	//std::vector<Waypoint> waypoints;

	void loadSector(SectorType type, MapSector *sector, Script *script);
	bool loadSector(SectorType type, const wxFileName &filename);
	bool loadPatch(SectorType type, const wxFileName &filename);
	bool loadSpawns(const wxString &filename);
	bool loadHouseAreas(const wxString &filename);
	bool loadHouses(const wxString &filename);
	bool loadMeta(const wxString &filename);
	bool load(const wxString &projectDir);

	bool saveSector(const wxString &dir, const MapSector *sector);
	bool savePatch(const wxString &dir, const MapSector *sector, int patchNumber);
	bool saveSpawns(const wxString &filename);
	bool saveHouseAreas(const wxString &filename);
	bool saveHouses(const wxString &filename);
	bool saveMeta(const wxString &filename);
	bool save(const wxString &projectDir);

	bool backupPatch(const wxString &projectDir, const wxFileName &outputName, bool del = false);
	bool backupMap(const wxString &projectDir, const wxFileName &outputName, bool del = false);
	bool exportPatch(const wxString &projectDir,
			const wxString &filename, bool commit /*= false*/);

	void clear(void);

	MapSector *getSectorAt(int x, int y, int z);
	MapSector *getOrCreateSectorAt(int x, int y, int z);
	Tile *getTile(int x, int y, int z);
	Tile *getOrCreateTile(int x, int y, int z);
	Tile *getTile(Position pos) { return getTile(pos.x, pos.y, pos.z); }
	Tile *getOrCreateTile(Position pos) { return getOrCreateTile(pos.x, pos.y, pos.z); }
	bool isEmpty(void) const { return sectors.empty(); }

	const Tile *getTile(int x, int y, int z) const { return const_cast<Map*>(this)->getTile(x, y, z); }
	const Tile *getTile(Position pos)        const { return const_cast<Map*>(this)->getTile(pos); }

	Tile *swapTile(Tile &other) {
		Tile *tile = getOrCreateTile(other.pos);
		tile->swap(other);
		return tile;
	}

	int getTileCount(void) const {
		return (int)sectors.size() * MAP_SECTOR_SIZE * MAP_SECTOR_SIZE;
	}

	int getWidth(void) const {
		return (maxSectorX - minSectorX + 1) * MAP_SECTOR_SIZE;
	}

	int getHeight(void) const {
		return (maxSectorY - minSectorY + 1) * MAP_SECTOR_SIZE;
	}

	Position getMinPosition(void) const {
		return Position{
			minSectorX * MAP_SECTOR_SIZE,
			minSectorY * MAP_SECTOR_SIZE,
			minSectorZ,
		};
	}

	Position getMaxPosition(void) const {
		return Position{
			maxSectorX * MAP_SECTOR_SIZE + (MAP_SECTOR_SIZE - 1),
			maxSectorY * MAP_SECTOR_SIZE + (MAP_SECTOR_SIZE - 1),
			maxSectorZ,
		};
	}

	Position getCenterPosition(void) const {
		Position minPos = getMinPosition();
		Position maxPos = getMaxPosition();
		return Position{
			(minPos.x + maxPos.x) / 2,
			(minPos.y + maxPos.y) / 2,
			rme::MapGroundLayer,
		};
	}


	void checkTiles(bool showDialog = false);
	void cleanInvalidTiles(bool showDialog = false);
	bool exportMinimap(const wxFileName &filename,
						int floor = rme::MapGroundLayer,
						bool showDialog = false);

	// void f(Tile *tile, double progress)
	template<typename F>
	void forEachTile(F &&f){
		int numProcessed = 0;
		int numTiles = getTileCount();
		for(auto &[sectorId, sector]: sectors){
			for(Tile &tile: sector.tiles){
				double progress = (double)numProcessed / (double)numTiles;
				f(&tile, progress);
				numProcessed += 1;
			}
		}
	}

	// bool f(Tile *tile, Item *item, double progress)
	// NOTE(fusion): The return value here is used to determine whether we should
	// recurse when the item is a container (true) or skip it (false).
	template<typename F>
	void forEachItem(F &&f){
		int numProcessed = 0;
		int numTiles = getTileCount();
		std::queue<Item*> containers;
		for(auto &[sectorId, sector]: sectors){
			for(Tile &tile: sector.tiles){
				for(Item *item = tile.items; item != NULL; item = item->next){
					double progress = ((double)numProcessed / (double)numTiles);
					if(!f(&tile, item, progress)){
						continue;
					}

					if(item->getFlag(CONTAINER) || item->getFlag(CHEST)){
						containers.push(item);
						while(!containers.empty()){
							Item *container = containers.front();
							containers.pop();
							for(Item *inner = container->content; inner != NULL; inner = inner->next){
								if(!f(&tile, inner, progress)){
									continue;
								}

								if(inner->getFlag(CONTAINER) || inner->getFlag(CHEST)){
									containers.push(inner);
								}
							}
						}
					}
				}
				numProcessed += 1;
			}
		}
	}

	// bool predicate(Tile *tile, double progress)
	template<typename Pred>
	int clearTiles(Pred &&predicate){
		int numCleared = 0;
		int numProcessed = 0;
		int numTiles = getTileCount();
		for(auto &[sectorId, sector]: sectors){
			for(Tile &tile: sector.tiles){
				if(predicate(&tile, ((double)numProcessed / (double)numTiles))){
					tile.clear();
					numCleared += 1;
				}
				numProcessed += 1;
			}
		}
		return numCleared;
	}

	// bool predicate(Item *item, double progress);
	template<typename Pred>
	int removeItems(Pred &&predicate){
		int numRemoved = 0;
		int numProcessed = 0;
		int numTiles = getTileCount();
		for(auto &[sectorId, sector]: sectors){
			for(Tile &tile: sector.tiles){
				double progress = ((double)numProcessed / (double)numTiles);
				numRemoved += tile.removeItems(
					[&predicate, progress](Item *item){
						return predicate(item, progress);
					});
				numProcessed += 1;
			}
		}
		return numRemoved;
	}
};

#endif //RME_MAP_H_
