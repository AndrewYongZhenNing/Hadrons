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
 
struct SolverParameters: Serializable {
  GRID_SERIALIZABLE_CLASS_MEMBERS(SolverParameters,
                                  double, CGPrecision,
                                  int,    MaxIterations,
                                  double, mass,
                                  double, M5)
  SolverParameters() = default;
  template <class ReaderClass> SolverParameters(Reader<ReaderClass>& Reader){read(Reader,"Solver",*this);}
};

class PerambLightPar: Serializable
{
public:
    GRID_SERIALIZABLE_CLASS_MEMBERS(PerambLightPar,
		                    std::string, eigenPack,
                                    bool, multiFile,
                                    int, nvec,
				    int, Ls,          // For makeFiveDimGrid
                                    DistilParameters, Distil,
                                    SolverParameters, Solver);
};

template <typename GImpl>
class TPerambLight: public Module<PerambLightPar>
{
public:
    GAUGE_TYPE_ALIASES(GImpl,);
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
};

MODULE_REGISTER_TMP(PerambLight, TPerambLight<GIMPL>, MDistil);

/******************************************************************************
 *                 TPerambLight implementation                             *
 ******************************************************************************/
// constructor /////////////////////////////////////////////////////////////////
template <typename GImpl>
TPerambLight<GImpl>::TPerambLight(const std::string name)
: grid3d{nullptr}, grid4d{nullptr}, Module<PerambLightPar>(name)
{}

// destructor
template <typename GImpl>
TPerambLight<GImpl>::~TPerambLight(void)
{
  Cleanup();
};

// dependencies/products ///////////////////////////////////////////////////////
template <typename GImpl>
std::vector<std::string> TPerambLight<GImpl>::getInput(void)
{
    std::vector<std::string> in;

    //in.push_back(par().noise);
    in.push_back(par().eigenPack);
    
    return in;
}

template <typename GImpl>
std::vector<std::string> TPerambLight<GImpl>::getOutput(void)
{
    std::vector<std::string> out = {getName() + "_perambulator_light",getName() + "_noise"};
    
    return out;
}

// setup ///////////////////////////////////////////////////////////////////////
template <typename GImpl>
void TPerambLight<GImpl>::setup(void)
{
    Cleanup();

    // auto &noise = envGet(std::vector<std::vector<std::vector<SpinVector>>>, par().noise);
    const int nvec{par().nvec};
    const DistilParameters & Distil{par().Distil};

    //envCreate(Perambulator<SpinVector, 6>, getName() + "_perambulator_light", 1,
    env().template createObject<Perambulator<SpinVector, 6> >(getName() + "_perambulator_light", Environment::Storage::object, 1,
              Distil.Nt,nvec,Distil.LI,Distil.nnoise,Distil.Nt_inv,Distil.SI);
    envCreate(std::vector<Complex>, getName() + "_noise", 1,
              nvec*Distil.Ns*Distil.Nt*Distil.nnoise);

    grid4d = env().getGrid();
    grid3d = MakeLowerDimGrid(grid4d);//new GridCartesian(latt_size,simd_layout_3,mpi_layout,*grid4d);

    envTmpLat(GaugeField, "Umu");
    envTmpLat(LatticeSpinColourVector, "dist_source");
    envTmpLat(LatticeSpinColourVector, "tmp2");
    envTmpLat(LatticeSpinColourVector, "result");
    //envTmpLat(LatticeSpinColourVector, "result_single_component");
    envTmpLat(LatticeColourVector, "result_nospin");
    //envTmpLat(LatticeColourVector, "tmp_nospin");
    //envTmpLat(LatticeSpinVector, "peramb_tmp");
    envTmp(LatticeSpinColourVector, "tmp3d",1,LatticeSpinColourVector(grid3d));
    envTmp(LatticeColourVector, "tmp3d_nospin",1,LatticeColourVector(grid3d));
    envTmp(LatticeColourVector, "result_3d",1,LatticeColourVector(grid3d));
    envTmp(LatticeColourVector, "evec3d",1,LatticeColourVector(grid3d));
}

// clean up any temporaries created by setup (that aren't stored in the environment)
template <typename GImpl>
void TPerambLight<GImpl>::Cleanup(void)
{
  if( grid3d != nullptr ) {
    delete grid3d;
    grid3d = nullptr;
  }
  grid4d = nullptr;
}

