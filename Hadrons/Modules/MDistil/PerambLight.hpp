#ifndef Hadrons_MDistil_PerambLight_hpp_
#define Hadrons_MDistil_PerambLight_hpp_

#include <Hadrons/Global.hpp>
#include <Hadrons/Module.hpp>
#include <Hadrons/ModuleFactory.hpp>
#include <Hadrons/Solver.hpp>
#include <Hadrons/EigenPack.hpp>
#include <Hadrons/A2AVectors.hpp>
#include <Hadrons/DilutedNoise.hpp>

// These are members of Distillation
#include <Hadrons/Modules/MDistil/Distil.hpp>

BEGIN_HADRONS_NAMESPACE


/******************************************************************************
 *                             PerambLight                                    *
 ******************************************************************************/
BEGIN_MODULE_NAMESPACE(MDistil)

class PerambLightPar: Serializable
{
public:
    GRID_SERIALIZABLE_CLASS_MEMBERS(PerambLightPar,
		                    std::string, eigenPack,
                                    std::string, PerambFileName, //stem!!!
                                    std::string, UniqueIdentifier,
                                    bool, multiFile,
                                    int, nvec,
                                    DistilParameters, Distil,
				    std::string, solver);
};

template <typename FImpl>
class TPerambLight: public Module<PerambLightPar>
{
public:
    FERM_TYPE_ALIASES(FImpl,);
    SOLVER_TYPE_ALIASES(FImpl,);
    // constructor
    TPerambLight(const std::string name);
    // destructor
    virtual ~TPerambLight(void);
    // dependency relation
    virtual std::vector<std::string> getInput(void);
    virtual std::vector<std::string> getOutput(void);
    // setup
    virtual void setup(void);
    // execution
    virtual void execute(void);
protected:
    // These variables are created in setup() and freed in Cleanup()
    GridCartesian * grid3d; // Owned by me, so I must delete it
    GridCartesian * grid4d; // Owned by environment (so I won't delete it)
protected:
    virtual void Cleanup(void);
private:
        unsigned int Ls_;
};

MODULE_REGISTER_TMP(PerambLight, TPerambLight<FIMPL>, MDistil);

/******************************************************************************
 *                 TPerambLight implementation                             *
 ******************************************************************************/
// constructor /////////////////////////////////////////////////////////////////
template <typename FImpl>
TPerambLight<FImpl>::TPerambLight(const std::string name)
: grid3d{nullptr}, grid4d{nullptr}, Module<PerambLightPar>(name)
{}

// destructor
template <typename FImpl>
TPerambLight<FImpl>::~TPerambLight(void)
{
  Cleanup();
};

// dependencies/products ///////////////////////////////////////////////////////
template <typename FImpl>
std::vector<std::string> TPerambLight<FImpl>::getInput(void)
{
    std::vector<std::string> in;

    in.push_back(par().eigenPack);
    in.push_back(par().solver);
    
    return in;
}

template <typename FImpl>
std::vector<std::string> TPerambLight<FImpl>::getOutput(void)
{
    std::vector<std::string> out = {getName() + "_perambulator_light",getName() + "_noise",getName() + "_unsmeared_sink"};
    
    return out;
}

