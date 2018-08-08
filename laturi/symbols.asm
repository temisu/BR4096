; Copyright (C) Teemu Suutari

%ifdef SYMBOL_SEARCH
%define SOLVE(x) SOLVE_C(x)
%else
%include "c-symbols.inc"
%define SOLVE(x) x
%endif
