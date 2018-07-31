//===- Types.h - MLIR Type Classes ------------------------------*- C++ -*-===//
//
// Copyright 2019 The MLIR Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// =============================================================================

#ifndef MLIR_IR_TYPES_H
#define MLIR_IR_TYPES_H

#include "mlir/Support/LLVM.h"
#include "llvm/ADT/ArrayRef.h"

namespace mlir {
class AffineMap;
class MLIRContext;
class IntegerType;
class FloatType;
class OtherType;

/// Instances of the Type class are immutable, uniqued, immortal, and owned by
/// MLIRContext.  As such, they are passed around by raw non-const pointer.
///
class Type {
public:
  /// Integer identifier for all the concrete type kinds.
  enum class Kind {
    // Target pointer sized integer, used (e.g.) in affine mappings.
    AffineInt,

    // TensorFlow types.
    TFControl,

    /// These are marker for the first and last 'other' type.
    FIRST_OTHER_TYPE = AffineInt,
    LAST_OTHER_TYPE = TFControl,

    // Floating point.
    BF16,
    F16,
    F32,
    F64,
    FIRST_FLOATING_POINT_TYPE = BF16,
    LAST_FLOATING_POINT_TYPE = F64,

    // Derived types.
    Integer,
    Function,
    Vector,
    RankedTensor,
    UnrankedTensor,
    MemRef,
  };

  /// Return the classification for this type.
  Kind getKind() const {
    return kind;
  }

  /// Return the LLVMContext in which this type was uniqued.
  MLIRContext *getContext() const { return context; }

  // Convenience predicates.  This is only for 'other' and floating point types,
  // derived types should use isa/dyn_cast.
  bool isAffineInt() const { return getKind() == Kind::AffineInt; }
  bool isTFControl() const { return getKind() == Kind::TFControl; }
  bool isBF16() const { return getKind() == Kind::BF16; }
  bool isF16() const { return getKind() == Kind::F16; }
  bool isF32() const { return getKind() == Kind::F32; }
  bool isF64() const { return getKind() == Kind::F64; }

  // Convenience factories.
  static IntegerType *getInteger(unsigned width, MLIRContext *ctx);
  static FloatType *getBF16(MLIRContext *ctx);
  static FloatType *getF16(MLIRContext *ctx);
  static FloatType *getF32(MLIRContext *ctx);
  static FloatType *getF64(MLIRContext *ctx);
  static OtherType *getAffineInt(MLIRContext *ctx);
  static OtherType *getTFControl(MLIRContext *ctx);

  /// Print the current type.
  void print(raw_ostream &os) const;
  void dump() const;

protected:
  explicit Type(Kind kind, MLIRContext *context)
      : context(context), kind(kind), subclassData(0) {}
  explicit Type(Kind kind, MLIRContext *context, unsigned subClassData)
      : Type(kind, context) {
    setSubclassData(subClassData);
  }

  ~Type() {}

  unsigned getSubclassData() const { return subclassData; }

  void setSubclassData(unsigned val) {
    subclassData = val;
    // Ensure we don't have any accidental truncation.
    assert(getSubclassData() == val && "Subclass data too large for field");
  }

private:
  Type(const Type&) = delete;
  void operator=(const Type&) = delete;
  /// This refers to the MLIRContext in which this type was uniqued.
  MLIRContext *const context;

  /// Classification of the subclass, used for type checking.
  Kind kind : 8;

  // Space for subclasses to store data.
  unsigned subclassData : 24;
};

inline raw_ostream &operator<<(raw_ostream &os, const Type &type) {
  type.print(os);
  return os;
}

/// Integer types can have arbitrary bitwidth up to a large fixed limit of 4096.
class IntegerType : public Type {
public:
  static IntegerType *get(unsigned width, MLIRContext *context);

  /// Return the bitwidth of this integer type.
  unsigned getWidth() const {
    return width;
  }