// setup ///////////////////////////////////////////////////////////////////////
template <typename FImpl>
void TPerambLight<FImpl>::setup(void)
{
    Cleanup();

    const int nvec{par().nvec};
    const DistilParameters & Distil{par().Distil};
    const int LI{Distil.LI};
    const int nnoise{Distil.nnoise};
    const int Nt_inv{Distil.Nt_inv}; // TODO: PROBABLY BETTER: if (full_tdil) Nt_inv=1; else Nt_inv = TI;
    const int Ns{Distil.Ns};
    std::array<std::string,6> sIndexNames{"Nt", "nvec", "LI", "nnoise", "Nt_inv", "SI"};

    envCreate(Perambulator<SpinVector COMMA 6 COMMA sizeof(Real)>, getName() + "_perambulator_light", 1,
              sIndexNames,Distil.Nt,nvec,Distil.LI,Distil.nnoise,Distil.Nt_inv,Distil.SI);
    envCreate(std::vector<Complex>, getName() + "_noise", 1,
              nvec*Distil.Ns*Distil.Nt*Distil.nnoise);
    envCreate(std::vector<FermionField>, getName() + "_unsmeared_sink", 1, 
            nnoise*LI*Ns*Nt_inv, envGetGrid(FermionField));

    grid4d = env().getGrid();
    grid3d = MakeLowerDimGrid(grid4d);//new GridCartesian(latt_size,simd_layout_3,mpi_layout,*grid4d);

    envTmpLat(LatticeSpinColourVector, "dist_source");
    envTmpLat(LatticeSpinColourVector, "tmp2");
    envTmpLat(LatticeSpinColourVector, "result");
    envTmpLat(LatticeColourVector, "result_nospin");
    envTmp(LatticeSpinColourVector, "tmp3d",1,LatticeSpinColourVector(grid3d));
    envTmp(LatticeColourVector, "tmp3d_nospin",1,LatticeColourVector(grid3d));
    envTmp(LatticeColourVector, "result_3d",1,LatticeColourVector(grid3d));
    envTmp(LatticeColourVector, "evec3d",1,LatticeColourVector(grid3d));

    Ls_ = env().getObjectLs(par().solver);
    envTmpLat(FermionField, "v4dtmp");
    envTmpLat(FermionField, "v5dtmp", Ls_);
    envTmpLat(FermionField, "v5dtmp_sol", Ls_);

}

// clean up any temporaries created by setup (that aren't stored in the environment)
template <typename FImpl>
void TPerambLight<FImpl>::Cleanup(void)
{
  if( grid3d != nullptr ) {
    delete grid3d;
    grid3d = nullptr;
  }
  grid4d = nullptr;
}

