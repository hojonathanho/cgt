#include "stdlib.h"
#include "assert.h"
#include "memory.h"
#include "stdio.h"
#include "cgt_common.h"

#ifdef CGT_ENABLE_CUDA
#include "cgt_cuda.h"
#endif

// ================================================================
// Object alloc/dealloc 
// ================================================================

cgtArray::cgtArray(size_t ndim, const size_t* shape, cgtDtype dtype, cgtDevtype devtype)
    : cgtObject(ObjectKind::ArrayKind),
      ndim_(ndim),
      dtype_(dtype),
      devtype_(devtype),
      ownsdata_(true) {
  shape_ = new size_t[ndim];
  memcpy(const_cast<size_t*>(shape_), shape, ndim * sizeof(size_t));
  data_ = cgt_alloc(devtype_, nbytes());
}

cgtArray::cgtArray(size_t ndim, const size_t* shape, cgtDtype dtype, cgtDevtype devtype, void* fromdata, bool copy)
    : cgtObject(ObjectKind::ArrayKind),
      ndim_(ndim),
      shape_(shape),
      dtype_(dtype),
      devtype_(devtype),
      ownsdata_(copy) {
  cgt_assert(fromdata != NULL);
  shape_ = new size_t[ndim];
  memcpy(const_cast<size_t*>(shape_), shape, ndim * sizeof(size_t));
  if (copy) {
    data_ = cgt_alloc(devtype, nbytes());
    cgt_memcpy(devtype, cgtCPU, data_, fromdata, nbytes());
  } else {
    data_ = fromdata;
  }
}

void cgtArray::print() {
  printf("Array{shape=(");
  if (ndim_ > 0) printf("%zu",shape_[0]);
  for (int i=1; i < ndim_; ++i) {
    printf(", %zu", shape_[i]);
  }
  printf("), dtype=%i}", dtype_);
}

cgtArray::~cgtArray() {
  delete[] shape_;
  if (ownsdata_) cgt_free(devtype_, data_);
}

cgtTuple::cgtTuple(size_t len)
    : cgtObject(ObjectKind::TupleKind), len(len) {
  members = new IRC<cgtObject>[len];
}

cgtTuple::~cgtTuple() {
  delete[] members;
}


bool cgt_arrays_equal(const cgtArray* a1, const cgtArray* a2) {
  if (a1->ndim() != a2->ndim()) return false;

  for (int i = 0; i < a1->ndim(); ++i) {
    if (a1->shape()[i] != a2->shape()[i]) return false;
  }

  if (a1->dtype() != a2->dtype()) return false;

  if (a1->devtype() != a2->devtype()) return false;

  assert(a1->nbytes() == a2->nbytes());
  const uint8_t *a1data = reinterpret_cast<const uint8_t*>(a1->data());
  const uint8_t *a2data = reinterpret_cast<const uint8_t*>(a2->data());
  for (int i = 0; i < a1->nbytes(); ++i) {
    if (a1data[i] != a2data[i]) return false;
  }

  return true;
}


// ================================================================
// Copying
// ================================================================

void cgt_copy_object(cgtObject* to, cgtObject* from) {
  cgt_assert(to->kind() == from->kind());
  if (to->kind() == cgtObject::ArrayKind) {    
    cgt_copy_array(static_cast<cgtArray*>(to), static_cast<cgtArray*>(from));
  }
  else if (to->kind() == cgtObject::TupleKind) {
    cgt_copy_tuple(static_cast<cgtTuple*>(to), static_cast<cgtTuple*>(from));
  }
  else cgt_assert(0 && "unreachable");
}

void cgt_copy_array(cgtArray* to, cgtArray* from) {
  cgt_assert(from->size() == to->size() && from->dtype() == to->dtype()) ;
  cgt_memcpy(to->devtype(), from->devtype(), to->data(), from->data(), from->nbytes());
}

void cgt_copy_tuple(cgtTuple* to, cgtTuple* from) {
  for (int i=0; i < to->size(); ++i) cgt_copy_object(to->getitem(i), from->getitem(i));
}



// ================================================================
// Error handling 
// ================================================================

void cgt_abort() {
  abort();
}

cgtStatus cgtGlobalStatus = cgtStatusOK;
char cgtGlobalErrorMsg[1000];


// ================================================================
// Memory management 
// ================================================================

void *cgt_alloc(cgtDevtype devtype, size_t size) {
  if (devtype == cgtCPU) {
    return malloc(size);
  }
  else {
#ifdef CGT_ENABLE_CUDA
    void* out;
    CUDA_CHECK(cudaMalloc(&out, size));
    return out;
#else
    cgt_assert(0 && "CUDA disabled");
#endif
  }
}

void cgt_free(cgtDevtype devtype, void *ptr) {
  if (devtype == cgtCPU) {
    free(ptr);
  }
  else {
#ifdef CGT_ENABLE_CUDA
    CUDA_CHECK(cudaFree(ptr));
#else
    cgt_assert(0 && "CUDA disabled");
#endif
  }
}

void cgt_memcpy(cgtDevtype dest_type, cgtDevtype src_type, void *dest_ptr, const void *src_ptr, size_t nbytes) {
  if (src_type == cgtCPU && dest_type == cgtCPU) {
    memcpy(dest_ptr, src_ptr, nbytes);
  } else {
#ifdef CGT_ENABLE_CUDA
    enum cudaMemcpyKind kind;
    if       (src_type == cgtCPU && dest_type == cgtGPU) kind = cudaMemcpyHostToDevice;
    else if  (src_type == cgtGPU && dest_type == cgtCPU) kind = cudaMemcpyDeviceToHost;
    else if  (src_type == cgtGPU && dest_type == cgtGPU) kind = cudaMemcpyDeviceToDevice;
    else cgt_assert(0 && "invalid src/dest types");
    CUDA_CHECK(cudaMemcpy(dest_ptr, src_ptr, nbytes, kind));
#else
    cgt_assert(0 && "CUDA disabled");
#endif
  }
}

