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

#include <stdexcept>
#include <CLog.h>
#include "cLuaState.h"

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

using namespace scripting;

void cLuaState::load(const std::string &file) {
    if(luaL_loadfile(m_State, file.c_str())) {
        throw std::runtime_error("Could not load lua file: " + file + ": " + get_error());
    }

    if (pcall(0, 1)) {
        throw std::runtime_error("Error running lua file " + file + ": " + get_error());
    }
}

int cLuaState::pcall(int nargs, int nresults) {
    return lua_pcall(m_State, nargs, nresults, 0);
}

cLuaState cLuaState::newthread() const {
    return cLuaState{lua_newthread(m_State)};
}

std::string cLuaState::get_error() {
    std::string errstr = lua_tostring(m_State, -1);
    lua_pop(m_State, 1);
    return std::move(errstr);
}

bool cLuaState::get_function(const std::string &name) {
    lua_getglobal(m_State, name.c_str());
    return lua_isfunction(m_State, -1);
}

void cLuaState::settable(int index, const char *key, int value) {
    lua_pushstring(m_State, key);
    lua_pushinteger(m_State, value);
    lua_settable(m_State, index - 2);
}

cLuaInterpreter::cLuaInterpreter() : cLuaState(lua_open()) {
    if(!get_state())
        throw std::runtime_error("Could not create lua state");
    luaL_openlibs(get_state());
}

cLuaInterpreter::~cLuaInterpreter() {
    auto state = get_state();
    if(state) lua_close(state);
}
