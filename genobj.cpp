#include "stdafx.h"

#ifdef CXX_GENERATOR
#include "cxx_core.h"
#else  // CXX_GENERATOR
#include "c_core.h"
#endif  // CXX_GENERATOR

#include "sparc.h"

void output_data(const std::pair<std::string, std::vector<COMPILER::usr*> >& pair);

void genobj(const COMPILER::scope* tree)
{
  using namespace std;
  using namespace COMPILER;
  const map<string, vector<usr*> >& usrs = tree->m_usrs;
  for_each(usrs.begin(),usrs.end(),output_data);
  const vector<scope*>& vec = tree->m_children;
  for_each(vec.begin(),vec.end(),genobj);
}

void output_data2(const COMPILER::usr* entry);

void output_data(const std::pair<std::string, std::vector<COMPILER::usr*> >& pair)
{
  using namespace std;
  using namespace COMPILER;
  const vector<usr*>& vec = pair.second;
  for_each(vec.begin(), vec.end(), output_data2);
}

void constant_data(const COMPILER::usr* entry);

void string_literal_data(const COMPILER::usr* entry);

int initial(int offset, const std::pair<int, COMPILER::var*>&);

int static_counter;

void output_data2(const COMPILER::usr* entry)
{
  using namespace std;
  using namespace COMPILER;
  usr::flag_t flag = entry->m_flag;
  if (flag & usr::TYPEDEF)
    return;
  if (address_descriptor.find(entry) != address_descriptor.end())
    return;

  usr::flag_t mask = usr::flag_t(usr::EXTERN | usr::FUNCTION);
  if (flag & mask) {
    address_descriptor[entry] = new mem(entry);
    return;
  }

  if ( entry->isconstant() )
    return constant_data(entry);

  if ( string_literal(entry) )
    return string_literal_data(entry);

  if (!is_top(entry->m_scope) && !(flag & usr::STATIC))
    return;

  string label = entry->m_name;
  const type* T = entry->m_type;
  output_section(T->modifiable() ? ram : rom);

  if ( flag & usr::STATIC ){
    if ( !is_top(entry->m_scope) ){
      ostringstream os;
      os << '.' << static_counter++;
      label += os.str();
    }
  }
  else
    out << '\t' << ".global" << '\t' << label << '\n';

  out << '\t' << ".align" << '\t' << T->align() << '\n';
  if ( flag & usr::WITH_INI ){
    const with_initial* wi = static_cast<const with_initial*>(entry);
    out << label << ":\n";
    const map<int, var*>& value = wi->m_value;
    if (int n = T->size() - accumulate(value.begin(), value.end(), 0, initial))
      out << '\t' << ".space" << '\t' << n << '\n';
  }
  else {
    int size = T->size();
    int align = T->align();
    out << '\t' << ".comm" << '\t' << label << ", ";
    out << size << ", " << align << '\n';
  }
  address_descriptor[entry] = new mem(entry);
}

void constant_data(const COMPILER::usr* entry)
{
  using namespace std;
  using namespace COMPILER;
  const type* T = entry->m_type;
  if ( T->real() || T->size() > 4 ){
    output_section(rom);
    out << '\t' << ".align" << '\t' << T->align() << '\n';
    string label = new_label();
    out << label << ":\n";
    pair<int, var*> tmp(0, (var*)entry);
    initial(0, tmp);
    address_descriptor[entry] = new mem(entry, label);
  }
  else
    address_descriptor[entry] = new imm(entry);
}

void string_literal_data(const COMPILER::usr* entry)
{
  using namespace std;
  output_section(rom);
  string label = new_label();
  out << label << ":\n";
  string name = entry->m_name;
  name.erase(name.size()-1);
  name += "\\0\"";
  out << '\t' << ".ascii" << '\t' << name << '\n';
  address_descriptor[entry] = new mem(entry, label);
}

int initial_real(int offset, COMPILER::var*);

int initial_notreal(int offset, COMPILER::var*);

