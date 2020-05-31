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

#pragma once

#ifndef __CDUNGEON_H
#define __CDUNGEON_H


#include <string>
#include <fstream>
#include <memory>

#include "cGirls.h"

using namespace std;

class cGirlTorture;

// Keeps track of customers in the dungeon
typedef struct sDungeonCust
{
	sDungeonCust();				// constructor
	~sDungeonCust();			// destructor

	bool			m_Feeding;	// are you feeding them
	bool			m_Tort;		// if true then have already tortured today
	int				m_Reason;	// the reason they are here
	int				m_Weeks;	// the number of weeks they have been here

	// customer data
	int				m_NumDaughters;
	bool			m_HasWife;
	sDungeonCust*	m_Next;
	sDungeonCust*	m_Prev;
	int				m_Health;
	void OutputCustDetailString(string& Data, const string& detailName);
}sDungeonCust;

// Keeps track of girls in the dungeon
struct sDungeonGirl
{
	sDungeonGirl();						// constructor
	~sDungeonGirl();					// destructor

	sDungeonGirl(sDungeonGirl&&) = default;
    sDungeonGirl& operator=(sDungeonGirl&&) = default;

	bool			m_Feeding  = true;		// are you feeding them
	int				m_Reason;			    // the reason they are here
	int				m_Weeks    = 0;			// the number of weeks they have been here

	// customer data
	std::unique_ptr<sGirl> m_Girl;
	void OutputGirlDetailString(string& Data, const string& detailName);
};


// The dungeon
class cDungeon
{
private:
    bool m_TortureDone = false;			// WD:	Have we got a torturer working today

	std::vector<sDungeonGirl> m_Girls;
	sDungeonCust* m_Custs;
	sDungeonCust* m_LastDCusts;
	unsigned long m_NumberDied;				// the total number of people that have died in the players dungeon
	int m_NumCusts;

	int m_NumGirlsTort;						//	WD:	Tracking for Torturer
	int m_NumCustsTort;
	void updateGirlTurnDungeonStats(sDungeonGirl* d_girl);	

public:
	cDungeon();								// constructor
	~cDungeon();							// destructor
    tinyxml2::XMLElement& SaveDungeonDataXML(tinyxml2::XMLElement * pRoot);	// saves dungeon data
	bool LoadDungeonDataXML(tinyxml2::XMLElement* pDungeon);
	bool SendGirlToDungeon(sGirl& girl);
	void AddGirl(sGirl* girl, int reason);
	void AddCust(int reason, int numDaughters, bool hasWife);
	void OutputGirlRow(int i, vector<string>& Data, const vector<string>& columnNames);
	void OutputCustRow(int i, vector<string>& Data, const vector<string>& columnNames);
	sDungeonGirl* GetGirl(int i);
	sDungeonGirl* GetGirlByName(string name);
	sDungeonCust* GetCust(int i);
	int GetDungeonPos(sGirl* girl);
	std::unique_ptr<sGirl> RemoveGirl(sGirl* girl);
	std::unique_ptr<sGirl> RemoveGirl(sDungeonGirl* girl);	// releases or kills a girl
	void RemoveCust(sDungeonCust* cust);	// releases or kills a customer
	void ClearDungeonGirlEvents();
	void Update();

	int GetGirlPos(sGirl* girl);
	int GetNumCusts()				{ return m_NumCusts; }
	int GetNumGirls()				{ return m_Girls.size(); }
	unsigned long GetNumDied()		{ return m_NumberDied; }

	int NumGirlsTort()				{ return m_NumGirlsTort; }
	int NumGirlsTort(int n)			{ m_NumGirlsTort += n; return m_NumGirlsTort; }
 	int NumCustsTort()				{ return m_NumCustsTort; }
	int NumCustsTort(int n)			{ m_NumCustsTort += n; return m_NumCustsTort; }

    std::vector<sDungeonGirl>& girls() { return m_Girls; }


	// WD:	Torturer tortures dungeon girl. 
	//void doTorturer(sDungeonGirl* d_girl, sGirl* t_girl, string& summary);	{ cGirlTorture::cGirlTorture(d_girl, t_girl) }
 
	void PlaceDungeonGirl(sDungeonGirl newGirl);
	void PlaceDungeonCustomer(sDungeonCust* newCust);

	void SetTortureDone();
};


#endif  /* __CDUNGEON_H */