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
#include "character/sGirl.hpp"
#include "IBuilding.hpp"
#include "buildings/cBrothel.h"
#include "interface/constants.h"

#include "cDungeon.h"
#include "cGangs.h"
#include "cGirlTorture.h"
#include "cObjectiveManager.hpp"
#include "Game.hpp"
#include "scripting/GameEvents.h"
#include "CLog.h"
#include "xml/util.h"

extern cRng				g_Dice;


// strut sDungeonCust
sDungeonCust::sDungeonCust()		// constructor
{
	m_Prev = m_Next = nullptr;
	m_Weeks = 0;
	m_Tort = false;
	m_Feeding = true;
	m_Health = 100;
}

sDungeonCust::~sDungeonCust()		// destructor
{
	if (m_Next) delete m_Next;
	m_Next = nullptr;
}

// strut sDungeonGirl
sDungeonGirl::sDungeonGirl()		// constructor
{
}

sDungeonGirl::~sDungeonGirl() = default;

// class cDungeon
cDungeon::cDungeon()		// constructor
{
	m_LastDCusts = m_Custs = nullptr;
	m_NumberDied = 0;
	m_NumCusts = 0;
}

cDungeon::~cDungeon()		// destructor
{
    m_Girls.clear();
    delete m_Custs;
    m_LastDCusts = m_Custs = nullptr;
    m_NumberDied = 0;
    m_NumCusts = 0;
}

tinyxml2::XMLElement& cDungeon::SaveDungeonDataXML(tinyxml2::XMLElement * pRoot)// saves all the people (they are stored with the dungeon)
{
	auto& elDungeon = PushNewElement(*pRoot, "Dungeon");

	// save number died
	elDungeon.SetAttribute("NumberDied", m_NumberDied);

	// save girls
	auto& elDungeonGirls = PushNewElement(elDungeon, "Dungeon_Girls");
	for(sDungeonGirl& girl : m_Girls)
	{
        g_LogFile.log(ELogLevel::DEBUG, "Saving Dungeon Girl: ", girl.m_Girl->FullName());

		girl.m_Girl->m_DayJob = girl.m_Girl->m_NightJob = JOB_INDUNGEON;
		auto& elGirl = girl.m_Girl->SaveGirlXML(elDungeonGirls);

		auto& elDungeonData = PushNewElement(elGirl, "Dungeon_Data");
		elDungeonData.SetAttribute("Feeding", girl.m_Feeding);
		elDungeonData.SetAttribute("Reason", girl.m_Reason);
		elDungeonData.SetAttribute("Weeks", girl.m_Weeks);
	}

    g_LogFile.log(ELogLevel::DEBUG, "Saving Customers");
	// save customers
	auto& elDungeonCustomers = PushNewElement(elDungeon, "Dungeon_Customers");
	sDungeonCust* cust = m_Custs;
	while (cust)
	{
		auto& elCustomer = PushNewElement(elDungeonCustomers, "Customer");
		elCustomer.SetAttribute("Feeding", cust->m_Feeding);
		elCustomer.SetAttribute("Tort", cust->m_Tort);
		elCustomer.SetAttribute("HasWife", cust->m_HasWife);
		elCustomer.SetAttribute("NumDaughters", cust->m_NumDaughters);
		elCustomer.SetAttribute("Reason", cust->m_Reason);
		elCustomer.SetAttribute("Weeks", cust->m_Weeks);
		elCustomer.SetAttribute("Health", cust->m_Health);

		cust = cust->m_Next;
	}
	return elDungeon;
}


