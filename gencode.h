#ifndef _GENCODE_H_
#define _GENCODE_H_

class gencode {
  struct table;
  static table m_table;
  friend struct table;
  static void assign(const COMPILER::tac*);
  static void assign_scalar(const COMPILER::tac*);
  static void assign_aggregate(const COMPILER::tac*);
  static void add(const COMPILER::tac*);
  static void sub(const COMPILER::tac*);
  static void mul(const COMPILER::tac*);
  static void div(const COMPILER::tac*);
  static void mod(const COMPILER::tac*);
  static void lsh(const COMPILER::tac*);
  static void rsh(const COMPILER::tac*);
  static void _and(const COMPILER::tac*);
  static void _xor(const COMPILER::tac*);
  static void _or(const COMPILER::tac*);
  static void binop_notlonglong(const COMPILER::tac*, std::string);
  static void binop_longlong(const COMPILER::tac*, std::string, std::string);
  static void runtime_notlonglong(const COMPILER::tac*, std::string);
  static void runtime_longlong(const COMPILER::tac*, std::string);
  static void runtime_longlong_shift(const COMPILER::tac*, std::string);
  static void binop_real(const COMPILER::tac*, std::string);
  static void binop_single(const COMPILER::tac*, std::string);
  static void binop_double(const COMPILER::tac*, std::string);
  static void binop_quad(const COMPILER::tac*, std::string);
  static void uminus(const COMPILER::tac*);
  static void tilde(const COMPILER::tac*);
  static void unaop_notlonglong(const COMPILER::tac*, std::string);
  static void unaop_real(const COMPILER::tac*, std::string);
  static void unaop_longlong(const COMPILER::tac*, std::string, std::string);
  static void cast(const COMPILER::tac*);
  static int cast_id(const COMPILER::type*);
  struct cast_table_t;
  static cast_table_t cast_table;
  friend struct cast_table_t;
  static void sint08_sint08(const COMPILER::tac*);
  static void sint08_uint08(const COMPILER::tac*);
  static void sint08_sint16(const COMPILER::tac*);
  static void sint08_uint16(const COMPILER::tac*);
  static void sint08_sint32(const COMPILER::tac*);
  static void sint08_uint32(const COMPILER::tac*);
  static void sint08_sint64(const COMPILER::tac*);
  static void sint08_uint64(const COMPILER::tac*);
  static void sint08_single(const COMPILER::tac*);
  static void sint08_double(const COMPILER::tac*);
  static void sint08_quad(const COMPILER::tac*);
  static void uint08_sint08(const COMPILER::tac*);
  static void uint08_sint16(const COMPILER::tac*);
  static void uint08_uint16(const COMPILER::tac*);
  static void uint08_sint32(const COMPILER::tac*);
  static void uint08_uint32(const COMPILER::tac*);
  static void uint08_sint64(const COMPILER::tac*);
  static void uint08_uint64(const COMPILER::tac*);
  static void uint08_single(const COMPILER::tac*);
  static void uint08_double(const COMPILER::tac*);
  static void uint08_quad(const COMPILER::tac*);
  static void sint16_sint08(const COMPILER::tac*);
  static void sint16_uint08(const COMPILER::tac*);
  static void sint16_uint16(const COMPILER::tac*);
  static void sint16_sint32(const COMPILER::tac*);
  static void sint16_uint32(const COMPILER::tac*);
  static void sint16_sint64(const COMPILER::tac*);
  static void sint16_uint64(const COMPILER::tac*);
  static void sint16_single(const COMPILER::tac*);
  static void sint16_double(const COMPILER::tac*);
  static void sint16_quad(const COMPILER::tac*);
  static void uint16_sint08(const COMPILER::tac*);
  static void uint16_uint08(const COMPILER::tac*);
  static void uint16_sint16(const COMPILER::tac*);
  static void uint16_sint32(const COMPILER::tac*);
  static void uint16_uint32(const COMPILER::tac*);
  static void uint16_sint64(const COMPILER::tac*);
  static void uint16_uint64(const COMPILER::tac*);
  static void uint16_single(const COMPILER::tac*);
  static void uint16_double(const COMPILER::tac*);
  static void uint16_quad(const COMPILER::tac*);
  static void sint32_sint08(const COMPILER::tac*);
  static void sint32_uint08(const COMPILER::tac*);
  static void sint32_sint16(const COMPILER::tac*);
  static void sint32_uint16(const COMPILER::tac*);
  static void sint32_sint32(const COMPILER::tac*);
  static void sint32_uint32(const COMPILER::tac*);
  static void sint32_sint64(const COMPILER::tac*);
  static void sint32_uint64(const COMPILER::tac*);
  static void sint32_single(const COMPILER::tac*);
  static void sint32_double(const COMPILER::tac*);
  static void sint32_quad(const COMPILER::tac*);
  static void uint32_sint08(const COMPILER::tac*);
  static void uint32_uint08(const COMPILER::tac*);
  static void uint32_sint16(const COMPILER::tac*);
  static void uint32_uint16(const COMPILER::tac*);
  static void uint32_sint32(const COMPILER::tac*);
  static void uint32_uint32(const COMPILER::tac*);
  static void uint32_sint64(const COMPILER::tac*);
  static void uint32_uint64(const COMPILER::tac*);
  static void uint32_single(const COMPILER::tac*);
  static void uint32_double(const COMPILER::tac*);
  static void uint32_real(const COMPILER::tac*);
  static void uint32_quad(const COMPILER::tac*);
  static void sint64_sint08(const COMPILER::tac*);
  static void sint64_uint08(const COMPILER::tac*);
  static void sint64_sint16(const COMPILER::tac*);
  static void sint64_uint16(const COMPILER::tac*);
  static void sint64_sint32(const COMPILER::tac*);
  static void sint64_uint32(const COMPILER::tac*);
  static void sint64_uint64(const COMPILER::tac*);
  static void sint64_single(const COMPILER::tac*);
  static void sint64_double(const COMPILER::tac*);
  static void sint64_quad(const COMPILER::tac*);
  static void uint64_sint08(const COMPILER::tac*);
  static void uint64_uint08(const COMPILER::tac*);
  static void uint64_sint16(const COMPILER::tac*);
  static void uint64_uint16(const COMPILER::tac*);
  static void uint64_sint32(const COMPILER::tac*);
  static void uint64_uint32(const COMPILER::tac*);
  static void uint64_sint64(const COMPILER::tac*);
  static void uint64_single(const COMPILER::tac*);
  static void uint64_double(const COMPILER::tac*);
  static void uint64_quad(const COMPILER::tac*);
  static void single_sint08(const COMPILER::tac*);
  static void single_uint08(const COMPILER::tac*);
  static void single_sint16(const COMPILER::tac*);
  static void single_uint16(const COMPILER::tac*);
  static void single_sint32(const COMPILER::tac*);
  static void single_uint32(const COMPILER::tac*);
  static void single_sint64(const COMPILER::tac*);
  static void single_uint64(const COMPILER::tac*);
  static void single_double(const COMPILER::tac*);
  static void single_quad(const COMPILER::tac*);
  static void double_sint08(const COMPILER::tac*);
  static void double_uint08(const COMPILER::tac*);
  static void double_sint16(const COMPILER::tac*);
  static void double_uint16(const COMPILER::tac*);
  static void double_sint32(const COMPILER::tac*);
  static void double_uint32(const COMPILER::tac*);
  static void double_sint64(const COMPILER::tac*);
  static void double_uint64(const COMPILER::tac*);
  static void double_single(const COMPILER::tac*);
  static void double_quad(const COMPILER::tac*);
  static void quad_sint08(const COMPILER::tac*);
  static void quad_uint08(const COMPILER::tac*);
  static void quad_sint16(const COMPILER::tac*);
  static void quad_uint16(const COMPILER::tac*);
  static void quad_sint32(const COMPILER::tac*);
  static void quad_uint32(const COMPILER::tac*);
  static void quad_sint64(const COMPILER::tac*);
  static void quad_uint64(const COMPILER::tac*);
  static void quad_single(const COMPILER::tac*);
  static void quad_double(const COMPILER::tac*);
  static void quad_real(const COMPILER::tac*, std::string);
  static void real_quad(const COMPILER::tac*, std::string);
  static void quad_common(const COMPILER::tac*, bool, int);
  static void extend(const COMPILER::tac*, bool, int);
  static void extend64(const COMPILER::tac*, bool, int);
  static void common_longlong(const COMPILER::tac*);
  static void common_real(const COMPILER::tac*, std::string, bool, int);
  static void common_quad(const COMPILER::tac*, bool, int);
  static void real_common(const COMPILER::tac*, std::string, bool, int);
  static void real_sint64(const COMPILER::tac*, std::string);
  static void real_uint64(const COMPILER::tac*, std::string, std::string);
  static void longlong_real(const COMPILER::tac*, std::string);
  static void longlong_quad(const COMPILER::tac*, std::string);
  static void addr(const COMPILER::tac*);
  static void invladdr(const COMPILER::tac*);
  static void invladdr_byte(const COMPILER::tac*);
  static void invladdr_half(const COMPILER::tac*);
  static void invladdr_word(const COMPILER::tac*);
  static void invladdr_dword(const COMPILER::tac*);
  static void invladdr_quad(const COMPILER::tac*);
  static void invladdr_aggregate(const COMPILER::tac*);
  static void invladdr_common(const COMPILER::tac*, std::string);
  static void invraddr(const COMPILER::tac*);
  static void invraddr_byte(const COMPILER::tac*);
  static void invraddr_half(const COMPILER::tac*);
  static void invraddr_word(const COMPILER::tac*);
  static void invraddr_dword(const COMPILER::tac*);
  static void invraddr_quad(const COMPILER::tac*);
  static void invraddr_aggregate(const COMPILER::tac*);
  static void invraddr_common(const COMPILER::tac*, std::string);
  static void loff(const COMPILER::tac*);
  static void loff_byte(const COMPILER::tac*);
  static void loff_half(const COMPILER::tac*);
  static void loff_word(const COMPILER::tac*);
  static void loff_dword(const COMPILER::tac*);
  static void loff_quad(const COMPILER::tac*);
  static void loff_quad(const COMPILER::tac*, int delta);
  static void loff_aggregate(const COMPILER::tac*);
  static void loff_aggregate(const COMPILER::tac*, int delta);
  static void loff_common(const COMPILER::tac*, std::string op);
  static void loff_common(const COMPILER::tac*, std::string op, int delta);
  static void roff(const COMPILER::tac*);
  static void roff_byte(const COMPILER::tac*);
  static void roff_half(const COMPILER::tac*);
  static void roff_word(const COMPILER::tac*);
  static void roff_dword(const COMPILER::tac*);
  static void roff_quad(const COMPILER::tac*);
  static void roff_quad(const COMPILER::tac*, int delta);
  static void roff_aggregate(const COMPILER::tac*);
  static void roff_aggregate(const COMPILER::tac*, int delta);
  static void roff_common(const COMPILER::tac*, std::string op);
  static void roff_common(const COMPILER::tac*, std::string op, int delta);
  static void param(const COMPILER::tac*);
  static void param_word(const COMPILER::tac*);
  static void param_dword(const COMPILER::tac*);
  static void param_quad(const COMPILER::tac*);
  static void param_aggregate(const COMPILER::tac*);
  static int m_offset;
  static void call(const COMPILER::tac*);
  static void call_void(const COMPILER::tac*);
  static void call_notlonglong(const COMPILER::tac*);
  static void call_real(const COMPILER::tac*);
  static void call_longlong(const COMPILER::tac*);
  static void call_aggregate(const COMPILER::tac*);
  static void _return(const COMPILER::tac*);
  static void _return_notlonglong(const COMPILER::tac*);
  static void _return_real(const COMPILER::tac*);
  static void _return_longlong(const COMPILER::tac*);
  static void _return_aggregate(const COMPILER::tac*);
  static const COMPILER::tac* m_last;
  static void _goto(const COMPILER::tac*);
  static void ifgoto(const COMPILER::tac*);
  static void ifgoto_notlonglong(const COMPILER::tac*);
  static void ifgoto_real(const COMPILER::tac*);
  static void ifgoto_longlong(const COMPILER::tac*);
  static void to(const COMPILER::tac*);
  struct ifgoto_table;
  static ifgoto_table m_ifgoto_table;
  struct ifgoto_longlong_table;
  static ifgoto_longlong_table m_ifgoto_longlong_table;
  static void _alloca_(const COMPILER::tac*);
  static void asm_(const COMPILER::tac*);
  static void _va_start(const COMPILER::tac*);
  static void _va_start16(const COMPILER::tac*);
  static void _va_start_common(const COMPILER::tac*, int);
  static void _va_arg(const COMPILER::tac*);
  static void _va_arg8(const COMPILER::tac*);
  static void _va_arg16(const COMPILER::tac*);
  static void _va_arg_common(const COMPILER::tac*, int);
  static void _va_end(const COMPILER::tac*);
  int m_counter;
  const std::vector<COMPILER::tac*>& m_v3ac;
  bool m_record_param;
public:
  gencode(const std::vector<COMPILER::tac*>& v3ac)
    : m_counter(-1), m_v3ac(v3ac), m_record_param(false)
    { m_last = v3ac.back(); }
  void operator()(const COMPILER::tac*);
};

#endif // _GENCODE_H_