int initial(int offset, const std::pair<int, COMPILER::var*>& pair)
{
  using namespace std;
  using namespace COMPILER;
  if (int n = pair.first - offset) {
    out << '\t' << ".space" << '\t' << n << '\n';
    offset = pair.first;
  }
  var* v = pair.second;
  const type* T = v->m_type;
  if ( T->real() )
    return initial_real(offset, v);
  if ( T->scalar() )
    return initial_notreal(offset, v);
  usr* entry = v->usr_cast();
  assert(entry && string_literal(entry));
  string name = entry->m_name;
  name.erase(name.size()-1);
  name += "\\0\"";
  out << '\t' << ".ascii" << '\t' << name << '\n';
  return offset + name.size() + 1;
}

int initial_notreal(int offset, COMPILER::var* v)
{
  using namespace std;
  using namespace COMPILER;
  const type* T = v->m_type;
  int size = T->size();
  out << '\t';
  switch (size) {
  case 1: out << ".byte"; break;
  case 2: out << ".half"; break;
  case 4: out << ".long"; break;
  }
  usr* entry = v->usr_cast();
  if (entry && size <= 4) {
    assert(entry->isconstant());
    switch (size) {
    case 1:
      {
	typedef constant<char> X;
	X* x = static_cast<X*>(entry);
	out << '\t' << int(x->m_value) << '\n';
	break;
      }
    case 2:
      {
	typedef constant<short> X;
	X* x = static_cast<X*>(entry);
	out << '\t' << int(x->m_value) << '\n';
	break;
      }
    default:
      {
	assert(size == 4);
	typedef constant<int> X;
	X* x = static_cast<X*>(entry);
	out << '\t' << x->m_value << '\n';
	break;
      }
    }
    return offset + size;
  }
  if (addrof* addr = v->addrof_cast()) {
    var* v = addr->m_ref;
    usr* u = v->usr_cast();
    assert(u);
    int off = addr->m_offset;
    out << '\t' << u->m_name;
    if (off)
      out << "+" << off;
    return offset + addr->m_type->size();
  }

  union {
    unsigned __int64 ll;
    int i[2];
  } tmp = { strtoull(entry->m_name.c_str(),0,0) };

  int i = 1;
  if ( *(char*)&i )
    swap(tmp.i[0],tmp.i[1]);
  out << '\t' << ".long" << '\t' << tmp.i[0] << '\n';
  out << '\t' << ".long" << '\t' << tmp.i[1] << '\n';
  return offset + size;
}

void long_double_bit(int [4], const COMPILER::usr*);

int initial_real(int offset, COMPILER::var* v)
{
        using namespace std;
        using namespace COMPILER;
        usr* entry = v->usr_cast();
        assert(entry);
        const type* T = entry->m_type;
        int size = T->size();
        if ( size == 4 ){
                union {
                  float f;
                  int i;
                } tmp = { (float)atof(entry->m_name.c_str()) };
                ostringstream os;
                out << '\t' << ".long" << '\t' << tmp.i;
                return offset + size;
        }
        
        if ( size == 8 ){
                union {
                        double d;
                        int i[2];
                } tmp = { atof(entry->m_name.c_str()) };
                int i = 1;
                if ( *(char*)&i )
                        swap(tmp.i[0],tmp.i[1]);
                out << '\t' << ".long" << '\t' << tmp.i[0] << '\n';
                out << '\t' << ".long" << '\t' << tmp.i[1] << '\n';
            return offset + size;
        }

        union {
                long double ld;
                int i[4];
        } tmp;
        if ( sizeof(long double) == 16 ){
                tmp.ld = atof(entry->m_name.c_str());
                int i = 1;
                if (*(char*)&i) {
                        swap(tmp.i[0], tmp.i[3]);
                        swap(tmp.i[1], tmp.i[2]);
                }
        }
        else
                long_double_bit(&tmp.i[0],entry);
        for ( int i = 0 ; i != 4 ; ++i )
                out << '\t' << ".long" << '\t' << tmp.i[i] << '\n';
        return offset + size;
}

