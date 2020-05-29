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

#include <stdexcept>
#include "CLog.h"
#include <vector>
#include "Game.hpp"
#include "cGold.h"
#include "cInventory.h"
#include <main.h>

#include "interface/fwd.hpp"

#include "character/cPlayer.h"
#include "buildings/cDungeon.h"
#include "character/sGirl.hpp"

#include "cLuaScript.h"
#include "cLuaState.h"
#include "sLuaGirl.h"
#include "sLuaParameter.hpp"
#include "sStorage.hpp"
#include "Inventory.hpp"
#include "sLuaParameter.hpp"


extern cRng g_Dice;

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

using namespace scripting;

template <class F>
auto cify_int_arg(F&& f) {
    static F fn = std::forward<F>(f);
    return [] (lua_State* state) -> int {
        int arg = luaL_checkinteger(state, -1);
        fn(arg);
        return 0;
    };
}

template <class F>
auto cify_no_arg(F&& f) {
	static F fn = std::forward<F>(f);
	return [] (lua_State* state) -> int {
		lua_pushinteger(state, fn());
		return 1;
	};
}

static const luaL_Reg funx [] = {
        { "ChoiceBox",                   cLuaScript::ChoiceBox },
        { "Dialog",                      cLuaScript::Dialog },
        { "SetPlayerSuspicion", 			cify_int_arg([](int amount) { g_Game->player().suspicion(amount); })},
		{ "GetPlayerSuspicion", 		 	cify_no_arg([](){ return g_Game->player().suspicion(); })},
        { "SetPlayerDisposition",        cify_int_arg([](int amount) { g_Game->player().disposition(amount); })},
		{ "GetPlayerDisposition", 		cify_no_arg([](){ return g_Game->player().disposition(); })},
		/// TODO make separate fns for adding and for requesting beasts
        { "AddBeasts",                   cify_int_arg([](int amount) { g_Game->storage().add_to_beasts(amount); })},
        { "GetBeasts", 		            cify_no_arg([](){ return g_Game->storage().beasts(); })},
        { "AddPlayerGold",               cify_int_arg([](int amount) { g_Game->gold().misc_credit(amount); })},
        { "GameOver",                    cLuaScript::GameOver},
        { "GivePlayerRandomSpecialItem", cLuaScript::GivePlayerRandomSpecialItem},
        { "AddCustomerToDungeon",        cLuaScript::AddCustomerToDungeon},
        { "AddFamilyToDungeon",          cLuaScript::AddFamilyToDungeon},
        // rng
        { "Range",                       cLuaScript::Range},
        { "Percent",                     cLuaScript::Percent},
        { nullptr,                       nullptr }
};

cLuaScript::cLuaScript()
{
    luaL_register(m_State.get_state(), "wm", funx);
    sLuaGirl::init(m_State.get_state());
    sLuaCustomer::init(m_State.get_state());
}

sScriptValue cLuaScript::RunEvent(const std::string& event_name, std::initializer_list<sLuaParameter> params) {
    // create new thread
    auto thread = m_State.newthread();
    // find function
    if(!thread.get_function(event_name)) {
        g_LogFile.error("scripting", "Could not find lua function '", event_name, "'");
        throw std::runtime_error("Could not find lua function");
    }

    for(auto& arg : params) {
    	arg.push(thread);
    }

    // run thread
    lua_State* s = thread.get_state();
    int result = lua_resume(s, params.size());
    if(result != 0 && result != LUA_YIELD) {
        g_LogFile.error("scripting", thread.get_error());
    } else {
        int top = lua_gettop(s);
        if(top > 0) {
    	    if(lua_isnumber(s, top)) {
    	        return sScriptValue((float)lua_tonumber(s, top));
    	    } else if(lua_isstring(s, top)) {
                return sScriptValue(std::string(lua_tostring(s, top)));
    	    } else if(lua_isboolean(s, top)) {
                return sScriptValue((bool)lua_toboolean(s, top));
    	    } else {
    	        return boost::blank{};
    	    }
		}
    }
    return boost::blank{};
}

int cLuaScript::ChoiceBox(lua_State* state)
{
    int nargs = lua_gettop(state);
    std::string question;
    std::vector<std::string> options;
    for(int i = 1; i <= nargs; ++i) {
        const char* option = luaL_checkstring(state, i);

        if(i == 1) {
            question = option;
        } else {
            options.emplace_back(option);
        }
    }

    auto callback = [state](int choice) {
        lua_pushinteger(state, choice);
        auto result = lua_resume(state, 1);
        if(result != 0 && result != LUA_YIELD) {
            g_LogFile.error("scripting", cLuaState{state}.get_error());
        }
    };

    window_manager().InputChoice(question, std::move(options), callback);
    return 0;
}

