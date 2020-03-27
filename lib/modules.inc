modules_cc =\
  Modules/MAction/DWF.cc \
  Modules/MAction/MobiusDWF.cc \
  Modules/MAction/ScaledDWF.cc \
  Modules/MAction/Wilson.cc \
  Modules/MAction/WilsonClover.cc \
  Modules/MAction/ZMobiusDWF.cc \
  Modules/MContraction/A2AAslashField.cc \
  Modules/MContraction/A2AFourQuarkContraction.cc \
  Modules/MContraction/A2ALoop.cc \
  Modules/MContraction/A2AMesonField.cc \
  Modules/MContraction/Baryon.cc \
  Modules/MContraction/DiscLoop.cc \
  Modules/MContraction/Gamma3pt.cc \
  Modules/MContraction/Meson.cc \
  Modules/MContraction/SigmaToNucleonEye.cc \
  Modules/MContraction/SigmaToNucleonNonEye.cc \
  Modules/MContraction/WeakEye3pt.cc \
  Modules/MContraction/WeakMesonDecayKl2.cc \
  Modules/MContraction/WeakNonEye3pt.cc \
  Modules/MDistil/DistilPar.cc \
  Modules/MDistil/DistilVectors.cc \
  Modules/MDistil/LapEvec.cc \
  Modules/MDistil/Noises.cc \
  Modules/MDistil/PerambFromSolve.cc \
  Modules/MDistil/Perambulator.cc \
  Modules/MFermion/EMLepton.cc \
  Modules/MFermion/FreeProp.cc \
  Modules/MFermion/GaugeProp.cc \
  Modules/MGauge/Electrify.cc \
  Modules/MGauge/FundtoHirep.cc \
  Modules/MGauge/GaugeFix.cc \
  Modules/MGauge/Random.cc \
  Modules/MGauge/StochEm.cc \
  Modules/MGauge/StoutSmearing.cc \
  Modules/MGauge/Unit.cc \
  Modules/MGauge/UnitEm.cc \
  Modules/MIO/LoadA2AMatrixDiskVector.cc \
  Modules/MIO/LoadA2AVectors.cc \
  Modules/MIO/LoadBinary.cc \
  Modules/MIO/LoadCoarseEigenPack.cc \
  Modules/MIO/LoadCosmHol.cc \
  Modules/MIO/LoadDistilNoise.cc \
  Modules/MIO/LoadEigenPack.cc \
  Modules/MIO/LoadNersc.cc \
  Modules/MIO/LoadPerambulator.cc \
  Modules/MNoise/FullVolumeSpinColorDiagonal.cc \
  Modules/MNoise/SparseSpinColorDiagonal.cc \
  Modules/MNoise/TimeDilutedSpinColorDiagonal.cc \
  Modules/MNPR/Amputate.cc \
  Modules/MNPR/Bilinear.cc \
  Modules/MNPR/FourQuark.cc \
  Modules/MScalar/ChargedProp.cc \
  Modules/MScalar/FreeProp.cc \
  Modules/MScalarSUN/Div.cc \
  Modules/MScalarSUN/EMT.cc \
  Modules/MScalarSUN/Grad.cc \
  Modules/MScalarSUN/StochFreeField.cc \
  Modules/MScalarSUN/TransProj.cc \
  Modules/MScalarSUN/TrKinetic.cc \
  Modules/MScalarSUN/TrMag.cc \
  Modules/MScalarSUN/TrPhi.cc \
  Modules/MScalarSUN/TwoPoint.cc \
  Modules/MScalarSUN/TwoPointNPR.cc \
  Modules/MSink/Point.cc \
  Modules/MSink/Smear.cc \
  Modules/MSolver/A2AAslashVectors.cc \
  Modules/MSolver/A2AVectors.cc \
  Modules/MSolver/LocalCoherenceLanczos.cc \
  Modules/MSolver/MixedPrecisionRBPrecCG.cc \
  Modules/MSolver/RBPrecCG.cc \
  Modules/MSource/Convolution.cc \
  Modules/MSource/Gauss.cc \
  Modules/MSource/JacobiSmear.cc \
  Modules/MSource/Momentum.cc \
  Modules/MSource/MomentumPhase.cc \
  Modules/MSource/Point.cc \
  Modules/MSource/SeqAslash.cc \
  Modules/MSource/SeqConserved.cc \
  Modules/MSource/SeqGamma.cc \
  Modules/MSource/Wall.cc \
  Modules/MSource/Z2.cc \
  Modules/MUtilities/PrecisionCast.cc \
  Modules/MUtilities/RandomVectors.cc

