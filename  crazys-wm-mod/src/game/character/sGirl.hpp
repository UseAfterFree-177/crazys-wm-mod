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

#ifndef CRAZYS_WM_MOD_SGIRL_HPP
#define CRAZYS_WM_MOD_SGIRL_HPP

#include <string>
#include <map>
#include <vector>
#include <set>
#include <tinyxml2.h>

#include "cEvents.h"
#include "Constants.h"
#include "ICharacter.h"
#include "scripting/fwd.hpp"
#include "scripting/sLuaParameter.hpp"

class TraitSpec;
class sInventoryItem;
class sChild;
class IBuilding;
class cPlayer;
class sCustomer;
class sGirl;

typedef struct sChild
{
    int m_MultiBirth;
    std::string multibirth_str()
    {
        if (m_MultiBirth == 2) return "Twins";
        if (m_MultiBirth == 3) return "Triplets";
        if (m_MultiBirth == 4) return "Quads";
        if (m_MultiBirth == 5) return "Quints";
        // `J` anything else is single
        m_MultiBirth = 1;
        return "Single";
    };
    enum Gender {
        None = -1,
        Girl = 0,
        Boy = 1
    };
    Gender m_Sex;
    int m_GirlsBorn;			// if multiple births, how many are girls
    std::string boy_girl_str()
    {
        if (m_MultiBirth == 2)	return "twins";
        if (m_MultiBirth == 3)	return "triplets";
        if (m_MultiBirth > 3)	return "a litter";
        if (m_Sex == Boy)		return "a baby boy";
        if (m_Sex == Girl)		return "a baby girl";
        return "a baby";
    }
    bool is_boy()	{ return m_Sex == Boy; }
    bool is_girl()	{ return m_Sex == Girl; }

    static Gender RandomGender();

    int m_Age;	// grows up at 60 weeks
    bool m_IsPlayers;	// 1 when players
    unsigned char m_Unborn;	// 1 when child is unborn (for when stats are inherited from customers)

    // skills and stats from the father
    int m_Stats[NUM_STATS];
    int m_Skills[NUM_SKILLS];

    sChild* m_Next;
    sChild* m_Prev;

    sChild(bool is_players = false, Gender gender = None, int MultiBirth = 1);
    ~sChild(){ m_Prev = nullptr; if (m_Next)delete m_Next; m_Next = nullptr; }

    tinyxml2::XMLElement& SaveChildXML(tinyxml2::XMLElement& elRoot);
    bool LoadChildXML(tinyxml2::XMLElement* pChild);

}sChild;

// Represents a single girl
struct sGirl : public ICharacter
{
    sGirl();
    ~sGirl();

    std::string m_Name;								// The girls name
    /*	`J` adding first and surnames for future use.
    *	m_Realname will be used for girl tracking until first and surnames are fully integrated
    *	a girl id number system may be added in the future to allow for absolute tracking
    */
    std::string m_MotherName;						//	`J` added mother and father names
    std::string m_FatherName;						//	`J` added mother and father names
    /*
    *	MOD: changed from char* -- easier to change from lua -- doc
    */
    std::string m_Desc;								// Short story about the girl

    int house() const                               { return house_stat; }                /* It's NOT lupus! */
    void house(int n)                                { house_stat = std::min(std::max(0 ,n), 100); }
    int house_stat = 0;

    /// gets the girls job for the day or night shift
    JOBS get_job(bool night_shift) const;
    void FullJobReset(JOBS job);
    unsigned int m_DayJob;						// id for what job the girl is currently doing
    unsigned int m_NightJob;					// id for what job the girl is currently doing
    unsigned int m_PrevDayJob     = 255;		// id for what job the girl was doing
    unsigned int m_PrevNightJob   = 255;    	// id for what job the girl was doing
    unsigned int m_YesterDayJob   = 255;		// id for what job the girl did yesterday
    unsigned int m_YesterNightJob = 255;		// id for what job the girl did yesternight

    int m_Enjoyment[NUM_ACTIONTYPES];			// these values determine how much a girl likes an action
    int m_EnjoymentMods[NUM_ACTIONTYPES];		// `J` added perminant modifiers to stats
    int m_EnjoymentTemps[NUM_ACTIONTYPES];		// `J` added these go down (or up) by 30% each week until they reach 0
    // (-100 is hate, +100 is loves)

    bool m_UseAntiPreg;							// if true she will use anit preg measures

    unsigned char m_Withdrawals;				// if she is addicted to something this counts how many weeks she has been off

    int m_Money;

    int m_AccLevel;					// how good her Accommodation is, 0 is slave like and non-slaves will really hate it

    int m_Training[NUM_TRAININGTYPES];			// these values determine how far a girl is into her training CRAZY
    int m_TrainingMods[NUM_TRAININGTYPES];		// 
    int m_TrainingTemps[NUM_TRAININGTYPES];		// 
    // (starts at 0, 100 if fully trained)

    int m_RunAway;					// if 0 then off, if 1 then girl is removed from list,
    // otherwise will count down each week
    unsigned char m_Spotted;					// if 1 then she has been seen stealing but not punished yet

    unsigned long m_WeeksPast;					// number of weeks in your service
    unsigned int m_BDay;						// number of weeks in your service since last aging

    unsigned long m_NumCusts;					// number of customers this girl has slept with
    unsigned long m_NumCusts_old;				// number of customers this girl has slept with before this week

    bool m_Tort;								// if true then have already tortured today
    bool m_JustGaveBirth;						// did she give birth this current week?

    int m_Pay;									// used to keep track of pay this turn
    int m_Tips;									// used to keep track of tips this turn

