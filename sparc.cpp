#include "stdafx.h"

#ifdef CXX_GENERATOR
#include "cxx_core.h"
#else  // CXX_GENERATOR
#include "c_core.h"
#endif  // CXX_GENERATOR

#include "sparc.h"

std::ostream out(std::cout.rdbuf());

std::ofstream* ptr_out;

bool debug_flag;

extern "C" DLL_EXPORT int generator_seed()
{
#ifdef _MSC_VER
	int r = _MSC_VER;
#ifndef CXX_GENERATOR
	r += 10000000;
#else // CXX_GENERATOR
	r += 20000000;
#endif // CXX_GENERATOR
#ifdef WIN32
	r += 100000;
#endif // WIN32
#endif // _MSC_VER
#ifdef __GNUC__
	int r = (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__);
#ifndef CXX_GENERATOR
	r += 30000000;
#else // CXX_GENERATOR
	r += 40000000;
#endif // CXX_GENERATOR
#endif // __GNUC__
	return r;
}

void output_if(tc_pattern*);

extern "C" DLL_EXPORT void generator_generate(COMPILER::generator::interface_t* ptr)
{
  using namespace std;
  genobj(ptr->m_root);
  if ( ptr->m_func )
    genfunc(ptr->m_func,*ptr->m_code);

  const vector<tc_pattern*>& vec = tc_pattern::m_all;
  for_each(vec.begin(),vec.end(),output_if);
  delete tmp_dword;
  tmp_dword = 0;
  delete tmp_word;
  tmp_word = 0;
  delete tmp_quad[0];
  tmp_quad[0] = 0;
  delete tmp_quad[1];
  tmp_quad[1] = 0;
}

void output_if(tc_pattern* pattern)
{
  if ( !pattern->m_label.empty() ){
    if ( !pattern->m_done ){
      pattern->m_done = true;
      pattern->output();
    }
  }
}

class generator_option_table : public std::map<std::string, int (*)()> {
  static int set_debug_flag()
  {
    debug_flag = true;
    return 0;
  }
public:
  generator_option_table()
  {
    (*this)["--debug"] = set_debug_flag;
  }
} generator_option_table;

int option_handler(const char* option)
{
  using namespace std;
  map<string, int (*)()>::const_iterator p =
    generator_option_table.find(option);
  return p != generator_option_table.end() ? (p->second)() : 1;
}

extern "C" DLL_EXPORT void generator_option(int argc, const char** argv, int* error)
{
	++argv;
	--argc;
#ifdef _MSC_VER
	for (const char** p = &argv[0]; p != &argv[argc]; ++p, ++error)
		*error = option_handler(*p);
#else // _MSC_VER
    using namespace std;
    transform(&argv[0],&argv[argc],&error[0],option_handler);
#endif // _MSC_VER
}

extern "C" DLL_EXPORT int generator_open_file(const char* fn)
{
  ptr_out = new std::ofstream(fn);
  out.rdbuf(ptr_out->rdbuf());
  return 0;
}

inline void destroy_address(std::pair<const COMPILER::var*, address*> pair)
{
#ifndef CXX_GENERATOR
	delete pair.second;
#else // CXX_GENERATOR
	const storage_class* storage = pair.first->m_storage;
	typedef const enum_member_class MEM;
	typedef const anonymous_class ANONYMOUS;
    bool b = !dynamic_cast<MEM*>(storage) && !dynamic_cast<ANONYMOUS*>(storage);
	if (b)
		delete pair.second;
#endif // CXX_GENERATOR
}

#ifdef CXX_GENERATOR
inline void call_simply(std::string func)
{
  out << '\t' << "call" << '\t' << func << '\n';
  out << '\t' << "nop" << '\n';
}
#endif // CXX_GENERATOR

extern "C" DLL_EXPORT int generator_close_file()
{
  using namespace std;
#ifdef CXX_GENERATOR
  if ( !ctors.empty() ){
    output_section(rom);
    out << '\t' << ".align" << '\t' << 4 << '\n';
    std::string label = new_label();
    out << label << ":\n";
    std::for_each(ctors.begin(),ctors.end(),call_simply);
    out << '\t' << "ret" << '\n';
    out << '\t' << "nop" << '\n';
    output_section(ctor);
    out << '\t' << ".long" << '\t' << label << '\n';
  }
  if ( !dtor_name.empty() ){
    output_section(rom);
    out << '\t' << ".align" << '\t' << 4 << '\n';
    std::string label = new_label();
    out << label << ":\n";
    call_simply(dtor_name);
    out << '\t' << "ret" << '\n';
    out << '\t' << "nop" << '\n';
    output_section(dtor);
    out << '\t' << ".long" << '\t' << label << '\n';
  }
#endif // CXX_GENERATOR
  delete ptr_out;
  for_each(address_descriptor.begin(), address_descriptor.end(),destroy_address);
  return 0;
}