modules_hpp =\
  Modules/MAction/DWF.hpp \
  Modules/MAction/MobiusDWF.hpp \
  Modules/MAction/ScaledDWF.hpp \
  Modules/MAction/WilsonClover.hpp \
  Modules/MAction/Wilson.hpp \
  Modules/MAction/ZMobiusDWF.hpp \
  Modules/MContraction/A2AAslashField.hpp \
  Modules/MContraction/A2AFourQuarkContraction.hpp \
  Modules/MContraction/A2ALoop.hpp \
  Modules/MContraction/A2AMesonField.hpp \
  Modules/MContraction/Baryon.hpp \
  Modules/MContraction/DiscLoop.hpp \
  Modules/MContraction/Gamma3pt.hpp \
  Modules/MContraction/Meson.hpp \
  Modules/MContraction/SigmaToNucleonEye.hpp \
  Modules/MContraction/SigmaToNucleonNonEye.hpp \
  Modules/MContraction/WeakEye3pt.hpp \
  Modules/MContraction/WeakMesonDecayKl2.hpp \
  Modules/MContraction/WeakNonEye3pt.hpp \
  Modules/MDistil/Distil.hpp \
  Modules/MDistil/DistilPar.hpp \
  Modules/MDistil/DistilVectors.hpp \
  Modules/MDistil/LapEvec.hpp \
  Modules/MDistil/Noises.hpp \
  Modules/MDistil/PerambFromSolve.hpp \
  Modules/MDistil/Perambulator.hpp \
  Modules/MFermion/EMLepton.hpp \
  Modules/MFermion/FreeProp.hpp \
  Modules/MFermion/GaugeProp.hpp \
  Modules/MGauge/Electrify.hpp \
  Modules/MGauge/FundtoHirep.hpp \
  Modules/MGauge/GaugeFix.hpp \
  Modules/MGauge/Random.hpp \
  Modules/MGauge/StochEm.hpp \
  Modules/MGauge/StoutSmearing.hpp \
  Modules/MGauge/UnitEm.hpp \
  Modules/MGauge/Unit.hpp \
  Modules/MIO/LoadA2AMatrixDiskVector.hpp \
  Modules/MIO/LoadA2AVectors.hpp \
  Modules/MIO/LoadBinary.hpp \
  Modules/MIO/LoadCoarseEigenPack.hpp \
  Modules/MIO/LoadCosmHol.hpp \
  Modules/MIO/LoadDistilNoise.hpp \
  Modules/MIO/LoadEigenPack.hpp \
  Modules/MIO/LoadNersc.hpp \
  Modules/MIO/LoadPerambulator.hpp \
  Modules/MNoise/FullVolumeSpinColorDiagonal.hpp \
  Modules/MNoise/SparseSpinColorDiagonal.hpp \
  Modules/MNoise/TimeDilutedSpinColorDiagonal.hpp \
  Modules/MNPR/Amputate.hpp \
  Modules/MNPR/Bilinear.hpp \
  Modules/MNPR/FourQuark.hpp \
  Modules/MScalar/ChargedProp.hpp \
  Modules/MScalar/FreeProp.hpp \
  Modules/MScalar/Scalar.hpp \
  Modules/MScalarSUN/Div.hpp \
  Modules/MScalarSUN/EMT.hpp \
  Modules/MScalarSUN/Grad.hpp \
  Modules/MScalarSUN/StochFreeField.hpp \
  Modules/MScalarSUN/TransProj.hpp \
  Modules/MScalarSUN/TrKinetic.hpp \
  Modules/MScalarSUN/TrMag.hpp \
  Modules/MScalarSUN/TrPhi.hpp \
  Modules/MScalarSUN/TwoPoint.hpp \
  Modules/MScalarSUN/TwoPointNPR.hpp \
  Modules/MScalarSUN/Utils.hpp \
  Modules/MSink/Point.hpp \
  Modules/MSink/Smear.hpp \
  Modules/MSolver/A2AAslashVectors.hpp \
  Modules/MSolver/A2AVectors.hpp \
  Modules/MSolver/Guesser.hpp \
  Modules/MSolver/LocalCoherenceLanczos.hpp \
  Modules/MSolver/MixedPrecisionRBPrecCG.hpp \
  Modules/MSolver/RBPrecCG.hpp \
  Modules/MSource/Convolution.hpp \
  Modules/MSource/Gauss.hpp \
  Modules/MSource/JacobiSmear.hpp \
  Modules/MSource/Momentum.hpp \
  Modules/MSource/MomentumPhase.hpp \
  Modules/MSource/Point.hpp \
  Modules/MSource/SeqAslash.hpp \
  Modules/MSource/SeqConserved.hpp \
  Modules/MSource/SeqGamma.hpp \
  Modules/MSource/Wall.hpp \
  Modules/MSource/Z2.hpp \
  Modules/MUtilities/PrecisionCast.hpp \
  Modules/MUtilities/RandomVectors.hpp