  /// Methods for support type inquiry through isa, cast, and dyn_cast.
  static bool classof(const Type *type) {
    return type->getKind() == Kind::Integer;
  }
private:
  unsigned width;
  IntegerType(unsigned width, MLIRContext *context);
  ~IntegerType() = delete;
};

inline IntegerType *Type::getInteger(unsigned width, MLIRContext *ctx) {
  return IntegerType::get(width, ctx);
}

class FloatType : public Type {
public:
  /// Methods for support type inquiry through isa, cast, and dyn_cast.
  static bool classof(const Type *type) {
    return type->getKind() >= Kind::FIRST_FLOATING_POINT_TYPE &&
           type->getKind() <= Kind::LAST_FLOATING_POINT_TYPE;
  }

  static FloatType *get(Kind kind, MLIRContext *context);

private:
  FloatType(Kind kind, MLIRContext *context);
  ~FloatType() = delete;
};

inline FloatType *Type::getBF16(MLIRContext *ctx) {
  return FloatType::get(Kind::BF16, ctx);
}
inline FloatType *Type::getF16(MLIRContext *ctx) {
  return FloatType::get(Kind::F16, ctx);
}
inline FloatType *Type::getF32(MLIRContext *ctx) {
  return FloatType::get(Kind::F32, ctx);
}
inline FloatType *Type::getF64(MLIRContext *ctx) {
  return FloatType::get(Kind::F64, ctx);
}

/// This is a type for the random collection of special base types.
class OtherType : public Type {
public:
  /// Methods for support type inquiry through isa, cast, and dyn_cast.
  static bool classof(const Type *type) {
    return type->getKind() >= Kind::FIRST_OTHER_TYPE &&
           type->getKind() <= Kind::LAST_OTHER_TYPE;
  }
  static OtherType *get(Kind kind, MLIRContext *context);

private:
  OtherType(Kind kind, MLIRContext *context);
  ~OtherType() = delete;
};

inline OtherType *Type::getAffineInt(MLIRContext *ctx) {
  return OtherType::get(Kind::AffineInt, ctx);
}
inline OtherType *Type::getTFControl(MLIRContext *ctx) {
  return OtherType::get(Kind::TFControl, ctx);
}

/// Function types map from a list of inputs to a list of results.
class FunctionType : public Type {
public:
  static FunctionType *get(ArrayRef<Type*> inputs, ArrayRef<Type*> results,
                           MLIRContext *context);

  ArrayRef<Type*> getInputs() const {
    return ArrayRef<Type*>(inputsAndResults, getSubclassData());
  }

  ArrayRef<Type*> getResults() const {
    return ArrayRef<Type*>(inputsAndResults+getSubclassData(), numResults);
  }

  /// Methods for support type inquiry through isa, cast, and dyn_cast.
  static bool classof(const Type *type) {
    return type->getKind() == Kind::Function;
  }

private:
  unsigned numResults;
  Type *const *inputsAndResults;

  FunctionType(Type *const *inputsAndResults, unsigned numInputs,
               unsigned numResults, MLIRContext *context);
  ~FunctionType() = delete;
};

/// Vector types represent multi-dimensional SIMD vectors, and have a fixed
/// known constant shape with one or more dimension.
class VectorType : public Type {
public:
  static VectorType *get(ArrayRef<unsigned> shape, Type *elementType);

  ArrayRef<unsigned> getShape() const {
    return ArrayRef<unsigned>(shapeElements, getSubclassData());
  }

  Type *getElementType() const { return elementType; }

  /// Methods for support type inquiry through isa, cast, and dyn_cast.
  static bool classof(const Type *type) {
    return type->getKind() == Kind::Vector;
  }

private:
  const unsigned *shapeElements;
  Type *elementType;

  VectorType(ArrayRef<unsigned> shape, Type *elementType, MLIRContext *context);
  ~VectorType() = delete;
};

/// Tensor types represent multi-dimensional arrays, and have two variants:
/// RankedTensorType and UnrankedTensorType.
class TensorType : public Type {
public:
  Type *getElementType() const { return elementType; }