// execution ///////////////////////////////////////////////////////////////////
template <typename GImpl>
void TPerambLight<GImpl>::execute(void)
{
    const int nvec{par().nvec};
    const DistilParameters & Distil{par().Distil};
    const SolverParameters & Solver{par().Solver};
    const int LI{Distil.LI};
    //const int SI{Distil.SI};
    const int TI{Distil.TI};
    const int nnoise{Distil.nnoise};
    const int Nt{Distil.Nt};
    const int Nt_inv{Distil.Nt_inv};
    const int tsrc{Distil.tsrc};
    const int Ns{Distil.Ns};
    
    const bool full_tdil{TI==Nt};
    const bool exact_distillation{full_tdil && LI==nvec};

    //auto        &noise     = envGet(std::vector<std::vector<std::vector<SpinVector>>>, par().noise);
    auto        &noise   = envGet(std::vector<Complex>, getName() + "_noise");
    auto        &perambulator = //envGet(Perambulator<SpinVector>,
                                *env().template getObject<Perambulator<SpinVector,6> >(
                                    getName() + "_perambulator_light");
    auto        &epack   = envGet(Grid::Hadrons::EigenPack<LatticeColourVector>, par().eigenPack);

    envGetTmp(GaugeField, Umu);
  FieldMetaData header;
  if((1)){
    const std::vector<int> seeds({1, 2, 3, 4, 5});
    GridParallelRNG pRNG4d(grid4d);
    pRNG4d.SeedFixedIntegers(seeds);
    std::cout << GridLogMessage << "now hot config" << std::endl;
    SU<Nc>::HotConfiguration(pRNG4d, Umu);
    std::cout << GridLogMessage << "hot cfg done." << std::endl;

  // Set up the SAME gauge field on every time plane
  //  int Nt = grid4d->gDimensions()[Tdir];
  Grid_unquiesce_nodes();
  
  auto Usft = Umu;
  Lattice<iScalar<vInteger> > coor(grid4d);
  LatticeCoordinate(coor,Tdir);
  for(int t=1;t<Nt;t++){
    // t=1
    //  Umu                Usft
    // 0,1,2,3,4,5,6,7 -> 7,0,1,2,3,4,5,6 t=1
    // 0,0,2,3,4,5,6,7    6,7,0,1,2,3,4,5 t=2
    // 0,0,0,3,4,5,6,7    5,6,7,0,1,2,3,4 t=3
    //...
    
    Usft = Cshift(Usft,Tdir,-1);
    Umu = where(coor==t,Usft,Umu);
  }
  } else {
    std::string   fileName( "/home/dp008/dp008/dc-rich6/Scripts/ConfigsDeflQED/ckpoint_lat.3000" );
    std::cout << GridLogMessage << "Loading NERSC configuration from '" << fileName << "'" << std::endl;
    NerscIO::readConfiguration(Umu, header, fileName);
    std::cout << GridLogMessage << "reading done." << std::endl;
  }

  envGetTmp(LatticeSpinColourVector, dist_source);
  envGetTmp(LatticeSpinColourVector, tmp2);
  envGetTmp(LatticeSpinColourVector, result);
  //envGetTmp(LatticeSpinColourVector, result_single_component);
  envGetTmp(LatticeColourVector, result_nospin);
  //envGetTmp(LatticeColourVector, tmp_nospin);
    //envGetTmp(LatticeSpinVector, peramb_tmp);
  envGetTmp(LatticeSpinColourVector, tmp3d);
  envGetTmp(LatticeColourVector, tmp3d_nospin);
  envGetTmp(LatticeColourVector, result_3d);
  envGetTmp(LatticeColourVector, evec3d);

    const int Ntlocal{grid4d->LocalDimensions()[3]};
    const int Ntfirst{grid4d->LocalStarts()[3]};

  //Create Noises
  //std::cout << pszGaugeConfigFile << std::endl;
  //GridSerialRNG sRNG; sRNG.SeedUniqueString(std::string(pszGaugeConfigFile));
  GridSerialRNG sRNG; sRNG.SeedUniqueString("unique_string");
  Real rn;

  for (int inoise=0;inoise<nnoise;inoise++) {
    for (int t=0;t<Nt;t++) {
      for (int ivec=0;ivec<nvec;ivec++) {
        for (int is=0;is<Ns;is++) {
          if (exact_distillation)
            noise[inoise + nnoise*(t + Nt*(ivec+nvec*is))] = 1.;
            //noises[inoise][t][ivec]()(is)() = 1.;
          else{
            random(sRNG,rn); 
            noise[inoise + nnoise*(t + Nt*(ivec+nvec*is))] = (rn-0.5 > 0) - (rn-0.5 < 0); //TODO: This could be 0 if rn==0.5!!
            //noises[inoise][t][ivec]()(is)() = (rn-0.5 > 0) - (rn-0.5 < 0); //TODO: This could be 0 if rn==0.5!!
          }
        }
      }
    }
  }

    const Real mass{Solver.mass};
    const Real M5  {Solver.M5};
    std::cout << "init RBG "  << std::endl;
    GridRedBlackCartesian RBGrid(grid4d);
    std::cout << "init RBG done"  << std::endl;
   
    const int Ls{par().Ls};
    const double CGPrecision{Solver.CGPrecision};
    const int MaxIterations {Solver.MaxIterations};
    {
    GridCartesian         * FGrid   = SpaceTimeGrid::makeFiveDimGrid(Ls,grid4d);
    GridRedBlackCartesian * FrbGrid = SpaceTimeGrid::makeFiveDimRedBlackGrid(Ls,grid4d);
    
    typedef DomainWallFermionR FermionAction;
    
    FermionAction Dop(Umu,*FGrid,*FrbGrid,*grid4d,RBGrid,mass,M5);
    
    MdagMLinearOperator<FermionAction,LatticeFermion> HermOp(Dop);
    ConjugateGradient<LatticeFermion> CG(CGPrecision,MaxIterations);
    SchurRedBlackDiagMooeeSolve<LatticeFermion> SchurSolver(CG);

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
                  for (int is = ds; is < Ns; is += Ns){ //at the moment, full spin dilution is enforced
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
            std::cout <<  "Inversion for noise " << inoise << " and dilution component (d_k,d_t,d_alpha) : (" << dk << ","<< dt << "," << ds << ")" << std::endl;
            result=zero;
            LatticeFermion src5(FGrid);
            LatticeFermion sol5(FGrid);
            Dop.ImportPhysicalFermionSource(dist_source,src5);
            SchurSolver(Dop,src5,sol5);
            Dop.ExportPhysicalFermionSolution(sol5,result); //These are the meson sinks
            //if (compute_current_sink)
            //  current_sink[inoise+nnoise*(dk+LI*(dt+Nt_inv*ds))] = result;
            std::cout <<  "Contraction of perambulator from noise " << inoise << " and dilution component (d_k,d_t,d_alpha) : (" << dk << ","<< dt << "," << ds << ")" << std::endl;
            for (int is = 0; is < Ns; is++) {
              result_nospin = peekSpin(result,is);
              for (int t = Ntfirst; t < Ntfirst + Ntlocal; t++) {
                ExtractSliceLocal(result_3d,result_nospin,0,t-Ntfirst,Grid::QCD::Tdir);
                for (int ivec = 0; ivec < nvec; ivec++) {
                  ExtractSliceLocal(evec3d,epack.evec[ivec],0,t,3);
                  pokeSpin(perambulator(t, ivec, dk, inoise,dt,ds),innerProduct(evec3d, result_3d),is);
                  std::cout <<  "perambulator(t, ivec, dk, inoise,dt,ds)(is) = (" << t << "," << ivec << "," << dk << "," << inoise << "," << dt << "," << ds << ")(" << is << ") = " <<  perambulator(t, ivec, dk, inoise,dt,ds)()(is)() << std::endl;
                }
          }
        }
      }
    }
  }
}
        // Kill our 5 dimensional grid (avoid leaks). Should really declare these objects temporary
        delete FrbGrid;
        delete FGrid;
    }
    std::cout <<  "perambulator done" << std::endl;
    perambulator.SliceShare( grid3d, grid4d );

    // THIS IS WHERE WE WANT TO SAVE THE PERAMBULATORS TO DISK
    perambulator.WriteTemporary(std::string("perambulators/file"));

}

END_MODULE_NAMESPACE

END_HADRONS_NAMESPACE

#endif // Hadrons_MDistil_PerambLight_hpp_
