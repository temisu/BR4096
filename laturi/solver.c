/* Copyright (C) Teemu Suutari */
#include <stdio.h>
#include <mach-o/loader.h>
#include <dlfcn.h>

#define stringify(x) x
#define SOLVE_C(x) printf("%%define "stringify(#x)" 0x%x\n",x);

int main()
{