  /// Methods for support type inquiry through isa, cast, and dyn_cast.
  static bool classof(const Type *type) {
    return type->getKind() == Kind::RankedTensor ||
           type->getKind() == Kind::UnrankedTensor;
  }

protected:
  /// The type of each scalar element of the tensor.
  Type *elementType;

  TensorType(Kind kind, Type *elementType, MLIRContext *context);
  ~TensorType() {}
};

/// Ranked tensor types represent multi-dimensional arrays that have a shape
/// with a fixed number of dimensions. Each shape element can be a positive
/// integer or unknown (represented by any negative integer).
class RankedTensorType : public TensorType {
public:
  static RankedTensorType *get(ArrayRef<int> shape,
                               Type *elementType);

  ArrayRef<int> getShape() const {
    return ArrayRef<int>(shapeElements, getSubclassData());
  }

  unsigned getRank() const { return getShape().size(); }

  static bool classof(const Type *type) {
    return type->getKind() == Kind::RankedTensor;
  }

private:
  const int *shapeElements;

  RankedTensorType(ArrayRef<int> shape, Type *elementType,
                   MLIRContext *context);
  ~RankedTensorType() = delete;
};

/// Unranked tensor types represent multi-dimensional arrays that have an
/// unknown shape.
class UnrankedTensorType : public TensorType {
public:
  static UnrankedTensorType *get(Type *elementType);

  static bool classof(const Type *type) {
    return type->getKind() == Kind::UnrankedTensor;
  }

private:
  UnrankedTensorType(Type *elementType, MLIRContext *context);
  ~UnrankedTensorType() = delete;
};

/// MemRef types represent a region of memory that have a shape with a fixed
/// number of dimensions. Each shape element can be a positive integer or
/// unknown (represented by any negative integer). MemRef types also have an
/// affine map composition, represented as an array AffineMap pointers.
// TODO: Use -1 for unknown dimensions (rather than arbitrary negative numbers).
class MemRefType : public Type {
public:
  /// Get or create a new MemRefType based on shape, element type, affine
  /// map composition, and memory space.
  static MemRefType *get(ArrayRef<int> shape, Type *elementType,
                         ArrayRef<AffineMap*> affineMapComposition,
                         unsigned memorySpace);

  /// Returns an array of memref shape dimension sizes.
  ArrayRef<int> getShape() const {
    return ArrayRef<int>(shapeElements, getSubclassData());
  }

  unsigned getRank() const { return getShape().size(); }

  /// Returns the elemental type for this memref shape.
  Type *getElementType() const { return elementType; }

  /// Returns an array of affine map pointers representing the memref affine
  /// map composition.
  ArrayRef<AffineMap*> getAffineMaps() const;

  /// Returns the memory space in which data referred to by this memref resides.
  unsigned getMemorySpace() const { return memorySpace; }

  /// Returns the number of dimensions with dynamic size.
  unsigned getNumDynamicDims() const;

  static bool classof(const Type *type) {
    return type->getKind() == Kind::MemRef;
  }

private:
  /// The type of each scalar element of the memref.
  Type *elementType;
  /// An array of integers which stores the shape dimension sizes.
  const int *shapeElements;
  /// The number of affine maps in the 'affineMapList' array.
  const unsigned numAffineMaps;
  /// List of affine maps in the memref's layout/index map composition.
  AffineMap *const *const affineMapList;
  /// Memory space in which data referenced by memref resides.
  const unsigned memorySpace;

  MemRefType(ArrayRef<int> shape, Type *elementType,
             ArrayRef<AffineMap*> affineMapList, unsigned memorySpace,
             MLIRContext *context);
  ~MemRefType() = delete;
};

} // end namespace mlir

#endif  // MLIR_IR_TYPES_H