/*
  IEEE 754 128 bit 浮動小数点 x のビット表現 b = (b0 ... b127)
  x = (-1)^b0 2^{e-16383} (1+ \sum_{i=16}^{127} (b_i/2^{i-15}))
  e = \sum_{i=1}^{15} 2^{15-i} b_i
*/
void long_double_bit(int res[4], const COMPILER::usr* entry)
{
  using namespace std;
  memset(&res[0],0,sizeof(int)*4);
  union {
    double d;
    unsigned __int64 ll;
    int i[2];
  } tmp = { atof(entry->m_name.c_str()) };
  if ( !tmp.d )
    return;

  if ( tmp.d < 0 ){
    res[0] = 1 << 31;
    tmp.d = -tmp.d;
  }

  int n = log(tmp.d)/log(2) + 16383;

  int i;
  for ( i = 1 ; i < 16 ; ++i ){
    int m = 1 << (15-i);
    if ( m <= n ){
      res[0] |= m << 16;
      n -= m;
    }
  }

  union {
    unsigned __int64 frac;
    unsigned short int half[4];
  } tmp2 = { tmp.ll << 12 };

  i = 1;
  if ( *(char*)&i ){
    swap(tmp2.half[0],tmp2.half[3]);
    swap(tmp2.half[1],tmp2.half[2]);
  }

  res[0] |= tmp2.half[0];
  res[1] = tmp2.half[1] << 16 | tmp2.half[2];
  res[2] = tmp2.half[3] << 16;
}

#ifdef CXX_GENERATOR
struct cxxbuiltin_table : std::map<std::string, std::string> {
  cxxbuiltin_table()
  {
    (*this)["new"] = "_Znwj";
    (*this)["delete"] = "_ZdlPv";

    (*this)["new.a"] = "_Znaj";
    (*this)["delete.a"] = "_ZdaPv";
  }
} cxxbuiltin_table;

bool cxxbuiltin(const COMPILER::usr* entry, std::string* res)
{
  std::string name = entry->m_name;
  std::map<std::string, std::string>::const_iterator p =
    cxxbuiltin_table.find(name);
  if ( p != cxxbuiltin_table.end() ){
    *res = p->second;
    return true;
  }
  else
    return false;
}

std::string scope_name(COMPILER::scope* scope)
{
  if ( tag* T = dynamic_cast<tag*>(scope) )
    return scope_name(scope->m_parent) + func_name(T->name());
  if ( _namespace* nmsp = dynamic_cast<_namespace*>(scope) )
    return nmsp->name();
  return "";
}

class conv_fname {
  std::string* m_res;
public:
  conv_fname(std::string* res) : m_res(res) {}
  void operator()(int c)
  {
    switch ( c ){
    case ' ':   *m_res += "sp"; break;
    case '~':   *m_res += "ti"; break;
    case '[':   *m_res += "lb"; break;
    case ']':   *m_res += "rb"; break;
    case '(':   *m_res += "lp"; break;
    case ')':   *m_res += "rp"; break;
    case '&':   *m_res += "an"; break;
    case '<':   *m_res += "lt"; break;
    case '>':   *m_res += "gt"; break;
    case '=':   *m_res += "eq"; break;
    case '!':   *m_res += "ne"; break;
    case '+':   *m_res += "pl"; break;
    case '-':   *m_res += "mi"; break;
    case '*':   *m_res += "mu"; break;
    case '/':   *m_res += "di"; break;
    case '%':   *m_res += "mo"; break;
    case '^':   *m_res += "xo"; break;
    case '|':   *m_res += "or"; break;
    case ',':   *m_res += "cm"; break;
    case '\'':  *m_res += "da"; break;
    default:    *m_res += char(c); break;
    }
  }
};

std::string func_name(std::string name)
{
  std::string res;
  std::for_each(name.begin(),name.end(),conv_fname(&res));
  return res;
}

void entry_func_sub(const COMPILER::usr* func)
{
  if ( func->m_templ_func )
    return;
  if ( address_descriptor.find(func) == address_descriptor.end() ){
    std::string label;
    if ( !cxxbuiltin(func,&label) ){
      label = scope_name(func->m_scope);
      label += func_name(func->m_name);
      if ( !func->m_cCOMPILER::usr )
        label += signature(func->m_type);
    }
    address_descriptor[func] = new mem(label,func->m_type);
  }
}

void entry_func(const std::pair<std::string,std::vector<COMPILER::usr*> >& pair)
{
  const std::vector<COMPILER::usr*>& vec = pair.second;
  std::for_each(vec.begin(),vec.end(),entry_func_sub);
}
#endif // CXX_GENERATOR
