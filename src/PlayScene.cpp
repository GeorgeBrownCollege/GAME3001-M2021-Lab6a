#include "PlayScene.h"
#include "Game.h"
#include "EventManager.h"

// required for IMGUI
#include "imgui.h"
#include "imgui_sdl.h"
#include "Renderer.h"
#include "Util.h"

PlayScene::PlayScene()
{
	PlayScene::start();
}

PlayScene::~PlayScene()
= default;

void PlayScene::draw()
{
	drawDisplayList();
	SDL_SetRenderDrawColor(Renderer::Instance().getRenderer(), 255, 255, 255, 255);
}

//void PlayScene::moveStarShip() const
//{
//	if(m_bToggleGrid)
//	{
//		m_pStarShip->setDesiredVelocity(m_pTarget->getTransform()->position);
//		m_pStarShip->getRigidBody()->velocity = m_pStarShip->getDesiredVelocity();
//		m_pStarShip->getTransform()->position += m_pStarShip->getRigidBody()->velocity;
//	}
//}

void PlayScene::update()
{
	updateDisplayList();
}

void PlayScene::clean()
{
	removeAllChildren();
}

void PlayScene::handleEvents()
{
	EventManager::Instance().update();

	if (EventManager::Instance().isKeyDown(SDL_SCANCODE_ESCAPE))
	{
		TheGame::Instance().quit();
	}
}

void PlayScene::start()
{
	// Set GUI Title
	m_guiTitle = "Play Scene";

	// Setup the Grid
	m_buildGrid();
	auto offset = glm::vec2(Config::TILE_SIZE * 0.5f, Config::TILE_SIZE * 0.5f);
	
	// Add Target to Scene
	m_pTarget = new Target();
	m_pTarget->getTransform()->position = m_getTile(15, 11)->getTransform()->position + offset; // position in world space matches grid space
	m_pTarget->setGridPosition(15, 11);
	m_getTile(15, 11)->setTileStatus(GOAL);
	addChild(m_pTarget);

	// Add StarShip to Scene
	m_pStarShip = new StarShip();
	m_pStarShip->getTransform()->position = m_getTile(1, 3)->getTransform()->position + offset; // position in world space matches grid space
	m_pStarShip->setGridPosition(1, 3);
	m_getTile(1, 3)->setTileStatus(START);
	addChild(m_pStarShip);

	m_computeTileCosts();

	ImGuiWindowFrame::Instance().setGUIFunction(std::bind(&PlayScene::GUI_Function, this));
}

void PlayScene::GUI_Function()
{
	// TODO:
	// Toggle Visibility for the StarShip and the Target

	
	auto offset = glm::vec2(Config::TILE_SIZE * 0.5f, Config::TILE_SIZE * 0.5f);
	
	// Always open with a NewFrame
	ImGui::NewFrame();

	// See examples by uncommenting the following - also look at imgui_demo.cpp in the IMGUI filter
	//ImGui::ShowDemoWindow();
	
	ImGui::Begin("GAME3001 - M2021 - Lab 6", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_MenuBar);

	if(ImGui::Checkbox("Toggle Grid", &m_bToggleGrid))
	{
		// toggle grid on/off
		m_setGridEnabled(m_bToggleGrid);
	}

	ImGui::Separator();

	static int start_position[2] = { m_pStarShip->getGridPosition().x, m_pStarShip->getGridPosition().y };
	if(ImGui::SliderInt2("Start Position", start_position, 0, Config::COL_NUM - 1))
	{
		if(start_position[1] > Config::ROW_NUM - 1)
		{
			start_position[1] = Config::ROW_NUM - 1;
		}

		m_getTile(m_pStarShip->getGridPosition())->setTileStatus(UNVISITED);
		m_pStarShip->getTransform()->position = m_getTile(start_position[0], start_position[1])->getTransform()->position + offset;
		m_pStarShip->setGridPosition(start_position[0], start_position[1]);
		m_getTile(m_pStarShip->getGridPosition())->setTileStatus(START);

		m_resetGrid();
	}

	ImGui::Separator();
	
	static int goal_position[2] = { m_pTarget->getGridPosition().x, m_pTarget->getGridPosition().y};
	if(ImGui::SliderInt2("Goal Position", goal_position, 0, Config::COL_NUM - 1))
	{
		if (goal_position[1] > Config::ROW_NUM - 1)
		{
			goal_position[1] = Config::ROW_NUM - 1;
		}

		m_getTile(m_pTarget->getGridPosition())->setTileStatus(UNVISITED);
		m_pTarget->getTransform()->position = m_getTile(goal_position[0], goal_position[1])->getTransform()->position + offset;
		m_pTarget->setGridPosition(goal_position[0], goal_position[1]);
		m_getTile(m_pTarget->getGridPosition())->setTileStatus(GOAL);

		m_resetGrid();
		m_computeTileCosts();
	}

	ImGui::Separator();

	if (ImGui::Button("Find Shortest Path"))
	{
		m_findShortestPath();
	}

	
	ImGui::Separator();

	if (ImGui::Button("Reset"))
	{
		m_resetGrid();
	}

	
	ImGui::End();
}

