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
                                    bool, multiFile,
				    int, tsrc,
				    int, nnoise,
				    int, LI,
				    int, SI,
				    int, TI,
				    int, nvec,
				    int, Ns,
				    int, Nt,
				    int, Nt_inv,
				    Real, mass,
				    Real, M5,
				    int, Ls,
				    double, CGPrecision,
				    int, MaxIterations);
};

template <typename FImpl>
class TPerambLight: public Module<PerambLightPar>
{
public:
    // constructor
    TPerambLight(const std::string name);
    // destructor
    virtual ~TPerambLight(void) {};
    // dependency relation
    virtual std::vector<std::string> getInput(void);
    virtual std::vector<std::string> getOutput(void);
    // setup
    virtual void setup(void);
    // execution
    virtual void execute(void);
};

MODULE_REGISTER_TMP(PerambLight, TPerambLight<FIMPL>, MDistil);

/******************************************************************************
 *                 TPerambLight implementation                             *
 ******************************************************************************/
// constructor /////////////////////////////////////////////////////////////////
template <typename FImpl>
TPerambLight<FImpl>::TPerambLight(const std::string name)
: Module<PerambLightPar>(name)
{}

// dependencies/products ///////////////////////////////////////////////////////
template <typename FImpl>
std::vector<std::string> TPerambLight<FImpl>::getInput(void)
{
    std::vector<std::string> in;

    //in.push_back(par().noise);
    in.push_back(par().eigenPack);
    
    return in;
}

template <typename FImpl>
std::vector<std::string> TPerambLight<FImpl>::getOutput(void)
{
    std::vector<std::string> out = {getName() + "_perambulator_light",getName() + "_noise"};
    
    return out;
}

// setup ///////////////////////////////////////////////////////////////////////
template <typename FImpl>
void TPerambLight<FImpl>::setup(void)
{

  // auto &noise = envGet(std::vector<std::vector<std::vector<SpinVector>>>, par().noise);
  
  int LI=par().LI;
  int SI=par().SI;
  int TI=par().TI;
  int nnoise=par().nnoise;
  int Nt=par().Nt;
  int nvec=par().nvec;
 
  envCreate(Perambulator<SpinVector>, getName() + "_perambulator_light", 1, 
		                    LI*SI*TI*nnoise*nvec*Nt);
  envCreate(std::vector<Complex>, getName() + "_noise", 1, 
		                    nvec*Ns*Nt*nnoise);

  GridCartesian * grid4d = env().getGrid();
  std::vector<int> latt_size   = GridDefaultLatt();
  std::vector<int> simd_layout = GridDefaultSimd(Nd, vComplex::Nsimd());
  std::vector<int> mpi_layout  = GridDefaultMpi();
  std::vector<int> simd_layout_3 = GridDefaultSimd(Nd-1, vComplex::Nsimd());
  latt_size[Nd-1] = 1;
  simd_layout_3.push_back( 1 );
  mpi_layout[Nd-1] = 1;
  GridCartesian * grid3d = new GridCartesian(latt_size,simd_layout_3,mpi_layout,*grid4d);

  envTmp(LatticeSpinColourVector, "dist_source",1,LatticeSpinColourVector(grid4d));
  envTmp(LatticeSpinColourVector, "tmp2",1,LatticeSpinColourVector(grid4d));
  envTmp(LatticeSpinColourVector, "result",1,LatticeSpinColourVector(grid4d));
  envTmp(LatticeSpinColourVector, "result_single_component",1,LatticeSpinColourVector(grid4d));
  envTmp(LatticeColourVector, "result_nospin",1,LatticeColourVector(grid4d));
  envTmp(LatticeColourVector, "tmp_nospin",1,LatticeColourVector(grid4d));
  envTmp(LatticeSpinColourVector, "tmp3d",1,LatticeSpinColourVector(grid3d));
  envTmp(LatticeColourVector, "tmp3d_nospin",1,LatticeColourVector(grid3d));
  envTmp(LatticeColourVector, "result_3d",1,LatticeColourVector(grid3d));
  envTmp(LatticeColourVector, "evec3d",1,LatticeColourVector(grid3d));
  envTmp(LatticeSpinVector, "peramb_tmp",1,LatticeSpinVector(grid4d));

}

