#include "stdafx.h"

#ifdef CXX_GENERATOR
#include "cxx_core.h"
#else  // CXX_GENERATOR
#include "c_core.h"
#endif  // CXX_GENERATOR

#include "sparc.h"
#include "gencode.h"

struct gencode::table : std::map<COMPILER::tac::id_t, void (*)(const COMPILER::tac*)> {
  table()
  {
    using namespace COMPILER;
    (*this)[tac::ASSIGN] = assign;
    (*this)[tac::ADD] = add;
    (*this)[tac::SUB] = sub;
    (*this)[tac::MUL] = mul;
    (*this)[tac::DIV] = div;
    (*this)[tac::MOD] = mod;
    (*this)[tac::LSH] = lsh;
    (*this)[tac::RSH] = rsh;
    (*this)[tac::AND] = _and;
    (*this)[tac::XOR] = _xor;
    (*this)[tac::OR] = _or;
    (*this)[tac::UMINUS] = uminus;
    (*this)[tac::TILDE] = tilde;
    (*this)[tac::CAST] = cast;
    (*this)[tac::ADDR] = addr;
    (*this)[tac::INVLADDR] = invladdr;
    (*this)[tac::INVRADDR] = invraddr;
    (*this)[tac::LOFF] = loff;
    (*this)[tac::ROFF] = roff;
    (*this)[tac::PARAM] = param;
    (*this)[tac::CALL] = call;
    (*this)[tac::RETURN] = _return;
    (*this)[tac::GOTO] = _goto;
    (*this)[tac::TO] = to;
    (*this)[tac::ALLOCA] = _alloca_;
    (*this)[tac::ASM] = asm_;
    (*this)[tac::VASTART] = _va_start;
    (*this)[tac::VAARG] = _va_arg;
    (*this)[tac::VAEND] = _va_end;
  }
};

gencode::table gencode::m_table;

inline bool cmp_id(const COMPILER::tac* tac, COMPILER::tac::id_t id)
{
  return tac->m_id == id;
}

address* getaddr(COMPILER::var* entry)
{
  using namespace std;
  using namespace COMPILER;
  map<const var*, address*>::const_iterator p = address_descriptor.find(entry);
  assert(p != address_descriptor.end());
  return p->second;
}

// assumption
// %o0 : destination address is set
// %o1 : source address is set
void _copy(int size)
{
  if (size <= 4095)
    out << '\t' << "mov" << '\t' << size << ", %o2" << '\n';
  else {
    out << '\t' << "sethi" << '\t' << "%hi(" << size << "), %o2" << '\n';
    out << '\t' << "or" << '\t' << "%o2, %lo(" << size << "), %o2" << '\n';
  }
  out << '\t' << "call" << '\t' << "memcpy" << '\n';
  out << '\t' << "nop" << '\n';
}

void copy(address* dst, address* src, int size)
{
  if (dst && src) {
    dst->get("%o0");
    src->get("%o1");
    return _copy(size);
  }

  if (dst) {
    dst->get("%o0");
    return _copy(size);
  }

  if (src) {
    src->get("%o1");
    return _copy(size);
  }

  _copy(size);
}

void copy_record_param(const COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  assert(cmp_id(tac, tac::PARAM));
  var* y = tac->y;
  const type* T = y->m_type;
  int size = T->size();
  if (T->scalar() && size <= 8)
    return;
  map<var*, ::stack*>::const_iterator p = record_param.find(y);
  assert(p != record_param.end());
  ::stack* dst = p->second;
  address* src = getaddr(y);
  copy(dst, src, size);
}

void gencode::operator()(const COMPILER::tac* ptr)
{
  using namespace std;
  using namespace COMPILER;
  
  ++m_counter;
  if ( debug_flag ){
    out << '\t' << "/* ";
    output3ac(out, ptr);
    out << " */" << '\n';
  }

  if ( cmp_id(ptr, tac::PARAM) && !m_record_param ){
    vector<tac*>::const_iterator p =
      find_if(m_v3ac.begin() + m_counter, m_v3ac.end(), bind2nd(ptr_fun(cmp_id), tac::CALL));
    for_each(m_v3ac.begin() + m_counter, p, copy_record_param);
    m_record_param = true;
  }
  else if ( cmp_id(ptr, tac::CALL) )
    m_record_param = false;

  m_table[ptr->m_id](ptr);
}

void gencode::assign(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  T->scalar() ? assign_scalar(tac) : assign_aggregate(tac);
}

void gencode::assign_scalar(const COMPILER::tac* tac)
{
  address* y = getaddr(tac->y);
  address* x = getaddr(tac->x);
  y->load("%i0");
  x->store("%i0");
}

void gencode::assign_aggregate(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  x->get("%o0");
  y->get("%o1");
  const type* T = tac->x->m_type;
  int size = T->size();
  copy(x, y, size);
}

void gencode::add(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  if ( T->real() )
    binop_real(tac,"fadd");
  else if ( T->size() > 4 )
    binop_longlong(tac,"addx","addcc");
  else
    binop_notlonglong(tac,"add");
}

void gencode::sub(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  if (T->real())
        binop_real(tac,"fsub");
  else if ( T->size() > 4 )
    binop_longlong(tac,"subx","subcc");
  else
    binop_notlonglong(tac,"sub");
}

void gencode::mul(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  if ( T->real() )
    binop_real(tac,"fmul");
  else if ( T->size() > 4 )
    runtime_longlong(tac,"__mul64");
  else
    runtime_notlonglong(tac,".umul");
}

void gencode::div(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  if ( T->real() )
    binop_real(tac,"fdiv");
  else if ( T->size() > 4 )
    runtime_longlong(tac, T->_signed() ? "__div64" : "__udiv64");
  else
    runtime_notlonglong(tac, T->_signed() ? ".div" : ".udiv");
}

void gencode::mod(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  if ( T->size() > 4 )
    runtime_longlong(tac, T->_signed() ? "__rem64" : "__urem64");
  else
    runtime_notlonglong(tac, T->_signed() ? ".rem" : ".urem");
}

void gencode::lsh(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  if ( T->size() > 4 )
    runtime_longlong_shift(tac,"__ashldi3");
  else
    binop_notlonglong(tac,"sll");
}

void gencode::rsh(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  if ( T->size() > 4 )
    runtime_longlong_shift(tac, T->_signed() ? "__ashrdi3" : "__lshrdi3");
  else
    binop_notlonglong(tac, T->_signed() ? "sra" : "srl");
}

void gencode::_and(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  if ( T->size() > 4 )
    binop_longlong(tac,"and","and");
  else
    binop_notlonglong(tac,"and");
}

void gencode::_xor(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  if ( T->size() > 4 )
    binop_longlong(tac,"xor","xor");
  else
    binop_notlonglong(tac,"xor");
}

void gencode::_or(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  if ( T->size() > 4 )
    binop_longlong(tac,"or","or");
  else
    binop_notlonglong(tac,"or");
}

void gencode::binop_notlonglong(const COMPILER::tac* tac, std::string op)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);
  y->load("%i0");
  z->load("%i1");
  out << '\t' << op << '\t' << "%i0, %i1, %i0" << '\n';
  x->store("%i0");
}

void gencode::binop_longlong(const COMPILER::tac* tac, std::string lo, std::string hi)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);
  y->load("%i0");
  z->load("%i2");
  out << '\t' << lo << '\t' << "%i1, %i3, %i1" << '\n';
  out << '\t' << hi << '\t' << "%i0, %i2, %i0" << '\n';
  x->store("%i0");
}

void gencode::runtime_notlonglong(const COMPILER::tac* tac, std::string func)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);
  y->load("%o0");
  z->load("%o1");
  out << '\t' << "call" << '\t' << func << '\n';
  out << '\t' << "nop" << '\n';
  x->store("%o0");
}

void gencode::runtime_longlong(const COMPILER::tac* tac, std::string func)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);
  y->load("%o0");
  z->load("%o2");
  out << '\t' << "call" << '\t' << func << '\n';
  out << '\t' << "nop" << '\n';
  x->store("%o0");
}

void gencode::runtime_longlong_shift(const COMPILER::tac* tac, std::string func)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);
  y->load("%o0");
  z->load("%o2");
  const type* T = tac->z->m_type;
  int size = T->size();
  if ( size > 4 )
    out << '\t' << "mov" << '\t' << "%o3, %o2" << '\n';
  out << '\t' << "call" << '\t' << func << '\n';
  out << '\t' << "nop" << '\n';
  x->store("%o0");
}

