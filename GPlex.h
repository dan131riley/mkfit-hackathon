#ifndef _GPLEX_H_
#define _GPLEX_H_

#include <cuda_runtime.h>
#include <stdio.h>
#include <iostream>

#include "gpu_utils.h"

namespace GPlexBase
{
typedef int idx_t;
//------------------------------------------------------------------------------

template<typename T, idx_t D1, idx_t D2, idx_t N>
class GPlexBase
{
public:
   typedef T value_type;

   enum
   {
      /// return no. of matrix rows
      kRows = D1,
      /// return no. of matrix columns
      kCols = D2,
      /// return no of elements: rows*columns
      kSize = D1 * D2,
      /// size of the whole GPlexBase
      kTotSize = N * kSize
   };

   T fArray[kTotSize] __attribute__((aligned(64)));


   __host__ __device__ T  operator[](idx_t xx) const { return fArray[xx]; }
   __host__ __device__ T& operator[](idx_t xx)       { return fArray[xx]; }

   __host__ __device__ T ConstAt(idx_t n, idx_t i, idx_t j) const { return fArray[(i * D2 + j) * N + n]; }
   __host__ __device__ T& At(idx_t n, idx_t i, idx_t j) { return fArray[(i * D2 + j) * N + n]; }

   __host__ __device__ T  operator()(idx_t n, idx_t i, idx_t j) const { return fArray[(i * D2 + j) * N + n]; }
   __host__ __device__ T& operator()(idx_t n, idx_t i, idx_t j) { return fArray[(i * D2 + j) * N + n]; }

   size_t size() const { return kTotSize; }
};


template<typename T, idx_t D1, idx_t D2, idx_t N> using GPlexB = GPlexBase<T, D1, D2, N>;
}

//#include "gpu_constants.h"
__device__ __constant__ static int gplexSymOffsets[7][36] =
{
  {}, 
  {},
  { 0, 1, 1, 2 },
  { 0, 1, 3, 1, 2, 4, 3, 4, 5 }, // 3
  {},
  {},
  { 0, 1, 3, 6, 10, 15, 1, 2, 4, 7, 11, 16, 3, 4, 5, 8, 12, 17, 6, 7, 8, 9, 13, 18, 10, 11, 12, 13, 14, 19, 15, 16, 17, 18, 19, 20 }
};

constexpr GPlexBase::idx_t NN =  8; // "Length" of GPlexB.

constexpr GPlexBase::idx_t LL =  6; // Dimension of large/long  GPlexB entities
constexpr GPlexBase::idx_t HH =  3; // Dimension of small/short GPlexB entities

typedef GPlexBase::GPlexBase<float, LL, LL, NN>   GPlexBLL;

// The number of tracks is the fast dimension and is padded in order to have
// consecutive and aligned memory accesses. For cached reads, this result in a
// single memory transaction for the 32 threads of a warp to access 32 floats.
// See:
// http://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#global-memory-3-0
// In practice, The number of tracks (ntracks) is set to be MPT_SIZE

template <typename M>
struct GPlex { 
  using T = typename M::value_type;
  using value_type = T;

  static const size_t kRows = M::kRows;
  static const size_t kCols = M::kCols;
  static const size_t kSize = M::kSize;

  T* ptr;
  size_t pitch, stride, N;

  __host__ __device__ T  operator[](int xx) const { return ptr[xx]; }
  __host__ __device__ T& operator[](int xx)       { return ptr[xx]; }

  __host__ __device__ T  operator()(int n, int i, int j) const { return ptr[n + (i*kCols + j)*stride]; }
  __host__ __device__ T& operator()(int n, int i, int j)       { return ptr[n + (i*kCols + j)*stride]; }

  T* Ptr() { return ptr; }

  void allocate(size_t ntracks) {
    N = ntracks;
    cudaMallocPitch((void**)&ptr, &pitch, N*sizeof(T), kSize);
    stride = pitch/sizeof(T);  // Number of elements
    //std::cout << "Stride: " << stride << " Pitch: " << pitch << " wxh " << N*sizeof(T) << " x " << kSize << " = " << N*sizeof(T)*kSize << std::endl;
  }

  void allocateManaged(size_t ntracks, bool readMostly = false) {
    N = ntracks;
    cudaMallocManaged((void**) &ptr, N*sizeof(T)*kSize);
    if (readMostly) {
      cudaMemAdvise(ptr, N*sizeof(T)*kSize, cudaMemAdviseSetReadMostly, 0);
    }
    pitch = N*sizeof(T);
    stride = N;  // Number of elements
    //std::cout << "Stride: " << stride << " Pitch: " << pitch << " wxh " << N*sizeof(T) << " x " << kSize << " = " << N*sizeof(T)*kSize << std::endl;
  }

  void free() {
    cudaFree(ptr);
    N = 0; pitch = 0; stride = 0;
  }
  //cudaMemcpy2D(d_msErr.ptr, d_msErr.pitch, msErr.fArray, N*sizeof(T),
               //N*sizeof(T), HS, cudaMemcpyHostToDevice);

  void copyFromHost(const M& gplex) {
    cudaMemcpy2D(ptr, pitch, gplex.fArray, N*sizeof(T),
                 N*sizeof(T), kSize, cudaMemcpyHostToDevice);
    cudaCheckError();
  }
  void copyToHost(M& gplex) {
    cudaMemcpy2D(gplex.fArray, N*sizeof(T), ptr, pitch,
                 N*sizeof(T), kSize, cudaMemcpyDeviceToHost);
    cudaCheckError();
  }
  size_t size() const { return N*kSize; }
};


template <typename M>
struct GPlexSym : GPlex<M> {
  using T = typename GPlex<M>::T;
  using GPlex<M>::kRows;
  using GPlex<M>::kCols;
  using GPlex<M>::stride;
  using GPlex<M>::ptr;
  __device__ size_t Off(size_t i) const { return gplexSymOffsets[kRows][i]; }
  // Note: convenient but noticeably slower due to the indirection
  __device__ T& operator()(int n, int i, int j)       { return ptr[n + Off(i*kCols + j)*stride]; }
  __device__ T  operator()(int n, int i, int j) const { return ptr[n + Off(i*kCols + j)*stride]; }
  //__device__ T& operator()(int n, int i, int j)       { return ptr[n + i*stride]; }
  //__device__ T  operator()(int n, int i, int j) const { return ptr[n + i*stride]; }
};

using GPlexLL = GPlex<GPlexBLL>;

template <typename M>
struct GPlexReg {
  using T = typename M::value_type;
  using value_type = T;

  size_t kRows = M::kRows;
  size_t kCols = M::kCols;
  size_t kSize = M::kSize;

  __device__ T  operator[](int xx) const { return arr[xx]; }
  __device__ T& operator[](int xx)       { return arr[xx]; }

  __device__ T& operator()(int n, int i, int j)       { return arr[i*kCols + j]; }
  __device__ T  operator()(int n, int i, int j) const { return arr[i*kCols + j]; }

  __device__ void SetVal(T v)
  {
     for (int i = 0; i < kSize; ++i)
     {
        arr[i] = v;
     }
  }

  T arr[M::kSize];
};

using GPlexRegLL = GPlexReg<GPlexBLL>;
#endif  // _GPLEX_H_
