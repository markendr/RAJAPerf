//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2017-21, Lawrence Livermore National Security, LLC
// and RAJA Performance Suite project contributors.
// See the RAJAPerf/LICENSE file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#include "DAXPY.hpp"

#include "RAJA/RAJA.hpp"

#if defined(RAJA_ENABLE_HIP)

#include "common/HipDataUtils.hpp"

#include <iostream>

namespace rajaperf
{
namespace basic
{

#define DAXPY_DATA_SETUP_HIP \
  allocAndInitHipDeviceData(x, m_x, iend); \
  allocAndInitHipDeviceData(y, m_y, iend);

#define DAXPY_DATA_TEARDOWN_HIP \
  getHipDeviceData(m_y, y, iend); \
  deallocHipDeviceData(x); \
  deallocHipDeviceData(y);

__global__ void daxpy(Real_ptr y, Real_ptr x,
                      Real_type a,
                      Index_type iend)
{
   Index_type i = blockIdx.x * blockDim.x + threadIdx.x;
   if (i < iend) {
     DAXPY_BODY;
   }
}



template < size_t block_size >
void DAXPY::runHipVariantImpl(VariantID vid)
{
  const Index_type run_reps = getRunReps();
  const Index_type ibegin = 0;
  const Index_type iend = getActualProblemSize();

  DAXPY_DATA_SETUP;

  if ( vid == Base_HIP ) {

    DAXPY_DATA_SETUP_HIP;

    startTimer();
    for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

      const size_t grid_size = RAJA_DIVIDE_CEILING_INT(iend, block_size);
      hipLaunchKernelGGL((daxpy),dim3(grid_size), dim3(block_size), 0, 0, y, x, a,
                                        iend );
      hipErrchk( hipGetLastError() );

    }
    stopTimer();

    DAXPY_DATA_TEARDOWN_HIP;

  } else if ( vid == Lambda_HIP ) {

    DAXPY_DATA_SETUP_HIP;

    startTimer();
    for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

      auto daxpy_lambda = [=] __device__ (Index_type i) {
        DAXPY_BODY;
      };

      const size_t grid_size = RAJA_DIVIDE_CEILING_INT(iend, block_size);
      hipLaunchKernelGGL(lambda_hip_forall<decltype(daxpy_lambda)>,
        grid_size, block_size, 0, 0, ibegin, iend, daxpy_lambda);
      hipErrchk( hipGetLastError() );

    }
    stopTimer();

    DAXPY_DATA_TEARDOWN_HIP;

  } else if ( vid == RAJA_HIP ) {

    DAXPY_DATA_SETUP_HIP;

    startTimer();
    for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

      RAJA::forall< RAJA::hip_exec<block_size, true /*async*/> >(
        RAJA::RangeSegment(ibegin, iend), [=] __device__ (Index_type i) {
        DAXPY_BODY;
      });

    }
    stopTimer();

    DAXPY_DATA_TEARDOWN_HIP;

  } else {
     std::cout << "\n  DAXPY : Unknown Hip variant id = " << vid << std::endl;
  }
}

void DAXPY::runHipVariant(VariantID vid)
{
  if ( !gpu_block_size::invoke_or(
           gpu_block_size::RunHipBlockSize<DAXPY>(*this, vid), gpu_block_sizes_type()) ) {
    std::cout << "\n  DAXPY : Unsupported Hip block_size " << getActualGPUBlockSize()
              <<" for variant id = " << vid << std::endl;
  }
}

} // end namespace basic
} // end namespace rajaperf

#endif  // RAJA_ENABLE_HIP
