#include "serialization.h"

#include "cereal/cereal.hpp"
#include "cereal/archives/binary.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/types/string.hpp"
#include "cereal/access.hpp" // For LoadAndConstruct

using namespace cgt;

namespace cereal {

// cgtArray
template<class Archive>
void save(Archive& ar, const cgtArray& array) {
  ar(array.ndim());
  ar(cereal::binary_data(array.shape(), array.ndim()*sizeof(size_t)));
  ar(array.dtype());
  ar(array.devtype());
  ar(cereal::binary_data(array.data(), array.nbytes()));
}

template<> struct LoadAndConstruct<cgtArray> { template<class Archive>
static void load_and_construct(Archive& ar, cereal::construct<cgtArray>& construct) {
  size_t ndim; ar(ndim);

  size_t* shape = new size_t[ndim];
  ar(cereal::binary_data(shape, ndim*sizeof(size_t)));

  cgtDtype dtype; ar(dtype);
  cgtDevtype devtype; ar(devtype);

  size_t size = 1; for (int i = 0; i < ndim; ++i) { size *= shape[i]; }
  size_t nbytes = size * cgt_itemsize(dtype);
  std::vector<char> datavec(nbytes);
  ar(cereal::binary_data(&datavec[0], nbytes));

  construct(
    ndim,
    shape,
    dtype,
    devtype,
    &datavec[0],
    true /* copy */);
}};

// MemLocation
template<class Archive>
void save(Archive& ar, const MemLocation& loc) {
  ar(loc.index(), loc.devtype());
}

template<> struct LoadAndConstruct<MemLocation> { template<class Archive>
static void load_and_construct(Archive& ar, cereal::construct<MemLocation>& construct) {
  size_t index;
  cgtDevtype devtype;
  ar(index, devtype);
  construct(index, devtype);
}};


} // namespace cereal

namespace cgt {

std::string serialize(cgtArray* a) {
  std::unique_ptr<cgtArray> pa(a);
  std::ostringstream oss;
  {
    cereal::BinaryOutputArchive ar(oss);
    ar(pa);
  }
  pa.release();
  return oss.str();
}

cgtArray* deserializeArray(const std::string& s) {
  std::istringstream iss(s);
  std::unique_ptr<cgtArray> pa;
  {
    cereal::BinaryInputArchive ar(iss);
    ar(pa);
  }
  return pa.release();
}


std::string serialize(const MemLocation& loc) {
  std::ostringstream oss;
  {
    cereal::BinaryOutputArchive ar(oss);
    std::unique_ptr<const MemLocation> ploc(&loc);
    ar(ploc);
    ploc.release();
  }
  return oss.str();
}
MemLocation deserializeMemLocation(const std::string& s) {
  std::istringstream iss(s);
  MemLocation out_loc;
  {
    cereal::BinaryInputArchive ar(iss);
    std::unique_ptr<MemLocation> ploc;
    ar(ploc);
    out_loc = *ploc;
  }
  return out_loc;
}



} // namespace cgt
