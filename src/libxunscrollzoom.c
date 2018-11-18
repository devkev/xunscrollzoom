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
#include <string.h>

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
#include <X11/extensions/XInput2.h>


#if defined(DEBUG)
static void _dprintf(const char *name, const char *fmt, ...) {
	va_list ap;
    fprintf(stderr, "libxunscrollzoom: %s: ", name);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fflush(stderr);
}
#define MYNAME(x) static const char *myname = #x;
#else
#define _dprintf(...)
#define MYNAME(x)
#endif


static void _eprintf(int exitcode, const char *name, const char *fmt, ...) {
	va_list ap;
	fprintf(stderr, "libxunscrollzoom: %s: Error: ", name);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fflush(stderr);
}





static void *lib_handle = NULL;
static const char *lib_name = "libX11.so.6";


static void _libxunscrollzoom_init(void) __attribute__((constructor));
static void _libxunscrollzoom_fini(void) __attribute__((destructor));

static void _libxunscrollzoom_init(void) {
	_dprintf("init", "starting up\n");

    // FIXME: tabs vs spaces

    // FIXME: optional - a "ONESHOT" envvar which tells us to clear LD_PRELOAD and all our envvars.
    //unsetenv("LD_PRELOAD");
}

static void _libxunscrollzoom_fini(void) {
	_dprintf("fini", "shutting down\n");
}


Display *XOpenDisplay(_Xconst char *display_name) {
	static const char *myname = "XOpenDisplay";
	static Display *(*underlying) (_Xconst char *display_name);
	Display *retval;
	const char *err;

	_dprintf(myname, "entered\n");

    // This lives in XOpenDisplay, rather than init(), because it's not cool to be dlopening libX11.so for every single binary that gets run.
    // This way it only happens for things that actually want to use X11.
	if (!lib_handle) {
		dlerror();
#if defined(RTLD_NEXT)
		lib_handle = RTLD_NEXT;
#else
		lib_handle = dlopen(lib_name, RTLD_LAZY);
#endif
		_dprintf(myname, "lib_handle = 0x%x\n", lib_handle);
		if (!lib_handle) {
			_eprintf(1, myname, "Unable to find %s: %s\n", lib_name, dlerror());
		}
	}

    // FIXME: dlsym *ALL* the underlying function pointer at the same time - mostly to save the branch on every intercepted function call. but also to fail early.
	if (!underlying) {
		dlerror();
		underlying = dlsym(lib_handle, myname);
		_dprintf(myname, "underlying = 0x%x\n", underlying);
		err = dlerror();
		if (err) {
			_dprintf(myname, "err = \"%s\"\n", err);
		}
		if (!underlying || err) {
			_eprintf(1, myname, "Unable to find the underlying function: %s\n", dlerror());
		}
	}

	_dprintf(myname, "about to call underlying function\n");
	retval = (*underlying) (display_name);
	_dprintf(myname, "underlying function result = %d\n", retval);

	_dprintf(myname, "function result = %d\n", retval);
	return retval;
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
	MYNAME(fixXEvent)

	if (event == NULL) {
		return;
	}

	if (event->type == ButtonPress || event->type == ButtonRelease) {
        _dprintf(myname, "ButtonPress || ButtonRelease\n");
		if (event->xbutton.button == 4 || event->xbutton.button == 5) {
			_dprintf(myname, "scroll button\n");
            if (event->xbutton.state & ControlMask) {
                _dprintf(myname, "SCROLL ZOOMING\n");
            }
			event->xbutton.state &= ~ControlMask;
		} else {
            _dprintf(myname, "button = %d\n", event->xbutton.button);
        }
	}
}


#define FIX_EVENT fixXEvent(display, event);
#define CHECK_EVENT(code) if (retval) { code }


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
              CHECK_EVENT(FIX_EVENT)
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
              CHECK_EVENT(FIX_EVENT)
			 )

__INTERCEPT__(
              Bool,
              XCheckTypedEvent,
              (Display *display, int event_type, XEvent *event),
              (display, event_type, event),
			  ,
              CHECK_EVENT(FIX_EVENT)
			 )

__INTERCEPT__(
              Bool,
              XCheckTypedWindowEvent,
              (Display *display, Window w, int event_type, XEvent *event),
              (display, w, event_type, event),
			  ,
              CHECK_EVENT(FIX_EVENT)
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
              CHECK_EVENT(FIX_EVENT)
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


static Bool xi2initialised = False;
static int xi2opcode = -1;

static void fixXI2Event(Display *display, XGenericEventCookie *event) {
	MYNAME(fixXI2Event)

    if (event->evtype == XI_ButtonPress || event->evtype == XI_ButtonRelease) {
        _dprintf(myname, "XI_ButtonPress || XI_ButtonRelease\n");

        XIDeviceEvent *xi2ev = (XIDeviceEvent*) event->data;

        _dprintf(myname, "xi2ev->detail = %u\n", xi2ev->detail);
        _dprintf(myname, "xi2ev->mods.base = 0x%x\n", xi2ev->mods.base);
        _dprintf(myname, "xi2ev->mods.latched = 0x%x\n", xi2ev->mods.latched);
        _dprintf(myname, "xi2ev->mods.locked = 0x%x\n", xi2ev->mods.locked);
        _dprintf(myname, "xi2ev->mods.effective = 0x%x\n", xi2ev->mods.effective);

		if (xi2ev->detail == 4 || xi2ev->detail == 5) {
			_dprintf(myname, "scroll button\n");
            if (xi2ev->mods.effective & ControlMask) {
                _dprintf(myname, "SCROLL ZOOMING\n");
            }
			xi2ev->mods.base &= ~ControlMask;
			xi2ev->mods.latched &= ~ControlMask;
			xi2ev->mods.locked &= ~ControlMask;
			xi2ev->mods.effective &= ~ControlMask;
        }
	}
}


__INTERCEPT__(
              Bool,
              XQueryExtension,
              (Display* display, _Xconst char* name, int* major_opcode_return, int* first_event_return, int* first_error_return),
              (display, name, major_opcode_return, first_event_return, first_error_return),
			  ,
              if (retval && !strcmp(name, "XInputExtension") && major_opcode_return) {
                  xi2initialised = True;
                  xi2opcode = *major_opcode_return;
                  _dprintf("XQueryExtension", "XI2 initialised\n");
              }
			 )


static void fixCookieEvent(Display *display, XGenericEventCookie *event) {
	if (event == NULL) {
		return;
	}

    if (xi2initialised && event->extension == xi2opcode) {
        fixXI2Event(display, event);
    }
}

#define FIX_COOKIE_EVENT fixCookieEvent(display, event);

__INTERCEPT__(
              Bool,
              XGetEventData,
              (Display* display, XGenericEventCookie *event),
              (display, event),
			  ,
              CHECK_EVENT(FIX_COOKIE_EVENT)
			 )