int cLuaScript::Dialog(lua_State* state)
{
    int nargs = lua_gettop(state);
    std::string text = luaL_checkstring(state, -1);
    window_manager().PushMessage(text, 0, [state](){ lua_resume(state, 0); });
    return 0;
}

int cLuaScript::GameOver(lua_State* state) {
    // TODO how do we handle this message?
    g_Game->push_message("GAME OVER", COLOR_RED);
    window_manager().PopAll();
    return 0;
}

int cLuaScript::GivePlayerRandomSpecialItem(lua_State* state) {
    sInventoryItem* item = g_Game->inventory_manager().GetRandomItem();
    while (item == nullptr)
        item = g_Game->inventory_manager().GetRandomItem();

    bool ok = false;
    while (!ok)
    {
        if (item->m_Rarity >= RARITYSHOP05) ok = true;
        else
        {
            do { item = g_Game->inventory_manager().GetRandomItem(); } while (item == nullptr);
        }
    }

    if(!g_Game->player().inventory().add_item(item)) {
        /// TODO how to handle this message?
        g_Game->push_message(" Your inventory is full\n", COLOR_RED);
    }
    return 0;
}

int cLuaScript::Percent(lua_State* state) {
    double prob = luaL_checknumber(state, 1);
    bool result = g_Dice.percent(prob);
    lua_pushboolean(state, result);
    return 1;
}

int cLuaScript::Range(lua_State *state) {
    long lower = luaL_checkinteger(state, 1);
    long upper = luaL_checkinteger(state, 2);
    bool result = g_Dice.in_range(lower, upper);
    lua_pushinteger(state, result);
    return 1;
}

int cLuaScript::AddCustomerToDungeon(lua_State* state) {
    int reason = luaL_checkinteger(state, 1);
    int daughters = luaL_checkinteger(state, 2);
    bool wife = lua_toboolean(state, 3);
    g_Game->dungeon().AddCust(reason, daughters, wife);
    return 0;
}

