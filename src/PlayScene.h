#pragma once
#ifndef __PLAY_SCENE__
#define __PLAY_SCENE__

#include "Heuristic.h"
#include "Scene.h"
#include "StarShip.h"
#include "Target.h"
#include "Tile.h"


class PlayScene : public Scene
{
public:
	PlayScene();
	~PlayScene();

	// Scene LifeCycle Functions
	virtual void draw() override;
	virtual void update() override;
	virtual void clean() override;
	virtual void handleEvents() override;
	virtual void start() override;

	//void moveStarShip() const;
private:
	// IMGUI Function
	void GUI_Function();
	std::string m_guiTitle;
	
	glm::vec2 m_mousePosition;

	// Game Objects
	Target* m_pTarget;
	StarShip* m_pStarShip;


	// UI Controls
	bool m_bToggleGrid;

	// Pathfinding Objects and Functions
	
	std::vector<Tile*> m_pGrid; // Grid of Tile pointers
	bool m_isGridEnabled;
	
	void m_buildGrid();
	void m_computeTileCosts();

	//TODO:
	// void m_findShortestPath();
	// void m_displayPathList();
	// void m_moveShipAlongPath();
	
	void m_setGridEnabled(bool state);
	bool m_getGridEnabled() const;

	// convenience functions
	Tile* m_getTile(int col, int row);
	Tile* m_getTile(glm::vec2 grid_position);

	// heuristic
	Heuristic m_currentHeuristic;

	// potential data structures
	// std::vector<Tile*> m_pOpenList;
	// std::vector<Tile*> m_pClosedList;
	// std::vector<Tile*> m_pPathList;

	// Path of Visibility? or LOS? as an optimization
	

};

#endif /* defined (__PLAY_SCENE__) */