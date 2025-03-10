//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2017-21, Lawrence Livermore National Security, LLC
// and RAJA Performance Suite project contributors.
// See the RAJAPerf/LICENSE file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

// Uncomment to add compiler directives for loop unrolling
//#define USE_RAJA_UNROLL

#include "DIFFUSION3DPA.hpp"

#include "RAJA/RAJA.hpp"

#if defined(RAJA_ENABLE_CUDA)

#include "common/CudaDataUtils.hpp"

#include <iostream>

namespace rajaperf {
namespace apps {

#define DIFFUSION3DPA_DATA_SETUP_CUDA                                       \
  allocAndInitCudaDeviceData(Basis, m_B, DPA_Q1D *DPA_D1D);                 \
  allocAndInitCudaDeviceData(dBasis, m_G, DPA_Q1D *DPA_D1D);                \
  allocAndInitCudaDeviceData(D, m_D, DPA_Q1D *DPA_Q1D *DPA_Q1D *SYM *m_NE); \
  allocAndInitCudaDeviceData(X, m_X, DPA_D1D *DPA_D1D *DPA_D1D *m_NE);      \
  allocAndInitCudaDeviceData(Y, m_Y, DPA_D1D *DPA_D1D *DPA_D1D *m_NE);

#define DIFFUSION3DPA_DATA_TEARDOWN_CUDA                            \
  getCudaDeviceData(m_Y, Y, DPA_D1D *DPA_D1D *DPA_D1D *m_NE);       \
  deallocCudaDeviceData(Basis);                                     \
  deallocCudaDeviceData(dBasis);                                    \
  deallocCudaDeviceData(D);                                         \
  deallocCudaDeviceData(X);                                         \
  deallocCudaDeviceData(Y);

__global__ void Diffusion3DPA(Index_type NE, const Real_ptr Basis, const Real_ptr dBasis,
                              const Real_ptr D, const Real_ptr X, Real_ptr Y, bool symmetric) {

  const int e = blockIdx.x;

  DIFFUSION3DPA_0_GPU;

  GPU_FOREACH_THREAD(dy, y, DPA_D1D) {
    GPU_FOREACH_THREAD(dx, x, DPA_D1D) {
      DIFFUSION3DPA_1;
    }
    GPU_FOREACH_THREAD(qx, x, DPA_Q1D) {
      DIFFUSION3DPA_2;
    }
  }

  __syncthreads();
  GPU_FOREACH_THREAD(dy, y, DPA_D1D) {
    GPU_FOREACH_THREAD(qx, x, DPA_Q1D) {
      DIFFUSION3DPA_3;
    }
  }
  __syncthreads();
  GPU_FOREACH_THREAD(qy, y, DPA_Q1D) {
    GPU_FOREACH_THREAD(qx, x, DPA_Q1D) {
      DIFFUSION3DPA_4;
    }
  }
  __syncthreads();
  GPU_FOREACH_THREAD(qy, y, DPA_Q1D) {
    GPU_FOREACH_THREAD(qx, x, DPA_Q1D) {
      DIFFUSION3DPA_5;
    }
  }
  __syncthreads();
  GPU_FOREACH_THREAD(d, y, DPA_D1D) {
    GPU_FOREACH_THREAD(q, x, DPA_Q1D) {
      DIFFUSION3DPA_6;
    }
  }
  __syncthreads();
  GPU_FOREACH_THREAD(qy, y, DPA_Q1D) {
    GPU_FOREACH_THREAD(dx, x, DPA_D1D) {
      DIFFUSION3DPA_7;
    }
  }
  __syncthreads();
  GPU_FOREACH_THREAD(dy, y, DPA_D1D) {
    GPU_FOREACH_THREAD(dx, x, DPA_D1D) {
      DIFFUSION3DPA_8;
    }
  }
  __syncthreads();
  GPU_FOREACH_THREAD(dy, y, DPA_D1D) {
    GPU_FOREACH_THREAD(dx, x, DPA_D1D) {
      DIFFUSION3DPA_9;
    }
  }

}

void DIFFUSION3DPA::runCudaVariant(VariantID vid) {
  const Index_type run_reps = getRunReps();

  DIFFUSION3DPA_DATA_SETUP;

  switch (vid) {

  case Base_CUDA: {

    DIFFUSION3DPA_DATA_SETUP_CUDA;

    startTimer();
    for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

      dim3 nthreads_per_block(DPA_Q1D, DPA_Q1D, 1);

      Diffusion3DPA<<<NE, nthreads_per_block>>>(NE, Basis, dBasis, D, X, Y, symmetric);

      cudaErrchk( cudaGetLastError() );
    }
    stopTimer();

    DIFFUSION3DPA_DATA_TEARDOWN_CUDA;

    break;
  }

  case RAJA_CUDA: {

    DIFFUSION3DPA_DATA_SETUP_CUDA;

    using launch_policy = RAJA::expt::LaunchPolicy<RAJA::expt::seq_launch_t
                                                   ,RAJA::expt::cuda_launch_t<true>
                                                   >;

    using outer_x = RAJA::expt::LoopPolicy<RAJA::loop_exec
                                           ,RAJA::cuda_block_x_direct
                                           >;

    using inner_x = RAJA::expt::LoopPolicy<RAJA::loop_exec
                                             ,RAJA::cuda_thread_x_loop
                                             >;

    using inner_y = RAJA::expt::LoopPolicy<RAJA::loop_exec
                                             ,RAJA::cuda_thread_y_loop
                                             >;

    startTimer();
    for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

      RAJA::expt::launch<launch_policy>(
        RAJA::expt::DEVICE,
        RAJA::expt::Grid(RAJA::expt::Teams(NE),
                         RAJA::expt::Threads(DPA_Q1D, DPA_Q1D, 1)),
        [=] RAJA_HOST_DEVICE(RAJA::expt::LaunchContext ctx) {

          RAJA::expt::loop<outer_x>(ctx, RAJA::RangeSegment(0, NE),
            [&](int e) {

             DIFFUSION3DPA_0_GPU;

              RAJA::expt::loop<inner_y>(ctx, RAJA::RangeSegment(0, DPA_D1D),
                [&](int dy) {
                  RAJA::expt::loop<inner_x>(ctx, RAJA::RangeSegment(0, DPA_D1D),
                    [&](int dx) {
                      DIFFUSION3DPA_1;
                    }
                  );  // RAJA::expt::loop<inner_x>
                  RAJA::expt::loop<inner_x>(ctx, RAJA::RangeSegment(0, DPA_Q1D),
                    [&](int qx) {
                      DIFFUSION3DPA_2;
                    }
                  ); // RAJA::expt::loop<inner_x>
                } // lambda (dy)
             );  //RAJA::expt::loop<inner_y>

             ctx.teamSync();

             RAJA::expt::loop<inner_y>(ctx, RAJA::RangeSegment(0, DPA_D1D),
               [&](int dy) {
                 RAJA::expt::loop<inner_x>(ctx, RAJA::RangeSegment(0, DPA_Q1D),
                   [&](int qx) {
                     DIFFUSION3DPA_3;
                   }
                 ); // RAJA::expt::loop<inner_x>
               }
             ); // RAJA::expt::loop<inner_y>

             ctx.teamSync();

             RAJA::expt::loop<inner_y>(ctx, RAJA::RangeSegment(0, DPA_Q1D),
              [&](int qy) {
                RAJA::expt::loop<inner_x>(ctx, RAJA::RangeSegment(0, DPA_Q1D),
                  [&](int qx) {
                    DIFFUSION3DPA_4;
                  }
                ); // RAJA::expt::loop<inner_x>
               }
             ); // RAJA::expt::loop<inner_y>

             ctx.teamSync();

             RAJA::expt::loop<inner_y>(ctx, RAJA::RangeSegment(0, DPA_Q1D),
              [&](int qy) {
                RAJA::expt::loop<inner_x>(ctx, RAJA::RangeSegment(0, DPA_Q1D),
                  [&](int qx) {
                    DIFFUSION3DPA_5;
                  }
                ); // RAJA::expt::loop<inner_x>
               }
             ); // RAJA::expt::loop<inner_y>

             ctx.teamSync();

             RAJA::expt::loop<inner_y>(ctx, RAJA::RangeSegment(0, DPA_D1D),
               [&](int d) {
                 RAJA::expt::loop<inner_x>(ctx, RAJA::RangeSegment(0, DPA_Q1D),
                   [&](int q) {
                     DIFFUSION3DPA_6;
                   }
                 ); // RAJA::expt::loop<inner_x>
               }
             ); // RAJA::expt::loop<inner_y>

             ctx.teamSync();

             RAJA::expt::loop<inner_y>(ctx, RAJA::RangeSegment(0, DPA_Q1D),
               [&](int qy) {
                 RAJA::expt::loop<inner_x>(ctx, RAJA::RangeSegment(0, DPA_D1D),
                   [&](int dx) {
                     DIFFUSION3DPA_7;
                   }
                 ); // RAJA::expt::loop<inner_x>
               }
             ); // RAJA::expt::loop<inner_y>

             ctx.teamSync();

             RAJA::expt::loop<inner_y>(ctx, RAJA::RangeSegment(0, DPA_D1D),
               [&](int dy) {
                 RAJA::expt::loop<inner_x>(ctx, RAJA::RangeSegment(0, DPA_D1D),
                   [&](int dx) {
                     DIFFUSION3DPA_8;
                   }
                 ); // RAJA::expt::loop<inner_x>
               }
             ); // RAJA::expt::loop<inner_y>

              ctx.teamSync();
              RAJA::expt::loop<inner_y>(ctx, RAJA::RangeSegment(0, DPA_D1D),
                [&](int dy) {
                  RAJA::expt::loop<inner_x>(ctx, RAJA::RangeSegment(0, DPA_D1D),
                    [&](int dx) {
                      DIFFUSION3DPA_9;
                    }
                  ); // RAJA::expt::loop<inner_y>
                }
              ); // RAJA::expt::loop<inner_y>

            } // lambda (e)
          ); // RAJA::expt::loop<outer_x>

        } // outer lambda (ctx)
      ); // RAJA::expt::launch

    }  // loop over kernel reps
    stopTimer();

    DIFFUSION3DPA_DATA_TEARDOWN_CUDA;

    break;
  }

  default: {

    std::cout << "\n DIFFUSION3DPA : Unknown Cuda variant id = " << vid << std::endl;
    break;
  }
  }
}

} // end namespace apps
} // end namespace rajaperf

#endif // RAJA_ENABLE_CUDA