void(*output3ac)(std::ostream&, const COMPILER::tac*);

extern "C" DLL_EXPORT void generator_spell(void* arg)
{
	using namespace std;
	using namespace COMPILER;
	void* magic[] = {
		((char **)arg)[0],
	};
	int index = 0;
	memcpy(&output3ac, &magic[index++], sizeof magic[0]);
}

void output_section(section kind)
{
  static section current;
  if ( current != kind ){
    current = kind;
    switch ( kind ){
    case rom: out << '\t' << ".text" << '\n'; break;
    case ram: out << '\t' << ".data" << '\n'; break;
    case ctor:
    case dtor:
      out << '\t' << ".section" << '\t' << '"';
      out << (kind == ctor ? ".ctors" : ".dtors");
      out << '"' << ",#alloc,#write" << '\n';
      break;
    }
  }
}

bool character_constant(const COMPILER::usr* entry)
{
  int c = entry->m_name[0];
  if ( c == '\'' )
    return true;

  if ( c == 'L' ){
    c = entry->m_name[1];
    if ( c == '\'' )
      return true;
  }
  return false;
}


bool string_literal(const COMPILER::usr* entry)
{
  int c = entry->m_name[0];
  if ( c == 'L' )
    c = entry->m_name[1];
  return c == '"';
}

std::map<const COMPILER::var*, address*> address_descriptor;

std::string address::load_suffix(int size)
{
  switch ( size ){
  case 1: return "ub";
  case 2: return "uh";
  case 4: return "";
  case 8: return "d";
  default: throw bad_size(size);
  }
}

std::string address::store_suffix(int size)
{
  switch ( size ){
  case 1: return "b";
  case 2: return "h";
  case 4: return "";
  case 8: return "d";
  default: throw bad_size(size);
  }
}

imm::imm(const COMPILER::usr* entry) : m_pair(std::make_pair(0,0))
{
  if ( entry->m_name == "'\\0'" )
    m_expr = "0";
  else if ( entry->m_name == "'\\a'" ){
    std::ostringstream os;
    os << int('\a');
    m_expr = os.str();
  }
  else if ( entry->m_name == "'\\b'" ){
    std::ostringstream os;
    os << int('\b');
    m_expr = os.str();
  }
  else if ( entry->m_name == "'\\t'" ){
    std::ostringstream os;
    os << int('\t');
    m_expr = os.str();
  }
  else if ( entry->m_name == "'\\n'" ){
    std::ostringstream os;
    os << int('\n');
    m_expr = os.str();
  }
  else if ( entry->m_name == "'\\v'" ){
    std::ostringstream os;
    os << int('\v');
    m_expr = os.str();
  }
  else if ( entry->m_name == "'\\r'" ){
    std::ostringstream os;
    os << int('\r');
    m_expr = os.str();
  }
  else if ( character_constant(entry) )
    m_expr = entry->m_name;
#ifdef CXX_GENERATOR
  else if ( entry->m_name == "true" )
    m_expr = "1";
  else if ( entry->m_name == "false" )
    m_expr = "0";
#endif // CXX_GENERATOR
  else {
    m_expr = entry->m_name;
    while ( integer_suffix(m_expr[m_expr.size()-1]) )
      m_expr.erase(m_expr.size()-1);
    int sign;
    if ( m_expr[0] == '-' ){
      sign = -1;
      m_expr.erase(0,1);
    }
    else
      sign = 1;
    std::istringstream is(m_expr.c_str());
    unsigned int n;
    if ( m_expr[0] == '0' && m_expr.size() > 1 ){
      if ( m_expr[1] == 'x' || m_expr[1] == 'X' )
        is >> std::hex >> n;
      else
        is >> std::oct >> n;
    }
    else
      is >> n;
    n *= sign;
    int m = int(n) << 19 >> 19;
    if ( n != m ){
      m_expr.erase();
      m_pair.first = n & ~0x3ff;
      m_pair.second = n & 0x3ff;
    }
    else if ( sign == -1 )
      m_expr = '-' + m_expr;
  }
}

