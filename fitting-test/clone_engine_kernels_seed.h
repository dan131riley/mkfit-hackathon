#ifndef CLONE_ENGINE_KERNELS_SEED_H
#define CLONE_ENGINE_KERNELS_SEED_H 

#include "GPlex/GPlex.h"
#include "HitStructuresCU.h"
#include "GeometryCU.h"


void findInLayers_wrapper_seed(cudaStream_t &stream,
                          LayerOfHitsCU *layers, 
                          EventOfCombCandidatesCU &event_of_cands_cu,
                          GPlexQI &XHitSize, GPlexHitIdx &XHitArr,
                          GPlexLS &Err_iP, GPlexLV &Par_iP,
                          GPlexHS *msErr, GPlexHV *msPar,
                          GPlexLS &Err_iC, GPlexLV &Par_iC,
                          GPlexQF &Chi2, GPlexQI *HitsIdx,
                          GPlexQI &inChg, GPlexQI &Label,
                          GPlexQI &SeedIdx, GPlexQI &CandIdx,
                          GPlexQB& Valid,
                          GeometryCU &geom, 
                          int *maxSize, int N);

#endif  /* ifndef CLONE_ENGINE_KERNELS_SEED_H */