bool cDungeon::LoadDungeonDataXML(tinyxml2::XMLElement* pDungeon)	// loads all the people (they are stored with the dungeon)
{
	if (pDungeon == nullptr) { return false; }

	// load number died
	pDungeon->QueryAttribute("NumberDied", &m_NumberDied);

	// load girls
	auto* pDungeonGirls = pDungeon->FirstChildElement("Dungeon_Girls");
	if (pDungeonGirls)
	{
		// load each girl and add her
		for (auto& xml_girl : IterateChildElements(*pDungeonGirls, "Girl"))
		{
			sDungeonGirl girl;
			girl.m_Girl = std::unique_ptr<sGirl>(new sGirl());
			bool success = girl.m_Girl->LoadGirlXML(&xml_girl);
			if (success)
			{
                g_LogFile.log(ELogLevel::DEBUG, "Loading Dungeon Girl: ", girl.m_Girl->FullName());
			//	girl->m_Girl->m_DayJob = girl->m_Girl->m_NightJob = JOB_INDUNGEON;

				auto* pDungeonData = xml_girl.FirstChildElement("Dungeon_Data");
				if (pDungeonData)
				{
					pDungeonData->QueryAttribute("Feeding", &girl.m_Feeding);
					pDungeonData->QueryIntAttribute("Reason", &girl.m_Reason);
					pDungeonData->QueryIntAttribute("Weeks", &girl.m_Weeks);
				}

				PlaceDungeonGirl(std::move(girl));
			}
		}
	}

	// load customers
	m_NumCusts = 0;
    g_LogFile.log(ELogLevel::DEBUG, "Loading Customers");
    auto* pDungeonCustomers = pDungeon->FirstChildElement("Dungeon_Customers");
	if (pDungeonCustomers)
	{
		for (auto* pCustomer = pDungeonCustomers->FirstChildElement("Customer"); pCustomer != nullptr; pCustomer = pCustomer->NextSiblingElement("Customer"))
		{
			sDungeonCust* customer = new sDungeonCust();

			pCustomer->QueryAttribute("Feeding", &customer->m_Feeding);
			pCustomer->QueryAttribute("Tort", &customer->m_Tort);
			pCustomer->QueryAttribute("HasWife", &customer->m_HasWife);
			pCustomer->QueryIntAttribute("Reason", &customer->m_Reason);
			pCustomer->QueryIntAttribute("NumDaughters", &customer->m_NumDaughters);
			pCustomer->QueryIntAttribute("Weeks", &customer->m_Weeks);
			pCustomer->QueryIntAttribute("Health", &customer->m_Health);

			PlaceDungeonCustomer(customer);
		}
	}
	return true;
}

void cDungeon::AddGirl(sGirl* girl, int reason)
{
    // remove from previous building
    IBuilding* building = girl->m_Building;
    if(building) {
        building->remove_girl(girl);
    }

	if (reason == DUNGEON_GIRLKIDNAPPED)
	{
		if (g_Game->get_objective() && g_Game->get_objective()->m_Objective == OBJECTIVE_KIDNAPXGIRLS)
		{
            g_Game->get_objective()->m_SoFar++;
		}
	}

	girl->m_Building = nullptr;
	girl->m_DayJob = girl->m_NightJob = JOB_INDUNGEON;

	// by this stage they should no longer be a part of any other lists of girls
	sDungeonGirl newPerson;
	newPerson.m_Reason = reason;
	newPerson.m_Girl = std::unique_ptr<sGirl>(girl);

	// remove from girl manager if she is there
	g_Game->girl_pool().RemoveGirl(girl);

	// remove girl from brothels if she is there
	if(girl->m_Building) {
        girl->m_Building->remove_girl(girl);
    }
	PlaceDungeonGirl(std::move(newPerson));
}

void cDungeon::PlaceDungeonGirl(sDungeonGirl newGirl)
{
	m_Girls.emplace_back(std::move(newGirl));
}

void cDungeon::AddCust(int reason, int numDaughters, bool hasWife)
{
	sDungeonCust* newPerson = new sDungeonCust();
	newPerson->m_NumDaughters = numDaughters;
	newPerson->m_HasWife = hasWife;
	newPerson->m_Reason = reason;

	PlaceDungeonCustomer(newPerson);
}

void cDungeon::PlaceDungeonCustomer(sDungeonCust *newCust)
{
	if (m_Custs)
	{
		newCust->m_Prev = m_LastDCusts;
		m_LastDCusts->m_Next = newCust;
		m_LastDCusts = newCust;
	}
	else
	{
		m_LastDCusts = m_Custs = newCust;
	}

	m_NumCusts++;
}

int cDungeon::GetGirlPos(sGirl* girl)
{
	if (girl == nullptr || m_Girls.empty()) return -1;

	int count = 0;
	for(auto& current : m_Girls)
	{
		if (current.m_Girl.get() == girl) return count;
		count++;
	}
    return -1;
}

std::unique_ptr<sGirl> cDungeon::RemoveGirl(sGirl* girl)	// this returns the girl, it must be placed somewhere or deleted
{
    for(auto& current : m_Girls)
    {
        if (current.m_Girl.get() == girl) {
            return RemoveGirl(&current);
        }
    }
    return nullptr;
}