void PlayScene::m_buildGrid()
{
	auto tileSize = Config::TILE_SIZE; // alias for Tile size

	// add tiles to the grid
	for (int row = 0; row < Config::ROW_NUM; ++row)
	{
		for (int col = 0; col < Config::COL_NUM; ++col)
		{
			Tile* tile = new Tile(); // create a new empty tile
			tile->getTransform()->position = glm::vec2(col * tileSize, row * tileSize); // world position
			tile->setGridPosition(col, row);
			addChild(tile);
			tile->addLabels();
			tile->setEnabled(false);
			m_pGrid.push_back(tile);
		}
	}

	// create references (connections) for each tile (node) to its neighbours (nodes)
	for (int row = 0; row < Config::ROW_NUM; ++row)
	{
		for (int col = 0; col < Config::COL_NUM; ++col)
		{
			Tile* tile = m_getTile(col, row);

			// TopMost Row?
			if(row == 0)
			{
				tile->setNeighbourTile(TOP_TILE, nullptr);
			}
			else
			{
				// setup Top Neighbour
				tile->setNeighbourTile(TOP_TILE, m_getTile(col, row - 1));
			}
			
			// RightMost Col?
			if (col == Config::COL_NUM - 1)
			{
				tile->setNeighbourTile(RIGHT_TILE, nullptr);
			}
			else
			{
				// setup Right Neighbour
				tile->setNeighbourTile(RIGHT_TILE, m_getTile(col + 1, row));
			}

			// BottomMost Row?
			if (row == Config::ROW_NUM - 1)
			{
				tile->setNeighbourTile(BOTTOM_TILE, nullptr);
			}
			else
			{
				// Setup the Bottom Neighbour
				tile->setNeighbourTile(BOTTOM_TILE, m_getTile(col, row + 1));
			}

			// LeftMost Col?
			if (col == 0)
			{
				tile->setNeighbourTile(LEFT_TILE, nullptr);
			}
			else
			{
				// Setup the Left Neighbour
				tile->setNeighbourTile(LEFT_TILE, m_getTile(col - 1, row));
			}
		}
	}
}

void PlayScene::m_computeTileCosts()
{
	float dx = 0.0f;
	float dy = 0.0f;

	float f = 0.0f;
	float g = 0.0f;
	float h = 0.0f;
	
	for (auto tile : m_pGrid)
	{
		// Distance Function
		g = Util::distance(tile->getGridPosition(), m_pTarget->getGridPosition());
		
		// Heuristic Calculation (Manhattan Distance)
		dx = abs(tile->getGridPosition().x - m_pTarget->getGridPosition().x);
		dy = abs(tile->getGridPosition().y - m_pTarget->getGridPosition().y);
		h = dx + dy;

		// PathFinding function
		f = g + h;

		tile->setTileCost(f);
	}
}

