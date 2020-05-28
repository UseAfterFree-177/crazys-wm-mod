/*
* Copyright 2009, 2010, The Pink Petal Development Team.
* The Pink Petal Devloment Team are defined as the game's coders
* who meet on http://pinkpetal.org     // old site: http://pinkpetal .co.cc
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

#include "sStorage.hpp"

std::string sStorage::food_key    = "food";
std::string sStorage::drink_key   = "drink";
std::string sStorage::beast_key   = "beast";
std::string sStorage::goods_key   = "goods";
std::string sStorage::alchemy_key = "alchemy";

int sStorage::get_stored_amount(const std::string& key) const
{
    auto entry = m_Storage.find(key);
    if(entry == m_Storage.end())
        return 0;
    return entry->second.amount;
}

int sStorage::add_to_storage(const std::string& key, int amount)
{
    m_Storage[key].amount += amount;
}

int sStorage::get_excess(const std::string& key) const {
    int reserve = m_Storage.at(key).reserve;
    int amount = m_Storage.at(key).amount;
    if(reserve > amount)
        return 0;
    return amount - reserve;
}

void sStorage::load_from_xml(tinyxml2::XMLElement& el)
{
// TODO
}

