#include "cudamatrix/cu-choleskykernels-ansi.h"
#include <stdio.h>


#define TILE_SIZE 16

/***********************************************************************
 * CUDA kernels
 * some functions are templated to have the float/double operations
 */
__device__ int lex_index_2D (int r, int c, int row_length) {
  return c +  r*row_length;
}


__device__ int global_pos(int t_pos, int block_offset) {
  return t_pos + TILE_SIZE*block_offset;
}


template<typename T>
__global__
void __factorize_diagonal_block(T* A, int block_offset, MatrixDim d) {
  int global_row_length = d.stride;
  //int global_row_length = 32;
  int col = threadIdx.x;
  int row = threadIdx.y;

  int global_row = global_pos(row,block_offset);
  int global_col = global_pos(col,block_offset);
  int idx = lex_index_2D(global_row, global_col, global_row_length);
  
  //int row = (blockIdx.y)* blockDim.y + threadIdx.y + block_offset * TILE_SIZE;
  //int col = (blockIdx.x)* blockDim.x + threadIdx.x + block_offset * TILE_SIZE;

  //int idx = col + row*d.stride;

  __shared__ T L[TILE_SIZE][TILE_SIZE+1];

//  if ( row < d.rows && col < d.cols) {  
    L[row][col] = A[idx];

    __syncthreads();

    T fac;

      for (int k = 0; k < TILE_SIZE; k++) {
	__syncthreads();
        fac = rsqrtf(L[k][k]);
        __syncthreads();

        if ((row==k)&&(col>=k))
      	  L[col][row] = (L[col][row])*fac;

        __syncthreads();

        if ((row>=col)&&(col>k))
      	  L[row][col] = L[row][col] - L[col][k]*L[row][k];
      }
    __syncthreads();
    
    if (row >= col)
      A[idx] = L[row][col];
//  }
}

/*
template<typename T>
__global__
void __strip_update(T* A, int block_offset, int global_row_length) {
  global_row_length = 64;
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  int j = blockIdx.y * blockDim.y + threadIdx.y;
  int idx = i + j*global_row_length;
  //
  //__shared__ T topleft[TILE_SIZE][TILE_SIZE+1];
  //topleft[j][i] = A[idx];
  //__syncthreads();
  //int row2 = j + block_offset * TILE_SIZE;
  //int idx_w = i + row2 * global_row_length;
  //__shared__ T workingmat[TILE_SIZE][TILE_SIZE+1];
  //workingmat[i][j] = A[idx_w]; 
  //__syncthreads();
  //if (j==0) {
  //  for (int k = 0; k < TILE_SIZE; k++) {
  //    T sum = 0.0;
  //    for (int m = 0; m < k; m++) sum = sum + topleft[k][m]*workingmat[m][i];
  //    workingmat[k][i] = (workingmat[k][i] - sum) / topleft[k][k];
  //  }
  //}
  //__syncthreads();
  //
  A[idx] = 0;
}
*/
template<typename T>
__global__
void __strip_update(T* A, int block_offset, int global_row_length) {
  int boffy = block_offset;
  int boffx = blockIdx.x + boffy + 1;
  
  int col = threadIdx.x;
  int row = threadIdx.y;

  __shared__ T topleft[TILE_SIZE][TILE_SIZE+1];
  __shared__ T workingmat[TILE_SIZE][TILE_SIZE+1];

  int global_row = global_pos(row,block_offset);
  int global_col = global_pos(col,block_offset);

  int idx = lex_index_2D(global_row, global_col, global_row_length);

  //global_row_length = 64;
  //int col = blockIdx.x * blockDim.x + threadIdx.x;
  //int row = blockIdx.y * blockDim.y + threadIdx.y;
  //int idx = col + row * global_row_length; 
  
  topleft[row][col] = A[idx];
  __syncthreads();
  
  global_row = global_pos(row,boffx);
  
  int idx_w = lex_index_2D(global_row, global_col, global_row_length);
  //int row2 = row + block_offset * TILE_SIZE;
  //int idx_w = row2 + col*global_row_length;
  workingmat[col][row]=A[idx_w];

  __syncthreads();
  
  if (row==0) {
    for (int k = 0; k < TILE_SIZE; k++) {
      T sum=0.0;
      for (int m = 0; m < k; m++) 
        sum = sum + topleft[k][m]*workingmat[m][col];
	
      workingmat[k][col] = (workingmat[k][col] - sum) / topleft[k][k];
    }
  }

  __syncthreads();

  A[idx_w] = workingmat[col][row];
  //A[idx_w] = 1;
}


