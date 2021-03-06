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

#include <sstream>

#include "cArena.h"
#include "cGangs.h"
#include "IBuilding.h"
#include "Game.hpp"
#include "sStorage.h"
#include "cJobManager.h"
#include "scripting/GameEvents.h"
#include "cGirls.h"

extern cRng             g_Dice;

// // ----- Strut sArena Create / destroy
sArena::sArena() : IBuilding(BuildingType::ARENA, "Arena")
{
    m_FirstJob = JOB_FIGHTBEASTS;
    m_LastJob = JOB_CLEANARENA;
    m_MatronJob = JOB_DOCTORE;
}

sArena::~sArena() = default;

// Run the shifts
void sArena::UpdateGirls(bool is_night)    // Start_Building_Process_B
{
    // `J` When modifying Jobs, search for "J-Change-Jobs"  :  found in >> cArena.cpp
    std::stringstream ss;

    //  Handle the start of shift stuff for all girls.  //
    BeginShift(is_night);

    /////////////////////////////////////////////////////////////////
    //  All other Jobs in the Arena can be done at the same time.  //
    /////////////////////////////////////////////////////////////////
    /* `J` zzzzzz - Need to split up the jobs
    Done - JOB_ARENAREST, JOB_DOCTORE

    JOB_CLEANARENA

    JOB_FIGHTBEASTS
    JOB_FIGHTARENAGIRLS
    JOB_FIGHTTRAIN
    JOB_CITYGUARD

    //*/
    m_Girls->apply([&](auto& current){
        auto sw = (is_night ? current.m_NightJob : current.m_DayJob);
        if (current.is_dead() || sw == JOB_RESTING || sw == m_MatronJob)// || sw == JOB_RACING)
        {    // skip dead girls, resting girls and the matron and racers
            return;
        }
        auto sum = EVENT_SUMMARY; ss.str("");

        // fight beasts so if there is no beasts dont want them doing nothing
        if (sw == JOB_FIGHTBEASTS && g_Game->storage().beasts() < 1)
        {
            std::stringstream ssc;
            ssc << "There are no beasts to fight so ${name} was sent to ";

            if (current.health() < 50)
            {
                ssc << "rest and heal";
                sw = JOB_RESTING;
            }
            else if (current.combat() > 90 && current.magic() > 90 && current.agility() > 90 && current.constitution() > 90 && current.health() > 90)
            {
                ssc << "fight other girls";
                sw = JOB_FIGHTARENAGIRLS;
            }
            else
            {
                ssc << "train for combat";
                sw = JOB_FIGHTTRAIN;
            }
            ssc << " instead.\n"<<"\n";
            current.AddMessage(ssc.str(), IMGTYPE_PROFILE, is_night ? EVENT_NIGHTSHIFT : EVENT_DAYSHIFT);
        }

        // TODO this does not respect the changed job!
        // do their job
        bool refused = g_Game->job_manager().do_job(current, is_night);

        int totalPay   = current.m_Pay;
        int totalTips  = current.m_Tips;
        int totalGold  = current.m_Pay + current.m_Tips;
        CalculatePay(current, sw);

        //        Summary Messages
        if (refused)
        {
            m_Fame -= current.fame();
            ss << "${name} refused to work so made no money.";
        }
        else
        {
            ss << g_Game->job_manager().GirlPaymentText(this, current, totalTips, totalPay, totalGold, is_night);
            if (totalGold < 0) sum = EVENT_DEBUG;

            m_Fame += current.fame();
        }
        if (ss.str().length() > 0) current.AddMessage(ss.str(), IMGTYPE_PROFILE, sum);
    });

    EndShift(is_night);
}

