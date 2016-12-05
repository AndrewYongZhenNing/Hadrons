/*******************************************************************************
Grid physics library, www.github.com/paboyle/Grid 

Source file: programs/Hadrons/Module.hpp

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

#ifndef Hadrons_Module_hpp_
#define Hadrons_Module_hpp_

#include <Grid/Hadrons/Global.hpp>
#include <Grid/Hadrons/Environment.hpp>

BEGIN_HADRONS_NAMESPACE

// module registration macros
#define MODULE_REGISTER(mod)\
class mod##ModuleRegistrar\
{\
public:\
    mod##ModuleRegistrar(void)\
    {\
        ModuleFactory &modFac = ModuleFactory::getInstance();\
        modFac.registerBuilder(#mod, [&](const std::string name)\
                              {\
                                  return std::unique_ptr<mod>(new mod(name));\
                              });\
    }\
};\
static mod##ModuleRegistrar mod##ModuleRegistrarInstance;

#define MODULE_REGISTER_NS(mod, ns)\
class ns##mod##ModuleRegistrar\
{\
public:\
    ns##mod##ModuleRegistrar(void)\
    {\
        ModuleFactory &modFac = ModuleFactory::getInstance();\
        modFac.registerBuilder(#ns "::" #mod, [&](const std::string name)\
                              {\
                                  return std::unique_ptr<ns::mod>(new ns::mod(name));\
                              });\
    }\
};\
static ns##mod##ModuleRegistrar ns##mod##ModuleRegistrarInstance;

/******************************************************************************
 *                            Module class                                    *
 ******************************************************************************/
// base class
class ModuleBase
{
public:
    // constructor
    ModuleBase(const std::string name);
    // destructor
    virtual ~ModuleBase(void) = default;
    // access
    std::string getName(void) const;
    Environment &env(void) const;
    // dependencies/products
    virtual std::vector<std::string> getInput(void) = 0;
    virtual std::vector<std::string> getOutput(void) = 0;
    // parse parameters
    virtual void parseParameters(XmlReader &reader, const std::string name) = 0;
    // setup
    virtual void setup(void) {};
    // execution
    void operator()(void);
    virtual void execute(void) = 0;
private:
    std::string name_;
    Environment &env_;
};

// derived class, templating the parameter class
template <typename P>
class Module: public ModuleBase
{
public:
    typedef P Par;
public:
    // constructor
    Module(const std::string name);
    // destructor
    virtual ~Module(void) = default;
    // parse parameters
    virtual void parseParameters(XmlReader &reader, const std::string name);
    // parameter access
    const P & par(void) const;
    void      setPar(const P &par);
private:
    P par_;
};

// no parameter type
class NoPar {};

template <>
class Module<NoPar>: public ModuleBase
{
public:
    // constructor
    Module(const std::string name): ModuleBase(name) {};
    // destructor
    virtual ~Module(void) = default;
    // parse parameters (do nothing)
    virtual void parseParameters(XmlReader &reader, const std::string name) {};
};

/******************************************************************************
 *                           Template implementation                          *
 ******************************************************************************/
template <typename P>
Module<P>::Module(const std::string name)
: ModuleBase(name)
{}

template <typename P>
void Module<P>::parseParameters(XmlReader &reader, const std::string name)
{
    read(reader, name, par_);
}

template <typename P>
const P & Module<P>::par(void) const
{
    return par_;
}

template <typename P>
void Module<P>::setPar(const P &par)
{
    par_ = par;
}

END_HADRONS_NAMESPACE

#endif // Hadrons_Module_hpp_
