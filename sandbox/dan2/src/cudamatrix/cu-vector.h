// cudamatrix/cu-vector.h

// Copyright 2009-2012  Karel Vesely
//                      Johns Hopkins University (author: Daniel Povey)
//                      Lucas Ondel

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



#ifndef KALDI_CUDAMATRIX_CU_VECTOR_H_
#define KALDI_CUDAMATRIX_CU_VECTOR_H_

#include "matrix/kaldi-vector.h"
#include "cudamatrix/cu-common.h"
#include "cudamatrix/cu-value.h"
#include "cudamatrix/cu-math.h"

namespace kaldi {

template<typename Real> class CuMatrixBase;

template<typename Real>
Real VecVec(const CuVectorBase<Real> &v1, const CuVectorBase<Real> &v2);


/**
 * Vector for CUDA computing
 */
template<typename Real>
class CuVectorBase {
 public:
  friend class CuVectorBase<float>;
  friend class CuVectorBase<double>;
  friend class CuMatrixBase<Real>;
  friend class CuPackedMatrix<Real>;
  friend class CuSpMatrix<Real>;
  friend class CuTpMatrix<Real>;

  template <class OtherReal>
  friend OtherReal VecVec(const CuVectorBase<OtherReal> &v1,
                          const CuVectorBase<OtherReal> &v2);
  friend void cu::Splice<Real>(const CuMatrix<Real> &src,
                               const CuStlVector<int32> &frame_offsets,
                               CuMatrix<Real> *tgt);
  friend class CuRand<Real>;
  
  /// Dimensions
  MatrixIndexT Dim() const { return dim_;  }   

  /// Returns a pointer to the start of the vector's data.
  inline Real* Data() { return data_; }
  /// Returns a pointer to the start of the vector's data (const).
  inline const Real* Data() const { return data_; }
  
  /// Copy functions; these will crash if the dimension
  /// do not match.  The operator = in class CuVector will
  /// also change the sizes for you.
  void CopyFromVec(const CuVectorBase<Real> &src);

  void CopyFromVec(const VectorBase<Real> &src);

  template<typename OtherReal>
  void CopyFromVec(const CuVectorBase<OtherReal> &M);
  
  void CopyToVec(VectorBase<Real> *dst) const;
  void CopyRowsFromMat(const CuMatrixBase<Real> &M);
  /// Math operations
  void SetZero();
  void Set(Real value);
  void Add(Real value);
  void Scale(Real value);
  
  void AddVec(Real alpha, const CuVectorBase<Real> &vec, Real beta = 0.0);

  /// Sum the rows of the matrix, add to vector
  void AddRowSumMat(Real alpha, const CuMatrixBase<Real> &mat, Real beta = 1.0);
  /// Sum the columns of the matrix, add to vector
  void AddColSumMat(Real alpha, const CuMatrixBase<Real> &mat, Real beta = 1.0); 

  /// Add triangular matrix times vector: this <-- beta*this + alpha*M*v.
  /// Works even if rv == *this.
  void AddTpVec(const Real alpha, const CuTpMatrix<Real>&M,
                const MatrixTransposeType trans, const CuVectorBase<Real> &v,
                const Real beta);  // **beta previously defaulted to 0.0**
  
  /// Multiplies this vector by lower-triangular marix:  *this <-- *this *M
  void MulTp(const CuTpMatrix<Real> &M, const MatrixTransposeType trans);
  
  void InvertElements(); 

  void ApplySoftMax();
  void ApplyExp();
  void ApplyLog();
  MatrixIndexT ApplyFloor(Real floor_val);
  Real Sum() const;
  void SetRandn();
  
  CuSubVector<Real> Range(const MatrixIndexT o, const MatrixIndexT l) {
    return CuSubVector<Real>(*this, o, l);
  }

  const CuSubVector<Real> Range(const MatrixIndexT o,
                                const MatrixIndexT l) const {
    return CuSubVector<Real>(*this, o, l);
  }

  void CopyColFromMat(const CuMatrixBase<Real> &mat, MatrixIndexT col);

  template<typename OtherReal>
  void CopyColFromMat(const CuMatrixBase<OtherReal> &mat, MatrixIndexT col);

  void AddMatVec(const Real alpha, const CuMatrixBase<Real> &M,
                 MatrixTransposeType trans, const CuVectorBase<Real> &v,
                 const Real beta);
  void AddVecVec(Real alpha, const CuVectorBase<Real> &v,
                 const CuVectorBase<Real> &r, Real beta);

  void AddDiagMat2(Real alpha, const CuMatrixBase<Real> &M,
                   MatrixTransposeType trans, Real beta);

  inline CuValue<Real> operator() (MatrixIndexT i) {
    KALDI_PARANOID_ASSERT(static_cast<UnsignedMatrixIndexT>(i) <
                          static_cast<UnsignedMatrixIndexT>(dim_));
    return CuValue<Real>(data_ + i);
  }

