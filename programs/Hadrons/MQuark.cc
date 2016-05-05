/*******************************************************************************
Grid physics library, www.github.com/paboyle/Grid 

Source file: programs/Hadrons/MQuark.cc

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

#include <Hadrons/MQuark.hpp>

using namespace Grid;
using namespace QCD;
using namespace Hadrons;

/******************************************************************************
 *                          MQuark implementation                             *
 ******************************************************************************/
// constructor /////////////////////////////////////////////////////////////////
MQuark::MQuark(const std::string name)
: Module(name)
{}

// parse parameters ////////////////////////////////////////////////////////////
void MQuark::parseParameters(XmlReader &reader, const std::string name)
{
    read(reader, name, par_);
}

// dependencies/products ///////////////////////////////////////////////////////
std::vector<std::string> MQuark::getInput(void)
{
    std::vector<std::string> in = {par_.source, par_.solver};
    
    return in;
}

std::vector<std::string> MQuark::getOutput(void)
{
    std::vector<std::string> out = {getName(), getName() + "_5d"};
    
    return out;
}

// setup ///////////////////////////////////////////////////////////////////////
void MQuark::setup(void)
{
    auto dim = env().getFermionMatrix(par_.solver)->Grid()->GlobalDimensions();
    
    if (dim.size() == Nd)
    {
        Ls_ = 1;
    }
    else
    {
        Ls_ = dim[0];
    }
}

// allocation //////////////////////////////////////////////////////////////////
void MQuark::allocate(void)
{
    env().create<LatticePropagator>(getName());
    quark_ = env().get<LatticePropagator>(getName());
    if (Ls_ > 1)
    {
        env().create<LatticePropagator>(getName() + "_5d", Ls_);
        quark5d_ = env().get<LatticePropagator>(getName() + "_5d");
    }
}

// execution ///////////////////////////////////////////////////////////////////
void MQuark::execute(void)
{
    LatticePropagator *fullSource;
    LatticeFermion    source(env().getGrid(Ls_)), sol(env().getGrid(Ls_));
    
    LOG(Message) << "Computing quark propagator '" << getName() << "'"
                 << std::endl;
    if (!env().isLattice5d(par_.source))
    {
        if (Ls_ == 1)
        {
            fullSource = env().get<LatticePropagator>(par_.source);
        }
        else
        {
            HADRON_ERROR("MQuark not implemented with 5D actions");
        }
    }
    else
    {
        if (Ls_ == 1)
        {
            HADRON_ERROR("MQuark not implemented with 5D actions");
        }
        else if (Ls_ != env().getLatticeLs(par_.source))
        {
            HADRON_ERROR("MQuark not implemented with 5D actions");
        }
        else
        {
            fullSource = env().get<LatticePropagator>(par_.source);
        }
    }
    LOG(Message) << "Inverting using solver '" << par_.solver
                 << "' on source '" << par_.source << "'" << std::endl;
    for (unsigned int s = 0; s < Ns; ++s)
    for (unsigned int c = 0; c < Nc; ++c)
    {
        PropToFerm(source, *fullSource, s, c);
        sol = zero;
        env().callSolver(par_.solver, sol, source);
        if (Ls_ == 1)
        {
            FermToProp(*quark_, sol, s, c);
        }
        else
        {
            HADRON_ERROR("MQuark not implemented with 5D actions");
        }
    }
}
