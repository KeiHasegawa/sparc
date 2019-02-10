#include "stdafx.h"

#ifdef CXX_GENERATOR
#include "cxx_core.h"
#else  // CXX_GENERATOR
#include "c_core.h"
#endif  // CXX_GENERATOR

#include "sparc.h"
#include "gencode.h"

void enter(const COMPILER::fundef* fundef, const std::vector<COMPILER::tac*>&);
void leave();

#ifdef CXX_GENERATOR
std::vector<std::string> ctors;
std::string dtor_name;
struct anonymous_table anonymous_table;
#endif // CXX_GENERATOR

std::string func_label;

void genfunc(const COMPILER::fundef* fundef, const std::vector<COMPILER::tac*>& v3ac)
{
  using namespace std;
  using namespace COMPILER;
  output_section(rom);
  usr* func = fundef->m_usr;
#ifndef CXX_GENERATOR
  func_label = func->m_name;
#else // CXX_GENERATOR
  func_label = scope_name(func->m_scope);
  func_label += func_name(func->m_name);
#endif // CXX_GENERATOR
  usr::flag_t flag = func->m_flag;
  usr::flag_t mask = usr::flag_t(usr::STATIC | usr::INLINE);
  if ( !(flag & mask) )
    out << '\t' << ".global" << '\t' << func_label << '\n';
  out << '\t' << ".align" << '\t' << 4 << '\n';
  out << func_label << ":\n";
  enter(fundef,v3ac);
  function_exit.m_label = new_label();
  function_exit.m_ref = false;
  if ( !v3ac.empty() )
    for_each(v3ac.begin(),v3ac.end(),gencode(v3ac));
  leave();
}

int sched_stack(const COMPILER::fundef*, const std::vector<COMPILER::tac*>&);

void save_parameter(const COMPILER::fundef*);

void enter(const COMPILER::fundef* fundef, const std::vector<COMPILER::tac*>& v3ac)
{
  if ( debug_flag )
    out << '\t' << "/* enter */" << '\n';
  out << '\t' << "!#PROLOGUE# 0" << '\n';
  int n = sched_stack(fundef,v3ac);
  if ( int m = n % 8 )
    n += 8 - m;

  if ( n <= 4096 )
    out << '\t' << "save" << '\t' << "%sp, " << -n << ", %sp" << '\n';
  else {
    out << '\t' << "sethi" << '\t' << "%hi(" << -n << "), %g1" << '\n';
    out << '\t' << "or" << '\t' << "%g1, %lo(" << -n << "), %g1" << '\n';
    out << '\t' << "save" << '\t' << "%sp, %g1, %sp" << '\n';
  }

  out << '\t' << "!#PROLOGUE# 1" << '\n';
  save_parameter(fundef);
}

bool is_top(const COMPILER::scope* ptr)
{
  using namespace COMPILER;
#ifndef CXX_GENERATOR
  return !ptr->m_parent;
#else // CXX_GENERATOR
  if ( !ptr->m_parent )
    return true;
  if ( ptr->m_id == scope::TAG )
    return is_top(ptr->m_parent);
  if ( ptr->m_id == scope::NAMESPACE )
    return is_top(ptr->m_parent);
  return false;
#endif // CXX_GENERATOR
}

void clear_address_descriptor()
{
  using namespace std;
  using namespace COMPILER;
  typedef map<const var*, address*>::iterator IT;
  for ( IT p = address_descriptor.begin() ; p != address_descriptor.end() ; ){
    const var* entry = p->first;
    scope* scope = entry->m_scope;
    if ( is_top(scope) )
      ++p;
    else {
      IT q = p++;
          delete q->second;
          address_descriptor.erase(q);
    }
  }
}

inline void destroy(const std::pair<const COMPILER::var*, stack*>& pair)
{
  delete pair.second;
}

void clear_record_param()
{
        using namespace std;
        for_each(record_param.begin(),record_param.end(),destroy);
        record_param.clear();
}

void leave()
{
  if ( debug_flag )
    out << '\t' << "/* leave */" << '\n';
  if ( function_exit.m_ref )
    out << function_exit.m_label << ":\n";
  out << '\t' << "nop" << '\n';
  out << '\t' << "ret" << '\n';
  out << '\t' << "restore" << '\n';

  clear_address_descriptor();
#ifdef CXX_GENERATOR
  anonymous_table.clear();
#endif // CXX_GENERATOR
}