  inline Real operator() (MatrixIndexT i) const {
    KALDI_PARANOID_ASSERT(static_cast<UnsignedMatrixIndexT>(i) <
                          static_cast<UnsignedMatrixIndexT>(dim_));
    return CuValue<Real>(data_ + i); // will be casted to Real.
  }    

  Real Min() const;
  void MulElements(const CuVectorBase<Real> &v);
 protected:

  // The following two functions should only be called if we did not compile
  // with CUDA or could not get a CUDA card; in that case the contents are
  // interpreted the same as a regular vector.
  inline const VectorBase<Real> &Vec() const {
    return *(reinterpret_cast<const VectorBase<Real>* >(this));
  }
  inline VectorBase<Real> &Vec() {
    return *(reinterpret_cast<VectorBase<Real>* >(this));
  }
  
  /// Default constructor: make it protected so the user cannot
  /// instantiate this class.
  CuVectorBase<Real>(): data_(NULL), dim_(0) { }
  
  Real *data_; ///< GPU data pointer (or regular data pointer
               ///< if CUDA is not compiled in or we have no GPU).
  MatrixIndexT dim_; ///< dimension of the vector
 private:
  KALDI_DISALLOW_COPY_AND_ASSIGN(CuVectorBase);
};

template<class Real>
class CuVector: public CuVectorBase<Real> {
  friend class CuVectorBase<float>;
  friend class CuVectorBase<double>;
  friend class CuMatrixBase<Real>;
  friend class CuPackedMatrix<Real>;
  friend class CuSpMatrix<Real>;
  friend class CuTpMatrix<Real>;
  
 public:
  CuVector() { }
  CuVector(MatrixIndexT dim, MatrixResizeType t = kSetZero) { Resize(dim, t); }
  
  CuVector(const CuVectorBase<Real> &v);
  CuVector(const VectorBase<Real> &v);  
  explicit CuVector(const CuVector<Real> &v) : CuVectorBase<Real>() {
    Resize(v.Dim(), kUndefined);
    this->CopyFromVec(v);
  }

  template<typename OtherReal>
  explicit CuVector(const CuVectorBase<OtherReal> &v) : CuVectorBase<Real>() {
    Resize(v.Dim(), kUndefined);
    this->CopyFromVec(v);
  }

  template<typename OtherReal>
  explicit CuVector(const VectorBase<OtherReal> &v) : CuVectorBase<Real>() {
    Resize(v.Dim(), kUndefined);
    this->CopyFromVec(Vector<Real>(v));
  }

  /// Allocate the memory
  void Resize(MatrixIndexT dim, MatrixResizeType t = kSetZero);
  
  ~CuVector() { Destroy(); }

  CuVector<Real> &operator = (const CuVectorBase<Real> &other) {
    Resize(other.Dim(), kUndefined);
    this->CopyFromVec(other);
    return *this;
  }

  CuVector<Real> &operator = (const CuVector<Real> &other) {
    Resize(other.Dim(), kUndefined);
    this->CopyFromVec(other);
    return *this;
  }
  CuVector<Real> &operator = (const VectorBase<Real> &other) {
    Resize(other.Dim());
    this->CopyFromVec(other);
    return *this;
  }
      

  /// I/O 
  void Read(std::istream &is, bool binary);
  void Write(std::ostream &is, bool binary) const;

  void Swap(Vector<Real> *vec);

 private:
  void Destroy();
};

// We'll fill out the following class if it's needed.
template<class Real>
class CuSubVector: public CuVectorBase<Real> {
 public:  
  CuSubVector(const CuVectorBase<Real> &t, const MatrixIndexT origin,
            const MatrixIndexT length) : CuVectorBase<Real>() {
    KALDI_ASSERT(static_cast<UnsignedMatrixIndexT>(origin)+
                 static_cast<UnsignedMatrixIndexT>(length) <=
                 static_cast<UnsignedMatrixIndexT>(t.Dim()));
    CuVectorBase<Real>::data_ = const_cast<Real*>(t.Data()+origin);
    CuVectorBase<Real>::dim_ = length;
  }
  /// Copy constructor
  /// this constructor needed for Range() to work in base class.
  CuSubVector(const CuSubVector &other) : CuVectorBase<Real> () {
    CuVectorBase<Real>::data_ = other.data_;
    CuVectorBase<Real>::dim_ = other.dim_;
  }

  CuSubVector(Real* data, MatrixIndexT length) : CuVectorBase<Real> () {
    CuVectorBase<Real>::data_ = data;
    CuVectorBase<Real>::dim_ = length;
  }
    
  /// This operation does not preserve const-ness, so be careful.
  CuSubVector(const CuMatrixBase<Real> &matrix, MatrixIndexT row) {
    CuVectorBase<Real>::data_ = const_cast<Real*>(matrix.RowData(row));
    CuVectorBase<Real>::dim_ = matrix.NumCols();
  }
  

};

/// I/O
template<typename Real>
std::ostream &operator << (std::ostream &out, const CuVectorBase<Real> &vec);
 
  
} // namespace


#include "cu-vector-inl.h"

#endif