std::unique_ptr<sGirl> cDungeon::RemoveGirl(sDungeonGirl* girl)	// this returns the girl, it must be placed somewhere or deleted
{
	girl->m_Girl->m_DayJob = girl->m_Girl->m_NightJob = JOB_RESTING;

	std::unique_ptr<sGirl> girlData = std::move(girl->m_Girl);
	girl->m_Girl = nullptr;

	// remove from girls list
    m_Girls.erase(std::remove_if(begin(m_Girls), end(m_Girls), [girl](sDungeonGirl& g){ return &g == girl; }),
            end(m_Girls));

	return std::move(girlData);
}

void cDungeon::RemoveCust(sDungeonCust* cust)
{
	if (cust == nullptr) return;
	if (cust->m_Prev)			cust->m_Prev->m_Next = cust->m_Next;
	if (cust->m_Next)			cust->m_Next->m_Prev = cust->m_Prev;
	if (cust == m_LastDCusts)	m_LastDCusts = cust->m_Prev;
	if (cust == m_Custs)		m_Custs = cust->m_Next;
	cust->m_Next = cust->m_Prev = nullptr;
	delete cust;
	cust = nullptr;
	m_NumCusts--;
}

void cDungeon::OutputGirlRow(int i, vector<string>& Data, const vector<string>& columnNames)
{
    Data.resize(columnNames.size());
	int tmp = 0;
    for(auto& current : m_Girls)
    {
		if (tmp == i) {
            for (unsigned int x = 0; x < columnNames.size(); ++x)
            {
                //for each column, write out the statistic that goes in it
                current.OutputGirlDetailString(Data[x], columnNames[x]);
            }
            break;
		};
		tmp++;
	}
}

void sDungeonGirl::OutputGirlDetailString(string& Data, const string& detailName)
{
	//given a statistic name, set a string to a value that represents that statistic
	static stringstream ss;
	ss.str("");
	if (detailName == "Rebelliousness")	// `J` Dungeon "Matron" can be a Torturer from any brothel
	{
		ss << cGirls::GetRebelValue(m_Girl.get(), random_girl_on_job(g_Game->buildings(), JOB_TORTURER, false) != nullptr);
	}
	else if (detailName == "Reason")
	{
		switch (m_Reason)
		{
		case DUNGEON_GIRLCAPTURED:				ss << "Newly Captured.";					break;
		case DUNGEON_GIRLKIDNAPPED:				ss << "Taken from her family.";			break;
		case DUNGEON_GIRLWHIM:					ss << "Your whim.";						break;
		case DUNGEON_GIRLSTEAL:					ss << "Not reporting true earnings.";		break;
		case DUNGEON_GIRLRUNAWAY:				ss << "Ran away and re-captured.";			break;
		case DUNGEON_NEWSLAVE:					ss << "This is a new slave.";				break;
		case DUNGEON_NEWGIRL:					ss << "This is a new girl.";				break;
		case DUNGEON_KID:						ss << "Child of one of your girls.";		break;
		case DUNGEON_NEWARENA:					ss << "This is a girl won in the arena.";	break;
		case DUNGEON_RECRUITED:					ss << "This girl was recruited for you.";	break;
		}
	}
	else if (detailName == "Duration")			{ ss << m_Weeks; }
	else if (detailName == "Feeding")			{ ss << ((m_Feeding) ? "Yes" : "No"); }
	else if (detailName == "Tortured")			{ ss << ((m_Girl->m_Tort) ? "Yes" : "No"); }
	else if (detailName == "Kidnapped")
	{
	    // TODO (traits) figure out something here!
	    /*
		auto info = m_Girl->raw_traits().get_trait_info("Kidnapped");
		if (info.trait && info.remaining_time > 0) ss << info.remaining_time;
		else ss << "-";
	    */
	}
	else
	{
		m_Girl->OutputGirlDetailString(Data, detailName);
		return;
	}

	Data = ss.str();
}