void gencode::binop_real(const COMPILER::tac* tac, std::string op)
{
  using namespace COMPILER;
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  int size = T->size();
  switch ( size ){
  case 4:  binop_single(tac,op); break;
  case 8:  binop_double(tac,op); break;
  case 16: binop_quad(tac,op); break;
  }
}

void gencode::binop_single(const COMPILER::tac* tac, std::string op)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);
  y->load("%f0");
  z->load("%f1");
  const type* T = tac->x->m_type;
  int size = T->size();
  out << '\t' << op << 's' << '\t' << "%f0, %f1, %f0" << '\n';
  x->store("%f0");
}

void gencode::binop_double(const COMPILER::tac* tac, std::string op)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);
  y->load("%f0");
  z->load("%f2");
  const type* T = tac->x->m_type;
  int size = T->size();
  out << '\t' << op << 'd' << '\t' << "%f0, %f2, %f0" << '\n';
  x->store("%f0");
}

void gencode::binop_quad(const COMPILER::tac* tac, std::string op)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);
  x->get("%i0");
  out << '\t' << "st" << '\t' << "%i0, [%sp+64]" << '\n';
  y->load("%i0");
  tmp_quad[0]->store("%i0");
  z->load("%i0");
  tmp_quad[1]->store("%i0");
  tmp_quad[0]->get("%o0");
  tmp_quad[1]->get("%o1");
  op = op.substr(1,op.size()-1);
  out << '\t' << "call" << '\t' << "_Q_" << op << '\n';
  out << '\t' << "nop" << '\n';
  out << '\t' << "unimp" << '\t' << 16 << '\n';
}

void gencode::uminus(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  if ( T->real() )
    unaop_real(tac,"fnegs");
  else if ( T->size() > 4 )
    unaop_longlong(tac,"subcc","subx");
  else
    unaop_notlonglong(tac,"sub");
}

void gencode::tilde(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  if ( T->size() > 4 )
    unaop_longlong(tac,"xnor","xnor");
  else
    unaop_notlonglong(tac,"xnor");
}

void gencode::unaop_notlonglong(const COMPILER::tac* tac, std::string op)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  y->load("%i0");
  out << '\t' << op << '\t' << "%g0, %i0, %i0" << '\n';
  x->store("%i0");
}

void gencode::unaop_real(const COMPILER::tac* tac, std::string op)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  y->load("%f0");
  out << '\t' << op << '\t' << "%f0, %f0" << '\n';
  x->store("%f0");
}

void gencode::unaop_longlong(const COMPILER::tac* tac, std::string lo, std::string hi)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  y->load("%i0");
  out << '\t' << lo << '\t' << "%g0, %i1, %i1" << '\n';
  out << '\t' << hi << '\t' << "%g0, %i0, %i0" << '\n';
  x->store("%i0");
}

struct gencode::cast_table_t : std::map<std::pair<int,int>, void (*)(const COMPILER::tac*)> {
public:
  cast_table_t()
  {
    using namespace std;
    (*this)[make_pair(4,4)] = sint08_sint08;
    (*this)[make_pair(4,6)] = sint08_uint08;
    (*this)[make_pair(4,8)] = sint08_sint16;
    (*this)[make_pair(4,10)] = sint08_uint16;
    (*this)[make_pair(4,16)] = sint08_sint32;
    (*this)[make_pair(4,18)] = sint08_uint32;
    (*this)[make_pair(4,32)] = sint08_sint64;
    (*this)[make_pair(4,34)] = sint08_uint64;
    (*this)[make_pair(4,17)] = sint08_single;
    (*this)[make_pair(4,33)] = sint08_double;
    (*this)[make_pair(4,65)] = sint08_quad;

    (*this)[make_pair(6,4)] = uint08_sint08;
    (*this)[make_pair(6,8)] = uint08_sint16;
    (*this)[make_pair(6,10)] = uint08_uint16;
    (*this)[make_pair(6,16)] = uint08_sint32;
    (*this)[make_pair(6,18)] = uint08_uint32;
    (*this)[make_pair(6,32)] = uint08_sint64;
    (*this)[make_pair(6,34)] = uint08_uint64;
    (*this)[make_pair(6,17)] = uint08_single;
    (*this)[make_pair(6,33)] = uint08_double;
    (*this)[make_pair(6,65)] = uint08_quad;

    (*this)[make_pair(8,4)] = sint16_sint08;
    (*this)[make_pair(8,6)] = sint16_uint08;
    (*this)[make_pair(8,10)] = sint16_uint16;
    (*this)[make_pair(8,16)] = sint16_sint32;
    (*this)[make_pair(8,18)] = sint16_uint32;
    (*this)[make_pair(8,32)] = sint16_sint64;
    (*this)[make_pair(8,34)] = sint16_uint64;
    (*this)[make_pair(8,17)] = sint16_single;
    (*this)[make_pair(8,33)] = sint16_double;
    (*this)[make_pair(8,65)] = sint16_quad;

    (*this)[make_pair(10,4)] = uint16_sint08;
    (*this)[make_pair(10,6)] = uint16_uint08;
    (*this)[make_pair(10,8)] = uint16_sint16;
    (*this)[make_pair(10,16)] = uint16_sint32;
    (*this)[make_pair(10,18)] = uint16_uint32;
    (*this)[make_pair(10,32)] = uint16_sint64;
    (*this)[make_pair(10,34)] = uint16_uint64;
    (*this)[make_pair(10,17)] = uint16_single;
    (*this)[make_pair(10,33)] = uint16_double;
    (*this)[make_pair(10,65)] = uint16_quad;

    (*this)[make_pair(16,4)] = sint32_sint08;
    (*this)[make_pair(16,6)] = sint32_uint08;
    (*this)[make_pair(16,8)] = sint32_sint16;
    (*this)[make_pair(16,10)] = sint32_uint16;
    (*this)[make_pair(16,16)] = sint32_sint32;
    (*this)[make_pair(16,18)] = sint32_uint32;
    (*this)[make_pair(16,32)] = sint32_sint64;
    (*this)[make_pair(16,34)] = sint32_uint64;
    (*this)[make_pair(16,17)] = sint32_single;
    (*this)[make_pair(16,33)] = sint32_double;
    (*this)[make_pair(16,65)] = sint32_quad;

    (*this)[make_pair(18,4)] = uint32_sint08;
    (*this)[make_pair(18,6)] = uint32_uint08;
    (*this)[make_pair(18,8)] = uint32_sint16;
    (*this)[make_pair(18,10)] = uint32_uint16;
    (*this)[make_pair(18,16)] = uint32_sint32;
    (*this)[make_pair(18,18)] = uint32_uint32;
    (*this)[make_pair(18,32)] = uint32_sint64;
    (*this)[make_pair(18,34)] = uint32_uint64;
    (*this)[make_pair(18,17)] = uint32_single;
    (*this)[make_pair(18,33)] = uint32_double;
    (*this)[make_pair(18,65)] = uint32_quad;

    (*this)[make_pair(32,4)] = sint64_sint08;
    (*this)[make_pair(32,6)] = sint64_uint08;
    (*this)[make_pair(32,8)] = sint64_sint16;
    (*this)[make_pair(32,10)] = sint64_uint16;
    (*this)[make_pair(32,16)] = sint64_sint32;
    (*this)[make_pair(32,18)] = sint64_uint32;
    (*this)[make_pair(32,34)] = sint64_uint64;
    (*this)[make_pair(32,17)] = sint64_single;
    (*this)[make_pair(32,33)] = sint64_double;
    (*this)[make_pair(32,65)] = sint64_quad;

    (*this)[make_pair(34,4)] = uint64_sint08;
    (*this)[make_pair(34,6)] = uint64_uint08;
    (*this)[make_pair(34,8)] = uint64_sint16;
    (*this)[make_pair(34,10)] = uint64_uint16;
    (*this)[make_pair(34,16)] = uint64_sint32;
    (*this)[make_pair(34,18)] = uint64_uint32;
    (*this)[make_pair(34,32)] = uint64_sint64;
    (*this)[make_pair(34,17)] = uint64_single;
    (*this)[make_pair(34,33)] = uint64_double;
    (*this)[make_pair(34,65)] = uint64_quad;
    
    (*this)[make_pair(17,4)] = single_sint08;
    (*this)[make_pair(17,6)] = single_uint08;
    (*this)[make_pair(17,8)] = single_sint16;
    (*this)[make_pair(17,10)] = single_uint16;
    (*this)[make_pair(17,16)] = single_sint32;
    (*this)[make_pair(17,18)] = single_uint32;
    (*this)[make_pair(17,32)] = single_sint64;
    (*this)[make_pair(17,34)] = single_uint64;
    (*this)[make_pair(17,33)] = single_double;
    (*this)[make_pair(17,65)] = single_quad;

    (*this)[make_pair(33,4)] = double_sint08;
    (*this)[make_pair(33,6)] = double_uint08;
    (*this)[make_pair(33,8)] = double_sint16;
    (*this)[make_pair(33,10)] = double_uint16;
    (*this)[make_pair(33,16)] = double_sint32;
    (*this)[make_pair(33,18)] = double_uint32;
    (*this)[make_pair(33,32)] = double_sint64;
    (*this)[make_pair(33,34)] = double_uint64;
    (*this)[make_pair(33,17)] = double_single;
    (*this)[make_pair(33,65)] = double_quad;

    (*this)[make_pair(65,4)] = quad_sint08;
    (*this)[make_pair(65,6)] = quad_uint08;
    (*this)[make_pair(65,8)] = quad_sint16;
    (*this)[make_pair(65,10)] = quad_uint16;
    (*this)[make_pair(65,16)] = quad_sint32;
    (*this)[make_pair(65,18)] = quad_uint32;
    (*this)[make_pair(65,32)] = quad_sint64;
    (*this)[make_pair(65,34)] = quad_uint64;
    (*this)[make_pair(65,17)] = quad_single;
    (*this)[make_pair(65,33)] = quad_double;
  }
};

