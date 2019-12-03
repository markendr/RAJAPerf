//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2017-19, Lawrence Livermore National Security, LLC
// and RAJA Performance Suite project contributors.
// See the RAJAPerf/COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

///
/// INIT_VIEW1D_OFFSET kernel reference implementation:
///
/// const Real_type val = ...;
///
/// for (Index_type i = ibegin; i < iend; ++i ) {
///   a[i-ibegin] = val;
/// }
///
/// RAJA variants use a "View" and "Layout" to do the same thing. These 
/// RAJA constructs provide little benfit in 1D, but they are used here
/// to exercise those RAJA mechanics in the simplest scenario.
///

#ifndef RAJAPerf_Basic_INIT_VIEW1D_OFFSET_HPP
#define RAJAPerf_Basic_INIT_VIEW1D_OFFSET_HPP


#define INIT_VIEW1D_OFFSET_BODY  \
  a[i-ibegin] = v;

#define INIT_VIEW1D_OFFSET_BODY_RAJA  \
  view(i) = v;

#define INIT_VIEW1D_OFFSET_VIEW_RAJA  \
  using ViewType = RAJA::View<Real_type, RAJA::OffsetLayout<1> >; \
  ViewType view(a, RAJA::make_offset_layout<1>({{1}}, {{iend+1}}));


#include "common/KernelBase.hpp"

namespace rajaperf 
{
class RunParams;

namespace basic
{

class INIT_VIEW1D_OFFSET : public KernelBase
{
public:

  INIT_VIEW1D_OFFSET(const RunParams& params);

  ~INIT_VIEW1D_OFFSET();

  void setUp(VariantID vid);
  void runKernel(VariantID vid); 
  void updateChecksum(VariantID vid);
  void tearDown(VariantID vid);

  void runCudaVariant(VariantID vid);
  void runOpenMPTargetVariant(VariantID vid);

private:
  Real_ptr m_a;
  Real_type m_val;
};

} // end namespace basic
} // end namespace rajaperf

#endif // closing endif for header file include guard