void imm::load(std::string reg, int) const
{
  if ( !m_expr.empty() )
    out << '\t' << "mov" << '\t' << m_expr << ", " << reg << '\n';
  else {
    std::string tmp;
    if ( reg[1] == 'f' )
      tmp = "%i0";
    else
      tmp = reg;
    out << '\t' << "sethi" << '\t' << "%hi(" << m_pair.first << "), " << tmp << '\n';
    out << '\t' << "or" << '\t' << tmp << ", " << m_pair.second << ", " << tmp << '\n';
    if ( reg[1] == 'f' ){
      tmp_word->store(tmp);
      tmp_word->load(reg);
    }
  }
}

stack::stack(int offset, int size)
  : m_offset(offset), m_size(size), m_unpromed(0), m_allocated(0)

{
  assert(offset < 0);
}

stack::stack(int offset, int size, int unpromed)
  : m_offset(offset), m_size(size), m_unpromed(unpromed), m_allocated(0)
{
  assert(offset > 0);
}

stack::stack(int offset, const COMPILER::var* allocated)
  : m_offset(offset), m_size(-1), m_unpromed(-1), m_allocated(allocated)
{
  assert(offset < 0);
}

void stack::load(std::string reg, int size) const
{
  if ( size ){
    std::string suffix = load_suffix(size);
    out << '\t' << "ld" << suffix << '\t';
    if ( m_offset < 0 )
      out << '[' << "%fp" <<        m_offset + m_size - size << ']';
    else
      out << '[' << "%fp" << '+' << m_offset + m_size - size << ']';
    out << ", " << reg << '\n';
  }
  else {
    if ( m_unpromed < 0 ){
      assert(m_offset > 0);
      std::string tmp = "%g1";
      out << '\t' << "ld" << '\t' << "[%fp+" << m_offset << "], " << tmp << '\n';
      out << '\t' << "ldd" << '\t' << '[' << tmp << "], " << reg << '\n';
      char c = reg[reg.size()-1];
      reg.erase(reg.size()-1); 
      reg += c + 4;
      out << '\t' << "ldd" << '\t' << '[' << tmp << "+8], " << reg << '\n';
    }
    else if ( m_offset < 0 || !(m_offset % m_size) ){
      int size = m_size;
      if ( m_size == 16 )
	size = 8;
      std::string suffix = load_suffix(size);
      std::string tmp;
      if ( -4096 <= m_offset ){
	out << '\t' << "ld" << suffix << '\t' << '[' << "%fp";
	if ( m_offset > 0 )
	  out << '+';
	out << m_offset << "], " << reg << '\n';
      }
      else {
	tmp = "%g1";
	out << '\t' << "sethi" << '\t' << "%hi(" << m_offset << "), " << tmp << '\n';
	out << '\t' << "or" << '\t' << tmp << ", %lo(" << m_offset << "), " << tmp << '\n';
	out << '\t' << "add" << '\t' << "%fp, " << tmp << ", " << tmp << '\n';
	out << '\t' << "ld" << suffix << '\t' << '[' << tmp << "], " << reg << '\n';
      }
      if ( m_size == 16 ){
	char c = reg[reg.size()-1];
	reg.erase(reg.size()-1);
	reg += c + 4;
	if ( -4096 <= m_offset ){ 
	  out << '\t' << "ld" << suffix << '\t' << '[' << "%fp";
	  if ( m_offset > 0 )
	    out << '+';
	  out << m_offset + 8 << "], " << reg << '\n';
	}
	else {
	  out << '\t' << "add" << '\t' << "8, " << tmp << ", " << tmp << '\n';
	  out << '\t' << "ld" << suffix << '\t' << '[' << tmp << "], ";
	  out << reg << '\n';
	}
      }
    }
    else {
      assert(m_size == 8);
      out << '\t' << "ld" << '\t' << "[%fp+" << m_offset     << "], " << reg << '\n';
      char c = reg[reg.size()-1];
      reg.erase(reg.size()-1);
      reg += ++c;
      out << '\t' << "ld" << '\t' << "[%fp+" << m_offset + 4 << "], " << reg << '\n';
    }
  }
}