gencode::cast_table_t gencode::cast_table;

void gencode::cast(const COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  int x = cast_id(tac->x->m_type);
  int y = cast_id(tac->y->m_type);
  cast_table[make_pair(x,y)](tac);
}

int gencode::cast_id(const COMPILER::type* T)
{
  using namespace COMPILER;
  int n = (T->integer() && !T->_signed()) ? 1 : 0;
  return (T->size() << 2) + (n << 1) + T->real();
}

void gencode::sint08_sint08(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  assign(tac);
}

void gencode::sint08_uint08(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  assign(tac);
}

void gencode::sint08_sint16(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,true,8);
}

void gencode::sint08_uint16(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,true,8);
}

void gencode::sint08_sint32(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,true,8);
}

void gencode::sint08_uint32(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,true,8);
}

void gencode::sint08_sint64(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,true,8);
}

void gencode::sint08_uint64(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,true,8);
}

void gencode::sint08_single(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  common_real(tac,"fstoi",true,8);
}

void gencode::sint08_double(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  common_real(tac,"fdtoi",true,8);
}

void gencode::sint08_quad(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  common_quad(tac,true,8);
}

void gencode::uint08_sint08(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  assign(tac);
}

void gencode::uint08_sint16(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,false,8);
}

void gencode::uint08_uint16(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,false,8);
}

void gencode::uint08_sint32(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,false,8);
}

void gencode::uint08_uint32(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,false,8);
}

void gencode::uint08_sint64(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,false,8);
}

void gencode::uint08_uint64(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,false,8);
}

void gencode::uint08_single(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  common_real(tac,"fstoi",false,8);
}

void gencode::uint08_double(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  common_real(tac,"fdtoi",false,8);
}

void gencode::uint08_quad(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  common_quad(tac,false,8);
}

void gencode::sint16_sint08(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,true,8);
}

void gencode::sint16_uint08(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,false,8);
}

void gencode::sint16_uint16(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  assign(tac);
}

void gencode::sint16_sint32(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,true,16);
}

void gencode::sint16_uint32(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,true,16);
}

void gencode::sint16_sint64(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,true,16);
}

void gencode::sint16_uint64(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,true,16);
}

void gencode::sint16_single(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  common_real(tac,"fstoi",true,16);
}

void gencode::sint16_double(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  common_real(tac,"fdtoi",true,16);
}

void gencode::sint16_quad(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  common_quad(tac,true,16);
}

void gencode::uint16_sint08(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,true,8);
}

void gencode::uint16_uint08(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,false,8);
}

void gencode::uint16_sint16(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  assign(tac);
}

void gencode::uint16_sint32(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,false,16);
}

void gencode::uint16_uint32(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,false,16);
}

void gencode::uint16_sint64(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,false,16);
}

void gencode::uint16_uint64(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,false,16);
}

void gencode::uint16_single(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  common_real(tac,"fstoi",false,16);
}

void gencode::uint16_double(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  common_real(tac,"fdtoi",false,16);
}

void gencode::uint16_quad(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  common_quad(tac,false,16);
}

void gencode::sint32_sint08(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,true,8);
}

void gencode::sint32_uint08(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,false,8);
}

void gencode::sint32_sint16(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,true,16);
}

void gencode::sint32_uint16(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,false,16);
}

void gencode::sint32_sint32(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  assign(tac);
}

void gencode::sint32_uint32(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  assign(tac);
}

void gencode::sint32_sint64(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  common_longlong(tac);
}

void gencode::sint32_uint64(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  common_longlong(tac);
}

void gencode::sint32_single(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  common_real(tac,"fstoi",true,32);
}

void gencode::sint32_double(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  common_real(tac,"fdtoi",true,32);
}

void gencode::sint32_quad(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  common_quad(tac,true,32);
}

void gencode::uint32_sint08(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,true,8);
}

void gencode::uint32_uint08(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,false,8);
}

void gencode::uint32_sint16(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,true,16);
}

void gencode::uint32_uint16(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend(tac,false,16);
}

void gencode::uint32_sint32(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  assign(tac);
}

void gencode::uint32_uint32(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  assign(tac);
}

void gencode::uint32_sint64(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  common_longlong(tac);
}

void gencode::uint32_uint64(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  common_longlong(tac);
}

void gencode::uint32_single(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  uint32_real(tac);
}

void gencode::uint32_double(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  uint32_real(tac);
}

void gencode::uint32_real(const COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  y->load("%f0");
  const type* T = tac->y->m_type;
  if ( T->size() != 8 )
    out << '\t' << "fstod" << '\t' << "%f0, %f0" << '\n';
  if ( double_0x80000000.m_label.empty() )
    double_0x80000000.m_label = new_label();
  if ( T->size() != 8 )
    T = T->varg();
  out << '\t' << "mov" << '\t' << double_0x80000000.m_label << ", %f2" << '\n';
  out << '\t' << "fcmped" << '\t' << "%f0, %f2" << '\n';
  out << '\t' << "nop" << '\n';
  string label = new_label();
  out << '\t' << "fbge" << '\t' << label << '\n';
  out << '\t' << "nop" << '\n';
  out << '\t' << "fdtoi" << '\t' << "%f0, %f0" << '\n';
  x->store("%f0");
  string label2 = new_label();
  out << '\t' << "b" << '\t' << label2 << '\n';
  out << '\t' << "nop" << '\n';

  out << label << ":" << '\n';
  out << '\t' << "fsubd" << '\t' << "%f0, %f2, %f2" << '\n';
  out << '\t' << "fdtoi" << '\t' << "%f2, %f2" << '\n';
  x->store("%f2");
  out << '\t' << "sethi" << '\t' << "%hi(-2147483648), %i0" << '\n';
  x->load("%i1");
  out << '\t' << "xor" << '\t' << "%i1, %i0, %i0" << '\n';
  x->store("%i0");
  out << label2 << ":" << '\n';
}

void gencode::uint32_quad(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  common_quad(tac,false,32);
}

void gencode::sint64_sint08(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend64(tac,true,8);
}

void gencode::sint64_uint08(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend64(tac,false,8);
}

void gencode::sint64_sint16(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend64(tac,true,16);
}

void gencode::sint64_uint16(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend64(tac,false,16);
}

void gencode::sint64_sint32(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend64(tac,true,32);
}

void gencode::sint64_uint32(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend64(tac,false,32);
}

void gencode::sint64_uint64(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  assign(tac);
}

void gencode::sint64_single(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  longlong_real(tac,"__ftoll");
}

