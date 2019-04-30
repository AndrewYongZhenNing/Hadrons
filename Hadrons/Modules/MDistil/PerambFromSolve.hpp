/*************************************************************************************
 
 Grid physics library, www.github.com/paboyle/Grid
 
 Source file: Hadrons/Modules/MDistil/PerambFromSolve.hpp
 
 Copyright (C) 2019
 
 Author: Felix Erben <ferben@ed.ac.uk>
 Author: Michael Marshall <Michael.Marshall@ed.ac.uk>
 
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
 
 See the full license in the file "LICENSE" in the top level distribution directory
 *************************************************************************************/
/*  END LEGAL */

#ifndef Hadrons_MDistil_PerambFromSolve_hpp_
#define Hadrons_MDistil_PerambFromSolve_hpp_

#include <Hadrons/Global.hpp>
#include <Hadrons/Module.hpp>
#include <Hadrons/ModuleFactory.hpp>
#include <Hadrons/Solver.hpp>
#include <Hadrons/EigenPack.hpp>
#include <Hadrons/A2AVectors.hpp>
#include <Hadrons/DilutedNoise.hpp>

// These are members of Distillation
#include <Hadrons/Distil.hpp>
 
BEGIN_HADRONS_NAMESPACE

/******************************************************************************
 *                         PerambFromSolve                                 *
 ******************************************************************************/
BEGIN_MODULE_NAMESPACE(MDistil)
/*
struct DistilParameters: Serializable {
  GRID_SERIALIZABLE_CLASS_MEMBERS(DistilParameters,
                                  int, TI,
                                  int, LI,
                                  int, nnoise,
                                  int, tsrc,
                                  int, SI,
                                  int, Ns,
                                  int, Nt,
                                  int, Nt_inv)
  DistilParameters() = default;
  template <class ReaderClass> DistilParameters(Reader<ReaderClass>& Reader){read(Reader,"Distil",*this);}
};
*/

class PerambFromSolvePar: Serializable
{
public:
    GRID_SERIALIZABLE_CLASS_MEMBERS(PerambFromSolvePar,
                                    std::string, eigenPack,
                                    std::string, PerambFileName,
                                    std::string, solve,
                                    int, nvec,
                                    int, nvec_reduced,
                                    int, LI_reduced,
                                    DistilParameters, Distil);
};

template <typename FImpl>
class TPerambFromSolve: public Module<PerambFromSolvePar>
{
public:
    FERM_TYPE_ALIASES(FImpl,);
    // constructor
    TPerambFromSolve(const std::string name);
    // destructor
    virtual ~TPerambFromSolve(void);
    // dependency relation
    virtual std::vector<std::string> getInput(void);
    virtual std::vector<std::string> getOutput(void);
    // setup
    virtual void setup(void);
    // execution
    virtual void execute(void);
protected:
	GridCartesian * grid3d; // Owned by me, so I must delete it
	GridCartesian * grid4d;
protected:
	    virtual void Cleanup(void);

};

MODULE_REGISTER_TMP(PerambFromSolve, TPerambFromSolve<FIMPL>, MDistil);

/******************************************************************************
 *                 TPerambFromSolve implementation                             *
 ******************************************************************************/
// constructor /////////////////////////////////////////////////////////////////
template <typename FImpl>
TPerambFromSolve<FImpl>::TPerambFromSolve(const std::string name)
:grid3d{nullptr}, grid4d{nullptr}, Module<PerambFromSolvePar>(name)
{}
//destructor
template <typename FImpl>
TPerambFromSolve<FImpl>::~TPerambFromSolve(void)
{
  Cleanup();
};


// dependencies/products ///////////////////////////////////////////////////////
template <typename FImpl>
std::vector<std::string> TPerambFromSolve<FImpl>::getInput(void)
{
    std::vector<std::string> in;

    in.push_back(par().solve);
    in.push_back(par().eigenPack);
    
    return in;
}

template <typename FImpl>
std::vector<std::string> TPerambFromSolve<FImpl>::getOutput(void)
{
    std::vector<std::string> out = {getName()};
    
    return out;
}

