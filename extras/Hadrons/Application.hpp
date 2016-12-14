/*******************************************************************************
Grid physics library, www.github.com/paboyle/Grid 

Source file: programs/Hadrons/Application.hpp

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

#ifndef Hadrons_Application_hpp_
#define Hadrons_Application_hpp_

#include <Grid/Hadrons/Global.hpp>
#include <Grid/Hadrons/Environment.hpp>
#include <Grid/Hadrons/ModuleFactory.hpp>
#include <Grid/Hadrons/Modules.hpp>

BEGIN_HADRONS_NAMESPACE

/******************************************************************************
 *                         Main program manager                               *
 ******************************************************************************/
class Application
{
public:
    class TrajRange: Serializable
    {
    public:
        GRID_SERIALIZABLE_CLASS_MEMBERS(TrajRange,
                                        unsigned int, start,
                                        unsigned int, end,
                                        unsigned int, step);
    };
    class GeneticPar: Serializable
    {
    public:
        GeneticPar(void):
            popSize{20}, maxGen{1000}, maxCstGen{100}, mutationRate{.1} {};
    public:
        GRID_SERIALIZABLE_CLASS_MEMBERS(GeneticPar,
                                        unsigned int, popSize,
                                        unsigned int, maxGen,
                                        unsigned int, maxCstGen,
                                        double      , mutationRate);
    };
    class GlobalPar: Serializable
    {
    public:
        GRID_SERIALIZABLE_CLASS_MEMBERS(GlobalPar,
                                        TrajRange,   trajCounter,
                                        GeneticPar,  genetic,
                                        std::string, seed);
    };
public:
    // constructors
    Application(void);
    Application(const GlobalPar &par);
    Application(const std::string parameterFileName);
    // destructor
    virtual ~Application(void) = default;
    // access
    void              setPar(const GlobalPar &par);
    const GlobalPar & getPar(void);
    // module creation
    template <typename M>
    void createModule(const std::string name);
    template <typename M>
    void createModule(const std::string name, const typename M::Par &par);
    // execute
    void run(void);
    // XML parameter file I/O
    void parseParameterFile(const std::string parameterFileName);
    void saveParameterFile(const std::string parameterFileName);
    // schedule computation
    void schedule(void);
    void saveSchedule(const std::string filename);
    void loadSchedule(const std::string filename);
    void printSchedule(void);
    // loop on configurations
    void configLoop(void);
private:
    long unsigned int         locVol_;
    std::string               parameterFileName_{""};
    GlobalPar                 par_;
    Environment               &env_;
    std::vector<unsigned int> program_;
    Environment::Size         memPeak_;
    bool                      scheduled_{false};
};

/******************************************************************************
 *                     Application template implementation                    *
 ******************************************************************************/
// module creation /////////////////////////////////////////////////////////////
template <typename M>
void Application::createModule(const std::string name)
{
    env_.createModule<M>(name);
}

template <typename M>
void Application::createModule(const std::string name,
                               const typename M::Par &par)
{
    env_.createModule<M>(name, par);
}

END_HADRONS_NAMESPACE

#endif // Hadrons_Application_hpp_