void cDungeon::OutputCustRow(int i, vector<string>& Data, const vector<string>& columnNames)
{
    Data.resize(columnNames.size());
	sDungeonCust* cust = m_Custs;
	int tmp = 0;
	while (cust)
	{
		if (tmp == i) break;
		tmp++;
		cust = cust->m_Next;
	}
	if (cust)
	{
		for (unsigned int x = 0; x < columnNames.size(); ++x)
		{
			//for each column, write out the statistic that goes in it
			cust->OutputCustDetailString(Data[x], columnNames[x]);
		}
	}
}

void sDungeonCust::OutputCustDetailString(string& Data, const string& detailName)
{
	//given a statistic name, set a string to a value that represents that statistic
	static stringstream ss;
	ss.str("");
	if (detailName == "Name")					{ ss << "Customer"; }
	else if (detailName == "Health")	{ if (m_Health <= 0) ss << "DEAD"; else ss << m_Health << "%"; }
	else if (detailName == "Reason")
	{
		switch (m_Reason)
		{
		case DUNGEON_CUSTNOPAY:			ss << "Not paying.";			break;
		case DUNGEON_CUSTBEATGIRL:		ss << "Beating your girls.";	break;
		case DUNGEON_CUSTSPY:			ss << "Being a rival's spy.";	break;
		case DUNGEON_RIVAL:				ss << "Is a rival.";			break;
		}
	}
	else if (detailName == "Duration")			{ ss << (int)m_Weeks; }
	else if (detailName == "Feeding")			{ ss << ((m_Feeding) ? "Yes" : "No"); }
	else if (detailName == "Tortured")			{ ss << ((m_Tort) ? "Yes" : "No"); }
	else
	{
		ss << "---";
	}
	Data = ss.str();
}

sDungeonGirl* cDungeon::GetGirl(int i)
{
	if (i < 0)				    { i = i + m_Girls.size(); }
	if (i >= m_Girls.size())	{ i = i - m_Girls.size(); }

	int tmp = 0;
    for(auto& current : m_Girls)
    {
		if (tmp == i) { return &current; };
		tmp++;
	}
	return nullptr;
}

sDungeonGirl* cDungeon::GetGirlByName(string name)
{
	if (name.empty()) return nullptr;
    for(auto& current : m_Girls)
    {
		if (name == current.m_Girl->FullName())
			return &current;
	}
	return nullptr;
}

int cDungeon::GetDungeonPos(sGirl* girl)
{
	int tmp = 0;
    for(auto& current : m_Girls)
    {
		if (current.m_Girl.get() == girl) return tmp;
		tmp++;
	}
	return tmp;
}

sDungeonCust* cDungeon::GetCust(int i)
{
	sDungeonCust* cust = m_Custs;
	int tmp = 0;
	while (cust)
	{
		if (tmp == i) break;
		tmp++;
		cust = cust->m_Next;
	}
	return cust;
}

void cDungeon::ClearDungeonGirlEvents()
{
    for(auto& current : m_Girls) {
        // Clear the girls' events from the last turn
        current.m_Girl->m_Events.Clear();
    }
}