int local_variable(const COMPILER::fundef*);
int fun_arg(const std::vector<COMPILER::tac*>&);

struct stack_layout stack_layout;

int sched_stack(const COMPILER::fundef* fundef, const std::vector<COMPILER::tac*>& v3ac)
{
  stack_layout.m_local = local_variable(fundef);
  stack_layout.m_allocated.clear();
  return stack_layout.m_local + fun_arg(v3ac) + aggre_regwin;
}

int parameter(int offset, COMPILER::usr* entry);

class recursive_locvar {
  int* m_res;
  static int add(int, const std::pair<std::string, std::vector<COMPILER::usr*> >&);
  static int add2(int n, COMPILER::usr* entry);
  static int add3(int n, COMPILER::var* v);
public:
  recursive_locvar(int* p) : m_res(p) {}
  void operator()(const COMPILER::scope*);
};

int align(int, int);

int local_variable(const COMPILER::fundef* fundef)
{
  using namespace std;
  using namespace COMPILER;
  const scope* p = fundef->m_param;
  assert(p->m_id == scope::PARAM);
#ifdef CXX_GENERATOR
  typedef scope param_scope;
#endif // CXX_GENERATOR
  const param_scope* param = static_cast<const param_scope*>(p);
  const vector<usr*>& vec = param->m_order;
  accumulate(vec.begin(),vec.end(),aggre_regwin,parameter);
  assert(param->m_children.size() == 1);
  p = param->m_children[0];

  int n = 0;
  recursive_locvar recursive_locvar(&n);
  recursive_locvar(p);
  n += 8;
  n = align(n,8);
  tmp_dword = new ::stack(-n,8);
  tmp_word = new ::stack(-n,4);
  n = align(n,16);
  tmp_quad[0] = new ::stack(-n,16);
  n += 16;
  tmp_quad[1] = new ::stack(-n,16);
  return n;
}

int parameter(int offset, COMPILER::usr* entry)
{
  using namespace COMPILER;
#ifdef CXX_GENERATOR
  usr::flag_t flag = entry->m_flag;
  if ( flag & usr::TYPEDEF )
    return offset;
#endif // CXX_GENERATOR
  const type* T = entry->m_type;
  int unpromed = T->size();
  if ( T->scalar() && unpromed < 16 ){
          T = T->promotion();
          int size = T->size();
          address_descriptor[entry] = new stack(offset,size,unpromed);
          return offset + size;
  }
  else {
    address_descriptor[entry] = new stack(offset,4,-1);
    return offset + 4;
  }
}

int align(int offset, int size)
{
  if ( int n = offset % size )
    return offset + size - n;
  else
    return offset;
}

void recursive_locvar::operator()(const COMPILER::scope* ptr)
{
  using namespace std;
  using namespace COMPILER;
#ifdef CXX_GENERATOR
  if ( dynamic_cast<const tag*>(ptr) )
    return;
#endif // CXX_GENERATOR
  const map<string,vector<usr*> >& usrs = ptr->m_usrs;
  *m_res = accumulate(usrs.begin(),usrs.end(),*m_res,add);
  assert(ptr->m_id == scope::BLOCK);
  const block* b = static_cast<const block*>(ptr);
  const vector<var*>& vars = b->m_vars;
  *m_res = accumulate(vars.begin(), vars.end(), *m_res, add3);

  const vector<scope*>& vec = ptr->m_children;
  for_each(vec.begin(),vec.end(),recursive_locvar(m_res));
}

int recursive_locvar::add(int n, const std::pair<std::string, std::vector<COMPILER::usr*> >& pair)
{
        using namespace std;
        using namespace COMPILER;
        const vector<usr*>& vec = pair.second;
        return accumulate(vec.begin(), vec.end(), n, add2);
}

