/*******************************************************************************
Grid physics library, www.github.com/paboyle/Grid 

Source file: programs/Hadrons/GLoad.cc

Copyright (C) 2016

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

#include <Hadrons/GLoad.hpp>

using namespace Grid;
using namespace Hadrons;

/******************************************************************************
*                          GLoad implementation                               *
******************************************************************************/
// constructor /////////////////////////////////////////////////////////////////
GLoad::GLoad(const std::string name)
: Module(name)
{}

// parse parameters ////////////////////////////////////////////////////////////
void GLoad::parseParameters(XmlReader &reader, const std::string name)
{
   read(reader, name, par_);
}

// dependencies/products ///////////////////////////////////////////////////////
std::vector<std::string> GLoad::getInput(void)
{
    std::vector<std::string> in;
    
    return in;
}

std::vector<std::string> GLoad::getOutput(void)
{
    std::vector<std::string> out = {getName()};
    
    return out;
}

// allocation //////////////////////////////////////////////////////////////////
void GLoad::allocate(Environment &env)
{
    env.createGauge(getName());
    gauge_ = env.getGauge(getName());
}

// execution ///////////////////////////////////////////////////////////////////
void GLoad::execute(Environment &env)
{
    NerscField  header;
    std::string fileName = par_.file + "."
                           + std::to_string(env.getTrajectory());
    
    LOG(Message) << "loading NERSC configuration from file '" << fileName
                 << "'" << std::endl;
    NerscIO::readConfiguration(*gauge_, header, fileName);
}