void gencode::sint64_double(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  longlong_real(tac,"__dtoll");
}

void gencode::sint64_quad(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  longlong_quad(tac,"__fixtfdi");
}

void gencode::uint64_sint08(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend64(tac,true,8);
}

void gencode::uint64_uint08(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend64(tac,false,8);
}

void gencode::uint64_sint16(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend64(tac,true,16);
}

void gencode::uint64_uint16(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend64(tac,false,16);
}

void gencode::uint64_sint32(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend64(tac,true,32);
}

void gencode::uint64_uint32(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  extend64(tac,false,32);
}

void gencode::uint64_sint64(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  assign(tac);
}

void gencode::uint64_single(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  longlong_real(tac,"__ftoull");
}

void gencode::uint64_double(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  longlong_real(tac,"__dtoull");
}

void gencode::uint64_quad(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  longlong_quad(tac,"__fixunstfdi");
}

void gencode::single_sint08(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  real_common(tac,"fitos",true,8);
}

void gencode::single_uint08(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  real_common(tac,"fitos",false,8);
}

void gencode::single_sint16(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  real_common(tac,"fitos",true,16);
}

void gencode::single_uint16(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  real_common(tac,"fitos",false,16);
}

void gencode::single_sint32(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  real_common(tac,"fitos",true,32);
}

void gencode::single_uint32(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  if ( typeid(*y) == typeid(imm) ){
    y->load("%i0");
    tmp_word->store("%i0");
    y = tmp_word;
  }
  y->load("%f0");
  out << '\t' << "fitod" << '\t' << "%f0, %f0" << '\n';
  tmp_dword->store("%f0");
  y->load("%i0");
  out << '\t' << "cmp" << '\t' << "%i0, 0" << '\n';
  std::string label = new_label();
  out << '\t' << "bge" << '\t' << label << '\n';
  out << '\t' << "nop" << '\n';
  if ( double_0x100000000.m_label.empty() )
    double_0x100000000.m_label = new_label();
  out << '\t' << "mov" << '\t' << double_0x100000000.m_label << ", %f0" << '\n';
  tmp_dword->load("%f2");
  out << '\t' << "faddd" << '\t' << "%f0, %f2, %f0" << '\n';
  tmp_dword->store("%f0");
  out << label << ":\n";
  tmp_dword->load("%f0");
  out << '\t' << "fdtos" << '\t' << "%f0, %f0" << '\n';
  x->store("%f0");
}

void gencode::single_sint64(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  real_sint64(tac,"__floatdisf");
}

void gencode::single_uint64(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  real_uint64(tac,"__floatdisf","fadds");
}

void gencode::single_double(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  y->load("%f0");
  out << '\t' << "fdtos" << '\t' << "%f0, %f0" << '\n';
  x->store("%f0");
}

void gencode::single_quad(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  real_quad(tac,"_Q_qtos");
}

void gencode::double_sint08(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  real_common(tac,"fitod",true,8);
}

void gencode::double_uint08(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  real_common(tac,"fitod",false,8);
}

void gencode::double_sint16(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  real_common(tac,"fitod",true,16);
}

void gencode::double_uint16(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  real_common(tac,"fitod",false,16);
}

void gencode::double_sint32(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  real_common(tac,"fitod",true,32);
}

void gencode::real_common(const COMPILER::tac* tac, std::string op, bool signextend,
                          int bit)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  if ( typeid(*y) == typeid(imm) ){
    y->load("%i0");
    tmp_word->store("%i0");
    y = tmp_word;
  }
  else if ( bit != 32 ){
    y->load("%i0",bit/8);
    std::string op = signextend ? "sra" : "srl";
    out << '\t' << "sll" << '\t' << "%i0, " << 32 - bit << ", %i0" << '\n';
    out << '\t' <<  op   << '\t' << "%i0, " << 32 - bit << ", %i0" << '\n';
    tmp_word->store("%i0");
    y = tmp_word;
  }
  y->load("%f0");
  out << '\t' << op << '\t' << "%f0, %f0" << '\n';
  x->store("%f0");
}

void gencode::double_uint32(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  if ( typeid(*y) == typeid(imm) ){
    y->load("%i0");
    tmp_word->store("%i0");
    y = tmp_word;
  }
  y->load("%f0");
  out << '\t' << "fitod" << '\t' << "%f0, %f0" << '\n';
  x->store("%f0");
  y->load("%i0");
  out << '\t' << "cmp" << '\t' << "%i0, 0" << '\n';
  std::string label = new_label();
  out << '\t' << "bge" << '\t' << label << '\n';
  out << '\t' << "nop" << '\n';
  if ( double_0x100000000.m_label.empty() )
    double_0x100000000.m_label = new_label();
  out << '\t' << "ld" << '\t' << double_0x100000000.m_label << ", %f0" << '\n';
  x->load("%f2");
  out << '\t' << "faddd" << '\t' << "%f0, %f2, %f0" << '\n';
  x->store("%f0");
  out << label << ":\n";
}

void gencode::double_sint64(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  real_sint64(tac,"__floatdidf");
}

void gencode::double_uint64(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  real_uint64(tac,"__floatdidf","faddd");
}

void gencode::double_single(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  y->load("%f0");
  out << '\t' << "fstod" << '\t' << "%f0, %f0" << '\n';
  x->store("%f0");
}

void gencode::double_quad(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  real_quad(tac,"_Q_qtod");
}

void gencode::quad_sint08(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  quad_common(tac,true,8);
}

void gencode::quad_uint08(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  quad_common(tac,false,8);
}

void gencode::quad_sint16(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  quad_common(tac,true,16);
}

void gencode::quad_uint16(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  quad_common(tac,false,16);
}

void gencode::quad_sint32(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  quad_common(tac,true,32);
}

void gencode::quad_common(const COMPILER::tac* tac, bool signextend, int bit)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  x->get("%i0");
  out << '\t' << "st" << '\t' << "%i0, [%sp+64]" << '\n';
  y->load("%o0");
  if ( bit != 32 ){
    std::string op = signextend ? "sra" : "srl";
    out << '\t' << "sll" << '\t' << "%o0, " << 32 - bit << ", %o0" << '\n';
    out << '\t' <<  op   << '\t' << "%o0, " << 32 - bit << ", %o0" << '\n';
  }
  out << '\t' << "call" << '\t' << "_Q_itoq" << '\n';
  out << '\t' << "nop" << '\n';
  out << '\t' << "unimp" << '\t' << 16 << '\n';
}

void gencode::quad_uint32(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  quad_common(tac,false,32);
  address* x = getaddr(tac->x);
  x->get("%o0");
  out << '\t' << "ld" << '\t' << "[%o0], %o0" << '\n';
  out << '\t' << "cmp" << '\t' << "%o0, 0" << '\n';
  std::string label = new_label();
  out << '\t' << "bge" << '\t' << label << '\n';
  out << '\t' << "nop" << '\n';

  x->load("%i0");
  tmp_quad[0]->store("%i0");

  x->get("%i0");
  out << '\t' << "st" << '\t' << "%i0, [%sp+64]" << '\n';
  tmp_quad[0]->get("%o0");
  if ( long_double_0x100000000.m_label.empty() )
    long_double_0x100000000.m_label = new_label();

  out << '\t' << "ld" << '\t' << long_double_0x100000000.m_label << ", %o1" << '\n';
  out << '\t' << "call" << '\t' << "_Q_add" << '\n';
  out << '\t' << "nop" << '\n';
  out << '\t' << "unimp" << '\t' << 16 << '\n';

  out << label << ":\n";
}

void gencode::quad_sint64(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  x->get("%i0");
  out << '\t' << "st" << '\t' << "%i0, [%sp+64]" << '\n';
  y->load("%o0");
  out << '\t' << "call" << '\t' << "__floatditf" << '\n';
  out << '\t' << "nop" << '\n';
  out << '\t' << "unimp" << '\t' << 16 << '\n';
}