    std::set<Fetishs> m_FetishTypes;            // the types of fetishes this girl has

    char m_Flags[NUM_GIRLFLAGS];				// flags used by scripts

    void AddMessage(std::string message, int nImgType, int nEvent);
    cEvents m_Events;							// Each girl keeps track of all her events that happened to her in the last turn

    // triggers
    scripting::pEventMapping m_EventMapping;
    scripting::sScriptValue TriggerEvent(scripting::sEventID id);
    template<class ...T>
    scripting::sScriptValue TriggerEvent(scripting::sEventID id, T&&... args) {
		return m_EventMapping->RunEvent(id, *this, std::forward<T>(args)...);
    }

    unsigned char m_DaysUnhappy;				// used to track how many days they are really unhappy for

    int m_WeeksPreg;							// number of weeks pregnant or inseminated
    int m_PregCooldown;							// number of weeks until can get pregnant again
    std::vector<std::unique_ptr<sChild>> m_Children;
    int m_ChildrenCount[CHILD_COUNT_TYPES];

    std::vector<std::string> m_Canonical_Daughters;

    IBuilding* m_Building = nullptr;
    int m_PrevWorkingDay;						// `J` save the last count of the number of working days
    int m_WorkingDay;							// count the number of working day
    int m_SpecialJobGoal;						// `J` Special Jobs like surgeries will have a specific goal
    bool m_Refused_To_Work_Day;					// `J` to track better if she refused to work her assigned job
    bool m_Refused_To_Work_Night;				// `J` to track better if she refused to work her assigned job

	static const char	*training_jobs[];
    static const char	*children_type_names[];	// `J` added

    /*
    *	we need to be able to go the other way, too:
    *	from std::string to number. The maps map stat/skill names
    *	onto index numbers. The setup flag is so we can initialise
    * 	the maps the first time an sGirl is constructed
    */
    static bool		m_maps_setup;
    static std::map<std::string, unsigned int>	jobs_lookup;
    static void		setup_maps();

    static int lookup_jobs_code(std::string s);


    tinyxml2::XMLElement& SaveGirlXML(tinyxml2::XMLElement& elRoot);
    bool LoadGirlXML(tinyxml2::XMLElement* pGirl);
    static std::unique_ptr<sGirl> LoadFromTemplate(const tinyxml2::XMLElement& root);

    /*
    *	stream operator - used for debug
    */
    friend std::ostream& operator<<(std::ostream& os, sGirl &g);

    int get_stat(int stat_id) const override;
    void set_stat(int stat, int amount) override;

    void upd_temp_stat(int stat_id, int amount, bool usetraits=false) override;
    int upd_base_stat(int stat_id, int amount, bool usetraits = true) override;

    int upd_temp_Enjoyment(Action_Types stat_id, int amount);
    int upd_Enjoyment(Action_Types stat_id, int amount);
    int upd_Training(int stat_id, int amount, bool usetraits = true);

    int rebel();
    bool FixFreeTimeJobs();

    int get_enjoyment(Action_Types actiontype) const;
    int get_training(int actiontype) const;

    /*
    *	convenience func. Also easier to read like this
    */
    bool carrying_monster() const;
    bool carrying_human() const;
    bool carrying_players_child() const;
    bool carrying_customer_child() const;
    bool is_pregnant() const;
    int get_preg_duration() const;
    bool is_mother() const;
    bool is_poisoned() const;
    void clear_pregnancy();
    void clear_dating();

    int preg_chance(int base_pc, bool good = false, double factor = 1.0);
    bool calc_pregnancy(int chance, int type, const ICharacter& father);

    bool calc_pregnancy(cPlayer* player, double factor = 1.0, bool nomessage=false);
    bool calc_pregnancy(const sCustomer& cust, double factor = 1.0);
    bool calc_insemination(const sCustomer& cust, bool good = false, double factor = 1.0);
    bool calc_group_pregnancy(const sCustomer& cust, bool good = false, double factor = 1.0);
    /*
    *	let's overload that...
    *	should be able to do the same using sCustomer as well...
    */

    int breast_size() const;
    bool is_dead() const;		// `J` replaces a few DeadGirl checks
    bool is_fighter(bool canbehelped = false);

    sGirl* run_away();

    bool keep_tips() const;
    /// returns whether the girl receives payment. This is not the case if she is a slave and you don't pay your slaves
    bool is_unpaid() const;
    bool is_slave()	const;
    bool is_free() const;
    void set_slave();
    bool is_monster();
    bool is_human();
    bool is_arena();
    bool is_yourdaughter();
    bool is_isdaughter();
    bool is_resting();
    bool is_havingsex();
    bool was_resting();

    void OutputGirlRow(std::vector<std::string>& Data, const std::vector<std::string>& columnNames);
    void OutputGirlDetailString(std::string& Data, const std::string& detailName);

    // END MOD

    double job_performance(JOBS job, bool estimate=true) const;

    // more useful functions
    int get_num_item_equiped(int Type);
    void set_default_house_percent();

    bool equip(const sInventoryItem* item, bool force);

    bool unequip(const sInventoryItem* item) override;
    bool can_equip(const sInventoryItem* item) const override;

    bool disobey_check(int action, JOBS job=NUM_JOBS);

    void add_tiredness();

    bool fights_back();

    void set_status(STATUS stat);
    bool has_status(STATUS stat) const;
    void remove_status(STATUS stat);

private:
    long m_States = 0;								// Holds the states the girl has
};

#endif //CRAZYS_WM_MOD_SGIRL_HPP