void PlayScene::m_findShortestPath()
{
	if(m_pPathList.empty())
	{
		// Step 1 - Add Start position to the open list
		auto startTile = m_getTile(m_pStarShip->getGridPosition());
		startTile->setTileStatus(OPEN);
		m_pOpenList.push_back(startTile);

		bool goalFound = false;

		// Step 2 - Loop until the OpenList is empty or the Goal is found
		while(!m_pOpenList.empty() && !goalFound)
		{
			auto min = INFINITY;
			Tile* minTile; // temp Tile pointer - initialized as nullptr
			int minTileIndex = 0;
			int count = 0;

			std::vector<Tile*> neighbourList;
			for (int index = 0; index < NUM_OF_NEIGHBOUR_TILES; ++index)
			{
				if (m_pOpenList[0]->getNeighbourTile(static_cast<NeighbourTile>(index)) != nullptr)
				{
					neighbourList.push_back(m_pOpenList[0]->getNeighbourTile(static_cast<NeighbourTile>(index)));
				}
				
			}

			// for each  neighbour Tile pointer in the neighbourList -> Find the minimum neighbour Tile pointer
			for (auto neighbour : neighbourList)
			{
				if(neighbour != nullptr)
				{
					//if the neighbour we are exploring is not the goal
					if(neighbour->getTileStatus() != GOAL)
					{
						if(neighbour->getTileCost() < min)
						{
							min = neighbour->getTileCost();
							minTile = neighbour;
							minTileIndex = count;
						}
						count++;
					}
					//else if it is the goal
					else
					{
						minTile = neighbour;
						m_pPathList.push_back(minTile);
						goalFound = true;
						break;
					}
				}
			}

			// remove the reference of the current tile in the open list
			m_pPathList.push_back(m_pOpenList[0]);
			m_pOpenList.pop_back(); // empties the open list

			// add the minTile to the openList
			m_pOpenList.push_back(minTile);
			minTile->setTileStatus(OPEN);
			neighbourList.erase(neighbourList.begin() + minTileIndex);

			// push all remaining neighbours onto the closed list
			for (auto neighbour : neighbourList)
			{
				if(neighbour->getTileStatus() == UNVISITED)
				{
					neighbour->setTileStatus(CLOSED);
					m_pClosedList.push_back(neighbour);
				}
			}
		}
	}
}

void PlayScene::m_setGridEnabled(const bool state)
{
	for (auto tile : m_pGrid)
	{
		tile->setEnabled(state);
		tile->setLabelsEnabled(state);
	}

	m_isGridEnabled = state;
	
}

bool PlayScene::m_getGridEnabled() const
{
	return m_isGridEnabled;
}

Tile* PlayScene::m_getTile(const int col, const int row)
{
	return m_pGrid[(row * Config::COL_NUM) + col];
}

Tile* PlayScene::m_getTile(glm::vec2 grid_position)
{
	const auto col = grid_position.x;
	const auto row = grid_position.y;
	
	return m_pGrid[(row * Config::COL_NUM) + col];
}

void PlayScene::m_resetGrid()
{
	for (auto tile : m_pOpenList)
	{
		tile->setTileStatus(UNVISITED);
	}

	for (auto tile : m_pClosedList)
	{
		tile->setTileStatus(UNVISITED);
	}

	for (auto tile : m_pPathList)
	{
		tile->setTileStatus(UNVISITED);
	}

	m_pOpenList.clear();
	m_pClosedList.clear();
	m_pPathList.clear();

	// reset the start tile
	m_getTile(m_pStarShip->getGridPosition())->setTileStatus(START);
	
	// reset the Goal Tile
	m_getTile(m_pTarget->getGridPosition())->setTileStatus(GOAL);
}