// execution ///////////////////////////////////////////////////////////////////
template <typename FImpl>
void TPerambLight<FImpl>::execute(void)
{

    //auto        &noise     = envGet(std::vector<std::vector<std::vector<SpinVector>>>, par().noise);
    auto        &noise   = envGet(std::vector<Complex>, getName() + "_noise");
    auto        &perambulator   = envGet(Perambulator<SpinVector>, getName() + "_perambulator_light");
    auto        &epack   = envGet(Grid::Hadrons::EigenPack<LatticeColourVector>, par().eigenPack);

  GridCartesian * grid4d = env().getGrid();
  std::vector<int> latt_size   = GridDefaultLatt();
  std::vector<int> simd_layout = GridDefaultSimd(Nd, vComplex::Nsimd());
  std::vector<int> mpi_layout  = GridDefaultMpi();
  std::vector<int> simd_layout_3 = GridDefaultSimd(Nd-1, vComplex::Nsimd());
  latt_size[Nd-1] = 1;
  simd_layout_3.push_back( 1 );
  mpi_layout[Nd-1] = 1;
  GridCartesian * grid3d = new GridCartesian(latt_size,simd_layout_3,mpi_layout,*grid4d);

  LatticeGaugeField Umu(grid4d);
  FieldMetaData header;
  if((1)){
    const std::vector<int> seeds({1, 2, 3, 4, 5});
    GridParallelRNG pRNG4d(grid4d);
    pRNG4d.SeedFixedIntegers(seeds);
    std::cout << GridLogMessage << "now hot config" << std::endl;
    SU<Nc>::HotConfiguration(pRNG4d, Umu);
    std::cout << GridLogMessage << "hot cfg done." << std::endl;
  } else {
    std::string   fileName( "/home/dp008/dp008/dc-rich6/Scripts/ConfigsDeflQED/ckpoint_lat.3000" );
    std::cout << GridLogMessage << "Loading NERSC configuration from '" << fileName << "'" << std::endl;
    NerscIO::readConfiguration(Umu, header, fileName);
    std::cout << GridLogMessage << "reading done." << std::endl;
  }

  envGetTmp(LatticeSpinColourVector, dist_source);
  envGetTmp(LatticeSpinColourVector, tmp2);
  envGetTmp(LatticeSpinColourVector, result);
  envGetTmp(LatticeSpinColourVector, result_single_component);
  envGetTmp(LatticeColourVector, result_nospin);
  envGetTmp(LatticeColourVector, tmp_nospin);
  envGetTmp(LatticeSpinColourVector, tmp3d);
  envGetTmp(LatticeColourVector, tmp3d_nospin);
  envGetTmp(LatticeColourVector, result_3d);
  envGetTmp(LatticeColourVector, evec3d);
  envGetTmp(LatticeSpinVector, peramb_tmp);

  int Ntlocal = grid4d->LocalDimensions()[3];
  int Ntfirst = grid4d->LocalStarts()[3];

  
  int tsrc=par().tsrc;
  int nnoise=par().nnoise;
  int LI=par().LI;
  int Ns=par().Ns;
  int Nt_inv=par().Nt_inv;
  int Nt=par().Nt;
  int TI=par().TI;
  int nvec=par().nvec;
  
  bool full_tdil=(TI==Nt);
  bool exact_distillation = (full_tdil && LI==nvec);

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

    Real mass=par().mass;    // TODO Infile
    Real M5  =par().M5;     // TODO Infile
    std::cout << "init RBG "  << std::endl;
    GridRedBlackCartesian RBGrid(grid4d);
    std::cout << "init RBG done"  << std::endl;
   
    int Ls=par().Ls;

    double CGPrecision = par().CGPrecision;
    int MaxIterations = par().MaxIterations;
    
    GridCartesian         * FGrid   = SpaceTimeGrid::makeFiveDimGrid(Ls,grid4d);
    GridRedBlackCartesian * FrbGrid = SpaceTimeGrid::makeFiveDimRedBlackGrid(Ls,grid4d);
    
    typedef DomainWallFermionR FermionAction;
    
    FermionAction Dop(Umu,*FGrid,*FrbGrid,*grid4d,RBGrid,mass,M5);
    
    MdagMLinearOperator<FermionAction,LatticeFermion> HermOp(Dop);
    ConjugateGradient<LatticeFermion> CG(CGPrecision,MaxIterations);
    SchurRedBlackDiagMooeeSolve<LatticeFermion> SchurSolver(CG);

    for (int inoise = 0; inoise < nnoise; inoise++) {
      for (int dk = 0; dk < LI; dk++) {
        for (int dt = 0; dt < Nt_inv; dt++) {
          if(full_tdil) dt=tsrc; //this works for now, as longs as tsrc=0, but will crash otherwise!
          for (int ds = 0; ds < Ns; ds++) {
            std::cout <<  "LapH source vector from noise " << inoise << " and dilution component (d_k,d_t,d_alpha) : (" << dk << ","<< dt << "," << ds << ")" << std::endl;
            dist_source = zero;
            tmp3d_nospin = zero;
            evec3d = zero;
            for (int it = dt; it < Nt; it += TI){
              if( it >= Ntfirst && it < Ntfirst + Ntlocal ) {
                for (int ik = dk; ik < nvec; ik += LI){
                  for (int is = ds; is < Ns; is += Ns){ //at the moment, full spin dilution is enforced
                    ExtractSliceLocal(evec3d,epack.evec[ik],0,it,3);
                    tmp3d_nospin = evec3d * noise[inoise + nnoise*(it + Nt*(ik+nvec*is))]; 
                    //tmp3d_nospin = evec3d * noise[inoise][it][ik]()(is)(); //noises do not have to be a spin vector
                    tmp3d=zero;
                    pokeSpin(tmp3d,tmp3d_nospin,is);
                    tmp2=zero;
                    InsertSliceLocal(tmp3d,tmp2,0,it-Ntfirst,Grid::QCD::Tdir);
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
            std::cout <<  "1" << std::endl;
                  ExtractSliceLocal(evec3d,epack.evec[ivec],0,t,3);
            std::cout <<  "2" << std::endl;
                  pokeSpin(perambulator(t, ivec, dk, inoise,dt,ds),innerProduct(evec3d, result_3d),is);
            std::cout <<  "3" << std::endl;
                }
          }
        }
      }
    }
  }
}
    std::cout <<  "perambulator done" << std::endl;
    perambulator.SliceShare( grid3d, grid4d );

    // THIS IS WHERE WE WANT TO SAVE THE PERAMBULATORS TO DISK
    perambulator.WriteTemporary(std::string("perambulators/file"));

}

END_MODULE_NAMESPACE

END_HADRONS_NAMESPACE

#endif // Hadrons_MDistil_PerambLight_hpp_