void stack::_store(std::string reg, bool save_flag) const
{
  if ( m_offset < 0 ){
    int size = m_size;
    if ( m_size == 16 )
      size = 8;
    std::string suffix = store_suffix(size);
    std::string tmp;
    if ( -4096 <= m_offset ){
      out << '\t' << "st" << suffix << '\t' << reg << ", ";
      out << '[' << "%fp" << m_offset << ']' << '\n';
    }
    else {
      tmp = reg == "%i0" ? "%o0" : "%i0";
      out << '\t' << "sethi" << '\t' << "%hi(" << m_offset << "), " << tmp << '\n';
      out << '\t' << "or" << '\t' << tmp << ", %lo(" << m_offset << "), " << tmp << '\n';
      out << '\t' << "add" << '\t' << "%fp, " << tmp << ", " << tmp << '\n';
      out << '\t' << "st" << suffix << '\t' << reg << ", ";
      out << '[' << tmp << ']' << '\n';
    }

    if ( m_size == 16 ){
      char c = reg[reg.size()-1];
      reg.erase(reg.size()-1);
      reg += c + 4;
      if ( -4096 <= m_offset ){
	out << '\t' << "st" << suffix << '\t' << reg << ", ";
	out << '[' << "%fp" << m_offset + 8 << ']' << '\n';
      }
      else {
	out << '\t' << "st" << suffix << '\t' << reg << ", ";
      out << '\t' << "add" << '\t' << "8, " << tmp << ", " << tmp << '\n';
	out << '[' << tmp << ']' << '\n';
      }
    }
  }
  else {
    if ( m_unpromed < 0 && !save_flag ){
      std::string tmp = "%g1";
      out << '\t' << "ld" << '\t' << "[%fp+" << m_offset << "], " << tmp << '\n';
      out << '\t' << "std" << '\t' << reg << ", [" << tmp << "]\n";
      char c = reg[reg.size()-1];
      reg.erase(reg.size()-1);
      reg += c + 4;
      out << '\t' << "std" << '\t' << reg << ", [" << tmp << "+8]\n";
    }
    else {
      out << '\t' << "st" << '\t' << reg << ", ";
      out << '[' << "%fp" << '+' << m_offset << ']' << '\n';
      if ( should_store_high(m_size,save_flag,m_offset) ){
	char index = reg[reg.size()-1];
	reg.erase(reg.size()-1);
	reg += ++index;
	out << '\t' << "st" << '\t' << reg << ", ";
	out << '[' << "%fp" << '+' << m_offset + 4 << ']' << '\n';
      }
    }
  }
}

bool stack::should_store_high(int size, bool save_flag, int offset)
{
  if ( size != 8 )
    return false;
  if ( !save_flag )
    return true;
  return offset != 88;
}

class add_allocated {
  std::string m_reg;
  std::string m_tmp;
public:
  add_allocated(std::string reg)
    : m_reg(reg), m_tmp(m_reg == "%i0" ? "%i1" : "%i0") {}
  void operator()(COMPILER::var* size)
  {
    address* z = getaddr(size);
    z->load(m_tmp);
    out << '\t' << "sub" << '\t' << m_reg << ", " << m_tmp << ", " << m_reg << '\n';
    out << '\t' << "and" << '\t' << m_reg << ", -8, " << m_reg << '\n';
  }
};

void stack::get(std::string reg) const
{
  using namespace std;
  using namespace COMPILER;
  if ( m_unpromed < 0 ){
    if ( m_allocated ){
      out << '\t' << "add" << '\t' << "%fp" << ", " << m_offset << ", " << reg << '\n';
      vector<var*>& vec = stack_layout.m_allocated;
      vector<var*>::iterator p = find(vec.begin(),vec.end(),m_allocated);
      for_each(vec.begin(),++p,add_allocated(reg));
    }
    else {
      out << '\t' << "add" << '\t' << "%fp" << ", " << m_offset << ", " << reg << '\n';
      out << '\t' << "ld" << '\t' << '[' << reg << "], " << reg << '\n';
    }
  }
  else {
    int offset = m_offset;
    if ( m_offset > 0 )
      offset += m_size - m_unpromed;

    if ( -4096 <= offset && offset < 4096 )
      out << '\t' << "add" << '\t' << "%fp" << ", " << offset << ", " << reg << '\n';
    else {
      out << '\t' << "mov" << '\t' << "%fp, " << reg << '\n';
      std::string tmp = "%g1";
      out << '\t' << "sethi" << '\t' << "%hi(" << offset << "), " << tmp << '\n';
      out << '\t' << "or" << '\t' << tmp << ", %lo(" << offset << "), " << tmp << '\n';
      out << '\t' << "add" << '\t' << reg << ", " << tmp << ", " << reg << '\n';
    }
  }
}

