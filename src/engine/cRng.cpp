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
#include"cRng.h"

int cRng::in_range(int min, int max, int range)
{
    int diff = max - min;
    if (min < 0 && max>0) diff++;
    if(diff == 0) return max;
    if(diff < 0) return random(range);
    return min + random(diff);
}

int cRng::bell(int min, int max)    // `J` added - not sure how well it will work, I'm not too good at math
{
    if (min == max) return max;
    double bmin, bmax, bmid, blow, bhii, test;
    if (min < max) { bmin = min; bmax = max; } else { bmin = max; bmax = min; }
    bmax++;// to correct random+1
    if (bmin < 0) bmin--;
    bmid = (bmin + bmax) / 2.0;
    blow = bmid - bmin;
    bhii = bmax - bmid;
    test = in_range((int)bmin, (int)bmax);
    /* */if (test < bmid) test += randomd(bhii);
    else if (test >= bmid) test -= randomd(blow);

    if (test < min)    return min;
    if (test > max) return max;
    return (int)test;
}

const char* cRng::select_text(std::initializer_list<const char*> options) {
    auto option = random(options.size());
    return *(options.begin() + option);
}

//end mod