template<typename T>
__global__
void __diag_update(T* A, int block_offset, int global_row_length) {
  int boffx = blockIdx.x + block_offset + 1;

  int col = threadIdx.x;
  int row = threadIdx.y;

  int global_row = global_pos(row,boffx);
  int global_col = global_pos(col,block_offset);

  int idx = lex_index_2D(global_row, global_col, global_row_length);

  __shared__ T left[TILE_SIZE][TILE_SIZE+1];

  left[row][col] = A[idx];
  
  __syncthreads();

  T sum = 0.0;


  if (row >= col) {
    for (int kk = 0; kk < TILE_SIZE; kk++)
      sum = sum + left[row][kk]*left[col][kk];
    global_col = global_pos(col, boffx);
    idx = lex_index_2D(global_row, global_col, global_row_length);

    A[idx] = A[idx] - sum;
  }
}


template<typename T>
__global__
void __lo_update(T* A, int block_offset, int n_blocks, int global_row_length) {
  int col = threadIdx.x;
  int row = threadIdx.y;
  
  int boffy = blockIdx.y + block_offset + 1;
  int boffx = boffy + 1;

  __shared__ T left[TILE_SIZE][TILE_SIZE];

  __shared__ T upt[TILE_SIZE][TILE_SIZE + 1];
  
  int global_row = global_pos(row,boffy);
  int global_col = global_pos(col,block_offset);

  int idx = lex_index_2D(global_row, global_col, global_row_length);

  upt[row][col] = A[idx];
  for (; boffx < n_blocks; boffx++) {
    global_row = global_pos(row,boffx);
    idx = lex_index_2D(global_row, global_col, global_row_length);
    
    left[row][col] = A[idx];
    
    __syncthreads();

    T matrixprod = 0.0;
    
    for (int kk = 0; kk < TILE_SIZE; kk++)
      matrixprod += left[row][kk]*upt[col][kk];

    __syncthreads();

    global_col = global_pos(col,boffy);
    idx = lex_index_2D(global_row, global_col, global_row_length);

    A[idx] -= matrixprod;
  }
}

/***********************************************************************
 * ANSI-C wrappers of CUDA kernels
 */

/*
 * float
 */

void cudaF_factorize_diagonal_block(float* A, int block_offset, MatrixDim d) {
  dim3 threads(TILE_SIZE,TILE_SIZE);
  __factorize_diagonal_block<<<1,threads>>>(A,block_offset,d);
  cudaThreadSynchronize();
}

void cudaF_strip_update(float* A, int block_offset, int n_rows_padded, int n_remaining_blocks) {
  dim3 stripgrid(n_remaining_blocks-1);
  dim3 threads(TILE_SIZE,TILE_SIZE);
  __strip_update<<<stripgrid,threads>>>(A,block_offset,n_rows_padded);
  cudaThreadSynchronize();
}

void cudaF_diag_update(float* A, int block_offset, int n_row_padded, int n_remaining_blocks) {
  dim3 diaggrid(n_remaining_blocks-1);
  dim3 threads(TILE_SIZE,TILE_SIZE);
  __diag_update<<<diaggrid,threads>>>(A,block_offset,n_row_padded);
  cudaThreadSynchronize();
}

void cudaF_lo_update(float* A, int block_offset, int n_blocks, int n_rows_padded, int n_remaining_blocks) {
  dim3 logrid;
  logrid.x = 1;
  logrid.y = n_remaining_blocks-2;
  dim3 threads(TILE_SIZE,TILE_SIZE);
  __lo_update<<<logrid,threads>>>(A,block_offset,n_blocks,n_rows_padded);
  cudaThreadSynchronize();
}
/*
 * double
 */
void cudaD_factorize_diagonal_block(double* A, int block_offset, MatrixDim d) {
  dim3 threads(TILE_SIZE,TILE_SIZE);
  __factorize_diagonal_block<<<1,threads>>>(A,block_offset,d);
  cudaThreadSynchronize();
}

void cudaD_strip_update(double* A, int block_offset, int n_rows_padded, int n_remaining_blocks) {
  dim3 stripgrid(n_remaining_blocks-1);
  dim3 threads(TILE_SIZE,TILE_SIZE);
  __strip_update<<<stripgrid,threads>>>(A,block_offset,n_rows_padded);
  cudaThreadSynchronize();
}

void cudaD_diag_update(double* A, int block_offset, int n_rows_padded, int n_remaining_blocks) {
  dim3 diaggrid(n_remaining_blocks-1);
  dim3 threads(TILE_SIZE,TILE_SIZE);
  __diag_update<<<diaggrid,threads>>>(A,block_offset,n_rows_padded);
  cudaThreadSynchronize();
}

void cudaD_lo_update(double* A, int block_offset, int n_blocks, int n_rows_padded, int n_remaining_blocks) {
  dim3 logrid;
  logrid.x = 1;
  logrid.y = n_remaining_blocks-2;
  dim3 threads(TILE_SIZE,TILE_SIZE);
  __lo_update<<<logrid,threads>>>(A,block_offset,n_blocks,n_rows_padded);
  cudaThreadSynchronize();
}