// execution ///////////////////////////////////////////////////////////////////
template <typename FImpl>
void TPerambLight<FImpl>::execute(void)
{
    const int nvec{par().nvec};
    const DistilParameters & Distil{par().Distil};
    const int LI{Distil.LI};
    //const int SI{Distil.SI};
    const int TI{Distil.TI};
    const int nnoise{Distil.nnoise};
    const int Nt{Distil.Nt};
    const int Nt_inv{Distil.Nt_inv}; // TODO: PROBABLY BETTER: if (full_tdil) Nt_inv=1; else Nt_inv = TI;
    const int tsrc{Distil.tsrc};
    const int Ns{Distil.Ns};
  
    auto &solver=envGet(Solver, par().solver);
    auto &mat = solver.getFMat();
    envGetTmp(FermionField, v4dtmp);
    envGetTmp(FermionField, v5dtmp);
    envGetTmp(FermionField, v5dtmp_sol);


    const bool full_tdil{TI==Nt};
    const bool exact_distillation{full_tdil && LI==nvec};

    const std::string &UniqueIdentifier{par().UniqueIdentifier};

    auto        &noise   = envGet(std::vector<Complex>, getName() + "_noise");
    auto        &perambulator = envGet(Perambulator<SpinVector COMMA 6 COMMA sizeof(Real)>,
                                       getName() + "_perambulator_light");
    auto        &epack   = envGet(Grid::Hadrons::EigenPack<LatticeColourVector>, par().eigenPack);
    auto        &unsmeared_sink       = envGet(std::vector<FermionField>, getName() + "_unsmeared_sink");

    //Create Noises
    GridSerialRNG sRNG; 
    sRNG.SeedUniqueString(UniqueIdentifier + std::to_string(vm().getTrajectory())); //maybe add more??
    Real rn;
    
    for (int inoise=0;inoise<nnoise;inoise++) {
        for (int t=0;t<Nt;t++) {
            for (int ivec=0;ivec<nvec;ivec++) {
                for (int is=0;is<Ns;is++) {
                    if (exact_distillation)
                        noise[inoise + nnoise*(t + Nt*(ivec+nvec*is))] = 1.;
                    else{
                        random(sRNG,rn);
                        // We could use a greater number of complex roots of unity
                        // ... but this seems to work well
                        noise[inoise + nnoise*(t + Nt*(ivec+nvec*is))] = (rn > 0.5) ? -1 : 1;
                    }
                }
            }
        }
    }

    // Load perambulator if it exists on disk instead of creating it
    // Not sure this is how we want it - rather specify an input flag 'read' 
    // and assert that the file is there.
/*    const std::string &PerambFileName{par().PerambFileName};
    if( PerambFileName.length() ){
        bool bExists = false;
        {
            std::ifstream f(PerambFileName, std::ios::binary);
            if( f.is_open() )
                bExists = true;
        }
        if( bExists ) {
            perambulator.ReadBinary(PerambFileName);
            return;
        }
    }*/

  envGetTmp(LatticeSpinColourVector, dist_source);
  envGetTmp(LatticeSpinColourVector, tmp2);
  envGetTmp(LatticeSpinColourVector, result);
  envGetTmp(LatticeColourVector, result_nospin);
  envGetTmp(LatticeSpinColourVector, tmp3d);
  envGetTmp(LatticeColourVector, tmp3d_nospin);
  envGetTmp(LatticeColourVector, result_3d);
  envGetTmp(LatticeColourVector, evec3d);

    const int Ntlocal{grid4d->LocalDimensions()[3]};
    const int Ntfirst{grid4d->LocalStarts()[3]};

    {

    int t_inv;
    for (int inoise = 0; inoise < nnoise; inoise++) {
      for (int dk = 0; dk < LI; dk++) {
        for (int dt = 0; dt < Nt_inv; dt++) {
          for (int ds = 0; ds < Ns; ds++) {
            std::cout <<  "LapH source vector from noise " << inoise << " and dilution component (d_k,d_t,d_alpha) : (" << dk << ","<< dt << "," << ds << ")" << std::endl;
            dist_source = zero;
            tmp3d_nospin = zero;
            evec3d = zero;
            for (int it = dt; it < Nt; it += TI){
              if (full_tdil) t_inv = tsrc; else t_inv = it;
              if( t_inv >= Ntfirst && t_inv < Ntfirst + Ntlocal ) {
                for (int ik = dk; ik < nvec; ik += LI){
                  for (int is = ds; is < Ns; is += Ns){ // TODO: Also allow non-full spin dilution (re-define exact_distillation?)
                    ExtractSliceLocal(evec3d,epack.evec[ik],0,t_inv,3);
                    tmp3d_nospin = evec3d * noise[inoise + nnoise*(t_inv + Nt*(ik+nvec*is))]; 
                    tmp3d=zero;
                    pokeSpin(tmp3d,tmp3d_nospin,is);
                    tmp2=zero;
                    InsertSliceLocal(tmp3d,tmp2,0,t_inv-Ntfirst,Grid::QCD::Tdir);
                    dist_source += tmp2;
                  }
                }
              }
            }
            result=zero;
	    v4dtmp = dist_source;
	    if (Ls_ == 1){
	      solver(result, v4dtmp);
	    } else {
	       mat.ImportPhysicalFermionSource(v4dtmp, v5dtmp);
	       solver(v5dtmp_sol, v5dtmp);
	       mat.ExportPhysicalFermionSolution(v5dtmp_sol, v4dtmp);
	       result = v4dtmp;
	    }
            if ((1)) // comment out if unsmeared sink is too large???
              unsmeared_sink[inoise+nnoise*(dk+LI*(dt+Nt_inv*ds))] = result;
            for (int is = 0; is < Ns; is++) {
              result_nospin = peekSpin(result,is);
              for (int t = Ntfirst; t < Ntfirst + Ntlocal; t++) {
                ExtractSliceLocal(result_3d,result_nospin,0,t-Ntfirst,Grid::QCD::Tdir);
                for (int ivec = 0; ivec < nvec; ivec++) {
                  ExtractSliceLocal(evec3d,epack.evec[ivec],0,t,3);
                  pokeSpin(perambulator(t, ivec, dk, inoise,dt,ds),innerProduct(evec3d, result_3d),is);
                }
          }
        }
      }
    }
  }
}
    }
    std::cout <<  "perambulator done" << std::endl;
    perambulator.SliceShare( grid3d, grid4d );

    if(PerambFileName.length())
        perambulator.WriteBinary(PerambFileName + "." + std::to_string(vm().getTrajectory()));
}

END_MODULE_NAMESPACE

END_HADRONS_NAMESPACE

#endif // Hadrons_MDistil_PerambLight_hpp_