void gencode::quad_uint64(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  quad_sint64(tac);

  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  y->get("%i0");
  out << '\t' << "ld" << '\t' << "[%i0], %o0" << '\n';
  out << '\t' << "ld" << '\t' << "[%i0+4], %o0" << '\n';
  out << '\t' << "mov" << '\t' << "0, %o2" << '\n';
  out << '\t' << "mov" << '\t' << "0, %o3" << '\n';
  out << '\t' << "call" << '\t' << "__cmpdi2" << '\n';
  out << '\t' << "nop" << '\n';
  out << '\t' << "cmp" << '\t' << "%o0, 1" << '\n';
  std::string label = new_label();
  out << '\t' << "bge" << '\t' << label << '\n';
  out << '\t' << "nop" << '\n';

  x->load("%i0");
  tmp_quad[0]->store("%i0");

  x->get("%i0");
  out << '\t' << "st" << '\t' << "%i0, [%sp+64]" << '\n';
  tmp_quad[0]->get("%o0");
  if ( long_double_0x10000000000000000.m_label.empty() )
    long_double_0x10000000000000000.m_label = new_label();

  out << '\t' << "mov" << long_double_0x10000000000000000.m_label << ", %o1" << '\n';
  out << '\t' << "call" << '\t' << "_Q_add" << '\n';
  out << '\t' << "nop" << '\n';
  out << '\t' << "unimp" << '\t' << 16 << '\n';

  out << label << ":\n";
}

void gencode::quad_single(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  quad_real(tac,"_Q_stoq");
}

void gencode::quad_double(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  quad_real(tac,"_Q_dtoq");
}

void gencode::quad_real(const COMPILER::tac* tac, std::string func)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  x->get("%i0");
  out << '\t' << "st" << '\t' << "%i0, [%sp+64]" << '\n';
  y->load("%o0");
  out << '\t' << "call" << '\t' << func << '\n';
  out << '\t' << "nop" << '\n';
  out << '\t' << "unimp" << '\t' << 16 << '\n';
}

void gencode::real_quad(const COMPILER::tac* tac, std::string func)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  y->get("%o0");
  out << '\t' << "call" << '\t' << func << '\n';
  out << '\t' << "nop" << '\n';
  x->store("%f0");
}

void gencode::extend(const COMPILER::tac* tac, bool signextend, int bit)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  y->load("%i0",bit/8);
  std::string op = signextend ? "sra" : "srl";
  out << '\t' << "sll" << '\t' << "%i0, " << 32 - bit << ", %i0" << '\n';
  out << '\t' <<  op   << '\t' << "%i0, " << 32 - bit << ", %i0" << '\n';
  x->store("%i0");
}

void gencode::extend64(const COMPILER::tac* tac, bool signextend, int bit)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  y->load("%i1",bit/8);
  if ( bit != 32 ){
    std::string op = signextend ? "sra" : "srl";
    out << '\t' << "sll" << '\t' << "%i1, " << 32 - bit << ", %i1" << '\n';
    out << '\t' <<  op   << '\t' << "%i1, " << 32 - bit << ", %i1" << '\n';
  }

  if ( signextend ){
    out << '\t' << "mov" << '\t' << "%i1, %i0" << '\n';
    out << '\t' << "sra" << '\t' << "%i0, 31, %i0" << '\n';
  }
  else
    out << '\t' << "mov" << '\t' << "0, %i0" << '\n';

  x->store("%i0");
}

void gencode::common_longlong(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  y->load("%i0",4);
  x->store("%i0");
}

void gencode::common_real(const COMPILER::tac* tac, std::string op,
                          bool signextend, int bit)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  y->load("%f0");
  out << '\t' << op << '\t' << "%f0, %f0" << '\n';
  tmp_word->store("%f0");
  tmp_word->load("%i0");
  if ( bit != 32 ){
    std::string op = signextend ? "sra" : "srl";
    out << '\t' << "sll" << '\t' << "%i0, " << 32 - bit << ", %i0" << '\n';
    out << '\t' <<  op   << '\t' << "%i0, " << 32 - bit << ", %i0" << '\n';
  }
  x->store("%i0");
}

void gencode::common_quad(const COMPILER::tac* tac, bool signextend, int bit)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  y->get("%o0");
  out << '\t' << "call" << '\t' << "_Q_qtoi" << '\n';
  out << '\t' << "nop" << '\n';
  if ( bit != 32 ){
    std::string op = signextend ? "sra" : "srl";
    out << '\t' << "sll" << '\t' << "%o0, " << 32 - bit << ", %o0" << '\n';
    out << '\t' <<  op   << '\t' << "%o0, " << 32 - bit << ", %o0" << '\n';
  }
  x->store("%o0");
}

void gencode::real_sint64(const COMPILER::tac* tac, std::string func)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  y->load("%o0");
  out << '\t' << "call" << '\t' << func << '\n';
  out << '\t' << "nop" << '\n';
  x->store("%f0");
}

void gencode::real_uint64(const COMPILER::tac* tac, std::string func, std::string fadd)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  y->load("%o0");
  out << '\t' << "mov" << '\t' << "0, %o2" << '\n';
  out << '\t' << "mov" << '\t' << "0, %o3" << '\n';
  out << '\t' << "call" << '\t' << "__cmpdi2" << '\n';
  out << '\t' << "nop" << '\n';
  out << '\t' << "cmp" << '\t' << "%o0, 1" << '\n';
  std::string label = new_label();
  out << '\t' << "bl" << '\t' << label << '\n';
  out << '\t' << "nop" << '\n';
  y->load("%o0");
  out << '\t' << "call" << '\t' << func << '\n';
  out << '\t' << "nop" << '\n';
  x->store("%f0");
  std::string label2 = new_label();
  out << '\t' << "b" << '\t' << label2 << '\n';
  out << '\t' << "nop" << '\n';

  out << label << ":\n";
  y->load("%i0");
  out << '\t' << "srl" << '\t' << "%i0, 1, %o0" << '\n';
  out << '\t' << "sll" << '\t' << "%i0, 31, %o1" << '\n';
  out << '\t' << "srl" << '\t' << "%i1, 1, %i1" << '\n';
  out << '\t' << "or" << '\t'  << "%o1, %i1, %o1" << '\n';
  out << '\t' << "call" << '\t' << func << '\n';
  out << '\t' << "nop" << '\n';
  out << '\t' << fadd << '\t' << "%f0, %f0, %f0" << '\n';
  x->store("%f0");
  out << label2 << ":\n";
}

void gencode::longlong_real(const COMPILER::tac* tac, std::string func)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  y->load("%o0");
  out << '\t' << "call" << '\t' << func << '\n';
  out << '\t' << "nop" << '\n';
  x->store("%o0");
}

void gencode::longlong_quad(const COMPILER::tac* tac, std::string func)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);

  y->get("%o0");
  out << '\t' << "call" << '\t' << func << '\n';
  out << '\t' << "nop" << '\n';
  x->store("%o0");
}

void gencode::addr(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  y->get("%i0");
  x->store("%i0");
}

void gencode::invladdr(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->z->m_type;
  if ( !T->scalar() )
    invladdr_aggregate(tac);
  else {
    int size = T->size();
    switch ( size ){
    case 1:   invladdr_byte(tac);  break;
    case 2:   invladdr_half(tac);  break;
    case 4:   invladdr_word(tac);  break;
    case 8:   invladdr_dword(tac); break;
    case 16:  invladdr_quad(tac);  break;
    }
  }
}

void gencode::invladdr_byte(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  invladdr_common(tac,"stb");
}

void gencode::invladdr_half(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  invladdr_common(tac,"sth");
}

void gencode::invladdr_word(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  invladdr_common(tac,"st");
}

void gencode::invladdr_dword(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  invladdr_common(tac,"std");
}

void gencode::invladdr_quad(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);
  y->load("%o1");
  z->load("%i0");
  out << '\t' << "std" << '\t' << "%i0 , [%o1]" << '\n';
  out << '\t' << "std" << '\t' << "%i4 , [%o1+8]" << '\n';
}

void gencode::invladdr_aggregate(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);
  y->load("%o0");
  z->get("%o1");
  const type* T = tac->z->m_type;
  int size = T->size();
  copy(0, z, size);
}

void gencode::invladdr_common(const COMPILER::tac* tac, std::string op)
{
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);
  y->load("%i2");
  z->load("%i0");
  out << '\t' << op << '\t' << "%i0 , [%i2]" << '\n';
}

void gencode::invraddr(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  if ( !T->scalar() )
    invraddr_aggregate(tac);
  else {
    int size = T->size();
    switch ( size ){
    case 1:   invraddr_byte(tac); break;
    case 2:   invraddr_half(tac); break;
    case 4:   invraddr_word(tac); break;
    case 8:   invraddr_dword(tac); break;
    case 16:  invraddr_quad(tac); break;
    }
  }
}

