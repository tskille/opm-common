/*
  Copyright 2018 Equinor ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <algorithm>

#include <opm/parser/eclipse/EclipseState/Schedule/Action/Actions.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionX.hpp>

namespace Opm {
namespace Action {


Actions::Actions(const std::vector<ActionX>& action)
    : actions(action)
{}


size_t Actions::size() const {
    return this->actions.size();
}


bool Actions::empty() const {
    return this->actions.empty();
}


void Actions::add(const ActionX& action) {
    auto iter = std::find_if( this->actions.begin(), this->actions.end(), [&action](const ActionX& arg) { return arg.name() == action.name(); });
    if (iter == this->actions.end())
        this->actions.push_back(action);
    else
        *iter = action;
}


const ActionX& Actions::get(const std::string& name) const {
    const auto iter = std::find_if( this->actions.begin(), this->actions.end(), [&name](const ActionX& action) { return action.name() == name; });
    if (iter == this->actions.end())
        throw std::range_error("No such action: " + name);

    return *iter;
}

const ActionX& Actions::get(std::size_t index) const {
    return this->actions[index];
}

int Actions::max_input_lines() const {
    std::size_t max_il = 0;
    for (const auto& act : this-> actions) {
        if (act.keyword_strings().size() > max_il) max_il = act.keyword_strings().size() ;
    }
    return static_cast<int>(max_il);
}


bool Actions::ready(std::time_t sim_time) const {
    for (const auto& action : this->actions) {
        if (action.ready(sim_time))
            return true;
    }
    return false;
}


std::vector<const ActionX *> Actions::pending(std::time_t sim_time) const {
    std::vector<const ActionX *> action_vector;
    for (const auto& action : this->actions) {
        if (action.ready(sim_time))
            action_vector.push_back( &action );
    }
    return action_vector;
}

std::vector<ActionX>::const_iterator Actions::begin() const {
    return this->actions.begin();
}

std::vector<ActionX>::const_iterator Actions::end() const {
    return this->actions.end();
}


const std::vector<ActionX>& Actions::getActions() const {
    return actions;
}


bool Actions::operator==(const Actions& data) const {
    return actions == data.actions;
}

}
}