// setup ///////////////////////////////////////////////////////////////////////
template <typename FImpl>
void TPerambFromSolve<FImpl>::setup(void)
{
		  Cleanup();
    
	const int nvec{par().nvec};
    const DistilParameters & Distil{par().Distil};
    const int LI{Distil.LI};
    const int nnoise{Distil.nnoise};
    const int Nt_inv{Distil.Nt_inv};
    const int Ns{Distil.Ns};
    std::array<std::string,6> sIndexNames{"Nt", "nvec", "LI", "nnoise", "Nt_inv", "SI"};

    grid4d = env().getGrid();
        grid3d = MakeLowerDimGrid(grid4d);
  const int Nt{grid4d->GlobalDimensions()[Tdir]};


	const int nvec_reduced{par().nvec_reduced};
	const int LI_reduced{par().LI_reduced};
    //envCreate(Perambulator<SpinVector COMMA 6 COMMA sizeof(Real)>, getName(), 1,
    //          sIndexNames,Distil.Nt,nvec,Distil.LI,Distil.nnoise,Distil.Nt_inv,Distil.SI);
    envCreate(Perambulator<SpinVector COMMA 6 COMMA sizeof(Real)>, getName(), 1,
              sIndexNames,Nt,nvec_reduced,LI_reduced,Distil.nnoise,Distil.Nt_inv,Distil.SI);
    envCreate(std::vector<Complex>, getName() + "_noise", 1,
              nvec*Distil.Ns*Nt*Distil.nnoise);

    envTmp(LatticeColourVector, "result_3d",1,LatticeColourVector(grid3d));
        envTmp(LatticeColourVector, "evec3d",1,LatticeColourVector(grid3d));
	    envTmpLat(LatticeColourVector, "result_nospin");


}

template <typename FImpl>
void TPerambFromSolve<FImpl>::Cleanup(void)
{
	  if( grid3d != nullptr ) {
		      delete grid3d;
		          grid3d = nullptr;
			    }
	    grid4d = nullptr;
}


// execution ///////////////////////////////////////////////////////////////////
template <typename FImpl>
void TPerambFromSolve<FImpl>::execute(void)
{
  GridCartesian * grid4d = env().getGrid();
  const int Nt{grid4d->GlobalDimensions()[Tdir]};
  const int Ntlocal{grid4d->LocalDimensions()[3]};
  const int Ntfirst{grid4d->LocalStarts()[3]};

  const int nvec_reduced{par().nvec_reduced};
  const int LI_reduced{par().LI_reduced};
  const int nvec{par().nvec};
  const DistilParameters & Distil{par().Distil};
  const int LI{Distil.LI};
  const int TI{Distil.TI};
  const int SI{Distil.SI};
  const int nnoise{Distil.nnoise};
  const int Nt_inv{Distil.Nt_inv};
  const int tsrc{Distil.tsrc};
  const int Ns{Distil.Ns};
  const bool full_tdil{TI==Nt};
  const bool exact_distillation{full_tdil && LI==nvec};
  auto &perambulator = envGet(Perambulator<SpinVector COMMA 6 COMMA sizeof(Real)>, getName());
  auto &solve       = envGet(std::vector<FermionField>, par().solve);
  auto &epack   = envGet(Grid::Hadrons::EigenPack<LatticeColourVector>, par().eigenPack);

  envGetTmp(LatticeColourVector, result_nospin);
  envGetTmp(LatticeColourVector, result_3d);
  envGetTmp(LatticeColourVector, evec3d);

  for (int inoise = 0; inoise < nnoise; inoise++) {
    for (int dk = 0; dk < LI_reduced; dk++) {
      for (int dt = 0; dt < Nt_inv; dt++) {
        for (int ds = 0; ds < SI; ds++) {
          for (int is = 0; is < Ns; is++) {
            result_nospin = peekSpin(solve[inoise+nnoise*(dk+LI*(dt+Nt_inv*ds))],is);
            for (int t = Ntfirst; t < Ntfirst + Ntlocal; t++) {
              ExtractSliceLocal(result_3d,result_nospin,0,t-Ntfirst,Grid::QCD::Tdir);
              for (int ivec = 0; ivec < nvec_reduced; ivec++) {
                ExtractSliceLocal(evec3d,epack.evec[ivec],0,t,3);
                pokeSpin(perambulator(t, ivec, dk, inoise,dt,ds),static_cast<Complex>(innerProduct(evec3d, result_3d)),is);
                std::cout <<  "perambulator(t, ivec, dk, inoise,dt,ds)(is) = (" << t << "," << ivec << "," << dk << "," << inoise << "," << dt << "," << ds << ")(" << is << ") = " <<  perambulator(t, ivec, dk, inoise,dt,ds)()(is)() << std::endl;
              }
            }
          }
        }
      }
    }
  }

  if(grid4d->IsBoss()) {
    std::string sPerambName{par().PerambFileName};
    if( sPerambName.length() == 0 )
      sPerambName = getName();
    sPerambName.append( "." );
    sPerambName.append( std::to_string(vm().getTrajectory()));
    //perambulator.WriteBinary(sPerambName);
    perambulator.write(sPerambName.c_str());
  }
}

END_MODULE_NAMESPACE

END_HADRONS_NAMESPACE

#endif // Hadrons_MDistil_PerambFromSolve_hpp_
