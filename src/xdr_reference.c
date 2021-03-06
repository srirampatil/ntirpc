/*
 * Copyright (c) 2009, Sun Microsystems, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of Sun Microsystems, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include <sys/cdefs.h>

/*
 * xdr_reference.c, Generic XDR routines impelmentation.
 *
 * Copyright (C) 1987, Sun Microsystems, Inc.
 *
 * These are the "non-trivial" xdr primitives used to serialize and de-serialize
 * "pointers".  See xdr.h for more info on the interface to xdr.
 */

#include "namespace.h"
#if !defined(_WIN32)
#include <err.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rpc/types.h>
#include <rpc/xdr_inline.h>
#include <rpc/rpc.h>

#if defined(__FreeBSD__) || defined(__NetBSD__)
#include <libc_private.h>
#endif

/*
 * XDR an indirect pointer
 * xdr_reference is for recursively translating a structure that is
 * referenced by a pointer inside the structure that is currently being
 * translated.  pp references a pointer to storage. If *pp is null
 * the  necessary storage is allocated.
 * size is the sizeof the referneced structure.
 * proc is the routine to handle the referenced structure.
 */
bool
xdr_reference(XDR *xdrs, void **pp,	/* the pointer to work on */
	      u_int size,	/* size of the object pointed to */
	      xdrproc_t proc /* xdr routine to handle the object */)
{
	void *loc = *pp;
	bool stat;

	if (loc == NULL)
		switch (xdrs->x_op) {
		case XDR_FREE:
			return (true);

		case XDR_DECODE:
			*pp = loc = mem_zalloc(size);
			break;

		case XDR_ENCODE:
			break;
		}

	stat = (*proc) (xdrs, loc);

	if (xdrs->x_op == XDR_FREE) {
		mem_free(loc, size);
		*pp = NULL;
	}
	return (stat);
}

/*
 * xdr_pointer():
 *
 * XDR a pointer to a possibly recursive data structure. This
 * differs with xdr_reference in that it can serialize/deserialiaze
 * trees correctly.
 *
 *  What's sent is actually a union:
 *
 *  union object_pointer switch (boolean b) {
 *  case true: object_data data;
 *  case false: void nothing;
 *  }
 *
 * > objpp: Pointer to the pointer to the object.
 * > obj_size: size of the object.
 * > xdr_obj: routine to XDR an object.
 *
 */
bool xdr_pointer(XDR *xdrs, void **objpp, u_int obj_size, xdrproc_t xdr_obj)
{

	bool_t more_data;

	more_data = (*objpp != NULL);
	if (!inline_xdr_bool(xdrs, &more_data))
		return (false);
	if (!more_data) {
		*objpp = NULL;
		return (true);
	}
	return (xdr_reference(xdrs, objpp, obj_size, xdr_obj));
}
