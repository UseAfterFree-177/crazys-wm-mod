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
#pragma region //	Includes and Externs			//

#include "buildings/cBrothel.h"
#include "cRng.h"
#include <sstream>
#include "Game.hpp"
#include "sStorage.hpp"

#pragma endregion

// `J` Job Brothel - Bar
bool WorkBarmaid(sGirl& girl, bool Day0Night1, cRng& rng)
{
    auto brothel = girl.m_Building;
#pragma region //	Job setup				//
	Action_Types actiontype = ACTION_WORKBAR;
	stringstream ss;
	int roll_jp = rng.d100(), roll_e = rng.d100(), roll_c = rng.d100();
	//Does she work?
	if (girl.libido() >= 90 && girl.has_active_trait("Nymphomaniac") && rng.percent(20))
	{
		ss << "${name} let lust get the better of her and she ended up missing her " << (Day0Night1 ? "night" : "day") << " shift.";
		girl.upd_temp_stat(STAT_LIBIDO, -20);
		girl.AddMessage(ss.str(), IMGTYPE_MAST, EVENT_NOWORK);
		return true;
	}
	else if (girl.disobey_check(actiontype, JOB_BARMAID))
	{
		//Making mssg more informative (what was refused?)
		ss << "${name} refused to work as a barmaid in your bar " << (Day0Night1 ? "tonight." : "today.");
		girl.AddMessage(ss.str(), IMGTYPE_PROFILE, EVENT_NOWORK);
		return true;
	}
	else if (brothel->m_TotalCustomers < 1)
	{
		ss << "There were no customers in the bar on the " << (Day0Night1 ? "night" : "day") << " shift so ${name} just cleaned up a bit.";
		brothel->m_Filthiness -= 20 + girl.service() * 2;
		girl.AddMessage(ss.str(), IMGTYPE_PROFILE, EVENT_NOWORK);
		return false;
	}
	ss << "${name} worked as a barmaid.\n \n";

	cGirls::UnequipCombat(&girl);	// put that shit away, you'll scare off the customers!

	int wages = 0;
	double tips = 0;
	int enjoy = 0, fame = 0;				// girl
	int Bhappy = 0, Bfame = 0, Bfilth = 0;	// brothel
	int imagetype = IMGTYPE_WAIT;
	int msgtype = Day0Night1;

#pragma endregion
#pragma region //	Job Performance			//

	double jobperformance = girl.job_performance(JOB_BARMAID, false);

	int numbarmaid = brothel->num_girls_on_job(JOB_BARMAID, Day0Night1);
	int numbarwait = brothel->num_girls_on_job(JOB_WAITRESS, Day0Night1);
	int numbargirls = numbarmaid + numbarwait;
	int numallcust = brothel->m_TotalCustomers;
	int numhercust = (numallcust / numbargirls)
		+ rng % ((girl.charisma() / 10) - 3)
		+ rng % ((girl.beauty() / 10) - 1);
	if (numhercust < 0) numhercust = 1;
	if (numhercust > numallcust) numhercust = numallcust;

	double drinkssold = 0;											// how many drinks she can sell in a shift
	for (int i = 0; i < numhercust; i++)
	{
		drinkssold += 1 + ((rng % (int)jobperformance) / 30);	// 200jp can serve up to 7 drinks per customer
	}
	double drinkswasted = 0;										// for when she messes up an order

	if (rng.percent(20))
	{
		if (actiontype >= 75)
		{
			ss << "Excited to get to work ${name} brings her 'A' game " << (Day0Night1 ? "tonight." : "today.");
			jobperformance += 40;
		}
		else if (actiontype <= 25)
		{
			ss << "The thought of working " << (Day0Night1 ? "tonight." : "today.") << " made ${name} feel uninspired so she didn't really try.";
			jobperformance -= 40;
		}
	}

	//what is she wearing?
	
	if (girl.has_item("Bourgeoise Gown") && rng.percent(60))
	{
		int bg = rng.bell(-1, 1);
		roll_e += bg;					// enjoy adj
		/* */if (bg < 0)	ss << "A few customers did not really like ${name}'s Bourgeoise Gown.";
		else if (bg > 0)	ss << "A few customers complimented ${name}'s Bourgeoise Gown.";
		else/*        */	ss << "${name}'s Bourgeoise Gown didn't really help or hurt her tips.";
		ss << "\n \n";
	}
	else if (girl.has_item("Maid Uniform"))
	{
		int bg = rng.bell(-1, 1);
		roll_e += bg;					// enjoy adj
		/* */if (bg < 0)	ss << "A few customers teased ${name} for wearing a Maid's Uniform in a bar.";
		else if (bg > 0)	ss << "${name}'s Maid Uniform didn't do much for most of the patrons, but a few of them seemed to really like it.";
		else/*        */	ss << "${name}'s Maid Uniform didn't do much to help her.";
		ss << "\n \n";
	}

	//a little pre-game randomness
	if (girl.has_active_trait("Alcoholic"))
	{
		if (rng.percent(10))
		{
			ss << "${name}'s alcoholic nature caused her to drink several bottles of booze becoming drunk and her serving suffered cause of it.";
			jobperformance -= 50;
			drinkswasted += 10 + rng % 11;
		}
		ss << "\n \n";
	}

	if (jobperformance >= 245)
	{
		drinkssold *= 1.6;
		roll_e += 10;		// enjoy adj
		ss << "She must be the perfect bartender, customers go on and on about her and always come to see her when she works.\n";
		if (roll_jp <= 14)
		{
			ss << "${name} was sliding drinks all over the bar without spilling a drop she put on quite a show for the patrons.";
			Bfame += rng % 6 + 5;
		}
		else if (roll_jp <= 28)
		{
			ss << "She agree to play a bar game with a client. Knowing every type of bar game there is, ${name} easily wins. The customer pays double for his drinks and leaves the bar saying that he will win next time.";
			wages += 25;
		}
		else if (roll_jp <= 42)
		{
			ss << "${name} made an 11 layer drink like it was nothing. The amazed customer left her a big tip!";
			Bfame += rng % 6 + 5;
			tips += rng % 26 + 15;
		}
		else if (roll_jp <= 56)
		{
			ss << "She pours eleven 100ml shots from a single, one litre bottle. Now there's a good barmaid!";
			drinkssold *= 1.1;
		}
		else if (roll_jp <= 70)
		{
			ss << "${name} noticed that a client was upset about something. After a pleasant conversation she managed to cheer him up. The client left full of willpower, leaving a generous tip behind.";
			Bhappy += rng % 6 + 5;
			tips += rng % 26 + 15;
		}
		else if (roll_jp <= 84)
		{
			ss << "Bottles fly high under the ceiling when ${name} is pouring drinks for the customers. The amazed crowd loudly applaudes every caught bottle and leave big tips for the girl.";
			Bfame += rng % 6 + 5;
			tips += rng % 26 + 15;
		}
		else
		{
			ss << "${name} mixed up what some patrons called the perfect drink.  It got them drunk faster then anything they had before.";
			Bhappy += rng % 6 + 5;
			Bfame += rng % 6 + 5;
		}
	}
	else if (jobperformance >= 185)
	{
		drinkssold *= 1.3;
		roll_e += 7;		// enjoy adj
		ss << " She's unbelievable at this and is always getting praised by the customers for her work.\n";
		if (roll_jp <= 14)
		{
			ss << "${name} had the bar filled with happy drunks.  She didn't miss a beat all shift.";
			Bhappy += rng % 5 + 4;
		}
		else if (roll_jp <= 28)
		{
			ss << "Today wasn't really exciting for ${name}. From boredom she spent some time trying to make more complicated drinks from the menu.";
		}
		else if (roll_jp <= 42)
		{
			ss << "${name} propose to a client to play a drinking game with her. If she loses she will serve nude to the end of her shift, "
                  "but if she wins he will be paying double. Some other patrons join the wager on the same terms. "
                  "After a few hours the last of them drops drunk and ${name} cleaned up on money.";
			wages += rng % 19 + 10;
		}
		else if (roll_jp <= 56)
		{
			ss << "When taking orders from customers, ${name} talked them into buying more expensive drinks, that let you make a solid profit today.";
			wages += rng % 19 + 10;
		}
		else if (roll_jp <= 70)
		{
			ss << "${name} is great at this job. At happy hour she was irreplaceable getting all the orders right. Later on she even prevented a fight between customers.";
			Bfame += rng % 5 + 4;
		}
		else if (roll_jp <= 84)
		{
			ss << "Her shift was slow and hardly anyone was buying. ${name} took the initiative, announcing a special promotion.";
			int promo = 2;
			/* */if (girl.has_active_trait("Flat Chest")) promo = 1;
			else if (girl.has_active_trait("Petite Breasts")) promo = 1;
			else if (girl.has_active_trait("Small Boobs")) promo = rng.percent(50) ? 1 : 2;
			else if (girl.has_active_trait("Busty Boobs")) promo = rng.percent(80) ? 2 : 3;
			else if (girl.has_active_trait("Big Boobs")) promo = rng.percent(70) ? 2 : 3;
			else if (girl.has_active_trait("Giant Juggs")) promo = rng.percent(60) ? 2 : 3;
			else if (girl.has_active_trait("Massive Melons")) promo = rng.percent(50) ? 2 : 3;
			else if (girl.has_active_trait("Abnormally Large Boobs")) promo = rng.percent(40) ? 2 : 3;
			else if (girl.has_active_trait("Titanic Tits")) promo = rng.percent(30) ? 2 : 3;
			if (promo == 1)
			{
				ss << "Every third shot ordered by a client could be drunk from her navel.\n";
			}
			else if (promo == 3)
			{
				ss << "Every pitcher ordered She would pour from between her breasts.\n";
			}
			else
			{
				ss << "Every third shot ordered by a client could be drunk from a shot-glass between her breasts.\n";
			}
			ss << "The promotion was such a success that you almost run out of booze.";
			drinkssold *= 1.2;
		}
		else
		{
			ss << "People love seeing ${name} work and they pour into the bar during her shift.  She mixes wonderful drinks and doesn't mess orders up so they couldn't be happier.";
			Bhappy += rng % 5 + 4;
			Bfame += rng % 4 + 2;
		}
	}
	else if (jobperformance >= 145)
	{
		drinkssold *= 1.1;
		roll_e += 3;		// enjoy adj
		ss << " She's good at this job and gets praised by the customers often.\n";
		if (roll_jp <= 14)
		{
			ss << "${name} didn't mix up any orders and kept the patrons drunk and happy.";
			Bhappy += rng % 5 + 2;
		}
		else if (roll_jp <= 28)
		{
			ss << "${name} certainly knows what she is doing behind the bar counter. She spends her shift without making any mistakes and earning a lot from tips.";
			tips += rng % 16 + 5;
		}
		else if (roll_jp <= 42)
		{
			ss << "${name} didn't make any mistakes today. She even earned some tips from happy customers.";
			Bhappy += rng % 5 + 2;
			tips += rng % 16 + 5;
		}
		else if (roll_jp <= 56)
		{
			ss << "When mixing one of the more complicated cocktails, ${name} noticed that she made a mistake and remakes the order. She wasted some alcohol, but the customer has happy with his drink.";
			Bhappy += rng % 5 + 2;
			drinkswasted += 1;
		}
		else if (roll_jp <= 70)
		{
			ss << "${name} spent more time talking with customers than filling their glasses. She didn't earn much today.";
		}
		else if (roll_jp <= 84)
		{
			ss << "Her shift as a barmaid goes smoothly. ${name} feels more confident in her skills after today.";
		}
		else
		{
			ss << "${name} had some regulars come in.  She knows just how to keep them happy and spending gold.";
			Bhappy += rng % 5 + 2;
		}
	}
	else if (jobperformance >= 100)
	{
		drinkssold *= 1.0;
		roll_e += 0;		// enjoy adj
		ss << " She made a few mistakes but overall she is okay at this.\n";
		if (roll_jp <= 14)
		{
			ss << "${name} mixed up a few drink orders...  But they ordered the same drink so it didn't matter too much.";
		}
		else if (roll_jp <= 28)
		{
			ss << "${name} made few mistakes but none of them were lethal.";
		}
		else if (roll_jp <= 42)
		{
			ss << "Trying her best she accidentally knocks down a bottle containing one of the bars most expensive liquors.";
			Bfilth += 5;
			drinkswasted += 10;
		}
		else if (roll_jp <= 56)
		{
			ss << "Maybe she isn't the best and fastest, but at least every drop of a drink that she pours stays in the glass.";
		}
		else if (roll_jp <= 70)
		{
			ss << "She agreed to play a bar game with a client. ${name} loses and the client spends the rest of his stay drinking on the house.";
			drinkswasted += 10;		// free drinks
		}
		else if (roll_jp <= 84)
		{
			ss << "${name} focused all her attention on taking orders and making drinks. Her attitude wasn't too appealing to clients. Some customers left feeling mistreated and unhappy.";
			Bhappy -= rng % 5 + 1;
		}
		else
		{
			ss << "${name} wasted a few drinks by forgetting to put ice in them but it wasn't anything major.";
			drinkswasted += 1 + rng % 5 + ((100 - girl.intelligence()) / 20);
		}
	}
	else if (jobperformance >= 70)
	{
		drinkssold *= 0.9;
		roll_e -= 3;		// enjoy adj
		ss << " She was nervous and made a few mistakes. She isn't that good at this.\n";
		if (roll_jp <= 14)
		{
			ss << "${name} mixed up people's drink orders...  When she only had four patrons drinking.";
			drinkswasted += 1 + rng % 10 + ((100 - girl.intelligence()) / 10);
		}
		else if (roll_jp <= 28)
		{
			ss << "${name} is having a bad day and she isn't trying to hide it. Her bad attitude shows and rubs off on the customers, leaving a negative impression on them.";
			Bhappy -= rng % 6 + 2;
		}
		else if (roll_jp <= 42)
		{
			ss << "Not being very good at this, she makes few mistakes. ${name} feels that she didn't improve today.";
			drinkswasted += 1 + rng % 5;
		}
		else if (roll_jp <= 56)
		{
			ss << "Wanting to impress a client, she throws a bottle of an expensive liquor into the air. Trying to catch it behind her back, ${name} fails.";
			Bfilth += 5;
			drinkswasted += 10;
		}
		else if (roll_jp <= 70)
		{
			ss << "One patron, looking for encouragement or understanding from the barmaid, unfortunately approached ${name}. After a short conversation, he left crying.";
			Bhappy -= rng % 6 + 2;
		}
		else if (roll_jp <= 84)
		{
			ss << "${name} tried to tap a new keg of beer; consequently she spends the rest of her shift mopping the floor.";
			Bfilth += 10;
			drinkswasted += 10 + rng % 20;
		}
		else
		{
			ss << "${name} gave someone a drink she mixed that made them sick.  It was nothing but coke and ice so who knows how she did it.";
			Bhappy -= rng % 6 + 2;
		}
	}
	else
	{
		drinkssold *= 0.8;
		roll_e -= 7;		// enjoy adj
		ss << " She was nervous and constantly making mistakes. She really isn't very good at this job.\n";
		if (roll_jp <= 14)
		{
			ss << "${name} was giving orders to the wrong patrons and letting a lot people walk out without paying their tab.";
			drinkswasted += 5 + rng % 15;
		}
		else if (roll_jp <= 28)
		{
			ss << "She mixed the ordered cocktails in the wrong proportions, making the clients throw up from the intoxication after just one shot! Besides swearing at her and yelling that they will never come here again, they left without paying.";
			Bhappy -= rng % 6 + 5;
			Bfame -= rng % 5 + 3;
			drinkswasted += 5 + rng % 15;
		}
		else if (roll_jp <= 42)
		{
			ss << "You can see that standing behind the bar isn't her happy place. Being tense she made a lot of mistakes today.";
			Bhappy -= rng % 4 + 2;
			drinkswasted += 1 + rng % 15;
		}
		else if (roll_jp <= 56)
		{
			ss << "Not having any experience at this kind of job, ${name} tries her best.. Regrettably without results.";
			Bhappy -= rng % 4 + 2;
			drinkswasted += 1 + rng % 10;
		}
		else if (roll_jp <= 70)
		{
			ss << "She gets in a drinking game with a customer. Being a total lightweight, she gets drunk quickly and passes out on the job.";
			drinkssold *= 0.5;
		}
		else if (roll_jp <= 84)
		{
			ss << "She spends most of her shift flirting with one client and not paying much attention to the others. What's worse, the guy she was flirting with skips without paying the bill!";
			drinkssold *= 0.5;
			drinkswasted += 1 + rng % 5;
		}
		else
		{
			ss << "${name} spilled drinks all over the place and mixed the wrong stuff when trying to make drinks for people.";
			Bhappy -= rng % 5 + 3;
			drinkswasted += 10 + rng % 10;
			Bfilth += 5;
		}
	}
	ss << "\n \n";

#pragma endregion
#pragma region	//	Tips and Adjustments		//

	tips += (drinkssold - drinkswasted) * ((double)roll_e / 100.0);	//base tips

	//try and add randomness here
	if (girl.beauty() > 85 && rng.percent(20))
	{
		ss << "Stunned by her beauty a customer left her a great tip.\n \n";
		tips += 25;
	}

	if (girl.beauty() > 99 && rng.percent(5))
	{
		tips += 50;
		ss << "${name} looked absolutely stunning during her shift and was unable to hide it. Instead of her ass or tits, the patrons couldn't take their eyes off her face, and spent a lot more than usual on tipping her.\n";
	}

	if (girl.charisma() > 85 && rng.percent(20))
	{
		tips += 35;
		ss << "${name} surprised a couple of gentlemen discussing some complicated issue by her insightful comments when she was taking her order. They decided her words were worth a heavy tip.\n";
	}

	if (girl.intelligence() < 30 && rng.percent(20))
	{
		ss << "${name} got confused when calculating the tabs, and seems to have damn near given away most of the alcohol.\n";
		drinkswasted += 5 + rng % 26;
		Bhappy += 5;
	}

	if (girl.has_active_trait("Clumsy") && rng.percent(15))
	{
		ss << "Her clumsy nature caused her to spill a drink on a customer resulting in them storming off without paying.\n";
		drinkswasted += 1 + rng % 5;
	}

	if (girl.has_active_trait("Pessimist") && rng.percent(5))
	{
		if (jobperformance < 125)
		{
			ss << "Her pessimistic mood depressed the customers making them tip less.\n";
			tips -= 10;
		}
		else
		{
			ss << "${name} was in a poor mood so the patrons gave her a bigger tip to try and cheer her up.\n";
			tips += 10;
		}
	}

	if (girl.has_active_trait("Optimist") && rng.percent(5))
	{
		if (jobperformance < 125)
		{
			ss << "${name} was in a cheerful mood but the patrons thought she needed to work more on her services.\n";
			tips -= 10;
		}
		else
		{
			ss << "Her optimistic mood made patrons cheer up increasing the amount they tip.\n";
			tips += 10;
		}
	}
	// `J` slightly lower percent compared to sleazy barmaid, I would think regular barmaid's uniform is less revealing
	if ((rng.percent(3) && girl.has_active_trait("Busty Boobs")) ||
        (rng.percent(6) && girl.has_active_trait("Big Boobs")) ||
        (rng.percent(9) && girl.has_active_trait("Giant Juggs")) ||
        (rng.percent(12) && girl.has_active_trait("Massive Melons")) ||
        (rng.percent(16) && girl.has_active_trait("Abnormally Large Boobs")) ||
        (rng.percent(20) && girl.has_active_trait("Titanic Tits")))
	{
		ss << "A patron was obviously staring at her large breasts. ";
		if (jobperformance < 150)
		{
			ss << "But she had no idea how to take advantage of it.\n";
		}
		else
		{
			ss << "So she over-charged them for drinks while they were too busy drooling to notice the price.\n";
			wages += 15;
		}
	}

	if (girl.has_active_trait("Psychic") && rng.percent(20))
	{
		ss << "She used her Psychic skills to know exactly what the patrons wanted to order and when to refill their mugs, keeping them happy and increasing tips.\n";
		tips += 15;
	}

	if (girl.has_active_trait("Assassin") && rng.percent(5))
	{
		if (jobperformance < 150)
		{
			ss << "A patron pissed her off and using her Assassin skills she killed him before she even realised. In the chaos that followed a number of patrons fled without paying.\n";
			drinkswasted += 5 + rng % 31;	// customers flee without paying
			drinkssold /= 2;					// customers don't come back
			wages -= 50;						// pay off the victims family or officials to cover it up
			Bhappy -= 10;						//
		}
		else
		{
			ss << "A patron pissed her off but she was able to keep her cool as she is getting used to this kinda thing.\n";
		}
	}

	if (girl.has_active_trait("Horrific Scars") && rng.percent(15))
	{
		if (jobperformance < 150)
		{
			ss << "A patron gasped at her Horrific Scars making her sad. But they didn't feel sorry for her.\n";
		}
		else
		{
			ss << "A patron gasped at her Horrific Scars making her sad. Feeling bad about it as she did a wonderful job they left a good tip.\n";
			tips += 25;
		}
	}

	if (girl.morality() >= 80 && rng.percent(20))
	{
		if (roll_jp <= 50)
		{
			tips += 35;
			ss << "During her shift ${name} spotted a depressed-looking lone man sinking his sorrows in alcohol. She spent a short while cheering him up. Surprised with her kindness, the client left her a generous tip.\n";
		}
		else
		{
			tips += 25;
			ss << "One of the patrons paid way too much for his order. When ${name} quickly pointed out his mistake, he said not to worry about it and told her to keep the extra as a reward for her honesty.\n";
		}
	}

	if (girl.morality() <= -20 && rng.percent(20))
	{
		if (roll_jp <= 33)
		{
			tips += 35;
			ss << "During her shift, ${name} spotted a lone fellow passed out from alcohol alone at a table in a corner, his wallet bulging out of his pocket. Without a second thought, she discreetly snatched it out and claimed for herself.\n";
		}
		else if (roll_jp <= 66)
		{
			tips += 25;
			ss << "One of the patrons paid way too much for his order... and ${name} didn't really feel like pointing it out, considering the extra money a generous tip.\n";
		}
		else
		{
			tips -= 15;
			ss << "${name} responded to one of the vulgar remarks by a client in a much more vulgar way. Needless to say, this didn't earn her any favors with the patrons that shift, and her tips were a bit less than usual.\n";
		}
	}

	if (girl.morality() <= -20 && girl.dignity() <= -20 && rng.percent(20))
	{
		tips += 40;
		ss << "A drunk patron suddenly walked up to ${name} and just started groping her body. Instead of pushing him away immediately, ${name} allowed him to take his time with her tits and butt while she helped herself to his pockets and all the money inside them. The rowdy client left with a dumb glee on his face, probably to find out his fondling was much, much overpriced.\n";
	}

	if (girl.dignity() <= -20 && rng.percent(20))
	{
		if (roll_jp <= 50)
		{
			tips += 15;
			ss << "When taking an order, ${name} made sure to lean in really close for the client to get a full view of her cleavage. Giving him an eyefull of tits was promptly rewarded with some extra cash in tips.\n";
		}
		else
		{
			tips += 20;
			ss << "One of the rowdier clients gently slapped the butt of ${name} when she was passing through. Her coy giggle only encouraged more clients to occasionally fondle her butt through the rest of her work, which earned her some extra tips.\n";
		}
	}

	if (girl.dignity() <= -20 && rng.percent(20) && (girl.has_active_trait("Big Boobs") ||
                                                     girl.has_active_trait("Abnormally Large Boobs") ||
                                                     girl.has_active_trait("Titanic Tits") ||
                                                     girl.has_active_trait("Massive Melons") ||
                                                     girl.has_active_trait("Giant Juggs")))
	{
		tips += 25;
		ss << "${name} got an odd request from a client to carry a small drink he ordered between her tits to his table. "
              "After pouring the drink in a thin glass, ${name} handled the task with minimal difficulty and earned a bigger tip.\n";
	}

	if (girl.morality() <= -20 && rng.percent(10))
	{
		ss << "A patron came up to her and said he wanted to order some milk but that he wanted it straight from the source. ";
		if (girl.lactation() >= 20)
		{
			ss << "With a smile she said she was willing to do it for an extra charge. The patron quickly agreed and ${name} proceed to take out one of her tits and let the patron suck out some milk.\n";
			girl.lactation(-20);
			tips += 40;
		}
		else
		{
			ss << "She was willing to do it but didn't have enough milk production.";
		}
	}

	if (girl.is_pregnant() && rng.percent(10))
	{
		ss << "A customer tried to buy ${name} a drink, but she refused for the sake of her unborn child.";
	}

	if ((girl.has_active_trait("Deep Throat") || girl.has_active_trait("No Gag Reflex")) && rng.percent(5))
	{
		ss << "Some customers were having a speed drinking contest and challenged ${name} to take part.\n";
		if (girl.is_pregnant()) ss << "She refused for the sake of her unborn child.";
		else
		{
			ss << "Her talent at getting things down her throat meant she could pour the drink straight down. She won easily, earning quite a bit of gold.";
			tips += 30;
		}
	}

	if (girl.has_item("Golden Pendant") && rng.percent(10))//zzzzz FIXME need more CRAZY
	{
		ss << "A patron complimented her gold necklace, you're not sure if it was an actual compliment or ";
		if (girl.has_active_trait("Massive Melons") || girl.has_active_trait("Abnormally Large Boobs")
            || girl.has_active_trait("Titanic Tits") || girl.has_active_trait("Big Boobs")
            || girl.has_active_trait("Busty Boobs") || girl.has_active_trait("Giant Juggs"))
		{
			ss << "an excuse to stare at her ample cleavage.";
		}
		else
		{
			ss << "an attempt to get a discount on their bill.";
		}
		girl.happiness(5);//girls like compliments
	}

#pragma endregion
#pragma region	//	Enjoyment and Tiredness		//

	//enjoyed the work or not
	if (roll_e <= 10)
	{
		enjoy -= rng % 3 + 1;
		drinkssold *= 0.9;
		ss << "\nSome of the patrons abused her during the shift.";
	}
	else if (roll_e >= 90)
	{
		drinkssold *= 1.1;
		enjoy += rng % 3 + 1;
		ss << "\nShe had a pleasant time working.";
	}
	else
	{
		enjoy += rng % 2;
		ss << "\nOtherwise, the shift passed uneventfully.";
	}
	ss << "\n \n";

	if (girl.is_unpaid())
		drinkssold *= 0.9;

#pragma endregion
#pragma region	//	Money					//

	// drinks are sold for 3gp each, if there are not enough in stock they cost 1gp.
	int ds = max(0, (int)drinkssold);
	int dw = max(0, (int)drinkswasted);
	int d1 = ds + dw;													// all drinks needed
	int d2 = g_Game->storage().drinks() >= d1 ? d1 : g_Game->storage().drinks();		// Drinks taken from stock
	int d3 = g_Game->storage().drinks() >= d1 ? 0 : d1 - g_Game->storage().drinks();	// Drinks needed to be bought
	int profit = (ds * 3) - (d3 * 1);
	g_Game->storage().add_to_drinks(-d2);
	if (profit < 0) profit = profit;
	else/*       */ profit = profit;
	if ((int)d1 > 0)
	{
		ss << "\n${name}";
		/* */if ((int)drinkssold <= 0)	ss << " didn't sell any drinks.";
		else if ((int)drinkssold == 1)	ss << " only sold one drink.";
		else/*                      */	ss << " sold " << ds << " drinks.";
		/* */if ((int)dw > 0)	ss << "\n" << dw << " were not paid for or were spilled.";
		/* */if (d2 > 0)/*           */ ss << "\n" << d2 << " drinks were taken from the bar's stock.";
		/* */if (d3 > 0)/*           */ ss << "\n" << d3 << " drinks had to be restocked durring the week at a cost of 1 gold each.";
		ss << "\n \n${name}";
		/* */if (profit > 0)/*       */	ss << " made you a profit of " << profit << " gold.";
		else if (profit < 0)/*       */	ss << " cost you " << profit << " gold.";
		else if (d1 > 0)/*           */ ss << " broke even for the week.";
		else/*                       */ ss << " made no money.";
	}

	if (girl.is_unpaid())
	{
        bool keep_tips = girl.keep_tips();
        if (!keep_tips)
		{
			wages += (int)tips;
			tips = 0;
		}
		/* */if ((int)wages > 0)	ss << "\n${name} turned in an extra " << (int)wages << " gold from other sources.";
		else if ((int)wages < 0)	ss << "\nShe cost you " << (int)wages << " gold from other sources.";
		if ((int)tips > 0 && keep_tips)
		{
			ss << "\nShe made " << (int)tips << " gold in tips";
			if ((int)wages < 0)
			{
				ss << " but you made her pay back what she could of the losses";
				int l = (int)tips + (int)wages;
				if (l > 0)		// she can pay it all
				{
					tips -= l;
					wages += l;
				}
				else
				{
					wages += (int)tips;
					tips = 0;
				}
			}
			ss << ".";
		}
		profit += (int)wages;	// all of it goes to the house
		wages = 0;
	}
	else
	{
		if (profit >= 10)	// base pay is 10 unless she makes less
		{
			ss << "\n \n"<< "${name} made the bar a profit so she gets paid 10 gold for the shift.";
			wages += 10;
			profit -= 10;
		}
		if (profit > 0)
		{
			int b = profit / 50;
			if (b > 0) ss << "\nShe gets 2% of the profit from her drink sales as a bonus totaling " << b << " gold.";
			wages += b;					// 2% of profit from drinks sold
			profit -= b;
			girl.happiness(b / 5);
		}
		if (dw > 0)
		{
			girl.happiness(-(dw / 5));

			int c = min(dw, (int)wages);
			int d = min(dw - c, (int)tips);
			int e = min(0, dw - d);
			bool left = false;
			if (dw < (int)wages)					// she pays for all wasted drinks out of wages
			{
				ss << "\nYou take 1 gold out of her pay for each drink she wasted ";
				wages -= c;
				profit += c;
				left = true;
			}
			else if (dw < (int)wages + (int)tips)	// she pays for all wasted drinks out of wages and tips
			{
				ss << "\nYou take 1 gold from her wages and tips for each drink she wasted ";
				wages -= c;
				tips -= d;
				profit += c + d;
				left = true;
			}
			else									// no pay plus she has to pay from her pocket
			{
				wages -= c;
				tips -= d;
				profit += c + d;
				if (girl.m_Money < 1)				// she can't pay so you scold her
				{
					girl.pcfear(rng.bell(-1,5));
					ss << "\nYou take all her wages and tips and then scold her for wasting so many drinks";
				}
				else if (girl.m_Money >= e)		// she has enough to pay it back
				{
					girl.pcfear(rng.bell(-1, 2));
					girl.pchate(rng.bell(-1, 2));
					ss << "\nYou take all her wages and tips and then make her pay for the rest out of her own money";
					girl.m_Money -= e;
					profit += e;
				}
				else								// she does not have all but can pay some
				{
					girl.pcfear(rng.bell(-1, 4));
					girl.pchate(rng.bell(-1, 2));
					ss << "\nYou take all her wages and tips and then make her pay for what she can of the rest out of her own money";
					e = girl.m_Money;
					girl.m_Money -= e;
					profit += e;
				}
			}

			if (left)
			{
				ss << "leaving her with ";
				/* */if ((int)wages + (int)tips < 1)	ss << "nothing";
				else if ((int)wages + (int)tips < 2)	ss << "just one gold";
				else/*                            */	ss << (int)wages + (int)tips << "gold";
			}
			ss << ".";
		}
	}



	// tiredness
#if 1	// `J` had some issues with tiredness so replacing it with a less complicated method until a better way is found 'Mute' Updated to fix math logic if this doesnt work feel free to switch back
	int t0 = d1;
	int easydrinks = (girl.constitution() + girl.service()) / 4;
	int mediumdrinks = (girl.constitution() + girl.service()) /2;
	int haarddrinks = (girl.constitution() + girl.service());
	int t1 = easydrinks;					// 1 tired per 20 drinks
	int t2 = max(0, t0 - t1);		        // 1 tired per 10 drinks
	int t3 = max(0, t0 - (t1+t2));		    // 1 tired per 2 drinks
	int tired = max(0,(t1/20))+max(0,(t2/10))+max(0,(t3/2));
#else
	int tired = max(1, (600 - ((int)jobperformance + (girl.constitution() * 3))) / 10);

#endif

	// Money
	girl.m_Tips = max(0, (int)tips);
	girl.m_Pay = max(0, wages);

	g_Game->gold().bar_income(profit);

#pragma endregion
#pragma region	//	Finish the shift			//

	brothel->m_Happiness += Bhappy;
	brothel->m_Fame += Bfame;
	brothel->m_Filthiness += Bfilth;

	girl.AddMessage(ss.str(), imagetype, msgtype);


	// Improve stats
	int xp = 10, skill = 3;

	if (girl.has_active_trait("Quick Learner"))		{ skill += 1; xp += 3; }
	else if (girl.has_active_trait("Slow Learner"))	{ skill -= 1; xp -= 3; }
	if (girl.fame() < 10 && jobperformance >= 70)		{ fame += 1; }
	if (girl.fame() < 20 && jobperformance >= 100)		{ fame += 1; }
	if (girl.fame() < 40 && jobperformance >= 145)		{ fame += 1; }
	if (girl.fame() < 60 && jobperformance >= 185)		{ fame += 1; }
    girl.fame(fame);
    girl.tiredness(tired);

	girl.exp(xp);
	if (rng % 2 == 1)
		girl.intelligence(rng%skill);
	else
		girl.performance(rng%skill);
	girl.service(rng%skill + 1);

    girl.upd_Enjoyment(actiontype, enjoy);

	//gain traits
	cGirls::PossiblyGainNewTrait(&girl, "Charismatic", 60, ACTION_WORKBAR, "Dealing with customers at the bar and talking with them about their problems has made ${name} more Charismatic.", Day0Night1);
	if (jobperformance < 100 && roll_c <= 2) cGirls::PossiblyGainNewTrait(&girl, "Assassin", 10, ACTION_WORKBAR, "${name}'s lack of skill at mixing drinks has been killing people left and right making her into quite the Assassin.", Day0Night1);

	//lose traits
	cGirls::PossiblyLoseExistingTrait(&girl, "Nervous", 40, ACTION_WORKBAR, "${name} seems to finally be getting over her shyness. She's not always so Nervous anymore.", Day0Night1);

#pragma endregion
	return false;
}
double JP_Barmaid(const sGirl& girl, bool estimate)// not used
{
    //SIN - standardizing job performance calc per J's instructs
	double jobperformance =
		//main stats - first 100 - needs to be smart and good at service
		((girl.service() + girl.intelligence()) / 2) +
		//secondary stats - second 100 - charming, good performer and able to mix a drink
		((girl.charisma() + girl.performance() + girl.brewing()) / 3) +
		//add level
		girl.level();

	// next up tiredness penalty
	if (!estimate)
	{
		int t = girl.tiredness() - 80;
		if (t > 0)
			jobperformance -= (t + 2) * (t / 3);
	}

    jobperformance += girl.get_trait_modifier("work.barmaid");

	return jobperformance;
}