//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2017-21, Lawrence Livermore National Security, LLC
// and RAJA Performance Suite project contributors.
// See the RAJAPerf/LICENSE file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#include "LTIMES.hpp"

#include "RAJA/RAJA.hpp"

#include <iostream>

namespace rajaperf 
{
namespace apps
{


void LTIMES::runOpenMPVariant(VariantID vid)
{
#if defined(RAJA_ENABLE_OPENMP) && defined(RUN_OPENMP)

  const Index_type run_reps = getRunReps();

  LTIMES_DATA_SETUP;

  switch ( vid ) {

    case Base_OpenMP : {

      startTimer();
      for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

        #pragma omp parallel for
        for (Index_type z = 0; z < num_z; ++z ) {
          for (Index_type g = 0; g < num_g; ++g ) {
            for (Index_type m = 0; m < num_m; ++m ) {
              for (Index_type d = 0; d < num_d; ++d ) {
                LTIMES_BODY;
              }
            }
          }
        }  

      }
      stopTimer();

      break;
    }

    case Lambda_OpenMP : {

      auto ltimes_base_lam = [=](Index_type d, Index_type z, 
                                 Index_type g, Index_type m) {
                               LTIMES_BODY;
                             };

      startTimer();
      for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

        #pragma omp parallel for
        for (Index_type z = 0; z < num_z; ++z ) {
          for (Index_type g = 0; g < num_g; ++g ) {
            for (Index_type m = 0; m < num_m; ++m ) {
              for (Index_type d = 0; d < num_d; ++d ) {
                ltimes_base_lam(d, z, g, m);
              }
            }
          }
        }

      }
      stopTimer();

      break;
    }

    case RAJA_OpenMP : {

      LTIMES_VIEWS_RANGES_RAJA;

      auto ltimes_lam = [=](ID d, IZ z, IG g, IM m) {
                          LTIMES_BODY_RAJA;
                        };

      using EXEC_POL = 
        RAJA::KernelPolicy<
          RAJA::statement::For<1, RAJA::omp_parallel_for_exec, // z
            RAJA::statement::For<2, RAJA::loop_exec,           // g
              RAJA::statement::For<3, RAJA::loop_exec,         // m
                RAJA::statement::For<0, RAJA::loop_exec,       // d
                  RAJA::statement::Lambda<0>
                >
              > 
            >
          >
        >;

      startTimer();
      for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

        RAJA::kernel<EXEC_POL>( RAJA::make_tuple(IDRange(0, num_d),
                                                 IZRange(0, num_z),
                                                 IGRange(0, num_g),
                                                 IMRange(0, num_m)), 
                                ltimes_lam
                              );

      }
      stopTimer();

      break;
    }

    default : {
      std::cout << "\n LTIMES : Unknown variant id = " << vid << std::endl;
    }

  }

#else 
  RAJA_UNUSED_VAR(vid);
#endif
}

} // end namespace apps
} // end namespace rajaperf