void mem::load(std::string reg, int size) const
{
	using namespace std;
	using namespace COMPILER;
	const type* T = m_usr->m_type;
	int Tsz = T->size();
	string ireg = reg;
	if ( ireg[1] == 'f' )
		ireg = "%i0";
	if ( Tsz == 16 ){
		if ( ireg[1] == 'i' )
			ireg[1] = 'o';
		else
			ireg[1] = 'i';
	}
	get(ireg);
	if ( size ){
		string suffix = load_suffix(size);
		out << '\t' << "ld" << suffix << '\t';
		out << '[' << ireg << '+' << Tsz - size << "], " << reg << '\n';
	}
	else {
		int size = Tsz;
		if ( Tsz == 16 )
			size = 8;
		string suffix = load_suffix(size);
		out << '\t' << "ld" << suffix << '\t';
		out << '[' << ireg << "], " << reg << '\n';
		if ( Tsz == 16 ){
			char c = reg[reg.size()-1];
			reg.erase(reg.size()-1);
			reg += c + 4;
			out << '\t' << "ld" << suffix << '\t';
			out << '[' << ireg << "+8], " << reg << '\n';
		}
	}
}

void mem::store(std::string reg) const
{
	using namespace std;
	using namespace COMPILER;
	const type* T = m_usr->m_type;
	int Tsz = T->size();
	int size = Tsz;
	if ( Tsz == 16 )
		size = 8;
	string suffix = store_suffix(size);
	string ireg = reg == "%i0" ? "%o0" : "%i0";
	get(ireg);
	out << '\t' << "st" << suffix << '\t' << reg << ", [" << ireg << "]\n";
	if ( Tsz == 16 ){
		char c = reg[reg.size()-1];
		reg.erase(reg.size()-1);
		reg += c + 4;
		out << '\t' << "st" << suffix << '\t' << reg << ", [" << ireg << "+8]\n";
	}
}

void mem::get(std::string reg) const
{
	using namespace std;
	string label = m_label.empty() ? m_usr->m_name : m_label;
	out << '\t' << "sethi" << '\t' << "%hi(" << label << "), " << reg << '\n';
	out << '\t' << "or"    << '\t' << reg << ", %lo(" << label << "), " << reg << '\n';
}

struct function_exit function_exit;

std::string new_label()
{
	using namespace std;
	static int cnt;
	ostringstream os;
	os << ".L" << ++cnt;
	return os.str();
}

std::vector<tc_pattern*> tc_pattern::m_all;

struct double_0x80000000 double_0x80000000;

void double_0x80000000::output() const
{
	// (double)0x80000000 のビット表現
	output_section(rom);
	out << '\t' << ".align" << '\t' << 8 << '\n';
	out << m_label << ":\n";
	out << '\t' << ".long" << '\t' << 1105199104 << '\n';
	out << '\t' << ".long" << '\t' << 0 << '\n';
}

struct double_0x100000000 double_0x100000000;

void double_0x100000000::output() const
{
	// (double)0x100000000 のビット表現
	output_section(rom);
	out << '\t' << ".align" << '\t' << 8 << '\n';
	out << m_label << ":\n";
	out << '\t' << ".long" << '\t' << 1106247680 << '\n';
	out << '\t' << ".long" << '\t' << 0 << '\n';
}

struct long_double_0x100000000 long_double_0x100000000;

void long_double_0x100000000::output() const
{
  // (long double)0x100000000 のビット表現
  output_section(rom);
  out << '\t' << ".align" << '\t' << 16 << '\n';
  out << m_label << ":\n";
  out << '\t' << ".long" << '\t' << 1075773440 << '\n';
  out << '\t' << ".long" << '\t' << 0 << '\n';
  out << '\t' << ".long" << '\t' << 0 << '\n';
  out << '\t' << ".long" << '\t' << 0 << '\n';
}

struct long_double_0x10000000000000000 long_double_0x10000000000000000;

void long_double_0x10000000000000000::output() const
{
  // (long double)0x10000000000000000 のビット表現
  output_section(rom);
  out << '\t' << ".align" << '\t' << 16 << '\n';
  out << m_label << ":\n";
  out << '\t' << ".long" << '\t' << 1077870592 << '\n';
  out << '\t' << ".long" << '\t' << 0 << '\n';
  out << '\t' << ".long" << '\t' << 0 << '\n';
  out << '\t' << ".long" << '\t' << 0 << '\n';
}

stack* tmp_dword;
stack* tmp_word;
stack* tmp_quad[2];

bool integer_suffix(int c)
{
  return c == 'U' || c == 'L' || c == 'u' || c == 'l';
}

#ifdef CXX_GENERATOR
std::string signature(const type_expr* type)
{
#ifdef _MSC_VER
  use_ostream_magic obj;
#endif // _MSC_VER
  std::ostringstream os;
  type->encode(os);
  return func_name(os.str());
}
#endif // CXX_GENERATOR