void gencode::invraddr_byte(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  invraddr_common(tac,"ldub");
}

void gencode::invraddr_half(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  invraddr_common(tac,"lduh");
}

void gencode::invraddr_word(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  invraddr_common(tac,"ld");
}

void gencode::invraddr_dword(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  y->load("%i1");
  out << '\t' << "ld" << '\t' << "[%i1], %i0" << '\n';
  out << '\t' << "add" << '\t' << "%i1, 4, %i1" << '\n';
  out << '\t' << "ld" << '\t' << "[%i1], %i1" << '\n';
  x->store("%i0");
}

void gencode::invraddr_quad(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  y->load("%i4");
  out << '\t' << "ldd" << '\t' << "[%i4], %i0" << '\n';
  out << '\t' << "ldd" << '\t' << "[%i4+8], %i4" << '\n';
  x->store("%i0");
}

void gencode::invraddr_aggregate(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  const type* T = tac->x->m_type;
  int size = T->size();
  copy(x, 0, size);
}

void gencode::invraddr_common(const COMPILER::tac* tac, std::string op)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  y->load("%i0");
  out << '\t' << op << '\t' << "[%i0], %i0" << '\n';
  x->store("%i0");
}

void gencode::loff(const COMPILER::tac* tac)
{
        using namespace COMPILER;
        const type* T = tac->z->m_type;
        if (!T->scalar())
                loff_aggregate(tac);
        else {
                int size = T->size();
                switch (size) {
                case 1:   loff_byte(tac); break;
                case 2:   loff_half(tac); break;
                case 4:   loff_word(tac); break;
                case 8:   loff_dword(tac); break;
                case 16:  loff_quad(tac); break;
                }
        }
}

void gencode::loff_byte(const COMPILER::tac* tac)
{
        loff_common(tac, "stb");
}

void gencode::loff_half(const COMPILER::tac* tac)
{
        loff_common(tac, "sth");
}

void gencode::loff_word(const COMPILER::tac* tac)
{
        loff_common(tac, "st");
}

void gencode::loff_dword(const COMPILER::tac* tac)
{
        loff_common(tac, "std");
}

void gencode::loff_quad(const COMPILER::tac* tac)
{
        using namespace COMPILER;
        {
                var* y = tac->y;
                if (y->isconstant())
                        return loff_quad(tac, y->value());
        }
        using namespace COMPILER;
        address* y = getaddr(tac->y);
        address* z = getaddr(tac->z);
        y->load("%o1");
        z->load("%i0");
        out << '\t' << "std" << '\t' << "%i0 , [%o1]" << '\n';
        out << '\t' << "std" << '\t' << "%i4 , [%o1+8]" << '\n';
}

void gencode::loff_aggregate(const COMPILER::tac* tac)
{
        using namespace COMPILER;
        {
                var* y = tac->y;
                if (y->isconstant())
                        return loff_aggregate(tac, y->value());
        }
        address* x = getaddr(tac->x);
        address* y = getaddr(tac->y);
        address* z = getaddr(tac->z);
        const type* T = tac->z->m_type;
        int size = T->size();
        x->get("%o0");
        y->load("%i1");
        out << '\t' << "add" << '\t' << "%o0, %i1, %o0" << '\n';
        copy(0, z, size);
}

void gencode::loff_aggregate(const COMPILER::tac* tac, int delta)
{
        using namespace COMPILER;
        address* x = getaddr(tac->x);
        address* z = getaddr(tac->z);
        const type* T = tac->z->m_type;
        int size = T->size();
        x->get("%o0");
        out << '\t' << "add" << '\t' << "%o0, " << delta << ", %o0" << '\n';
        copy(0, z, size);
}

void gencode::loff_common(const COMPILER::tac* tac, std::string op)
{
        using namespace COMPILER;
        {
                var* y = tac->y;
                if (y->isconstant())
                        return loff_common(tac, op, y->value());
        }
        address* x = getaddr(tac->x);
        address* y = getaddr(tac->y);
        address* z = getaddr(tac->z);
        x->get("%i1");
        y->load("%i2");
        z->load("%i0");
        out << '\t' << "add" << '\t' << "%i2, %i1, %i2" << '\n';
        out << '\t' << op << '\t' << "%i0 , [%i2]" << '\n';
}

void gencode::loff_common(const COMPILER::tac* tac, std::string op, int delta)
{
        using namespace COMPILER;
        address* x = getaddr(tac->x);
        address* z = getaddr(tac->z);
        x->get("%i2");
        out << '\t' << op << '\t' << "%i0 , ";
        out << "[%i2";
        if (delta)
                out << '+' << delta;
        out << ']' << '\n';
}

void gencode::loff_quad(const COMPILER::tac* tac, int delta)
{
        using namespace COMPILER;
        address* x = getaddr(tac->x);
        address* z = getaddr(tac->z);

        x->get("%o1");
        z->load("%i0");
        out << '\t' << "std" << '\t' << "%i0 , ";
        out << "[%o1";
        if (delta)
                out << delta;
        out << ']' << '\n';
        out << '\t' << "std" << '\t' << "%i4 , ";
        if (delta)
                out << delta;
        out << "[%o1";
        if (int n = delta + 8)
                out << '+' << n;
        out << ']' << '\n';
}

void gencode::roff(const COMPILER::tac* tac)
{
        using namespace COMPILER;
        const type* T = tac->x->m_type;
        int size = T->size();
        if (T->aggregate())
                return roff_aggregate(tac);
        switch (size) {
        case 1: roff_byte(tac); break;
        case 2: roff_half(tac); break;
        case 4: roff_word(tac); break;
        case 8: roff_dword(tac); break;
        case 16: roff_quad(tac); break;
        }
}

void gencode::roff_byte(const COMPILER::tac* tac)
{
        roff_common(tac, "ldub");
}

void gencode::roff_half(const COMPILER::tac* tac)
{
        roff_common(tac, "lduh");
}

void gencode::roff_word(const COMPILER::tac* tac)
{
        roff_common(tac, "ld");
}

void gencode::roff_dword(const COMPILER::tac* tac)
{
        roff_common(tac, "ldd");
}

void gencode::roff_quad(const COMPILER::tac* tac)
{
        using namespace std;
        using namespace COMPILER;
        {
                var* z = tac->z;
                if (z->isconstant())
                        return roff_quad(tac, z->value());
        }
        address* x = getaddr(tac->x);
        address* y = getaddr(tac->y);
        address* z = getaddr(tac->z);
        y->get("%i4");
        z->load("%i1");
        out << '\t' << "add" << '\t' << "%i4, %i1, %i4" << '\n';
        out << '\t' << "ldd" << '\t' << "[%i4], %i0" << '\n';
        out << '\t' << "ldd" << '\t' << "[%i4+8], %i4" << '\n';
        x->store("%i0");
}

void gencode::roff_quad(const COMPILER::tac* tac, int delta)
{
        using namespace std;
        using namespace COMPILER;
        address* x = getaddr(tac->x);
        address* y = getaddr(tac->y);
        y->get("%i4");
        out << '\t' << "ldd" << '\t' << "[%i4";
        if (delta)
                out << '+' << delta;
        out << "], %i0" << '\n';
        out << '\t' << "ldd" << '\t' << "[%i4";
        if (int n = delta+8)
                out << '+' << n;
        out << "], %i4" << '\n';
        x->store("%i0");
}

void gencode::roff_aggregate(const COMPILER::tac* tac)
{
        using namespace std;
        using namespace COMPILER;
        {
                var* z = tac->z;
                if (z->isconstant())
                        return roff_aggregate(tac, z->value());
        }
        address* x = getaddr(tac->x);
        address* y = getaddr(tac->y);
        address* z = getaddr(tac->z);
        const type* T = tac->x->m_type;
        int size = T->size();
        y->get("%o1");
        z->load("%i1");
        out << '\t' << "add" << '\t' << "%o1, %i1, %o1" << '\n';
        copy(x, 0, size);
}

void gencode::roff_aggregate(const COMPILER::tac* tac, int delta)
{
        using namespace std;
        using namespace COMPILER;
        address* x = getaddr(tac->x);
        address* y = getaddr(tac->y);
        const type* T = tac->x->m_type;
        int size = T->size();
        y->get("%o1");
        out << '\t' << "add" << '\t' << "%o1, " << delta << ", %o1" << '\n';
        copy(x, 0, size);
}

