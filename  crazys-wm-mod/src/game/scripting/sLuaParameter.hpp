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


#ifndef CRAZYS_WM_MOD_SLUAPARAMETER_HPP
#define CRAZYS_WM_MOD_SLUAPARAMETER_HPP

#include <boost/variant.hpp>

class sGirl;
class sCustomer;

namespace scripting {
	class cLuaState;

	class sLuaParameter {
	public:
		explicit sLuaParameter(sGirl* girl);
		explicit sLuaParameter(sGirl& girl);
		explicit sLuaParameter(sCustomer* cust);
		explicit sLuaParameter(sCustomer& cust);

		void push(cLuaState& target) const;
	private:
	enum Type {
		GIRL, CUSTOMER
	} type;
	union {
		sGirl* girl;
		sCustomer* customer;
	} data;
	};
}

#endif //CRAZYS_WM_MOD_SLUAPARAMETER_HPP
