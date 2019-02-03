/*************************************************************************************
 
 Grid physics library, www.github.com/paboyle/Grid
 
 Source file: Hadrons/Modules/MDistil/Distil.hpp

 Copyright (C) 2015-2019
 
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

#ifndef Hadrons_MDistil_Distil_hpp_
#define Hadrons_MDistil_Distil_hpp_

#include <Hadrons/Global.hpp>
#include <Hadrons/Module.hpp>
#include <Hadrons/ModuleFactory.hpp>

#ifndef COMMA
#define COMMA ,
#endif

/******************************************************************************
 This potentially belongs in CartesianCommunicator
 Turns out I don't actually need this when running inside hadrons
 ******************************************************************************/

BEGIN_MODULE_NAMESPACE(Grid)
inline void SliceShare( GridBase * gridLowDim, GridBase * gridHighDim, void * Buffer, int BufferSize )
{
  // Work out which dimension is the spread-out dimension
  assert(gridLowDim);
  assert(gridHighDim);
  const int iNumDims{(const int)gridHighDim->_gdimensions.size()};
  assert(iNumDims == gridLowDim->_gdimensions.size());
  int dimSpreadOut = -1;
  std::vector<int> coor(iNumDims);
  for( int i = 0 ; i < iNumDims ; i++ ) {
    coor[i] = gridHighDim->_processor_coor[i];
    if( gridLowDim->_gdimensions[i] != gridHighDim->_gdimensions[i] ) {
      assert( dimSpreadOut == -1 );
      assert( gridLowDim->_processors[i] == 1 ); // easiest assumption to make for now
      dimSpreadOut = i;
    }
  }
  if( dimSpreadOut != -1 && gridHighDim->_processors[dimSpreadOut] != gridLowDim->_processors[dimSpreadOut] ) {
    // Make sure the same number of data elements exist on each slice
    const int NumSlices{gridHighDim->_processors[dimSpreadOut] / gridLowDim->_processors[dimSpreadOut]};
    assert(gridHighDim->_processors[dimSpreadOut] == gridLowDim->_processors[dimSpreadOut] * NumSlices);
    const int SliceSize{BufferSize/NumSlices};
    //CCC_DEBUG_DUMP(Buffer, NumSlices, SliceSize);
    assert(BufferSize == SliceSize * NumSlices);
//#ifndef USE_LOCAL_SLICES
//    assert(0); // Can't do this without MPI (should really test whether MPI is defined)
//#else
    const auto MyRank{gridHighDim->ThisRank()};
    std::vector<CommsRequest_t> reqs(0);
    int MySlice{coor[dimSpreadOut]};
    char * const _buffer{(char *)Buffer};
    char * const MyData{_buffer + MySlice * SliceSize};
    for(int i = 1; i < NumSlices ; i++ ){
      int SendSlice = ( MySlice + i ) % NumSlices;
      int RecvSlice = ( MySlice - i + NumSlices ) % NumSlices;
      char * const RecvData{_buffer + RecvSlice * SliceSize};
      coor[dimSpreadOut] = SendSlice;
      const auto SendRank{gridHighDim->RankFromProcessorCoor(coor)};
      coor[dimSpreadOut] = RecvSlice;
      const auto RecvRank{gridHighDim->RankFromProcessorCoor(coor)};
      std::cout << GridLogMessage << "Send slice " << MySlice << " (" << MyRank << ") to " << SendSlice << " (" << SendRank
      << "), receive slice from " << RecvSlice << " (" << RecvRank << ")" << std::endl;
      gridHighDim->SendToRecvFromBegin(reqs,MyData,SendRank,RecvData,RecvRank,SliceSize);
      //memcpy(RecvData,MyData,SliceSize); // Debug
    }
    gridHighDim->SendToRecvFromComplete(reqs);
    std::cout << GridLogMessage << "Slice data shared." << std::endl;
    //CCC_DEBUG_DUMP(Buffer, NumSlices, SliceSize);
//#endif
  }
}

/*************************************************************************************
 
 -Grad^2 (Peardon, 2009, pg 2, equation 3)
 Field      Type of field the operator will be applied to
 GaugeField Gauge field the operator will smear using
 
 TODO CANDIDATE for integration into laplacian operator
 should just require adding number of dimensions to act on to constructor,
 where the default=all dimensions, but we could specify 3 spatial dimensions
 
 *************************************************************************************/

