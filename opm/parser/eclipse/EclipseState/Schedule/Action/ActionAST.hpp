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


#ifndef ActionAST_HPP
#define ActionAST_HPP

#include <string>
#include <vector>
#include <memory>

#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionResult.hpp>

namespace Opm {
namespace Action {

class Context;
class ASTNode;


/*
  The Action::AST class implements a tree with the result of the parsing of the
  ACTIONX condition. The AST does not contain any context, that is supplied with
  a Action::Context instace when calling the eval() methoid is called.
*/

class AST{
public:
    AST() = default;
    explicit AST(const std::vector<std::string>& tokens);
    AST(const std::shared_ptr<ASTNode>& cond);
    Result eval(const Context& context) const;

    std::shared_ptr<ASTNode> getCondition() const;

    bool operator==(const AST& data) const;
private:
    /*
      The use of a pointer here is to be able to create this class with only a
      forward declaration of the ASTNode class. Would have prefered to use a
      unique_ptr, but that requires writing custom destructors - the use of a
      shared_ptr does not imply any shared ownership of the ASTNode.
    */
    std::shared_ptr<ASTNode> condition;
};
}
}
#endif
