/*
** filesystem-binding.cpp
**
** This file is part of mkxp.
**
** Copyright (C) 2013 - 2021 Amaryllis Kulla <ancurio@mapleshrine.eu>
**
** mkxp is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 2 of the License, or
** (at your option) any later version.
**
** mkxp is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with mkxp.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "binding-util.h"

#include "sharedstate.h"
#include "filesystem.h"
#include "util.h"

#include "ruby/intern.h"

static void
fileIntFreeInstance(void *inst)
{
	SDL_RWops *ops = static_cast<SDL_RWops*>(inst);

	SDL_RWclose(ops);
	SDL_FreeRW(ops);
}

DEF_ALLOCFUNC_CUSTOMFREE(FileInt, fileIntFreeInstance);


static VALUE
fileIntForPath(const char *path, bool rubyExc)
{
	SDL_RWops *ops = SDL_AllocRW();

	try
	{
		shState->fileSystem().openReadRaw(*ops, path);
	}
	catch (const Exception &e)
	{
		SDL_FreeRW(ops);

		if (rubyExc)
			raiseRbExc(e);
		else
			throw e;
	}

	VALUE klass = rb_const_get(rb_cObject, rb_intern("FileInt"));

	VALUE obj = rb_obj_alloc(klass);

	setPrivateData(obj, ops);

	return obj;
}

RB_METHOD(fileIntRead)
{

	int length = -1;
	rb_get_args(argc, argv, "i", &length RB_ARG_END);

	SDL_RWops *ops = getPrivateData<SDL_RWops>(self);

	if (length == -1)
	{
		Sint64 cur = SDL_RWtell(ops);
		Sint64 end = SDL_RWseek(ops, 0, SEEK_END);
		length = end - cur;
		SDL_RWseek(ops, cur, SEEK_SET);
	}

	if (length == 0)
		return Qnil;

	VALUE data = rb_str_new(0, length);

	SDL_RWread(ops, RSTRING_PTR(data), 1, length);

	return data;
}

RB_METHOD(fileIntClose)
{
	RB_UNUSED_PARAM;

	SDL_RWops *ops = getPrivateData<SDL_RWops>(self);
	SDL_RWclose(ops);

	return Qnil;
}

RB_METHOD(fileIntGetByte)
{
	RB_UNUSED_PARAM;

	SDL_RWops *ops = getPrivateData<SDL_RWops>(self);

	unsigned char byte;
	size_t result = SDL_RWread(ops, &byte, 1, 1);

	return (result == 1) ? rb_fix_new(byte) : Qnil;
}

RB_METHOD(fileIntBinmode)
{
	RB_UNUSED_PARAM;

	return Qnil;
}

RB_METHOD(fileIntPos) {
    SDL_RWops *ops = getPrivateData<SDL_RWops>(self);
    
    long long pos = SDL_RWtell(ops); // Will return -1 if it doesn't work
    return LL2NUM(pos);
}

VALUE
kernelLoadDataInt(const char *filename, bool rubyExc)
{
	rb_gc_start();

	VALUE port = fileIntForPath(filename, rubyExc);

	VALUE marsh = rb_const_get(rb_cObject, rb_intern("Marshal"));

	// FIXME need to catch exceptions here with begin rescue
	VALUE result = rb_funcall2(marsh, rb_intern("load"), 1, &port);

	rb_funcall2(port, rb_intern("close"), 0, NULL);

	return result;
}

RB_METHOD(kernelLoadData)
{
	RB_UNUSED_PARAM;

	const char *filename;
	rb_get_args(argc, argv, "z", &filename RB_ARG_END);

	return kernelLoadDataInt(filename, true);
}

RB_METHOD(kernelSaveData)
{
	RB_UNUSED_PARAM;

	VALUE obj;
	VALUE filename;

	rb_get_args(argc, argv, "oS", &obj, &filename RB_ARG_END);

	VALUE file = rb_file_open_str(filename, "wb");

	VALUE marsh = rb_const_get(rb_cObject, rb_intern("Marshal"));

	VALUE v[] = { obj, file };
	rb_funcall2(marsh, rb_intern("dump"), ARRAY_SIZE(v), v);

	rb_io_close(file);

	return Qnil;
}

void
fileIntBindingInit()
{
	VALUE klass = rb_define_class("FileInt", rb_cIO);
	rb_define_alloc_func(klass, FileIntAllocate);

	_rb_define_method(klass, "read", fileIntRead);
	_rb_define_method(klass, "getbyte", fileIntGetByte);
	
	
	rb_define_alias(klass, "getc", "getbyte");
    	_rb_define_method(klass, "pos", fileIntPos);

	_rb_define_method(klass, "binmode", fileIntBinmode);
	_rb_define_method(klass, "close", fileIntClose);

	_rb_define_module_function(rb_mKernel, "load_data", kernelLoadData);
	_rb_define_module_function(rb_mKernel, "save_data", kernelSaveData);

}