int recursive_locvar::add2(int n, COMPILER::usr* entry)
{
        using namespace COMPILER;
        usr::flag_t flag = entry->m_flag;
        usr::flag_t mask = usr::flag_t(usr::TYPEDEF | usr::STATIC | usr::EXTERN | usr::FUNCTION | usr::VL);
        if (flag & mask)
                return n;

    if ( entry->isconstant() )
      return n;

        const type* T = entry->m_type;

  int size = T->size();
  int al = T->align();
  n += size;
  n = align(n,al);
  address_descriptor[entry] = new stack(-n,size);

  return n;
}

int recursive_locvar::add3(int n, COMPILER::var* v)
{
        using namespace std;
        using namespace COMPILER;
        const type* T = v->m_type;
        n = align(n, T->align());
        int size = T->size();
        n += size;
        address_descriptor[v] = new ::stack(-n, size);
        return n;
}

class arg_count {
  int* m_res;
  int m_offset;
  const int m_org;
  int m_curr;
public:
  arg_count(int* res)
    : m_res(res), m_offset(stack_layout.m_local), m_org(stack_layout.m_local), m_curr(0) { *m_res = 0; }
  void operator()(const COMPILER::tac*);
};

int fun_arg(const std::vector<COMPILER::tac*>& v3ac)
{
        using namespace std;
        int n;
        for_each(v3ac.begin(),v3ac.end(),arg_count(&n));
        return n < 24 ? 24 : n;
}

std::map<COMPILER::var*, stack*> record_param;

void arg_count::operator()(const COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  if ( tac->m_id == tac::PARAM ){
    var* y = tac->y;
    const type* T = y->m_type;
        T = T->promotion();
    int size = T->size();
    if ( T->scalar() && size < 16 )
      m_curr += size;
    else {
      m_curr += 4;
      m_offset += size;
      m_offset = align(m_offset,16);
      record_param[y] = new ::stack(-m_offset,size);
    }
  }
  else if ( tac->m_id == tac::CALL ){
    *m_res = max(*m_res,m_curr);
    stack_layout.m_local = max(stack_layout.m_local,m_offset);
    m_curr = 0;
    m_offset = m_org;
  }
}

class save {
  int* m_offset;
public:
  save(int* offset) : m_offset(offset) { *m_offset = aggre_regwin; }
  void operator()(COMPILER::usr*);
};

void save_if_varg(const COMPILER::usr* func, int regno);

void save_parameter(const COMPILER::fundef* fundef)
{
  using namespace std;
  using namespace COMPILER;

  const COMPILER::usr* func = fundef->m_usr;
#ifdef CXX_GENERATOR
  typedef scope param_scope;
#endif // CXX_GENERATOR
  const param_scope* parameter = fundef->m_param;
  const vector<usr*>& vec = parameter->m_order;
  int offset;
  for_each(vec.begin(),vec.end(),save(&offset));
  save_if_varg(func,(offset - aggre_regwin)/4);
}

void save_if_varg(const COMPILER::usr* func, int regno)
{
  using namespace std;
  using namespace COMPILER;
  const type* T = func->m_type;
  typedef const func_type FUNC;
  FUNC* ft = static_cast<FUNC*>(T);
  const vector<const type*>& param = ft->param();
  if (param.empty())
    return;
  T = param.back();
  if (T->m_id != type::ELLIPSIS)
    return;
  for ( int i = regno ; i < 6 ; ++i ){
    string reg = "%i";
    reg += char('0' + i);
    int offset = 4 * i + 68;
    out << '\t' << "st" << '\t' << reg << ", [%fp+" << offset << "]\n";
  }
}

void save::operator()(COMPILER::usr* entry)
{
  using namespace std;
  using namespace COMPILER;
  usr::flag_t flag = entry->m_flag;
  if (flag & usr::TYPEDEF)
    return;

  if (*m_offset >= 4 * 6 + aggre_regwin)
    return;
        
  map<const var*, address*>::const_iterator p = address_descriptor.find(entry);
  assert(p != address_descriptor.end());
  string reg = "%i";
  int index = (*m_offset - aggre_regwin)/4;
  reg += char(index + '0');
  typedef ::stack STACK;
  STACK* q = static_cast<STACK*>(p->second);
  q->save(reg);
  const type* T = entry->m_type->promotion();
  if ( T->aggregate() || T->size() == 16 )
    *m_offset += 4;
  else
    *m_offset += T->size();
}