template<typename Field, typename GaugeField=LatticeGaugeFieldD>
class LinOpPeardonNabla : public LinearOperatorBase<Field>, public LinearFunction<Field> {
  typedef typename GaugeField::vector_type vCoeff_t;
protected: // I don't really mind if _gf is messed with ... so make this public?
  //GaugeField & _gf;
  int          nd; // number of spatial dimensions
  std::vector<Lattice<iColourMatrix<vCoeff_t> > > U;
public:
  // Construct this operator given a gauge field and the number of dimensions it should act on
  LinOpPeardonNabla( GaugeField& gf, int dimSpatial = Grid::QCD::Tdir ) : /*_gf(gf),*/ nd{dimSpatial} {
    assert(dimSpatial>=1);
    for( int mu = 0 ; mu < nd ; mu++ )
      U.push_back(PeekIndex<LorentzIndex>(gf,mu));
      }
  
  // Apply this operator to "in", return result in "out"
  void operator()(const Field& in, Field& out) {
    assert( nd <= in._grid->Nd() );
    conformable( in, out );
    out = ( ( Real ) ( 2 * nd ) ) * in;
    Field _tmp(in._grid);
    typedef typename GaugeField::vector_type vCoeff_t;
    //Lattice<iColourMatrix<vCoeff_t> > U(in._grid);
    for( int mu = 0 ; mu < nd ; mu++ ) {
      //U = PeekIndex<LorentzIndex>(_gf,mu);
      out -= U[mu] * Cshift( in, mu, 1);
      _tmp = adj( U[mu] ) * in;
      out -= Cshift(_tmp,mu,-1);
    }
  }
  
  void OpDiag (const Field &in, Field &out) { assert(0); };
  void OpDir  (const Field &in, Field &out,int dir,int disp) { assert(0); };
  void Op     (const Field &in, Field &out) { assert(0); };
  void AdjOp  (const Field &in, Field &out) { assert(0); };
  void HermOpAndNorm(const Field &in, Field &out,RealD &n1,RealD &n2) { assert(0); };
  void HermOp(const Field &in, Field &out) { operator()(in,out); };
};

template<typename Field>
class LinOpPeardonNablaHerm : public LinearFunction<Field> {
public:
  OperatorFunction<Field>   & _poly;
  LinearOperatorBase<Field> &_Linop;
  
  LinOpPeardonNablaHerm(OperatorFunction<Field> & poly,LinearOperatorBase<Field>& linop) : _poly(poly), _Linop(linop) {
  }
  
  void operator()(const Field& in, Field& out) {
    _poly(_Linop,in,out);
  }
};


END_MODULE_NAMESPACE // Grid

/******************************************************************************
 Common elements for distillation
 ******************************************************************************/

BEGIN_HADRONS_NAMESPACE

BEGIN_MODULE_NAMESPACE(MDistil)

typedef Grid::Hadrons::EigenPack<LatticeColourVector>       DistilEP;
typedef std::vector<std::vector<std::vector<SpinVector> > > DistilNoises;

/******************************************************************************
 Make a lower dimensional grid
 ******************************************************************************/

inline GridCartesian * MakeLowerDimGrid( GridCartesian * gridHD )
{
  //LOG(Message) << "MakeLowerDimGrid() begin" << std::endl;
  int nd{static_cast<int>(gridHD->_ndimension)};
  std::vector<int> latt_size   = gridHD->_gdimensions;
  latt_size[nd-1] = 1;

  std::vector<int> simd_layout = GridDefaultSimd(nd-1, vComplex::Nsimd());
  simd_layout.push_back( 1 );

  std::vector<int> mpi_layout  = gridHD->_processors;
  mpi_layout[nd-1] = 1;
  GridCartesian * gridLD = new GridCartesian(latt_size,simd_layout,mpi_layout,*gridHD);
  //LOG(Message) << "MakeLowerDimGrid() end" << std::endl;
  return gridLD;
}

/******************************************************************************
 Perambulator object
 ******************************************************************************/