void gencode::roff_common(const COMPILER::tac* tac, std::string op)
{
        using namespace std;
        using namespace COMPILER;
        {
                var* z = tac->z;
                if (z->isconstant())
                        return roff_common(tac, op, z->value());
        }
        address* x = getaddr(tac->x);
        address* y = getaddr(tac->y);
        address* z = getaddr(tac->z);
        y->get("%o1");
        z->load("%i1");
        out << '\t' << "add" << '\t' << "%o1, %i1, %o1" << '\n';
        out << '\t' << op << '\t' << "%o0, [%o1]" << '\n';
        x->store("%o0");
}

void gencode::roff_common(const COMPILER::tac* tac, std::string op, int delta)
{
        address* x = getaddr(tac->x);
        address* y = getaddr(tac->y);
        y->get("%o1");
        out << '\t' << op << '\t' << "%o0, [%o1";
        if (delta)
                out << '+' << delta;
        out << ']' << '\n';
        x->store("%o0");
}

void gencode::param(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->y->m_type->promotion();
  if ( !T->scalar() )
    param_aggregate(tac);
  else {
    int size = T->size();
    switch ( size ){
    case 4:  param_word(tac);      break;
    case 8:  param_dword(tac);     break;
    case 16: param_quad(tac);      break;
    }
  }
}

void gencode::param_word(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  if ( m_offset < 24 ){
    std::string reg = "%o";
    reg += char(m_offset/4 + '0');
    y->load(reg);
  }
  else {
    y->load("%i0");
    out << '\t' << "st" << '\t' << "%i0, ";
    out << '[' << "%sp + " << m_offset + aggre_regwin << ']' << '\n';
  }

  m_offset += 4;
}

void gencode::param_dword(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  if ( m_offset < 24 ){
    if ( m_offset/4 % 2 ){
      y->load("%i0");
      out << '\t' << "mov" << '\t' << "%i0, %o" << char(m_offset/4 + '0') << '\n';
      if ( m_offset != 20 )
        out << '\t' << "mov" << '\t' << "%i1, %o" << char(m_offset/4 + '1') << '\n';
      else
        out << '\t' << "st" << '\t' << "%i1, [%sp + 92]" << '\n';
    }
    else {
      std::string reg = "%o";
      reg += char(m_offset/4 + '0');
      y->load(reg);
    }
  }
  else {
    y->load("%i0");
    if ( m_offset/4 % 2 || (m_offset + aggre_regwin) % 8 ){
      out << '\t' << "st" << '\t' << "%i0, ";
      out << '[' << "%sp + " << m_offset     + aggre_regwin << ']' << '\n';
      out << '\t' << "st" << '\t' << "%i1, ";
      out << '[' << "%sp + " << m_offset + 4 + aggre_regwin << ']' << '\n';
    }
    else {
      out << '\t' << "std" << '\t' << "%i0, ";
      out << '[' << "%sp + " << m_offset + aggre_regwin << ']' << '\n';
    }
  }
  m_offset += 8;
}

void gencode::param_quad(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  param_aggregate(tac);
}

void gencode::param_aggregate(const COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  map<var*, ::stack*>::const_iterator p = record_param.find(tac->y);
  assert(p != record_param.end());
  address* y = p->second;
  if ( m_offset < 24 ){
    std::string reg = "%o";
    reg += char(m_offset/4 + '0');
    y->get(reg);
  }
  else {
    y->get("%i0");
    out << '\t' << "st" << '\t' << "%i0, ";
    out << '[' << "%sp + " << m_offset + aggre_regwin << ']' << '\n';
  }
  m_offset += 4;
}

int gencode::m_offset;

void gencode::call(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  m_offset = 0;
  if ( const var* ret = tac->x ){
    const type* T = ret->m_type;
    if ( !T->scalar() )
      call_aggregate(tac);
    else if ( T->real() )
      call_real(tac);
    else if ( T->size() > 4 )
      call_longlong(tac);
    else
      call_notlonglong(tac);
  }
  else call_void(tac);
}

void gencode::call_void(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  var* src1 = tac->y;
  const type* T = src1->m_type;
  address* y = getaddr(src1);
  if ( !T->scalar() ){
    mem* m = dynamic_cast<mem*>(y);
    out << '\t' << "call" << '\t' << m->label() << '\n';
  }
  else {
    y->load("%i0");
    out << '\t' << "call" << '\t' << "%i0" << '\n';
  }
  out << '\t' << "nop" << '\n';
}

void gencode::call_notlonglong(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  call_void(tac);
  address* x = getaddr(tac->x);
  x->store("%o0");
}

void gencode::call_real(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  call_void(tac);
  address* x = getaddr(tac->x);
  x->store("%f0");
}

void gencode::call_longlong(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  call_void(tac);
  address* x = getaddr(tac->x);
  x->store("%o0");
}

void gencode::call_aggregate(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  x->get("%i0");
  out << '\t' << "st" << '\t' << "%i0, [%sp+64]" << '\n';
  call_void(tac);
}

void gencode::_return(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  if ( tac->y ){
    const type* T = tac->y->m_type;
    if ( !T->scalar() )
      _return_aggregate(tac);
    else if ( T->real() )
      _return_real(tac);
    else if ( T->size() > 4 )
      _return_longlong(tac);
    else
      _return_notlonglong(tac);
  }

  if ( m_last != tac ){
    out << '\t' << "b" << '\t' << function_exit.m_label << '\n';
    out << '\t' << "nop" << '\n';
    function_exit.m_ref = true;
  }
}

void gencode::_return_notlonglong(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  y->load("%i0");
}

void gencode::_return_real(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  y->load("%f0");
}

void gencode::_return_longlong(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  y->load("%i0");
}

void gencode::_return_aggregate(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  stack ret(64,4,4);
  ret.load("%o0");
  address* y = getaddr(tac->y);
  const type* T = tac->y->m_type;
  int size = T->size();
  copy(0,y,size);
}

const COMPILER::tac* gencode::m_last;

void gencode::_goto(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const goto3ac* go = static_cast<const goto3ac*>(tac);
  if ( !go->m_op )
    out << '\t' << "b" << '\t' << '.' << func_label << go->m_to << '\n';
  else
    ifgoto(go);
}

void gencode::ifgoto(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->y->m_type;
  if ( T->real() )
    ifgoto_real(tac);
  else if ( T->size() > 4 )
    ifgoto_longlong(tac);
  else
    ifgoto_notlonglong(tac);
}

struct gencode::ifgoto_table : std::map<std::pair<COMPILER::goto3ac::op,bool>, std::string> {
  ifgoto_table()
  {
        using namespace std;
        using namespace COMPILER;
    typedef pair<goto3ac::op,bool> key;
    (*this)[key(goto3ac::EQ, true)] = "be";    (*this)[key(goto3ac::EQ, false)] = "be";
    (*this)[key(goto3ac::NE, true)] = "bne";   (*this)[key(goto3ac::NE, false)] = "bne";
    (*this)[key(goto3ac::LT, true)] = "bl";    (*this)[key(goto3ac::LT, false)] = "bcs";
    (*this)[key(goto3ac::GT, true)] = "bg";    (*this)[key(goto3ac::GT, false)] = "bgu";
    (*this)[key(goto3ac::LE, true)] = "ble";   (*this)[key(goto3ac::LE, false)] = "bleu";
    (*this)[key(goto3ac::GE, true)] = "bge";   (*this)[key(goto3ac::GE, false)] = "bcc";
  }
};

gencode::ifgoto_table gencode::m_ifgoto_table;

void gencode::ifgoto_notlonglong(const COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  address* x = getaddr(tac->y);
  address* y = getaddr(tac->z);
  x->load("%i0");
  y->load("%i1");
  out << '\t' << "cmp" << '\t' << "%i0, %i1" << '\n';
  const goto3ac* go = static_cast<const goto3ac*>(tac);
  const type* T = tac->y->m_type;
  pair<goto3ac::op,bool> key(go->m_op,T->_signed());
  out << '\t' << m_ifgoto_table[key] << '\t' << '.' << func_label << go->m_to << '\n';
  out << '\t' << "nop" << '\n';
}

