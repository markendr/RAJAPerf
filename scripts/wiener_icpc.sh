#!/usr/bin/env bash

###############################################################################
# Copyright (c) 2017-21, Lawrence Livermore National Security, LLC
# and RAJA project contributors. See the RAJAPerf/LICENSE file for details.
#
# SPDX-License-Identifier: (BSD-3-Clause)
###############################################################################

RAJA_HOSTCONFIG=../tpl/RAJA/host-configs/wiener_icpc_X_gcc_headers.cmake

echo
echo "Creating build directory build and generating configuration in it"
echo

rm -rf build 2>/dev/null
mkdir build && cd build

module load cmake

##
# CMake option -DENABLE_FORCEINLINE_RECURSIVE=Off used to speed up compile
# times at a potential cost of slower 'forall' execution.
##

cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=icpx \
  -DCMAKE_C_COMPILER=icx \
  -DRAJA_USE_PAPI=On \
  -DBLT_CXX_STD=c++11 \
  -C ${RAJA_HOSTCONFIG} \
  -DENABLE_OPENMP=On \
  -DCMAKE_INSTALL_PREFIX=../install \
  "$@" \
  ..

rval=$?
if [[ $rval -ne 0 ]] ; then
  exit $rval
fi

echo
echo "***********************************************************************"
echo "cd into directory build and run make to build RAJA Perf Suite"
echo "***********************************************************************"