template<typename Scalar_, int NumIndices_>
class NamedTensor : public Eigen::Tensor<Scalar_, NumIndices_, Eigen::RowMajor | Eigen::DontAlign>
{
public:
  typedef Eigen::Tensor<Scalar_, NumIndices_, Eigen::RowMajor | Eigen::DontAlign> ET;
  std::array<std::string,NumIndices_> IndexNames;
public:
  template<typename... IndexTypes>
  EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE NamedTensor(std::array<std::string,NumIndices_> &IndexNames_, Eigen::Index firstDimension, IndexTypes... otherDimensions)
  : IndexNames{IndexNames_}, Eigen::Tensor<Scalar_, NumIndices_, Eigen::RowMajor | Eigen::DontAlign>(firstDimension, otherDimensions...)
  {
    // The number of dimensions used to construct a tensor must be equal to the rank of the tensor.
    EIGEN_STATIC_ASSERT(sizeof...(otherDimensions) + 1 == NumIndices_, YOU_MADE_A_PROGRAMMING_MISTAKE)
  }

  // Share data for timeslices we calculated with other nodes
  inline void SliceShare( GridCartesian * gridLowDim, GridCartesian * gridHighDim ) {
    Grid::SliceShare( gridLowDim, gridHighDim, this->data(), (int) (this->size() * sizeof(Scalar_)));
  }

  // load and save - not virtual - probably all changes
  inline void load(const std::string filename);
  inline void save(const std::string filename) const;
  inline void ReadTemporary(const std::string filename);
  inline void WriteTemporary(const std::string filename) const;
};

/******************************************************************************
 Save NamedTensor binary format
 ******************************************************************************/

template<typename Scalar_, int NumIndices_>
void NamedTensor<Scalar_, NumIndices_>::WriteTemporary(const std::string filename) const {
  std::cout << GridLogMessage << "Writing NamedTensor to \"" << filename << "\"" << std::endl;
  std::ofstream w(filename, std::ios::binary);
  // total number of elements
  uint32_t ul = htonl( static_cast<uint32_t>( this->size() ) );
  w.write(reinterpret_cast<const char *>(&ul), sizeof(ul));
  // number of dimensions
  uint16_t us = htons( static_cast<uint16_t>( this->NumIndices ) );
  w.write(reinterpret_cast<const char *>(&us), sizeof(us));
  // dimensions together with names
  int d = 0;
  for( auto dim : this->dimensions() ) {
    // size of this dimension
    us = htons( static_cast<uint16_t>( dim ) );
    w.write(reinterpret_cast<const char *>(&us), sizeof(us));
    // length of this dimension name
    us = htons( static_cast<uint16_t>( IndexNames[d].size() ) );
    w.write(reinterpret_cast<const char *>(&us), sizeof(us));
    // dimension name
    w.write(IndexNames[d].c_str(), IndexNames[d].size());
    d++;
  }
  // Actual data
  w.write(reinterpret_cast<const char *>(this->data()),(int) (this->size() * sizeof(Scalar_)));
  // checksum
#ifdef USE_IPP
  ul = htonl( GridChecksum::crc32c(this->data(), (int) (this->size() * sizeof(Scalar_))) );
#else
  ul = htonl( GridChecksum::crc32(this->data(), (int) (this->size() * sizeof(Scalar_))) );
#endif
  w.write(reinterpret_cast<const char *>(&ul), sizeof(ul));
}

/******************************************************************************
 Load NamedTensor binary format
 ******************************************************************************/

