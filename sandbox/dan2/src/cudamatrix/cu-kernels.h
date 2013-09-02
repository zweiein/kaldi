// cudamatrix/cu-kernels.h

// Copyright 2009-2012  Karel Vesely
//                2013  Ehsan Variani
//                2014  Johns Hopkins University (author: Daniel Povey)

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.



#ifndef KALDI_CUDAMATRIX_CU_KERNELS_H_
#define KALDI_CUDAMATRIX_CU_KERNELS_H_

#if HAVE_CUDA == 1

#include "base/kaldi-error.h"
#include "cudamatrix/cu-kernels-ansi.h"

/*
 * In this file are C++ templated wrappers 
 * of the ANSI-C CUDA kernels
 */

namespace kaldi {


/*
 * CuMatrix 
 */
inline void cuda_ammdm_elements(dim3 Gr, dim3 Bl, float alpha, float* mat, const float* A, const float* B, const float* C, float beta, MatrixDim d) { cudaF_ammdm_elements(Gr,Bl,alpha,mat,A,B,C,beta,d); } 
inline void cuda_copy_from_tp_trans(int Gr, int Bl, float* A, const float* B, MatrixDim dmat) { cudaF_copy_from_tp_trans(Gr,Bl,A,B,dmat); }
inline void cuda_copy_from_tp_trans(int Gr, int Bl, float* A, const double* B, MatrixDim dmat) { cudaFD_copy_from_tp_trans(Gr,Bl,A,B,dmat); }
inline void cuda_copy_from_tp(int Gr, int Bl, float* A, const float* B, MatrixDim dmat) { cudaF_copy_from_tp(Gr,Bl,A,B,dmat); }
inline void cuda_copy_from_tp(int Gr, int Bl, float* A, const double* B, MatrixDim dmat) { cudaFD_copy_from_tp(Gr,Bl,A,B,dmat); }

inline void cuda_trace_sp_sp_fd(int Gr, int Bl, const float* A, const float* B, float* value, int dim) { cudaF_trace_sp_sp_fd(Gr,Bl,A,B,value,dim); }
inline void cuda_trace_sp_sp_df(int Gr, int Bl, const double* A, const float* B, double* value, int dim) { cudaF_trace_sp_sp_df(Gr,Bl,A,B,value,dim); }

inline void cuda_copy_from_mat(dim3 Gr, dim3 Bl, float* mat_out, const double* mat_in, MatrixDim d_out, MatrixDim d_in) {
  cuda_copy_from_mat_fd(Gr, Bl, mat_out, mat_in, d_out, d_in);
}
inline void cuda_copy_from_mat(dim3 Gr, dim3 Bl, float* mat_out, const float* mat_in, MatrixDim d_out, MatrixDim d_in) {
  cuda_copy_from_mat_ff(Gr, Bl, mat_out, mat_in, d_out, d_in);
}
inline void cuda_copy_from_mat(dim3 Gr, dim3 Bl, double* mat_out, const double* mat_in, MatrixDim d_out, MatrixDim d_in) {
  cuda_copy_from_mat_dd(Gr, Bl, mat_out, mat_in, d_out, d_in);
}
inline void cuda_copy_from_mat(dim3 Gr, dim3 Bl, double* mat_out, const float* mat_in, MatrixDim d_out, MatrixDim d_in) {
  cuda_copy_from_mat_df(Gr, Bl, mat_out, mat_in, d_out, d_in);
}

inline void cuda_copy_from_mat_trans(dim3 Gr, dim3 Bl, float* mat_out, const double* mat_in, MatrixDim d_out, MatrixDim d_in) {
  cuda_copy_from_mat_fd_trans(Gr, Bl, mat_out, mat_in, d_out, d_in);
}
inline void cuda_copy_from_mat_trans(dim3 Gr, dim3 Bl, float* mat_out, const float* mat_in, MatrixDim d_out, MatrixDim d_in) {
  cuda_copy_from_mat_ff_trans(Gr, Bl, mat_out, mat_in, d_out, d_in);
}
inline void cuda_copy_from_mat_trans(dim3 Gr, dim3 Bl, double* mat_out, const double* mat_in, MatrixDim d_out, MatrixDim d_in) {
  cuda_copy_from_mat_dd_trans(Gr, Bl, mat_out, mat_in, d_out, d_in);
}
inline void cuda_copy_from_mat_trans(dim3 Gr, dim3 Bl, double* mat_out, const float* mat_in, MatrixDim d_out, MatrixDim d_in) {
  cuda_copy_from_mat_df_trans(Gr, Bl, mat_out, mat_in, d_out, d_in);
}

inline void cuda_copy_col_from_vec(int Gr, int Bl, float* mat, const float* v, int col, MatrixDim d) { cudaF_copy_col_from_vec(Gr,Bl,mat,v,col,d); }
inline void cuda_apply_exp(dim3 Gr, dim3 Bl, float* mat, MatrixDim d) { cudaF_apply_exp(Gr,Bl,mat,d); }
inline void cuda_sum(dim3 Gr, dim3 Bl, float* mat, float* value, MatrixDim d) { cudaF_sum(Gr,Bl,mat,value,d); }
inline void cuda_apply_pow(dim3 Gr, dim3 Bl, float* mat, float power, MatrixDim dim) { cudaF_apply_pow(Gr,Bl,mat,power,dim); }
inline void cuda_apply_heaviside(dim3 Gr, dim3 Bl, float* mat, MatrixDim dim) { cudaF_apply_heaviside(Gr,Bl,mat,dim); }
inline void cuda_apply_floor(dim3 Gr, dim3 Bl, float* mat, float floor_val, MatrixDim dim) { cudaF_apply_floor(Gr,Bl,mat,floor_val,dim); }
inline void cuda_apply_ceiling(dim3 Gr, dim3 Bl, float* mat, float ceiling_val, MatrixDim dim) { cudaF_apply_ceiling(Gr,Bl,mat,ceiling_val,dim); }
inline void cuda_permute_columns(dim3 Gr, dim3 Bl, float* dst, const float* src, const int32_cuda* reorder, MatrixDim dst_dim, int src_stride) {
  cudaF_permute_columns(Gr, Bl, dst, src, reorder, dst_dim, src_stride);
}
inline void cuda_trace(int Gr, int Bl, float* mat, float* value, int dim) { cudaF_trace(Gr,Bl,mat,value,dim); }
inline void cuda_set_diag(int Gr, int Bl, float* mat, float value, MatrixDim d) { cudaF_set_diag(Gr,Bl,mat,value,d); }
inline void cuda_set_diag_packed(int Gr, int Bl, float* mat, float value, int dim) { cudaF_set_diag_packed(Gr,Bl,mat,value,dim); }
inline void cuda_add_diag_packed(int Gr, int Bl, float* mat, float value, int dim) { cudaF_add_diag_packed(Gr,Bl,mat,value,dim); }
inline void cuda_set_const(dim3 Gr, dim3 Bl, float *mat, float value, MatrixDim d) { cudaF_set_const(Gr,Bl,mat,value,d); }
inline void cuda_set_zero_above_diag(dim3 Gr, dim3 Bl, float* mat, MatrixDim d) { cudaF_set_zero_above_diag(Gr,Bl,mat,d); }
inline void cuda_add(dim3 Gr, dim3 Bl, float *mat, float value, MatrixDim d) { cudaF_add(Gr,Bl,mat,value,d); }
inline void cuda_add_vec2(dim3 Gr, dim3 Bl, float *mat, const float *vec, const float alpha, int dim) { cudaF_add_vec2(Gr,Bl,mat,vec,alpha,dim); }
inline void cuda_scale_diag(int Gr, int Bl, float* mat, float value, int dim) { cudaF_scale_diag(Gr,Bl,mat,value,dim); }
inline void cuda_scale(dim3 Gr, dim3 Bl, float *mat, float value, MatrixDim d) { cudaF_scale(Gr,Bl,mat,value,d); }
inline void cuda_apply_log(dim3 Gr, dim3 Bl, float *mat, MatrixDim d) { cudaF_apply_log(Gr,Bl,mat,d); }
inline void cuda_mul_elements(dim3 Gr, dim3 Bl, float *mat, const float *A, MatrixDim dst_d, int src_stride) {
  cudaF_mul_elements(Gr,Bl,mat,A,dst_d,src_stride);
}
inline void cuda_max(dim3 Gr, dim3 Bl, float *mat, const float *A, MatrixDim dst_d, int src_stride) {
  cudaF_max(Gr,Bl,mat,A,dst_d,src_stride);
}
inline void cuda_mul_cols_vec(dim3 Gr, dim3 Bl, float *mat, const float *scale, MatrixDim d) { cudaF_mul_cols_vec(Gr,Bl,mat,scale,d); }
inline void cuda_mul_rows_vec(dim3 Gr, dim3 Bl, float *mat, const float *scale, MatrixDim d) { cudaF_mul_rows_vec(Gr,Bl,mat,scale,d); }
inline void cuda_div_rows_vec(dim3 Gr, dim3 Bl, float *mat, const float *vec_div, MatrixDim d) { cudaF_div_rows_vec(Gr,Bl,mat,vec_div,d); }
inline void cuda_add_mat(dim3 Gr, dim3 Bl, float alpha, const float *A, float beta, float *dst, MatrixDim d) { cudaF_add_mat(Gr,Bl,alpha,A,beta,dst,d); }
inline void cuda_add_vec_to_cols(dim3 Gr, dim3 Bl, float alpha, const float *col, float beta, float *dst, MatrixDim d) { cudaF_add_vec_to_cols(Gr,Bl,alpha,col,beta,dst,d); }
inline void cuda_add_vec_to_rows(dim3 Gr, dim3 Bl, float alpha, const float *row, float beta, float *dst, MatrixDim d) { cudaF_add_vec_to_rows(Gr,Bl,alpha,row,beta,dst,d); }
 
/*
 * CuVector
 */
inline void cuda_set_bias_params(int Gr, int Bl, float* v, const float* a, float param_1, float param_2, float param_3, int* flag, int dim) { cudaF_set_bias_params(Gr,Bl,v,a,param_1,param_2,param_3,flag,dim); }
inline void cuda_copy_from_vec_df(int Gr, int Bl, double* v_out, const float* v_in, int dim) { cudaF_copy_from_vec_df(Gr,Bl,v_out,v_in,dim); }
inline void cuda_copy_from_vec_fd(int Gr, int Bl, float* v_out, const float* v_in, int dim) { cudaF_copy_from_vec_fd(Gr,Bl,v_out,v_in,dim); }
inline void cuda_vec_mul_elements(int Gr, int Bl, float* v, const float* a, int dim) { cudaF_vec_mul_elements(Gr,Bl,v,a,dim); }
inline void cuda_vec_soft_max(int Gr, int Bl, float* v, int dim) { cudaF_vec_soft_max(Gr,Bl,v,dim); }
inline void cuda_min(int Gr, int Bl, const float* v, float* value, int dim) { cudaF_min(Gr,Bl,v,value,dim); }
inline void cuda_trace_mat_mat_trans(int Gr, int Bl, const float* A, const float* B, MatrixDim dA, MatrixDim dB, float* value) { cudaF_trace_mat_mat_trans(Gr,Bl,A,B,dA,dB,value); }
inline void cuda_trace_mat_mat(int Gr, int Bl, const float* A, const float* B, MatrixDim dA, MatrixDim dB, float* value) { cudaF_trace_mat_mat(Gr,Bl,A,B,dA,dB,value); }
inline void cuda_add_diag_mat_trans(int Gr, int Bl, float alpha, float* v, const float* mat, float beta, MatrixDim dmat, int dim) { cudaF_add_diag_mat_trans(Gr,Bl,alpha,v,mat,beta,dmat,dim); }
inline void cuda_add_diag_mat(int Gr, int Bl, float alpha, float* v, const float* mat, float beta, MatrixDim dmat, int dim) { cudaF_add_diag_mat(Gr,Bl,alpha,v,mat,beta,dmat,dim); }
inline void cuda_add_vec_vec(int Gr, int Bl, float alpha, float* v, const float* x, const float* y, float beta, int dim) { cudaF_add_vec_vec(Gr,Bl,alpha,v,x,y,beta,dim); }
inline void cuda_copy_col_from_mat(int Gr, int Bl, float* v, int col, const float* mat, MatrixDim dmat, int dim) { cudaF_copy_col_from_mat(Gr,Bl,v,col,mat,dmat,dim); }
inline void cuda_copy_col_from_mat_df(int Gr, int Bl, double* v, int col, const float* mat, MatrixDim dmat, int dim) { cudaF_copy_col_from_mat_df(Gr,Bl,v,col,mat,dmat,dim); }
inline void cuda_copy_col_from_mat_fd(int Gr, int Bl, float* v, int col, const float* mat, MatrixDim dmat, int dim) { cudaF_copy_col_from_mat_fd(Gr,Bl,v,col,mat,dmat,dim); }
inline void cuda_vec_sum(int Gr, int Bl, float* v, float* value, int dim) { cudaF_vec_sum(Gr,Bl,v,value,dim); }
inline void cuda_vec_apply_floor(int Gr, int Bl, float* v, float floor_val, float* num, int dim) { cudaF_vec_apply_floor(Gr,Bl,v,floor_val,num,dim); }
inline void cuda_vec_apply_exp(int Gr, int Bl, float* v, int dim) { cudaF_vec_apply_exp(Gr,Bl,v,dim); }
inline void cuda_vec_apply_log(int Gr, int Bl, float* v, float* flag, int dim) { cudaF_vec_apply_log(Gr,Bl,v,flag,dim); }
inline void cuda_add_row_sum_mat(dim3 Gr, dim3 Bl, const float *mat, float *vec_sum, MatrixDim d) { cudaF_add_row_sum_mat(Gr,Bl,mat,vec_sum,d); }
inline void cuda_add_col_sum_mat(dim3 Gr, dim3 Bl, const float *mat, float *vec_sum, MatrixDim d) { cudaF_add_col_sum_mat(Gr,Bl,mat,vec_sum,d); }
inline void cuda_invert_elements(dim3 Gr, dim3 Bl, float *data, MatrixDim d) { cudaF_invert_elements(Gr,Bl,data,d); }

/*
 * cu::
 */
inline void cuda_soft_hinge(dim3 Gr, dim3 Bl, float *y, const float *x, MatrixDim d) { cudaF_soft_hinge(Gr,Bl,y,x,d); }
inline void cuda_sigmoid(dim3 Gr, dim3 Bl, float *y, const float *x, MatrixDim d) { cudaF_sigmoid(Gr,Bl,y,x,d); }
inline void cuda_diff_sigmoid(dim3 Gr, dim3 Bl, float *eout, const float *e, const float *y, MatrixDim d) { cudaF_diff_sigmoid(Gr,Bl,eout,e,y,d); }
inline void cuda_tanh(dim3 Gr, dim3 Bl, float *y, const float *x, MatrixDim d) { cudaF_tanh(Gr,Bl,y,x,d); }
inline void cuda_diff_tanh(dim3 Gr, dim3 Bl, float *eout, const float *e, const float *y, MatrixDim d) { cudaF_diff_tanh(Gr,Bl,eout,e,y,d); }
inline void cuda_softmax(size_t Gr, size_t Bl, float *y, const float *x, MatrixDim d) { cudaF_softmax(Gr,Bl,y,x,d); }
inline void cuda_softmax_part(dim3 Gr, dim3 Bl, const float *X, const int32_cuda *vec_ids, float* Y, MatrixDim d) { cudaF_softmax_part(Gr,Bl,X,vec_ids,Y,d); }

inline void cuda_regularize_l1(dim3 Gr, dim3 Bl, float *wei, float *grad, float l1, float lr, MatrixDim d) { cudaF_regularize_l1(Gr,Bl,wei,grad,l1,lr,d); }
inline void cuda_find_row_max_id(dim3 Gr, dim3 Bl, const float *mat, float *vec_val, int32_cuda *vec_id, int32_cuda voff, MatrixDim d) { cudaF_find_row_max_id(Gr,Bl,mat,vec_val,vec_id,voff,d); }
inline void cuda_diff_xent(dim3 Gr, dim3 Bl, const int32_cuda *vec_tgt, float *mat_net_out, float *vec_log_post, MatrixDim d) { cudaF_diff_xent(Gr,Bl,vec_tgt,mat_net_out,vec_log_post,d); }

inline void cuda_randomize(dim3 Gr, dim3 Bl, float *y, const float *x, const int32_cuda *copy_from, MatrixDim d_out, MatrixDim d_in) { cudaF_randomize(Gr,Bl,y,x,copy_from,d_out,d_in); }

inline void cuda_splice(dim3 Gr, dim3 Bl, float *y, const float *x, const int32_cuda *off, MatrixDim d_out, MatrixDim d_in) { cudaF_splice(Gr,Bl,y,x,off,d_out,d_in); }
inline void cuda_one(int Gr,int Bl,float* x,int dim) { cudaF_one(Gr,Bl,x,dim); }
inline void cuda_copy(dim3 Gr, dim3 Bl, float *y, const float *x, const int32_cuda *copy_from, MatrixDim d_out, MatrixDim d_in) { cudaF_copy(Gr,Bl,y,x,copy_from,d_out,d_in); }
inline void cuda_copy_diag(int Gr, int Bl, float* y, const float* x, int dim) { cudaF_copy_diag(Gr,Bl,y,x,dim); }
inline void cuda_copy_from_sp(int Gr, int Bl, const float* x, float* y, int d_in, MatrixDim d_out) { cudaF_copy_from_sp(Gr,Bl,x,y,d_in,d_out); }
inline void cuda_take_lower(dim3 Gr, dim3 Bl, const float* x, float* y, MatrixDim d_in, int d_out) { cudaF_take_lower(Gr,Bl,x,y,d_in,d_out); }
inline void cuda_take_upper(dim3 Gr, dim3 Bl, const float* x, float* y, MatrixDim d_in, int d_out) { cudaF_take_upper(Gr,Bl,x,y,d_in,d_out); }
inline void cuda_take_mean(dim3 Gr, dim3 Bl, const float* x, float* y, MatrixDim d_in, int d_out) { cudaF_take_mean(Gr,Bl,x,y,d_in,d_out); }

// double versions

/*
 * CuMatrix 
 */
inline void cuda_ammdm_elements(dim3 Gr, dim3 Bl, double alpha, double* mat, const double* A, const double* B, const double* C, double beta, MatrixDim d) { cudaD_ammdm_elements(Gr,Bl,alpha,mat,A,B,C,beta,d); }
inline void cuda_copy_from_tp_trans(int Gr, int Bl, double* A, const double* B, MatrixDim dmat) { cudaD_copy_from_tp_trans(Gr,Bl,A,B,dmat); }
inline void cuda_copy_from_tp_trans(int Gr, int Bl, double* A, const float* B, MatrixDim dmat) { cudaDF_copy_from_tp_trans(Gr,Bl,A,B,dmat); }
inline void cuda_copy_from_tp(int Gr, int Bl, double* A, const double* B, MatrixDim dmat) { cudaD_copy_from_tp(Gr,Bl,A,B,dmat); }
inline void cuda_copy_from_tp(int Gr, int Bl, double* A, const float* B, MatrixDim dmat) { cudaDF_copy_from_tp(Gr,Bl,A,B,dmat); }
inline void cuda_trace_sp_sp_fd(int Gr, int Bl, const float* A, const double* B, float* value, int dim) { cudaD_trace_sp_sp_fd(Gr,Bl,A,B,value,dim); }
inline void cuda_trace_sp_sp_df(int Gr, int Bl, const double* A, const double* B, double* value, int dim) { cudaD_trace_sp_sp_df(Gr,Bl,A,B,value,dim); }
inline void cuda_copy_col_from_vec(int Gr, int Bl, double* mat, const double* v, int col, MatrixDim d) { cudaD_copy_col_from_vec(Gr,Bl,mat,v,col,d); }
inline void cuda_apply_exp(dim3 Gr, dim3 Bl, double* mat, MatrixDim d) { cudaD_apply_exp(Gr,Bl,mat,d); }
inline void cuda_sum(dim3 Gr, dim3 Bl, double* mat, double* value, MatrixDim d) { cudaD_sum(Gr,Bl,mat,value,d); }
inline void cuda_apply_pow(dim3 Gr, dim3 Bl, double* mat, double power, MatrixDim dim) { cudaD_apply_pow(Gr,Bl,mat,power,dim); }
inline void cuda_apply_heaviside(dim3 Gr, dim3 Bl, double* mat, MatrixDim dim) { cudaD_apply_heaviside(Gr,Bl,mat,dim); }
inline void cuda_apply_floor(dim3 Gr, dim3 Bl, double* mat, double floor_val, MatrixDim dim) { cudaD_apply_floor(Gr,Bl,mat,floor_val,dim); }
inline void cuda_apply_ceiling(dim3 Gr, dim3 Bl, double* mat, double ceiling_val, MatrixDim dim) { cudaD_apply_ceiling(Gr,Bl,mat,ceiling_val,dim); }
inline void cuda_permute_columns(dim3 Gr, dim3 Bl, double* dst, const double* src, const int32_cuda* reorder, MatrixDim dst_dim, int src_stride) {
  cudaD_permute_columns(Gr, Bl, dst, src, reorder, dst_dim, src_stride);
}
inline void cuda_trace(int Gr, int Bl, double* mat, double* value, int dim) { cudaD_trace(Gr,Bl,mat,value,dim); }
inline void cuda_set_diag(int Gr, int Bl, double* mat, double value, MatrixDim d) { cudaD_set_diag(Gr,Bl,mat,value,d); }
inline void cuda_set_diag_packed(int Gr, int Bl, double* mat, double value, int dim) { cudaD_set_diag_packed(Gr,Bl,mat,value,dim); }
inline void cuda_add_diag_packed(int Gr, int Bl, double* mat, double value, int dim) { cudaD_add_diag_packed(Gr,Bl,mat,value,dim); }
inline void cuda_set_const(dim3 Gr, dim3 Bl, double *mat, double value, MatrixDim d) { cudaD_set_const(Gr,Bl,mat,value,d); }
inline void cuda_set_zero_above_diag(dim3 Gr, dim3 Bl, double* mat, MatrixDim d) { cudaD_set_zero_above_diag(Gr,Bl,mat,d); }
inline void cuda_add(dim3 Gr, dim3 Bl, double *mat, double value, MatrixDim d) { cudaD_add(Gr,Bl,mat,value,d); }
inline void cuda_add_vec2(dim3 Gr, dim3 Bl, double *mat, const double *vec, const double alpha, int dim) { cudaD_add_vec2(Gr,Bl,mat,vec,alpha,dim); }
inline void cuda_scale_diag(int Gr, int Bl, double* mat, double value, int dim) { cudaD_scale_diag(Gr,Bl,mat,value,dim); }
inline void cuda_scale(dim3 Gr, dim3 Bl, double *mat, double value, MatrixDim d) { cudaD_scale(Gr,Bl,mat,value,d); }
inline void cuda_apply_log(dim3 Gr, dim3 Bl, double *mat, MatrixDim d) { cudaD_apply_log(Gr,Bl,mat,d); }
inline void cuda_mul_elements(dim3 Gr, dim3 Bl, double *mat, const double *A, MatrixDim dst_d, int src_stride) {
  cudaD_mul_elements(Gr,Bl,mat,A,dst_d,src_stride);
}
inline void cuda_max(dim3 Gr, dim3 Bl, double *mat, const double *A, MatrixDim dst_d, int src_stride) {
  cudaD_max(Gr,Bl,mat,A,dst_d,src_stride);
}
inline void cuda_mul_cols_vec(dim3 Gr, dim3 Bl, double *mat, const double *scale, MatrixDim d) { cudaD_mul_cols_vec(Gr,Bl,mat,scale,d); }
inline void cuda_mul_rows_vec(dim3 Gr, dim3 Bl, double *mat, const double *scale, MatrixDim d) { cudaD_mul_rows_vec(Gr,Bl,mat,scale,d); }
inline void cuda_div_rows_vec(dim3 Gr, dim3 Bl, double *mat, const double *vec_div, MatrixDim d) { cudaD_div_rows_vec(Gr,Bl,mat,vec_div,d); }
inline void cuda_add_mat(dim3 Gr, dim3 Bl, double alpha, const double *A, double beta, double *dst, MatrixDim d) { cudaD_add_mat(Gr,Bl,alpha,A,beta,dst,d); }
inline void cuda_add_vec_to_cols(dim3 Gr, dim3 Bl, double alpha, const double *col, double beta, double *dst, MatrixDim d) { cudaD_add_vec_to_cols(Gr,Bl,alpha,col,beta,dst,d); }
inline void cuda_add_vec_to_rows(dim3 Gr, dim3 Bl, double alpha, const double *row, double beta, double *dst, MatrixDim d) { cudaD_add_vec_to_rows(Gr,Bl,alpha,row,beta,dst,d); }
 
/*
 * CuVector
 */
inline void cuda_set_bias_params(int Gr, int Bl, double* v, const double* a, double param_1, double param_2, double param_3, int* flag, int dim) { cudaD_set_bias_params(Gr,Bl,v,a,param_1,param_2,param_3,flag,dim); }
inline void cuda_copy_from_vec_df(int Gr, int Bl, double* v_out, const double* v_in, int dim) { cudaD_copy_from_vec_df(Gr,Bl,v_out,v_in,dim); }
inline void cuda_copy_from_vec_fd(int Gr, int Bl, float* v_out, const double* v_in, int dim) { cudaD_copy_from_vec_fd(Gr,Bl,v_out,v_in,dim); }
inline void cuda_vec_mul_elements(int Gr, int Bl, double* v, const double* a, int dim) { cudaD_vec_mul_elements(Gr,Bl,v,a,dim); }
inline void cuda_vec_soft_max(int Gr, int Bl, double* v, int dim) { cudaD_vec_soft_max(Gr,Bl,v,dim); }
inline void cuda_min(int Gr, int Bl, const double* v, double* value, int dim) { cudaD_min(Gr,Bl,v,value,dim); }
inline void cuda_trace_mat_mat_trans(int Gr, int Bl, const double* A, const double* B, MatrixDim dA, MatrixDim dB, double* value) { cudaD_trace_mat_mat_trans(Gr,Bl,A,B,dA,dB,value); }
inline void cuda_trace_mat_mat(int Gr, int Bl, const double* A, const double* B, MatrixDim dA, MatrixDim dB, double* value) { cudaD_trace_mat_mat(Gr,Bl,A,B,dA,dB,value); }
inline void cuda_add_diag_mat_trans(int Gr, int Bl, double alpha, double* v, const double* mat, double beta, MatrixDim dmat, int dim) { cudaD_add_diag_mat_trans(Gr,Bl,alpha,v,mat,beta,dmat,dim); }
inline void cuda_add_diag_mat(int Gr, int Bl, double alpha, double* v, const double* mat, double beta, MatrixDim dmat, int dim) { cudaD_add_diag_mat(Gr,Bl,alpha,v,mat,beta,dmat,dim); }
inline void cuda_add_vec_vec(int Gr, int Bl, double alpha, double* v, const double* x, const double* y, double beta, int dim) { cudaD_add_vec_vec(Gr,Bl,alpha,v,x,y,beta,dim); }
inline void cuda_copy_col_from_mat(int Gr, int Bl, double* v, int col, const double* mat, MatrixDim dmat, int dim) { cudaD_copy_col_from_mat(Gr,Bl,v,col,mat,dmat,dim); }
inline void cuda_copy_col_from_mat_df(int Gr, int Bl, double* v, int col, const double* mat, MatrixDim dmat, int dim) { cudaD_copy_col_from_mat_df(Gr,Bl,v,col,mat,dmat,dim); }
inline void cuda_copy_col_from_mat_fd(int Gr, int Bl, float* v, int col, const double* mat, MatrixDim dmat, int dim) { cudaD_copy_col_from_mat_fd(Gr,Bl,v,col,mat,dmat,dim); }
inline void cuda_vec_sum(int Gr, int Bl, double* v, double* value, int dim) { cudaD_vec_sum(Gr,Bl,v,value,dim); }
inline void cuda_vec_apply_floor(int Gr, int Bl, double* v, double floor_val, float* num, int dim) { cudaD_vec_apply_floor(Gr,Bl,v,floor_val,num,dim); }
inline void cuda_vec_apply_exp(int Gr, int Bl, double* v, int dim) { cudaD_vec_apply_exp(Gr,Bl,v,dim); }
inline void cuda_vec_apply_log(int Gr, int Bl, double* v, double* flag, int dim) { cudaD_vec_apply_log(Gr,Bl,v,flag,dim); }
inline void cuda_add_row_sum_mat(dim3 Gr, dim3 Bl, const double *mat, double *vec_sum, MatrixDim d) { cudaD_add_row_sum_mat(Gr,Bl,mat,vec_sum,d); }
inline void cuda_add_col_sum_mat(dim3 Gr, dim3 Bl, const double *mat, double *vec_sum, MatrixDim d) { cudaD_add_col_sum_mat(Gr,Bl,mat,vec_sum,d); }
inline void cuda_invert_elements(dim3 Gr, dim3 Bl, double *data, MatrixDim d) { cudaD_invert_elements(Gr,Bl,data,d); }

/*
 * cu::
 */
inline void cuda_soft_hinge(dim3 Gr, dim3 Bl, double *y, const double *x, MatrixDim d) { cudaD_soft_hinge(Gr,Bl,y,x,d); }
inline void cuda_sigmoid(dim3 Gr, dim3 Bl, double *y, const double *x, MatrixDim d) { cudaD_sigmoid(Gr,Bl,y,x,d); }
inline void cuda_diff_sigmoid(dim3 Gr, dim3 Bl, double *eout, const double *e, const double *y, MatrixDim d) { cudaD_diff_sigmoid(Gr,Bl,eout,e,y,d); }
inline void cuda_tanh(dim3 Gr, dim3 Bl, double *y, const double *x, MatrixDim d) { cudaD_tanh(Gr,Bl,y,x,d); }
inline void cuda_diff_tanh(dim3 Gr, dim3 Bl, double *eout, const double *e, const double *y, MatrixDim d) { cudaD_diff_tanh(Gr,Bl,eout,e,y,d); }
inline void cuda_softmax(size_t Gr, size_t Bl, double *y, const double *x, MatrixDim d) { cudaD_softmax(Gr,Bl,y,x,d); }
inline void cuda_softmax_part(dim3 Gr, dim3 Bl, const double *X, const int32_cuda *vec_ids, double* Y, MatrixDim d) { cudaD_softmax_part(Gr,Bl,X,vec_ids,Y,d); }

inline void cuda_regularize_l1(dim3 Gr, dim3 Bl, double *wei, double *grad, double l1, double lr, MatrixDim d) { cudaD_regularize_l1(Gr,Bl,wei,grad,l1,lr,d); }
inline void cuda_find_row_max_id(dim3 Gr, dim3 Bl, const double *mat, double *vec_val, int32_cuda *vec_id, int32_cuda voff, MatrixDim d) { cudaD_find_row_max_id(Gr,Bl,mat,vec_val,vec_id,voff,d); }
inline void cuda_diff_xent(dim3 Gr, dim3 Bl, const int32_cuda *vec_tgt, double *mat_net_out, double *vec_log_post, MatrixDim d) { cudaD_diff_xent(Gr,Bl,vec_tgt,mat_net_out,vec_log_post,d); }

inline void cuda_randomize(dim3 Gr, dim3 Bl, double *y, const double *x, const int32_cuda *copy_from, MatrixDim d_out, MatrixDim d_in) { cudaD_randomize(Gr,Bl,y,x,copy_from,d_out,d_in); }
inline void cuda_splice(dim3 Gr, dim3 Bl, double *y, const double *x, const int32_cuda *off, MatrixDim d_out, MatrixDim d_in) { cudaD_splice(Gr,Bl,y,x,off,d_out,d_in); }
inline void cuda_one(int Gr,int Bl,double* x,int dim) { cudaD_one(Gr,Bl,x,dim); }
inline void cuda_copy(dim3 Gr, dim3 Bl, double *y, const double *x, const int32_cuda *copy_from, MatrixDim d_out, MatrixDim d_in) { cudaD_copy(Gr,Bl,y,x,copy_from,d_out,d_in); }
inline void cuda_copy_diag(int Gr, int Bl, double* y, const double* x, int dim) { cudaD_copy_diag(Gr,Bl,y,x,dim); }
inline void cuda_copy_from_sp(int Gr, int Bl, const double* x, double* y, int d_in, MatrixDim d_out) { cudaD_copy_from_sp(Gr,Bl,x,y,d_in,d_out); }
inline void cuda_take_lower(dim3 Gr, dim3 Bl, const double* x, double* y, MatrixDim d_in, int d_out) { cudaD_take_lower(Gr,Bl,x,y,d_in,d_out); }
inline void cuda_take_upper(dim3 Gr, dim3 Bl, const double* x, double* y, MatrixDim d_in, int d_out) { cudaD_take_upper(Gr,Bl,x,y,d_in,d_out); }
inline void cuda_take_mean(dim3 Gr, dim3 Bl, const double* x, double* y, MatrixDim d_in, int d_out) { cudaD_take_mean(Gr,Bl,x,y,d_in,d_out); }


// Also include some template-friendly wrappers of cublas functions:
inline void cuda_axpy(int n, float alpha, const float *x, int incx, float *y, int incy) {
  cublasSaxpy(n, alpha, x, incx, y, incy);
}
inline void cuda_axpy(int n, double alpha, const double *x, int incx, double *y, int incy) {
  cublasDaxpy(n, alpha, x, incx, y, incy);
}
inline void cuda_scal(int n, float alpha, float *x, int incx) {
  cublasSscal(n, alpha, x, incx);
}
inline void cuda_scal(int n, double alpha, double *x, int incx) {
  cublasDscal(n, alpha, x, incx);
}


} // namespace kaldi



#endif // HAVE_CUDA

#endif
