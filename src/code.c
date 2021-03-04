#define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>

extern SEXP parent_sym;
extern SEXP name_sym;

SEXP class_names_(SEXP obj) {
  SEXP parent;
  R_xlen_t n = 0;

  parent = obj;
  while(parent != R_NilValue) {
    parent = Rf_getAttrib(parent, parent_sym);
    ++n;
  }

  SEXP out = PROTECT(Rf_allocVector(STRSXP, n));

  R_xlen_t i = 0;
  parent = obj;
  while(parent != R_NilValue) {
    SET_STRING_ELT(out, i, STRING_ELT(Rf_getAttrib(parent, name_sym), 0));
    parent = Rf_getAttrib(parent, parent_sym);
    ++i;
  }

  UNPROTECT(1);

  return out;
}

extern SEXP object_class_sym;

SEXP object_class_(SEXP obj) {
  if (Rf_inherits(obj, "r7_class")) {
    return(obj);
  }

  SEXP val = Rf_getAttrib(obj, object_class_sym);

  if (val != R_NilValue) {
    return val;
  }

  return Rf_getAttrib(obj, R_ClassSymbol);
}

extern SEXP r7_methods_sym;

Rboolean should_ignore(SEXP value, SEXP ignore) {
  for (R_xlen_t i = 0; i < Rf_xlength(ignore); ++i) {
    if (R_compute_identical(value, VECTOR_ELT(ignore, i), 16) == TRUE) {
      return TRUE;
    }
  }
  return FALSE;
}

SEXP method_internal(SEXP table, SEXP signature, R_xlen_t signature_itr, SEXP ignore) {
  if (signature_itr >= Rf_xlength(signature)) {
    return R_NilValue;
  }

  SEXP classes = VECTOR_ELT(signature, signature_itr);

  if (Rf_inherits(classes, "r7_class")) {
    while(classes != R_NilValue) {
      SEXP klass = Rf_install(CHAR(STRING_ELT(Rf_getAttrib(classes, name_sym), 0)));
      SEXP val = Rf_findVarInFrame(table, klass);
      if (TYPEOF(val) == ENVSXP) {
        val = method_internal(val, signature, signature_itr + 1, ignore);
      }
      if (TYPEOF(val) == CLOSXP && (ignore == R_NilValue || !should_ignore(val, ignore))) {
        return val;
      }
      classes = Rf_getAttrib(classes, parent_sym);
    }
  } else {
    for (R_xlen_t i = 0; i < Rf_xlength(classes); ++i) {
      SEXP klass = Rf_install(CHAR(STRING_ELT(classes, i)));
      SEXP val = Rf_findVarInFrame(table, klass);
      if (TYPEOF(val) == ENVSXP) {
        val = method_internal(val, signature, signature_itr + 1, ignore);
      }
      if (TYPEOF(val) == CLOSXP && (ignore == R_NilValue || !should_ignore(val, ignore))) {
        return val;
      }
    }
  }
  return R_NilValue;
}

/* TODO: handle errors when method is not found */
SEXP method_(SEXP generic, SEXP signature, SEXP ignore) {
  if (!Rf_inherits(generic, "r7_generic")) {
    return R_NilValue;
  }

  SEXP table = Rf_getAttrib(generic, Rf_install("methods"));

  return method_internal(table, signature, 0, ignore);
}
