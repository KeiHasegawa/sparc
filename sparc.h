#ifndef _SPARC_H_
#define _SPARC_H_

#ifdef CXX_GENERATOR
#define COMPILER cxx_compiler
#else  // CXX_GENERATOR
#define COMPILER c_compiler
#endif  // CXX_GENERATOR

extern std::ostream out;

extern void genobj(const COMPILER::scope* scope);

extern bool debug_flag;

extern void genfunc(const COMPILER::fundef* fundef, const std::vector<COMPILER::tac*>& code);

extern std::string func_label;

enum section { rom = 1, ram, ctor, dtor };

extern void output_section(section);

extern bool string_literal(const COMPILER::usr*);

class address {
protected:
  struct bad_size {
    int m_size;
    bad_size(int n) : m_size(n) {}
  };
public:
  static std::string load_suffix(int);
  static std::string store_suffix(int);
  virtual void load(std::string, int = 0) const = 0;
  virtual void store(std::string) const = 0;
  virtual void get(std::string) const = 0;
  virtual ~address(){}
};

class imm : public address {
  std::string m_expr;
  std::pair<int,int> m_pair;
  struct bad_op {};
public:
  imm(const COMPILER::usr*);
  void load(std::string, int = 0) const;
  void store(std::string) const { throw bad_op(); }
  void get(std::string) const { throw bad_op(); }
};

class stack : public address {
  int m_offset;
  int m_size;
  int m_unpromed;
  const COMPILER::var* m_allocated;
  void _store(std::string, bool) const;
  static bool should_store_high(int, bool, int);
public:
  stack(int, int);
  stack(int, int, int);
  stack(int, const COMPILER::var*);
  void load(std::string, int = 0) const;
  void store(std::string reg) const { _store(reg,false); }
  void get(std::string) const;
  void save(std::string reg) const { _store(reg,true); }
  int offset() const { return m_offset; }
};

class mem : public address {
        const COMPILER::usr* m_usr;
        std::string m_label;
public:
        mem(const COMPILER::usr* u) : m_usr(u) {};
        mem(const COMPILER::usr* u, std::string label) : m_usr(u), m_label(label) {};
        void load(std::string, int = 0) const;
        void store(std::string) const;
        void get(std::string) const;
        std::string label() const { return m_usr->m_name;; }
};

extern std::map<const COMPILER::var*, address*> address_descriptor;

extern address* getaddr(COMPILER::var*);

extern std::map<COMPILER::var*, stack*> record_param;

const int aggre_regwin = 4 + 16 * 4;

struct function_exit {
  std::string m_label;
  bool m_ref;
};

extern struct function_exit function_exit;

extern std::string new_label();

extern bool integer_suffix(int);

extern bool is_top(const COMPILER::scope*);

#ifdef CXX_GENERATOR
extern std::string scope_name(COMPILER::scope*);
extern std::string func_name(std::string);
extern std::string signature(const COMPILER::type*);
extern std::vector<std::string> ctors;
extern std::string dtor_name;
struct anonymous_table : std::map<const COMPILER::tag*, address*> {
  static void destroy(std::pair<const COMPILER::tag*, address*> pair)
  {
    delete pair.second;
  }
  void clear()
  {
          using namespace std;
          for_each(begin(),end(),destroy);
      map<const COMPILER::tag*, address*>::clear();
  }
  ~anonymous_table()
  {
    clear();
  }
};
extern anonymous_table anonymous_table;
#endif // CXX_GENEREATOR

struct stack_layout {
  int m_local;
  std::vector<COMPILER::var*> m_allocated;
};

extern struct stack_layout stack_layout;

struct tc_pattern {
  static std::vector<tc_pattern*> m_all;
  std::string m_label;
  bool m_done;
  tc_pattern()
  {
    m_all.push_back(this);
  }
  virtual void output() const = 0;
};

struct double_0x80000000 : public tc_pattern {
  void output() const;
};

struct double_0x100000000 : public tc_pattern {
  void output() const;
};

struct long_double_0x100000000 : public tc_pattern {
  void output() const;
};

struct long_double_0x10000000000000000 : public tc_pattern {
  void output() const;
};

extern struct double_0x80000000 double_0x80000000;
extern struct double_0x100000000 double_0x100000000;
extern struct long_double_0x100000000 long_double_0x100000000;
extern struct long_double_0x10000000000000000 long_double_0x10000000000000000;

extern stack* tmp_dword;
extern stack* tmp_word;
extern stack* tmp_quad[2];

#if defined(_MSC_VER)
#define DLL_EXPORT __declspec(dllexport)
#else // defined(_MSC_VER)
#define DLL_EXPORT
#endif // defined(_MSC_VER)

extern void(*output3ac)(std::ostream&, const COMPILER::tac*);

#endif // _SPARC_H_