int cLuaScript::AddFamilyToDungeon(lua_State *L) {
    int num_daughters = luaL_checkinteger(L, 1);
    bool mother = lua_toboolean(L, 2);
    bool kidnapped = lua_toboolean(L, 3);
    bool slave = lua_toboolean(L, 4);

    int reason = kidnapped ? DUNGEON_GIRLKIDNAPPED : DUNGEON_GIRLCAPTURED;
    bool allowNonHuman = lua_toboolean(L, 5);
    bool arena = lua_toboolean(L, 6);

    // Set the surname for the family
    string surname;
    for (int i = 0; i < 5; i++)
    {
        surname = g_SurnameList.random();
        if (i > 3) surname = surname + "-" + g_SurnameList.random();
        if (g_Game->SurnameExists(surname)) continue;
        break;
    }

    // `J` zzzzzz - this can probably be done easier
    sGirl* Daughter1 = nullptr;
    sGirl* Daughter2 = nullptr;
    sGirl* Daughter3 = nullptr;
    sGirl* Mother = nullptr;
    stringstream NGmsg1;
    stringstream NGmsg2;
    stringstream NGmsg3;
    stringstream NGmsgM;

    int oldest = 18;	// `J` Legal Note: 18 is the Legal Age of Majority for the USA where I live
    if (num_daughters > 0)
    {
        Daughter1 = g_Game->CreateRandomGirl((g_Dice % 13) + 13, slave, false, allowNonHuman, kidnapped, arena).release();
        if (Daughter1->age() > oldest) oldest = Daughter1->age();
        Daughter1->SetSurname(surname);
    }
    if (num_daughters > 1)
    {
        Daughter2 = g_Game->CreateRandomGirl((g_Dice % 13) + 13, slave, false, allowNonHuman, kidnapped, arena).release();
        if (Daughter2->age() == Daughter1->age())	// if only 2 daughters and their ages are the same, change that
        {											// if there is a third daughter, her age can be anything (to allow twins)
            if (Daughter1->age() > 20) Daughter2->age(-(g_Dice % 3 + 1));
            else Daughter2->age((g_Dice % 3 + 1));
        }
        if (Daughter2->age() > oldest) oldest = Daughter2->age();
        Daughter2->SetSurname(surname);
    }
    if (num_daughters > 2)
    {
        Daughter3 = g_Game->CreateRandomGirl((g_Dice % 13) + 13, slave, false, allowNonHuman, kidnapped, arena).release();
        if (Daughter3->age() > oldest) oldest = Daughter3->age();
        Daughter3->SetSurname(surname);
    }

    if (mother)	// there is a mother
    {
        Mother = g_Game->CreateRandomGirl((g_Dice % (50 - (oldest + 18))) + oldest + 18, slave, false, allowNonHuman, kidnapped, arena).release();	// `J` Legal Note: 18 is the Legal Age of Majority for the USA where I live
        Mother->SetSurname(surname);
        if (!g_Dice.percent(Mother->age())) Mother->gain_trait("MILF");	// the younger the mother the more likely she will be a MILF
        Mother->lose_trait("Virgin");

        string biography = "Daughter of " + Mother->FullName() + " and a deadbeat brothel client.";

        if (Daughter1)	Daughter1->m_Desc = Daughter1->m_Desc + "\n \n" + biography;
        if (Daughter2)	Daughter2->m_Desc = Daughter2->m_Desc + "\n \n" + biography;
        if (Daughter3)	Daughter3->m_Desc = Daughter3->m_Desc + "\n \n" + biography;

        Mother->m_ChildrenCount[CHILD00_TOTAL_BIRTHS] += num_daughters;
        Mother->m_ChildrenCount[CHILD02_ALL_GIRLS] += num_daughters;
        Mother->m_ChildrenCount[CHILD04_CUSTOMER_GIRLS] += num_daughters;

    }
    string kstring = (kidnapped ? "kidnapped from her family" : "captured");

    if (num_daughters > 0)
    {
        NGmsg1 << Daughter1->FullName() << " was " << kstring;
        if (num_daughters > 1 || mother)
        {
            NGmsg1 << " along with ";
            if (mother)					NGmsg1 << "her mother " << Mother->FullName();
            if (num_daughters > 1 && mother)	NGmsg1 << " and ";
            if (num_daughters > 1)
            {
                NGmsg1 << "her sister" << (num_daughters > 2 ? "s " : "") << Daughter2->FullName();
                if (num_daughters > 2)			NGmsg1 << " and " << Daughter3->FullName();
            }
        }
        NGmsg1 << ".";
        Daughter1->m_Events.AddMessage(NGmsg1.str(), IMGTYPE_PROFILE, EVENT_DUNGEON);
    }
    if (num_daughters > 1)
    {
        NGmsg2 << Daughter2->FullName() << " was " << kstring << " along with ";
        if (Mother) NGmsg2 << "her mother " << Mother->FullName() << " and ";
        NGmsg2 << "her sister" << (num_daughters > 2 ? "s " : " ") << Daughter1->FullName();
        if (num_daughters > 2) NGmsg2 << " and " << Daughter3->FullName();
        NGmsg2 << ".";
        Daughter2->m_Events.AddMessage(NGmsg2.str(), IMGTYPE_PROFILE, EVENT_DUNGEON);
    }
    if (num_daughters > 2)
    {
        NGmsg3 << Daughter3->FullName() << " was " << kstring << " along with ";
        if (mother) NGmsg3 << "her mother " << Mother->FullName() << " and ";
        NGmsg3 << "her sisters " << Daughter1->FullName() << " and " << Daughter2->FullName() << ".";
        Daughter3->m_Events.AddMessage(NGmsg3.str(), IMGTYPE_PROFILE, EVENT_DUNGEON);
    }
    if (mother)
    {
        NGmsgM << Mother->FullName() << " was " << kstring;
        if (num_daughters > 0)
        {
            NGmsgM << " along with her daughter" << (num_daughters > 2 ? "s " : " ") << Daughter1->FullName();
            if (num_daughters == 2)		NGmsgM << " and ";
            else if (num_daughters == 3) NGmsgM << ", ";
            if (num_daughters > 1)		NGmsgM << Daughter2->FullName();
            if (num_daughters > 2)		NGmsgM << " and " << Daughter3->FullName();
        }
        NGmsgM << ".";
        Mother->m_Events.AddMessage(NGmsgM.str(), IMGTYPE_PROFILE, EVENT_DUNGEON);
    }

    if (Daughter1)	g_Game->dungeon().AddGirl(Daughter1, reason);
    if (Daughter2)	g_Game->dungeon().AddGirl(Daughter2, reason);
    if (Daughter3)	g_Game->dungeon().AddGirl(Daughter3, reason);
    if (Mother)		g_Game->dungeon().AddGirl(Mother, reason);

    return 0;
}

void cLuaScript::LoadSource(const std::string &file) {
    m_State.load(file);
}

void cLuaScript::PushParameter(sLuaParameter param)
{
	param.push(m_State);
}