template<typename Scalar_, int NumIndices_>
void NamedTensor<Scalar_, NumIndices_>::ReadTemporary(const std::string filename) {
  std::cout << GridLogMessage << "Reading NamedTensor from \"" << filename << "\"" << std::endl;
  std::ifstream r(filename, std::ios::binary);
  // total number of elements
  uint32_t ul;
  r.read(reinterpret_cast<char *>(&ul), sizeof(ul));
  assert( this->size() == ntohl( ul ) && "Error: total number of elements" );
  // number of dimensions
  uint16_t us;
  r.read(reinterpret_cast<char *>(&us), sizeof(us));
  assert( this->NumIndices == ntohs( us ) && "Error: number of dimensions" );
  // dimensions together with names
  int d = 0;
  for( auto dim : this->dimensions() ) {
    // size of dimension
    r.read(reinterpret_cast<char *>(&us), sizeof(us));
    assert( dim == ntohs( us ) && "size of dimension" );
    // length of dimension name
    r.read(reinterpret_cast<char *>(&us), sizeof(us));
    size_t l = ntohs( us );
    assert( l == IndexNames[d].size() && "length of dimension name" );
    // dimension name
    std::string s( l, '?' );
    r.read(&s[0], l);
    assert( s == IndexNames[d] && "dimension name" );
    d++;
  }
  // Actual data
  r.read(reinterpret_cast<char *>(this->data()),(int) (this->size() * sizeof(Scalar_)));
  // checksum
  r.read(reinterpret_cast<char *>(&ul), sizeof(ul));
  ul = htonl( ul );
#ifdef USE_IPP
  ul -= GridChecksum::crc32c(this->data(), (int) (this->size() * sizeof(Scalar_)));
#else
  ul -= GridChecksum::crc32(this->data(), (int) (this->size() * sizeof(Scalar_)));
#endif
  assert( ul == 0 && "checksum");
}

/******************************************************************************
 Save NamedTensor Hdf5 format
 ******************************************************************************/

template<typename Scalar_, int NumIndices_>
void NamedTensor<Scalar_, NumIndices_>::save(const std::string filename) const {
  std::cout << GridLogMessage << "Writing NamedTensor to \"" << filename << "\"" << std::endl;
#ifndef HAVE_HDF5
  std::cout << GridErrorMessage << "Error: I/O for NamedTensor requires HDF5" << std::endl;
#else
  Hdf5Writer w(filename);
  //w << this->NumIndices << this->dimensions() << this->IndexNames;
#endif
}

/******************************************************************************
 Load NamedTensor Hdf5 format
 ******************************************************************************/

template<typename Scalar_, int NumIndices_>
void NamedTensor<Scalar_, NumIndices_>::load(const std::string filename) {
  std::cout << GridLogMessage << "Reading NamedTensor from \"" << filename << "\"" << std::endl;
#ifndef HAVE_HDF5
  std::cout << GridErrorMessage << "Error: I/O for NamedTensor requires HDF5" << std::endl;
#else
  Hdf5Reader r(filename);
  typename ET::Dimensions d;
  std::array<std::string,NumIndices_> n;
  //r >> this->NumIndices >> d >> n;
  //this->IndexNames = n;
#endif
}

/******************************************************************************
 Perambulator object
 ******************************************************************************/

template<typename Scalar_, int NumIndices_>
using Perambulator = NamedTensor<Scalar_, NumIndices_>;

/*************************************************************************************
 
 Rotate eigenvectors into our phase convention
 First component of first eigenvector is real and positive
 
 *************************************************************************************/

inline void RotateEigen(std::vector<LatticeColourVector> & evec)
{
  ColourVector cv0;
  auto grid = evec[0]._grid;
  std::vector<int> siteFirst(grid->Nd(),0);
  peekSite(cv0, evec[0], siteFirst);
  auto & cplx0 = cv0()()(0);
  if( std::imag(cplx0) == 0 )
    std::cout << GridLogMessage << "RotateEigen() : Site 0 : " << cplx0 << " => already meets phase convention" << std::endl;
  else {
    const auto cplx0_mag{std::abs(cplx0)};
    const auto phase{std::conj(cplx0 / cplx0_mag)};
    std::cout << GridLogMessage << "RotateEigen() : Site 0 : |" << cplx0 << "|=" << cplx0_mag << " => phase=" << (std::arg(phase) / 3.14159265) << " pi" << std::endl;
    {
      // TODO: Only really needed on the master slice
      for( int k = 0 ; k < evec.size() ; k++ )
        evec[k] *= phase;
      if(grid->IsBoss()){
        for( int c = 0 ; c < Nc ; c++ )
          cv0()()(c) *= phase;
        cplx0.imag(0); // This assumes phase convention is real, positive (so I get rid of rounding error)
        //pokeSite(cv0, evec[0], siteFirst);
        pokeLocalSite(cv0, evec[0], siteFirst);
      }
    }
  }
}

END_MODULE_NAMESPACE

END_HADRONS_NAMESPACE

#endif // Hadrons_MDistil_Distil_hpp_
