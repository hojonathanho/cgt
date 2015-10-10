#include "serialization.h"

#include "cereal/cereal.hpp"
#include "cereal/archives/binary.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"
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

// Instructions
template<class Archive>
void save(Archive& ar, const Instruction& instr) {
  ar(instr.kind(), instr.repr(), instr.get_readlocs(), instr.get_writeloc());
  switch (instr.kind()) {
  case LoadArgumentKind:
    ar(((const LoadArgument&) instr).get_ind());
    break;
  case AllocKind:
    ar(((const Alloc&) instr).get_dtype());
    break;
  case BuildTupKind:
    break;
  case ReturnByRefKind:
  // XXX
    break;
  case ReturnByValKind:
  // XXX
    break;
  default:
    cgt_assert(false);
    break;
  }
}

template<class Archive>
static Instruction* custom_load_and_construct_instruction(Archive& ar) {
  InstructionKind kind;
  std::string repr;
  vector<MemLocation> readlocs;
  MemLocation writeloc;
  //ar(kind, repr, readlocs, writeloc);
  ar(kind);
  ar(repr);
  ar(readlocs);
  ar(writeloc);

  switch (kind) {
  case LoadArgumentKind:
    int ind; ar(ind);
    return new LoadArgument(repr, ind, writeloc);
    break;
  case AllocKind:
    cgtDtype dtype; ar(dtype);
    return new Alloc(repr, dtype, readlocs, writeloc);
    break;
  case BuildTupKind:
    return new BuildTup(repr, readlocs, writeloc);
    break;
  case ReturnByRefKind:
    break;
  case ReturnByValKind:
    break;
  }
  cgt_assert(false);
  return nullptr;
}


// ExecutionGraph
template<class Archive>
void save(Archive& ar, const ExecutionGraph& eg) {
  ar(eg.n_instrs(), eg.n_args(), eg.n_locs());
  for (int i = 0; i < eg.n_instrs(); ++i) {
    ar(*eg.instrs()[i]);
  }
}

template<> struct LoadAndConstruct<ExecutionGraph> { template<class Archive>
static void load_and_construct(Archive& ar, cereal::construct<ExecutionGraph>& construct) {
  size_t n_instrs, n_args, n_locs;
  ar(n_instrs, n_args, n_locs);

  std::vector<Instruction*> instrs(n_instrs);
  for (int i = 0; i < n_instrs; ++i) {
    instrs[i] = custom_load_and_construct_instruction(ar);
  }

  construct(instrs, n_args, n_locs);
}};

} // namespace cereal


// Exposed (de)serialization functions
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

std::string serialize(const ExecutionGraph& eg) {
  std::ostringstream oss;
  {
    cereal::BinaryOutputArchive ar(oss);
    std::unique_ptr<const ExecutionGraph> peg(&eg);
    ar(peg);
    peg.release();
  }
  return oss.str();
}
ExecutionGraph* deserializeExecutionGraph(const std::string& s) {
  std::istringstream iss(s);
  std::unique_ptr<ExecutionGraph> peg;
  {
    cereal::BinaryInputArchive ar(iss);
    ar(peg);
  }
  return peg.release();
}

} // namespace cgt
