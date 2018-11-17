/* libxunscrollzoom: ignore when ctrl is held down while scrolling the mouse wheel
 * Copyright (C) 2018 Kevin Pulo
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 * Contact:
 * Kevin Pulo
 * kev@pulo.com.au
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

/* Using _GNU_SOURCE as the manpage suggests can be problematic.
 * We only want the GNU extensions for dlfcn.h, not others like stdio.h.
 * But dlfcn.h checks __USE_GNU, not _GNU_SOURCE.  __USE_GNU is set by
 * features.h when _GNU_SOURCE is set, but features.h has already been loaded
 * by stdio.h, so it doesn't get loaded again.  Ugh.  Setting _GNU_SOURCE
 * earlier drags in other junk (eg. dprintf).  Simplest is just to use
 * __USE_GNU directly.
 */
#define __USE_GNU
#include <dlfcn.h>
#undef __USE_GNU

#include <unistd.h>

#include <X11/Xlib.h>


static int debug = 0;

static const char libxunscrollzoom_debug_envvar[] = "LIBXUNSCROLLZOOM_DEBUG";

static int do_debug() {
	return (debug || getenv(libxunscrollzoom_debug_envvar));
}

static void _dprintf(const char *name, const char *fmt, ...) {
	va_list ap;
	if (do_debug()) {
		fprintf(stderr, "libxunscrollzoom: ");
		if (name) {
			fprintf(stderr, "%s: ", name);
		}
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
		fflush(stderr);
	}
}

static void _eprintf(int exitcode, const char *name, const char *fmt, ...) {
	va_list ap;
	fprintf(stderr, "libxunscrollzoom: ");
	if (name) {
		fprintf(stderr, "%s: ", name);
	}
	fprintf(stderr, "Error: ");
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fflush(stderr);
}





static void *lib_handle = NULL;


static void _libxunscrollzoom_init(void) __attribute__((constructor));
static void _libxunscrollzoom_fini(void) __attribute__((destructor));

static void _libxunscrollzoom_init(void) {
	static const char *myname = "init";
	static const char *mylib = "libX11.so.6";

	_dprintf(myname, "starting up\n");

	if (getenv(libxunscrollzoom_debug_envvar)) {
        debug = 1;
    }

	if (!lib_handle) {
		dlerror();
#if defined(RTLD_NEXT)
		lib_handle = RTLD_NEXT;
#else
		lib_handle = dlopen(mylib, RTLD_LAZY);
#endif
		_dprintf(myname, "lib_handle = 0x%x\n", lib_handle);
		if (!lib_handle) {
			_eprintf(1, myname, "Unable to find %s: %s\n", mylib, dlerror());
		}
	}

    // FIXME: default - clear LD_PRELOAD and all our envvars, unless there's an envvar set telling us not to.
    //unsetenv("LD_PRELOAD");
}

static void _libxunscrollzoom_fini(void) {
	_dprintf(NULL, "shutting down\n");
}


