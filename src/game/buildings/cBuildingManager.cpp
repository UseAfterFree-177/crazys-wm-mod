/*
 * Copyright 2009, 2010, The Pink Petal Development Team.
 * The Pink Petal Devloment Team are defined as the game's coders
 * who meet on http://pinkpetal.org
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "cBuildingManager.h"
#include "utils/string.hpp"
#include "xml/util.h"
#include "CLog.h"
#include "Game.hpp"

#include "cMovieStudio.h"
#include "cArena.h"
#include "cCentre.h"
#include "cClinic.h"
#include "cHouse.h"
#include "cFarm.h"
#include "cBrothel.h"
#include "cDungeon.h"


bool cBuildingManager::NameExists(string name) const
{
    return has_any_girl_with(HasName(std::move(name)));
}

bool cBuildingManager::SurnameExists(string name) const
{
    for(auto& current : m_Buildings)
    {
        for(sGirl* currentGirl : current->girls())
        {
            if (name == currentGirl->Surname())
                return true;
        }
    }
    return false;
}

tinyxml2::XMLElement * cBuildingManager::SaveXML(tinyxml2::XMLElement& root) const
{
    auto& elBrothels = PushNewElement(root, "Buildings");
    for(const auto& current : m_Buildings)
    {
        g_LogFile.log(ELogLevel::DEBUG,"Saving building (", current->type_str(), "): ", current->name());
        auto& elBrothel = PushNewElement(elBrothels, current->type_str());
        current->save_xml(elBrothel);
    }

    return &elBrothels;
}

void cBuildingManager::LoadXML(const tinyxml2::XMLElement& root)
{
    auto* pBrothels = root.FirstChildElement("Buildings");
    if (pBrothels)
    {
        for (auto& brothel : IterateChildElements(*pBrothels)) {
            auto current = create_building(brothel.Value());
            if(current) {
                g_LogFile.log(ELogLevel::DEBUG, "Loading ", brothel.Value());
                current->load_xml(brothel);
                AddBuilding(std::move(current));
            } else {
                g_LogFile.log(ELogLevel::WARNING, "Skipping XML node ", brothel.Value());
            }
        }
    }
}

IBuilding& cBuildingManager::AddBuilding(std::unique_ptr<IBuilding> building)
{
    m_Buildings.emplace_back(std::move(building));
    return *m_Buildings.back();
}

int cBuildingManager::total_girls() const
{
    int total = 0;
    for(auto& building : m_Buildings) {
        total += building->num_girls();
    }
    return total;
}

std::unique_ptr<IBuilding> cBuildingManager::create_building(std::string type) const
{
    // TODO cmp based on std::string is very wasteful here!
    if(iequals(type, "Arena")) {
        return std::make_unique<sArena>();
    } else  if ( iequals(type, "Clinic") ) {
        return std::make_unique<sClinic>();
    } else  if ( iequals(type, "Farm") ) {
        return std::make_unique<sFarm>();
    } else  if ( iequals(type, "HOUSE") ) {
        return std::make_unique<sHouse>();
    } else  if ( iequals(type, "Centre") ) {
        return std::make_unique<sCentre>();
    } else  if ( iequals(type, "MovieStudio") ) {
        return std::make_unique<sMovieStudio>();
    } else  if ( iequals(type, "Brothel") ) {
        return std::make_unique<sBrothel>();
    }

    return nullptr;
}

const IBuilding& cBuildingManager::get_building(int pos) const
{
    return *m_Buildings.at(pos);
}

IBuilding& cBuildingManager::get_building(int pos)
{
    return *m_Buildings.at(pos);
}

int cBuildingManager::num_buildings() const
{
    return m_Buildings.size();
}

IBuilding * cBuildingManager::building_with_type(BuildingType type, int index)
{
    int counter = 0;
    for(auto& b : m_Buildings) {
        if(b->type() == type) {
            if(counter == index)
                return b.get();
            counter += 1;
        }
    }

    return nullptr;
}

int get_total_player_girls()
{
    int total = g_Game->buildings().total_girls();
    total += g_Game->dungeon().GetNumGirls();
    return total;
}

int get_total_player_monster_girls()
{
    int total = 0;
    for (int i = 0; i < g_Game->dungeon().GetNumGirls(); i++)
    {
        sDungeonGirl* dgirl = g_Game->dungeon().GetGirl(i);
        if (dgirl->m_Girl->has_active_trait("Not Human"))
            total++;
    }

    total += g_Game->buildings().total_girls_with(is_nonhuman);
    return total;
}



sGirl* random_girl_on_job(const cBuildingManager& mgr, JOBS job, bool at_night)
{
    auto all_girls = mgr.all_girls_with( HasJob(job, at_night) );
    if(all_girls.empty())
        return nullptr;
    return all_girls[g_Dice % all_girls.size()];
}


sGirl* random_girl_on_job(const IBuilding& building, JOBS job, bool at_night)
{
    auto all_girls = building.find_all_girls( HasJob(job, at_night) );
    if(all_girls.empty())
        return nullptr;
    return all_girls[g_Dice % all_girls.size()];
}

int cBuildingManager::num_buildings(BuildingType type) const
{
    return std::count_if(begin(m_Buildings), end(m_Buildings), [type](const std::unique_ptr<IBuilding>& b){ return b->type() == type; });
}

bool cBuildingManager::has_building(BuildingType type) const
{
    return std::any_of(begin(m_Buildings), end(m_Buildings), [type](const std::unique_ptr<IBuilding>& b){ return b->type() == type; });
}

IBuilding * cBuildingManager::random_building_with_type(BuildingType type)
{
    RandomSelector<IBuilding> selector;
    for(auto& bld : m_Buildings) {
        if(bld->type() == type) {
            selector.process(bld.get());
        }
    }
    return selector.selection();
}

std::size_t cBuildingManager::find(const IBuilding * target) const
{
    auto found = std::find_if(begin(m_Buildings), end(m_Buildings),
                              [target](const std::unique_ptr<IBuilding>& b){ return b.get() == target; });
    if(found == m_Buildings.end()) {
        throw std::logic_error("Invalid building");
    }
    return std::distance(begin(m_Buildings), found);
}

IBuilding& cBuildingManager::AddBuilding(const BrothelCreationData& data)
{
    auto building = create_building(building_type_to_str(data.type));
    building->m_NumRooms = data.rooms;
    building->m_MaxNumRooms = data.maxrooms;
    building->set_background_image(data.background);
    return AddBuilding(std::move(building));
}