void gencode::to(const COMPILER::tac* tac)
{
        out << '.' << func_label << tac << ":\n";
}

int real_suffix(const COMPILER::type* T)
{
  using namespace COMPILER;
  int size = T->size();
  switch ( size ){
  case 4:  return 's';
  case 8:  return 'd';
  default: return 'q';
  }
}

void gencode::ifgoto_real(const COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  address* x = getaddr(tac->y);
  address* y = getaddr(tac->z);
  x->load("%f0");
  y->load("%f4");
  const type* T = tac->y->m_type;
  char suffix = real_suffix(T);
  out << '\t' << "fcmp" << suffix << '\t' << "%f0, %f4" << '\n';
  out << '\t' << "nop" << '\n';
  const goto3ac* go = static_cast<const goto3ac*>(tac);
  pair<goto3ac::op,bool> key(go->m_op,false);
  out << '\t' << 'f' << m_ifgoto_table[key] << '\t' << '.' << func_label << go->m_to << '\n';
  out << '\t' << "nop" << '\n';
}

struct gencode::ifgoto_longlong_table
  : std::map<std::pair<COMPILER::goto3ac::op,bool>, std::vector<std::string> > {
  ifgoto_longlong_table()
  {
    using namespace std;
    using namespace COMPILER;
    typedef pair<goto3ac::op,bool> key;
    {
      vector<string>& vec = (*this)[key(goto3ac::EQ,true)];
      vec.push_back(""); vec.push_back("bne"); vec.push_back("be");
    }
    {
      (*this)[key(goto3ac::EQ,false)] = (*this)[key(goto3ac::EQ,true)];
    }
    {
      vector<string>& vec = (*this)[key(goto3ac::NE,true)];
      vec.push_back("bne"); vec.push_back(""); vec.push_back("bne");
    }
    {
      (*this)[key(goto3ac::NE,false)] = (*this)[key(goto3ac::NE,true)];
    }
    {
      vector<string>& vec = (*this)[key(goto3ac::LT,true)];
      vec.push_back("bl"); vec.push_back("bg"); vec.push_back("bcs");
    }
    {
      vector<string>& vec = (*this)[key(goto3ac::LT,false)];
      vec.push_back("bcs"); vec.push_back("bgu"); vec.push_back("bcs");
    }
    {
      vector<string>& vec = (*this)[key(goto3ac::GT,true)];
      vec.push_back("bg"); vec.push_back("bl"); vec.push_back("bgu");
    }
    {
      vector<string>& vec = (*this)[key(goto3ac::GT,false)];
      vec.push_back("bgu"); vec.push_back("bcs"); vec.push_back("bgu");
    }
    {
      vector<string>& vec = (*this)[key(goto3ac::LE,true)];
      vec.push_back("bl"); vec.push_back("bg"); vec.push_back("bleu");
    }
    {
      vector<string>& vec = (*this)[key(goto3ac::LE,false)];
      vec.push_back("bcs"); vec.push_back("bgu"); vec.push_back("bleu");
    }
    {
      vector<string>& vec = (*this)[key(goto3ac::GE,true)];
      vec.push_back("bg"); vec.push_back("bl"); vec.push_back("bcc");
    }
    {
      vector<string>& vec = (*this)[key(goto3ac::GE,false)];
      vec.push_back("bgu"); vec.push_back("bcs"); vec.push_back("bcc");
    }
  }
};

gencode::ifgoto_longlong_table gencode::m_ifgoto_longlong_table;

void gencode::ifgoto_longlong(const COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  address* x = getaddr(tac->y);
  address* y = getaddr(tac->z);
  x->load("%i0");
  y->load("%i2");
  out << '\t' << "cmp" << '\t' << "%i0, %i2" << '\n';
  out << '\t' << "nop" << '\n';
  const goto3ac* go = static_cast<const goto3ac*>(tac);
  const type* T = tac->y->m_type;
  pair<goto3ac::op,bool> key(go->m_op,T->_signed());
  const vector<string>& op = m_ifgoto_longlong_table[key];
  if ( !op[0].empty() ){
    out << '\t' << op[0] << '\t' << '.' << func_label << go->m_to << '\n';
    out << '\t' << "nop" << '\n';
  }
  string label = new_label();
  if ( !op[1].empty() ){
    out << '\t' << op[1] << '\t' << label << '\n';
    out << '\t' << "nop" << '\n';
  }
  out << '\t' << "cmp" << '\t' << "%i1, %i3" << '\n';
  out << '\t' << "nop" << '\n';
  out << '\t' << op[2] << '\t' << '.' << func_label << go->m_to << '\n';
  out << '\t' << "nop" << '\n';
  out << label << ":\n";
}

void gencode::_alloca_(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* size = getaddr(tac->y);
  size->load("%i0");
  out << '\t' << "sub" << '\t' << "%sp, %i0, %sp" << '\n';
  out << '\t' << "and" << '\t' << "%sp, -8, %sp" << '\n';
  var* entry = tac->x;
  int n = stack_layout.m_local;
  stack_layout.m_allocated.push_back(tac->y);
  address_descriptor[entry] = new stack(-n,tac->y);
}

void gencode::asm_(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const asm3ac* p = static_cast<const asm3ac*>(tac);
  out << '\t' << p->m_inst << '\n';
}

void gencode::_va_start(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->y->m_type;
  int size = T->size();
  size == 16 ? _va_start16(tac) : _va_start_common(tac,size);
}

void gencode::_va_start16(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  stack* s = dynamic_cast<stack*>(y);
  int offset = s->offset();
  out << '\t' << "add" << '\t' << "%fp, " << offset << ", %i0" << '\n';
  out << '\t' << "add" << '\t' << "%i0, 4, %i0" << '\n';
  x->store("%i0");
}

void gencode::_va_start_common(const COMPILER::tac* tac, int size)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  y->get("%i0");
  out << '\t' << "add" << '\t' << "%i0, " << size << ", %i0" << '\n';
  x->store("%i0");
}

void gencode::_va_arg(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  int size = T->size();

  switch ( size ){
  case 8:  _va_arg8(tac);            break;
  case 16: _va_arg16(tac);           break;
  default: _va_arg_common(tac,size); break;
  }
}

void gencode::_va_arg_common(const COMPILER::tac* tac, int size)
{
  using namespace std;
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  y->load("%i0");
  string suffix = address::load_suffix(size);
  out << '\t' << "ld" << suffix << '\t' << "[%i0], %i0" << '\n';
  x->store("%i0");

  y->load("%i0");
  out << '\t' << "add" << '\t' << "%i0, " << size << ", %i0" << '\n';
  y->store("%i0");
}

void gencode::_va_arg8(const COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  string load_suff = address::load_suffix(4);
  string store_suff = address::store_suffix(4);

  y->load("%i0");
  out << '\t' << "ld" << load_suff << '\t' << "[%i0], %i0" << '\n';
  x->get("%i1");
  out << '\t' << "st" << store_suff << '\t' << "%i0, [%i1]" << '\n';

  y->load("%i0");
  out << '\t' << "ld" << load_suff << '\t' << "[%i0+4], %i0" << '\n';
  x->get("%i1");
  out << '\t' << "st" << store_suff << '\t' << "%i0, [%i1+4]" << '\n';

  y->load("%i0");
  out << '\t' << "add" << '\t' << "%i0, 8, %i0" << '\n';
  y->store("%i0");
}

void gencode::_va_arg16(const COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  string load_suff = address::load_suffix(8);
  string store_suff = address::store_suffix(8);

  y->load("%i0");
  out << '\t' << "ld" << '\t' << "[%i0], %i0" << '\n';
  out << '\t' << "ld" << load_suff << '\t' << "[%i0], %i0" << '\n';
  x->store("%i0");
  y->load("%i0");
  out << '\t' << "ld" << '\t' << "[%i0], %i0" << '\n';
  out << '\t' << "ld" << load_suff << '\t' << "[%i0+8], %i0" << '\n';
  x->get("%i2");
  out << '\t' << "st" << store_suff << '\t' << "%i0, [%i2+8]" << '\n';

  y->load("%i0");
  out << '\t' << "add" << '\t' << "%i0, 4, %i0" << '\n';
  y->store("%i0");
}

void gencode::_va_end(const COMPILER::tac*)
{
        // nothing to be done
}
