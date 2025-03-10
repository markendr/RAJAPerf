//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2017-21, Lawrence Livermore National Security, LLC
// and RAJA Performance Suite project contributors.
// See the RAJAPerf/LICENSE file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#include "POLYBENCH_2MM.hpp"

#include "RAJA/RAJA.hpp"

#if defined(RAJA_ENABLE_HIP)

#include "common/HipDataUtils.hpp"

#include <iostream>

namespace rajaperf
{
namespace polybench
{

//
// Define thread block size for Hip execution
//
constexpr size_t out_block_sz = 8;
constexpr size_t in_block_sz = 32;

#define POLY_2MM_THREADS_PER_BLOCK_HIP \
  dim3 nthreads_per_block(in_block_sz, out_block_sz, 1);

#define POLY_2MM_1_NBLOCKS_HIP \
  dim3 nblocks1(static_cast<size_t>(RAJA_DIVIDE_CEILING_INT(nj, in_block_sz)), \
                static_cast<size_t>(RAJA_DIVIDE_CEILING_INT(ni, out_block_sz)), \
                static_cast<size_t>(1));

#define POLY_2MM_2_NBLOCKS_HIP \
  dim3 nblocks2(static_cast<size_t>(RAJA_DIVIDE_CEILING_INT(nl, in_block_sz)), \
                static_cast<size_t>(RAJA_DIVIDE_CEILING_INT(ni, out_block_sz)), \
                static_cast<size_t>(1));


#define POLYBENCH_2MM_DATA_SETUP_HIP \
  allocAndInitHipDeviceData(tmp, m_tmp, m_ni * m_nj); \
  allocAndInitHipDeviceData(A, m_A, m_ni * m_nk); \
  allocAndInitHipDeviceData(B, m_B, m_nk * m_nj); \
  allocAndInitHipDeviceData(C, m_C, m_nj * m_nl); \
  allocAndInitHipDeviceData(D, m_D, m_ni * m_nl);


#define POLYBENCH_2MM_TEARDOWN_HIP \
  getHipDeviceData(m_D, D, m_ni * m_nl); \
  deallocHipDeviceData(tmp); \
  deallocHipDeviceData(A); \
  deallocHipDeviceData(B); \
  deallocHipDeviceData(C); \
  deallocHipDeviceData(D);

__global__ void poly_2mm_1(Real_ptr tmp, Real_ptr A, Real_ptr B,
                           Real_type alpha,
                           Index_type ni, Index_type nj, Index_type nk)
{
  Index_type i = blockIdx.y * blockDim.y + threadIdx.y;
  Index_type j = blockIdx.x * blockDim.x + threadIdx.x;

  if ( i < ni && j < nj ) {
    POLYBENCH_2MM_BODY1;
    for (Index_type k=0; k < nk; ++k) {
      POLYBENCH_2MM_BODY2;
    }
    POLYBENCH_2MM_BODY3;
  }
}

template< typename Lambda >
__global__ void poly_2mm_1_lam(Index_type ni, Index_type nj,
                               Lambda body)
{
  Index_type i = blockIdx.y * blockDim.y + threadIdx.y;
  Index_type j = blockIdx.x * blockDim.x + threadIdx.x;

  if ( i < ni && j < nj ) {
    body(i, j);
  }
}

__global__ void poly_2mm_2(Real_ptr tmp, Real_ptr C, Real_ptr D,
                           Real_type beta,
                           Index_type ni,  Index_type nl, Index_type nj)
{
  Index_type i = blockIdx.y * blockDim.y + threadIdx.y;
  Index_type l = blockIdx.x * blockDim.x + threadIdx.x;

  if ( i < ni && l < nl ) {
    POLYBENCH_2MM_BODY4;
    for (Index_type j=0; j < nj; ++j) {
      POLYBENCH_2MM_BODY5;
    }
    POLYBENCH_2MM_BODY6;
  }
}

template< typename Lambda >
__global__ void poly_2mm_2_lam(Index_type ni,  Index_type nl,
                               Lambda body)
{
  Index_type i = blockIdx.y * blockDim.y + threadIdx.y;
  Index_type l = blockIdx.x * blockDim.x + threadIdx.x;

  if ( i < ni && l < nl ) {
    body(i, l);
  }
}


void POLYBENCH_2MM::runHipVariant(VariantID vid)
{
  const Index_type run_reps = getRunReps();

  POLYBENCH_2MM_DATA_SETUP;

  if ( vid == Base_HIP ) {

    POLYBENCH_2MM_DATA_SETUP_HIP;

    startTimer();
    for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

      POLY_2MM_THREADS_PER_BLOCK_HIP;

      POLY_2MM_1_NBLOCKS_HIP;
      hipLaunchKernelGGL((poly_2mm_1), 
                         dim3(nblocks1), dim3(nthreads_per_block), 0, 0,
                         tmp, A, B, alpha,
                         ni, nj, nk);
      hipErrchk( hipGetLastError() );

      POLY_2MM_2_NBLOCKS_HIP;
      hipLaunchKernelGGL((poly_2mm_2), 
                         dim3(nblocks2), dim3(nthreads_per_block), 0, 0,
                         tmp, C, D, beta,
                         ni, nl, nj);
      hipErrchk( hipGetLastError() );

    }
    stopTimer();

    POLYBENCH_2MM_TEARDOWN_HIP;

  } else if (vid == Lambda_HIP) {

    POLYBENCH_2MM_DATA_SETUP_HIP;

    startTimer();
    for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

      POLY_2MM_THREADS_PER_BLOCK_HIP;

      auto poly_2mm_1_lambda = [=] __device__ (Index_type i, Index_type j) {
        POLYBENCH_2MM_BODY1;
        for (Index_type k=0; k < nk; ++k) {
          POLYBENCH_2MM_BODY2;
        }
        POLYBENCH_2MM_BODY3;
      };

      POLY_2MM_1_NBLOCKS_HIP;      
      hipLaunchKernelGGL((poly_2mm_1_lam<decltype(poly_2mm_1_lambda)>),
                         dim3(nblocks1), dim3(nthreads_per_block), 0, 0,
                         ni, nj, poly_2mm_1_lambda);
      hipErrchk( hipGetLastError() );
 
      auto poly_2mm_2_lambda = [=] __device__ (Index_type i, Index_type l) {
        POLYBENCH_2MM_BODY4;
        for (Index_type j=0; j < nj; ++j) {
          POLYBENCH_2MM_BODY5;
        }
        POLYBENCH_2MM_BODY6;
      };

      POLY_2MM_2_NBLOCKS_HIP;
      hipLaunchKernelGGL((poly_2mm_2_lam<decltype(poly_2mm_2_lambda)>),
                         dim3(nblocks2), dim3(nthreads_per_block), 0, 0,
                         ni, nl, poly_2mm_2_lambda);
      hipErrchk( hipGetLastError() );

    }
    stopTimer();

