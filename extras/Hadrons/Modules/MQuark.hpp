/*******************************************************************************
Grid physics library, www.github.com/paboyle/Grid 

Source file: programs/Hadrons/MQuark.hpp

Copyright (C) 2015

Author: Antonin Portelli <antonin.portelli@me.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

See the full license in the file "LICENSE" in the top level distribution 
directory.
*******************************************************************************/

#ifndef Hadrons_MQuark_hpp_
#define Hadrons_MQuark_hpp_

#include <Grid/Hadrons/Global.hpp>
#include <Grid/Hadrons/Module.hpp>
#include <Grid/Hadrons/ModuleFactory.hpp>

BEGIN_HADRONS_NAMESPACE

/******************************************************************************
 *                               MQuark                                       *
 ******************************************************************************/
class MQuarkPar: Serializable
{
public:
    GRID_SERIALIZABLE_CLASS_MEMBERS(MQuarkPar,
                                    std::string, source,
                                    std::string, solver);
};

class MQuark: public Module<MQuarkPar>
{
public:
    // constructor
    MQuark(const std::string name);
    // destructor
    virtual ~MQuark(void) = default;
    // dependencies/products
    virtual std::vector<std::string> getInput(void);
    virtual std::vector<std::string> getOutput(void);
    // setup
    virtual void setup(void);
    // execution
    virtual void execute(void);
private:
    unsigned int Ls_;
    SolverFn     *solver_{nullptr};
};

MODULE_REGISTER(MQuark);

END_HADRONS_NAMESPACE

#endif // Hadrons_MQuark_hpp_