void cDungeon::Update()
{
	/*
	*	WD: GetNumGirlsOnJob() not testing if the girl worked
	*
	*/
	sGirl* TorturerGirlref = nullptr;
	string girlName;
	stringstream msg;
	stringstream ss;

	// Reset counters
	m_NumGirlsTort = m_NumCustsTort = 0;

	// WD:	Did we torture the girls
	// WD: If so, who is the Torturer
	if (m_TortureDone) { TorturerGirlref = random_girl_on_job(g_Game->buildings(), JOB_TORTURER, 0); }

	/*********************************
	*	DO ALL DUNGEON GIRLS
	*********************************/
    for(auto& current : m_Girls)
    {
        sGirl* girl = current.m_Girl.get();
        // TODO when can this happen???
        if(!girl) continue;

        //			girl->m_Tort = false;// WD: Move till after Girls have been tortured so that we dont torture twice week
        girlName = girl->FullName();
        ss.str("");
        msg.str("");

        // Check for dead girls
        if (girl->is_dead())
        {
            // remove dead bodies from last week
            if (current.m_Reason == DUNGEON_DEAD)
            {
                sDungeonGirl* temp = &current;
                msg << girlName << "'s body has been removed from the dungeon since she was dead.";
                g_Game->push_message(msg.str(), COLOR_RED);
                RemoveGirl(temp);
                continue;
            }
            // Mark as dead
            else
            {
                m_NumberDied++;
                current.m_Reason = DUNGEON_DEAD;
                g_Game->RunEvent(EDefaultEvent::DUNGEON_GIRL_DIED);
                continue;
            }
        }
        /*
        *			DAILY Processing
        */
        string summary;

        current.m_Weeks++;						// the number of weeks they have been in the dungeon
        cGirls::CalculateGirlType(girl);		// update the fetish traits
        cGirls::updateGirlAge(girl, true);		// update birthday counter and age the girl
        cGirls::updateTemp(girl);			// update temp stuff
        cGirls::EndDayGirls(g_Game->buildings().get_building(0), girl);
        g_Game->girl_pool().HandleChildren(girl, summary);	// handle pregnancy and children growing up
        cGirls::updateSTD(girl);				// health loss to STD's - NOTE: Girl can die
        cGirls::updateHappyTraits(girl);		// Update happiness due to Traits - NOTE: Girl can die
        updateGirlTurnDungeonStats(&current);	// Update stats
        cGirls::updateGirlTurnStats(girl);		// Stat Code common to Dugeon and Brothel

        // Check again for dead girls
        if (girl->health() <= 0)
        {
            m_NumberDied++;
            current.m_Reason = DUNGEON_DEAD;
            g_Game->RunEvent(EDefaultEvent::DUNGEON_GIRL_DIED);

            msg.str(""); ss.str("");
            msg << girlName << " has died in the dungeon.";
            girl->m_Events.AddMessage(msg.str(), IMGTYPE_DEATH, EVENT_DANGER);
            ss << girlName << " has died.  Her body will be removed by the end of the week.\n";
            girl->m_Events.AddMessage(ss.str(), IMGTYPE_DEATH, EVENT_SUMMARY);

            // if there is a torturer send her a message
            if (m_TortureDone)
            {
                msg.str("");
                msg << girlName << " has died in the dungeon under her care!";
                TorturerGirlref->m_Events.AddMessage(msg.str(), IMGTYPE_PROFILE, EVENT_DUNGEON);
            }

            continue;
        }

        // Have dungeon girls tortured by the Torturer
        if (m_TortureDone)
        {
            cGirlTorture gt(&current, TorturerGirlref);
        }

        /*		`J` merged WARNING MESSAGES and SUMMARY MESSAGES
        *			Allow girl sorting in turn summary
        */
        //	`J` set the basics
        msg.str(""); ss.str("");
        msg << girlName << " is languishing in the dungeon.\n \n";
        int msgtype = EVENT_DUNGEON;
        int imgtype = IMGTYPE_JAIL;
        int	nHealth = girl->health();
        int	nTired = girl->tiredness();

        //	`J` check them for dangers or warnings
        if (nHealth < 20 || nTired > 80)
        {
            msg << "DANGER: " << girlName;
            msgtype = EVENT_DANGER;
        }
        else if (nHealth < 40 || nTired > 60)
        {
            msg << "WARNING: " << girlName;
            msgtype = EVENT_WARNING;
        }

        //	`J` did msgtype change?
        if (msgtype != EVENT_DUNGEON)
        {
            if (girl->m_Tort)
            {
                msg << " was tortured this week.";
                imgtype = IMGTYPE_TORTURE;
                if (nHealth < 40 || nTired > 60)	{ msg << "\nShe"; }
            }
            if (nHealth < 20)						{ msg << " is severely injured"; }
            else if (nHealth < 40)					{ msg << " is injured"; }
            if (nHealth < 40 && nTired > 60)		{ msg << " and"; }
            else if (nTired > 60)					{ msg << " is"; }
            else									{ msg << "."; }
            if (nTired > 80)						{ msg << " exhausted, it may effect her health."; }
            else if (nTired > 60)					{ msg << " tired."; }
            msg << "\n \nHer health is " << nHealth << ".\nHer tiredness is " << nTired << ".";
        }
        girl->m_Events.AddMessage(msg.str(), imgtype, msgtype);

        girl->m_Tort = false;
    }

    /*
    *			WD:  Torturer Girl summary
    *				Processed after all dGirls
    */
    if (m_TortureDone && !m_Girls.empty())
    {
        msg.str("");
        msg << TorturerGirlref->FullName() << " has tortured " << m_NumGirlsTort << " girls in the Dungeon.";
        TorturerGirlref->m_Events.AddMessage(msg.str(), IMGTYPE_DOM, EVENT_DUNGEON);
    }



	/*********************************
	*	DO ALL CUSTOMERS
	*********************************/
	if (m_Custs)
	{
		sDungeonCust* current = m_Custs;
		while (current)
		{
			current->m_Tort = false;
			if (current->m_Health <= 0)
			{
				m_NumberDied++;
				current->m_Reason = DUNGEON_DEAD;
                g_Game->RunEvent(EDefaultEvent::DUNGEON_CUST_DIED);
			}
			if (current->m_Reason == DUNGEON_DEAD)
			{
				sDungeonCust* temp = current;
				current = current->m_Next;
				RemoveCust(temp);
				continue;
			}

			/*
			*			lose health if not feeding
			*
			*			Mod: removed "no-effect" branch to silence
			*			compiler
			*/
			if (!current->m_Feeding) current->m_Health -= 5;

			current->m_Weeks++;
			if (current->m_Health <= 0)
			{
				m_NumberDied++;
				current->m_Reason = DUNGEON_DEAD;
                g_Game->RunEvent(EDefaultEvent::DUNGEON_CUST_DIED);
			}
			current = current->m_Next;
		}
	}
}