// signature and params need enclosing brackets
#define __INTERCEPT__(rettype, fnname, signature, params, precall, postcall) \
rettype fnname signature { \
	static rettype (*underlying) signature; \
	rettype retval; \
	const char *err; \
 \
	_dprintf(#fnname, "entered\n"); \
 \
	if (!underlying) { \
		dlerror(); \
		underlying = dlsym(lib_handle, #fnname); \
		_dprintf(#fnname, "underlying = 0x%x\n", underlying); \
		err = dlerror(); \
		if (err) { \
			_dprintf(#fnname, "err = \"%s\"\n", err); \
		} \
		if (!underlying || err) { \
			_eprintf(1, #fnname, "Unable to find the underlying function: %s\n", dlerror()); \
		} \
	} \
 \
	precall \
 \
	_dprintf(#fnname, "about to call underlying function\n"); \
	retval = (*underlying) params; \
	_dprintf(#fnname, "underlying function result = %d\n", retval); \
 \
	postcall \
 \
	_dprintf(#fnname, "function result = %d\n", retval); \
	return retval; \
} \



static void fixXEvent(Display *display, XEvent *event) {
	if (event == NULL) {
		return;
	}


	if (event->type == ButtonPress || event->type == ButtonRelease) {
        _dprintf("fixXEvent", "ButtonPress || ButtonRelease\n");
		if (event->xbutton.button == 4 || event->xbutton.button == 5) {
			_dprintf("fixXEvent", "scroll button\n");
            if (event->xbutton.state & ControlMask) {
                _dprintf("fixXEvent", "SCROLL ZOOMING\n");
            }
			event->xbutton.state &= ~ControlMask;
		} else {
            _dprintf("fixXEvent", "button = %d\n", event->xbutton.button);
        }
	//} else if (event->type == GenericEvent) {
    //    _dprintf("fixXEvent", "evtype = %d\n", event->xgeneric.evtype);
    //    _dprintf("fixXEvent", "evtype = %d\n", event->xcookie.evtype);
    //    if (event->xcookie.evtype == ButtonPress || event->xcookie.evtype == ButtonRelease) {
    //        _dprintf("fixXEvent", "ButtonPress || ButtonRelease\n");
    //        if (XGetEventData(display, &event->xcookie)) {
    //            XEvent *real = (XEvent*)event->xcookie.data;
    //            if (real->xbutton.button == 4 || real->xbutton.button == 5) {
    //                _dprintf("fixXEvent", "scroll button\n");
    //            }
    //        }
    //        // fucken blerk, this whole protocol ain't cool for interception.
    //        //XFreeEventData(display, &event->xcookie);
    //    }
    //} else {
    //    _dprintf("fixXEvent", "type = %d\n", event->type);
	}
}


#define FIX_EVENT fixXEvent(display, event);
#define CHECK_FIX_EVENT if (retval) FIX_EVENT


__INTERCEPT__(
              int,
              XNextEvent,
			  (Display *display, XEvent *event),
			  (display, event),
			  ,
              FIX_EVENT
			 )

__INTERCEPT__(
              int,
              XPeekEvent,
              (Display *display, XEvent *event),
              (display, event),
			  ,
              FIX_EVENT
			 )

__INTERCEPT__(
              int,
              XWindowEvent,
              (Display *display, Window w, long event_mask, XEvent *event),
              (display, w, event_mask, event),
			  ,
              FIX_EVENT
			 )

__INTERCEPT__(
              Bool,
              XCheckWindowEvent,
              (Display *display, Window w, long event_mask, XEvent *event),
              (display, w, event_mask, event),
			  ,
              CHECK_FIX_EVENT
			 )

__INTERCEPT__(
              int,
              XMaskEvent,
              (Display *display, long event_mask, XEvent *event),
              (display, event_mask, event),
			  ,
              FIX_EVENT
			 )

__INTERCEPT__(
              Bool,
              XCheckMaskEvent,
              (Display *display, long event_mask, XEvent *event),
              (display, event_mask, event),
			  ,
              CHECK_FIX_EVENT
			 )

__INTERCEPT__(
              Bool,
              XCheckTypedEvent,
              (Display *display, int event_type, XEvent *event),
              (display, event_type, event),
			  ,
              CHECK_FIX_EVENT
			 )

__INTERCEPT__(
              Bool,
              XCheckTypedWindowEvent,
              (Display *display, Window w, int event_type, XEvent *event),
              (display, w, event_type, event),
			  ,
              CHECK_FIX_EVENT
			 )


typedef struct {
	Bool (*origPredicate)();
	XPointer origArg;
} ShimArg;

static Bool shimPredicate(Display *display, XEvent *event, XPointer arg) {
    ShimArg *newArg = (ShimArg*) arg;
    FIX_EVENT
    return newArg->origPredicate(display, event, newArg->origArg);
}

#define SHIM_IFEVENT \
    ShimArg newArg; \
    newArg.origPredicate = predicate; \
    newArg.origArg = arg; \
    arg = (XPointer) &newArg; \
    predicate = shimPredicate; \


__INTERCEPT__(
              int,
              XIfEvent,
              (Display *display, XEvent *event, Bool (*predicate)(), XPointer arg),
              (display, event, predicate, arg),
              SHIM_IFEVENT
              ,
              FIX_EVENT
			 )

__INTERCEPT__(
              Bool,
              XCheckIfEvent,
              (Display *display, XEvent *event, Bool (*predicate)(), XPointer arg),
              (display, event, predicate, arg),
              SHIM_IFEVENT
              ,
              CHECK_FIX_EVENT
			 )

__INTERCEPT__(
              int,
              XPeekIfEvent,
              (Display *display, XEvent *event, Bool (*predicate)(), XPointer arg),
              (display, event, predicate, arg),
              SHIM_IFEVENT
              ,
              FIX_EVENT
			 )