void sArena::auto_assign_job(sGirl& target, std::stringstream& message, bool is_night)
{
    // shortcut
    std::stringstream& ss = message;

    ss << "The Doctore assigns " << target.FullName() << " to ";

    // need at least 1 guard and 1 cleaner (because guards must be free, they get assigned first)
    if (target.is_free() && num_girls_on_job(JOB_CITYGUARD, is_night) < 1)
    {
        target.m_DayJob = target.m_NightJob = JOB_CITYGUARD;
        ss << "work helping the city guard.";
    }
    else if (num_girls_on_job(JOB_CLEANARENA, is_night) < 1)
    {
        target.m_DayJob = target.m_NightJob = JOB_CLEANARENA;
        ss << "work cleaning the arena.";
    }
    else if (num_girls_on_job(JOB_BLACKSMITH, is_night) < 1)
    {
        target.m_DayJob = target.m_NightJob = JOB_BLACKSMITH;
        ss << "work making weapons and armor.";
    }
    else if (num_girls_on_job(JOB_COBBLER, is_night) < 1)
    {
        target.m_DayJob = target.m_NightJob = JOB_COBBLER;
        ss << "work making shoes and leather items.";
    }
    else if (num_girls_on_job(JOB_JEWELER, is_night) < 1)
    {
        target.m_DayJob = target.m_NightJob = JOB_JEWELER;
        ss << "work making jewelery.";
    }

        // next assign more guards and cleaners if there are a lot of girls to choose from
    else if (target.is_free() && num_girls_on_job(JOB_CITYGUARD, is_night) < num_girls() / 20)
    {
        target.m_DayJob = target.m_NightJob = JOB_CITYGUARD;
        ss << "work helping the city guard.";
    }
    else if (num_girls_on_job(JOB_CLEANARENA, is_night) < num_girls() / 20)
    {
        target.m_DayJob = target.m_NightJob = JOB_CLEANARENA;
        ss << "work cleaning the arena.";
    }
    else if (num_girls_on_job(JOB_BLACKSMITH, is_night) < num_girls() / 20)
    {
        target.m_DayJob = target.m_NightJob = JOB_BLACKSMITH;
        ss << "work making weapons and armor.";
    }
    else if (num_girls_on_job(JOB_COBBLER, is_night) < num_girls() / 20)
    {
        target.m_DayJob = target.m_NightJob = JOB_COBBLER;
        ss << "work making shoes and leather items.";
    }
    else if (num_girls_on_job(JOB_JEWELER, is_night) < num_girls() / 20)
    {
        target.m_DayJob = target.m_NightJob = JOB_JEWELER;
        ss << "work making jewelery.";
    }

        // Assign fighters - 50+ combat

        /*    Only fight beasts if there are 10 or more available
        *        and 1 girl per 10 beasts so they don't get depleted too fast.
        *    You can manually assign more if you want but I prefer to save beasts for the brothel
        *        until each building has their own beast supply.
        *    The farm will supply them when more work gets done to it
        */
    else if (target.combat() > 60 && g_Game->storage().beasts() >= 10 &&
            num_girls_on_job(JOB_FIGHTBEASTS, is_night) < g_Game->storage().beasts() / 10)
    {
        target.m_DayJob = target.m_NightJob = JOB_FIGHTBEASTS;
        ss << "work fighting beast in the arena.";
    }
        // if there are not enough beasts, have the girls fight other girls
    else if (target.combat() > 60 && num_girls_on_job(JOB_FIGHTARENAGIRLS, is_night) < 1)
    {
        target.m_DayJob = target.m_NightJob = JOB_FIGHTARENAGIRLS;
        ss << "work fighting other girls in the arena.";
    }

    else    // assign anyone else to Training
    {
        target.m_DayJob = target.m_NightJob = JOB_FIGHTTRAIN;
        ss << "train for the arena.";
    }
}

std::shared_ptr<sGirl> sArena::meet_girl() const
{
    // let's get a girl for the player to meet was to get arena.. dont think this should happen this is tryouts arena girl should be ready to fight. CRAZY
    auto girl = g_Game->GetRandomGirl(false, false, true);
    if(girl) {
        girl->TriggerEvent(EDefaultEvent::MEET_GIRL_ARENA);
    }
    return girl;
}

std::string sArena::meet_no_luck() const {
    return g_Dice.select_text(
            {
                "Your father once called this 'talent spotting' - "
                "and looking these girls over you see no talent for "
                "anything.",
                "The city is quiet and no one shows up.",
                "Married. Married. Bodyguard. Already works for you. Married. "
                "Hideous. Not a woman. Married. Escorted. Married... "
                "Might as well go home, there's nothing happening out here.",
                "It's not a bad life, if you can get paid for hanging around "
                "on street corners and eyeing up the pretty girls. Not a "
                "single decent prospect in the bunch of them, mind. "
                "Every silver lining has a cloud...",
                "You've walked and walked and walked, and the prettiest "
                "thing you've seen all day turned out not to be female. "
                "It's time to go home...",
                "When the weather is bad, the hunting is good. Get them cold "
                "and wet enough and girls too proud to spread their legs "
                "suddenly can't get their knickers off fast enough, if the job "
                "only comes with room and board. The down side is that you "
                "spend far too much time walking in the rain when everyone "
                "sane is warm inside. Time to head home for a mug of cocoa "
                "and a nice hot trollop.",
                "There's a bit of skirt over there with a lovely "
                "figure, and had a face that was pretty, ninety "
                "years ago. Over yonder, a sweet young thing frolicking "
                "through the marketplace. She's being ever so daring, "
                "spending her daddy's gold, and hasn't yet realised "
                "that there's a dozen of her daddy's goons keeping "
                "a discreet eye on her.  It's like that everywhere "
                "today. Maybe tomorrow will be better."
            }
            );
}