    POLYBENCH_2MM_TEARDOWN_HIP;

  } else if (vid == RAJA_HIP) {

    POLYBENCH_2MM_DATA_SETUP_HIP;

    POLYBENCH_2MM_VIEWS_RAJA;

    using EXEC_POL =
      RAJA::KernelPolicy<
        RAJA::statement::HipKernelFixedAsync<out_block_sz * in_block_sz,
          RAJA::statement::Tile<0, RAJA::tile_fixed<out_block_sz>,
                                   RAJA::hip_block_y_direct,
            RAJA::statement::Tile<1, RAJA::tile_fixed<in_block_sz>,
                                     RAJA::hip_block_x_direct,
              RAJA::statement::For<0, RAJA::hip_thread_y_direct,   // outer
                RAJA::statement::For<1, RAJA::hip_thread_x_direct, // inner
                  RAJA::statement::Lambda<0, RAJA::Params<0>>,
                  RAJA::statement::For<2, RAJA::seq_exec,
                    RAJA::statement::Lambda<1, RAJA::Segs<0,1,2>, RAJA::Params<0>>
                  >,
                  RAJA::statement::Lambda<2, RAJA::Segs<0,1>, RAJA::Params<0>>
                >
              >
            >
          >
        >
      >;

    startTimer();
    for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

      RAJA::kernel_param<EXEC_POL>(
        RAJA::make_tuple(RAJA::RangeSegment{0, ni},
                         RAJA::RangeSegment{0, nj},
                         RAJA::RangeSegment{0, nk}),
        RAJA::tuple<Real_type>{0.0},

        [=] __device__ ( Real_type &dot) {
          POLYBENCH_2MM_BODY1_RAJA;
        },
        [=] __device__ (Index_type i, Index_type j, Index_type k,
                        Real_type &dot) {
          POLYBENCH_2MM_BODY2_RAJA;
        },
        [=] __device__ (Index_type i, Index_type j,
                        Real_type &dot) {
          POLYBENCH_2MM_BODY3_RAJA;
        }
      );

      RAJA::kernel_param<EXEC_POL>(
        RAJA::make_tuple(RAJA::RangeSegment{0, ni},
                         RAJA::RangeSegment{0, nl},
                         RAJA::RangeSegment{0, nj}),
        RAJA::tuple<Real_type>{0.0},

        [=] __device__ (Real_type &dot) {
          POLYBENCH_2MM_BODY4_RAJA;
        },
        [=] __device__ (Index_type i, Index_type l, Index_type j,
                        Real_type &dot) {
          POLYBENCH_2MM_BODY5_RAJA;
        },
        [=] __device__ (Index_type i, Index_type l,
                        Real_type &dot) {
          POLYBENCH_2MM_BODY6_RAJA;
        }
      );

    }
    stopTimer();

    POLYBENCH_2MM_TEARDOWN_HIP;

  } else {
      std::cout << "\n  POLYBENCH_2MM : Unknown Hip variant id = " << vid << std::endl;
  }

}

} // end namespace polybench
} // end namespace rajaperf

#endif  // RAJA_ENABLE_HIP