void cDungeon::updateGirlTurnDungeonStats(sDungeonGirl* d_girl)
{
	/*
	*	WD: Update each turn the stats for girls in dudgeon
	*/

	sGirl* girl = d_girl->m_Girl.get();
	string msg;
	string girlName = girl->FullName();

	// Sanity check. Abort on dead girl
	if (girl->is_dead()) return;

	if (d_girl->m_Feeding)
	{
        girl->tiredness(-10);
        girl->pcfear(4);
        girl->pchate(1);
        girl->mana(5);
        girl->bdsm(1);

		if (girl->is_slave())	// Slave being fed
		{
			girl->confidence(-2);
			girl->obedience(2);
			girl->spirit(-2);
			girl->pclove(-1);
			girl->happiness(-1);
			girl->health(4);
		}
		else	// Free girl being fed
		{
			girl->confidence(-1);
			girl->obedience(1);
			girl->spirit(-1);
			girl->pclove(-4);
			girl->happiness(-5);
			girl->health(1);
		}
	}
	else	// feeding off
	{
        girl->confidence(-2);
        girl->obedience(2);
        girl->spirit(-2);
        girl->health(-5);
        girl->mana(1);
        girl->bdsm(2);
        if (girl->is_slave())	// Slave being starved
		{
			girl->pchate(1);
			girl->pclove(-2);
			girl->pcfear(4);
			girl->tiredness(1);
			girl->happiness(-3);
		}
		else	// Free girl being starved
		{
			girl->pchate(4);
			girl->pclove(-5);
			girl->pcfear(6);
			girl->tiredness(2);
			girl->happiness(-5);
		}
	}
}

void cDungeon::SetTortureDone()
{
    m_TortureDone = true;
}

bool cDungeon::SendGirlToDungeon(sGirl& girl)
{
    std::stringstream ss;
    bool success = true;
    int reason = girl.m_Spotted ? DUNGEON_GIRLSTEAL : DUNGEON_GIRLWHIM;

    auto result = AttemptEscape(girl);
    switch(result) {
        case EGirlEscapeAttemptResult::SUBMITS:
            ss << "She goes quietly with a sullen look on her face.";
            break;
        case EGirlEscapeAttemptResult::STOPPED_BY_GOONS:
            ss << "She puts up a fight ";
            ss << "but your goons manage to drag her unconscious to the dungeon.";
            break;
        case EGirlEscapeAttemptResult::STOPPED_BY_PLAYER:
            ss << "She manages to defeat your goons, but your personal intervention prevented her from escaping! ";
            ss << "You place her unconscious body in the dungeon.";
            break;
        case EGirlEscapeAttemptResult::SUCCESS:
            ss << "After defeating you goons and you, she escapes to the outside.\n";
            ss << "She will escape for good in 6 weeks if you don't send someone after her.";
            girl.run_away();
            success = false;
            stringstream smess;
            smess << girl.FullName() << " has run away";
            g_Game->push_message(smess.str(), 1);
    }
    g_Game->push_message(ss.str(), 0);

    if (success)
        g_Game->dungeon().AddGirl(&girl, reason);
    return success;
}
