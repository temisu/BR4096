/* Copyright (C) Teemu Suutari */
#include <stdio.h>

#include <Carbon/Carbon.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/glu.h>
#include <OpenGL/CGLTypes.h>
#include <OpenGL/CGLContext.h>
#include <AGL/agl.h>

#include <CoreAudio/CoreAudio.h>

#define SOLVE_C(x) printf("%%define "#x" 0x%x\n",(unsigned int)(void*)x)

int main(int argc,char **argv)
{
