/* libxunscrollzoom: ignore when ctrl is held down while scrolling the mouse wheel
 * Copyright (C) 2019 Kevin Pulo
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
#include <ctype.h>

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
static void debug(const char *name, const char *fmt, ...) {
	va_list ap;
	fprintf(stderr, "libxunscrollzoom: %s: ", name);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fflush(stderr);
}
#define MYNAME(x) static const char *myname = #x;
#else
#define debug(...)
#define MYNAME(x)
#endif


static void fatalError(int exitcode, const char *name, const char *fmt, ...) {
	va_list ap;
	fprintf(stderr, "libxunscrollzoom: %s: Error: ", name);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fflush(stderr);
	exit(exitcode);
}



static const char libxunscrollzoom_envvar_single[] = "XUNSCROLLZOOM_SINGLE";


static void _libxunscrollzoom_init(void) __attribute__((constructor));
static void _libxunscrollzoom_fini(void) __attribute__((destructor));

static void _libxunscrollzoom_init(void) {
	debug("init", "starting up, LD_PRELOAD = \"%s\"\n", getenv("LD_PRELOAD"));

	// FIXME: It'd be nice to have an envvar in which we could list patterns for argv[0] which
	// are a whitelist of programs that we shouldn't impact.
	// eg. I don't mind ctrl-wheel zooming in gimp or audacity.

	if (getenv(libxunscrollzoom_envvar_single)) {
		const char *ld_preload = getenv("LD_PRELOAD");
		if (ld_preload == NULL) {
			// wtf?
		} else {
			// FIXME: remove ourselves properly
			unsigned int i = 0;
			while (ld_preload[i] && ld_preload[i] != ':' && !isspace(ld_preload[i])) {
				i++;
			}
			setenv("LD_PRELOAD", ld_preload + i, 1);
		}
		// FIXME: should we also clear libxunscrollzoom_envvar_single ?  There are arguments for and against...
	}
}

static void _libxunscrollzoom_fini(void) {
	debug("fini", "shutting down\n");
}



// signature and params need enclosing brackets
#define __INTERCEPT__(fnname, rettype, signature, params, precall, postcall) \
static rettype (*underlying_##fnname) signature; \
rettype fnname signature { \
	rettype retval; \
 \
	debug(#fnname, "entered %p\n", fnname); \
 \
	precall \
 \
	debug(#fnname, "about to call underlying function at %p\n", underlying_##fnname); \
	retval = (*underlying_##fnname) params; \
	debug(#fnname, "underlying function result = 0x%x\n", retval); \
 \
	postcall \
 \
	debug(#fnname, "function result = 0x%x\n", retval); \
	return retval; \
} \

#define REGISTER_INTERCEPT(fnname) underlying_##fnname = registerIntercept(#fnname);
#define REGISTER_INTERCEPT_0





static void *lib_handle = NULL;
static const char *lib_name = "libX11.so.6";

static void registerLibHandle();
static void registerAllIntercepts();

#define REGISTER_INTERCEPT_1 REGISTER_INTERCEPT_0 REGISTER_INTERCEPT(XOpenDisplay)
__INTERCEPT__(
				XOpenDisplay,
				Display*,
				(_Xconst char *display_name),
				(display_name),
					registerLibHandle();
					registerAllIntercepts();
				,
			 )





// Seems most software (terminology and firefox at least) also zoom when ctrl + horizontal scroll.
// So include buttons 6 & 7 as well.
// These button numbers are pretty much well-known/canonical, even though technically they can be reassigned (I think).
#define MASKED_BUTTONS_CASES \
	case 4: \
	case 5: \
	case 6: \
	case 7


static void fixXEvent(Display *display, XEvent *event) {
	MYNAME(fixXEvent)

	if (event == NULL) {
		return;
	}

	if (event->type == ButtonPress || event->type == ButtonRelease) {
		if (event->type == ButtonPress) {
			debug(myname, "ButtonPress\n");
		} else if (event->type == ButtonRelease) {
			debug(myname, "ButtonRelease\n");
		}
		debug(myname, "button = %d\n", event->xbutton.button);
		switch (event->xbutton.button) {
			MASKED_BUTTONS_CASES:
				debug(myname, "scroll button\n");
				if (event->xbutton.state & ControlMask) {
					debug(myname, "SCROLL ZOOMING\n");
				}
				event->xbutton.state &= ~ControlMask;
				break;
		}
	}
}

#define FIX_EVENT fixXEvent(display, event);
#define CHECK_EVENT(code) if (retval) { code }







#define REGISTER_INTERCEPT_2 REGISTER_INTERCEPT_1 REGISTER_INTERCEPT(XNextEvent)
__INTERCEPT__(
				XNextEvent,
				int,
				(Display *display, XEvent *event),
				(display, event),
				,
				FIX_EVENT
			 )


#define REGISTER_INTERCEPT_3 REGISTER_INTERCEPT_2 REGISTER_INTERCEPT(XPeekEvent)
__INTERCEPT__(
				XPeekEvent,
				int,
				(Display *display, XEvent *event),
				(display, event),
				,
				FIX_EVENT
			 )

#define REGISTER_INTERCEPT_4 REGISTER_INTERCEPT_3 REGISTER_INTERCEPT(XWindowEvent)
__INTERCEPT__(
				XWindowEvent,
				int,
				(Display *display, Window w, long event_mask, XEvent *event),
				(display, w, event_mask, event),
				,
				FIX_EVENT
			 )

#define REGISTER_INTERCEPT_5 REGISTER_INTERCEPT_4 REGISTER_INTERCEPT(XCheckWindowEvent)
__INTERCEPT__(
				XCheckWindowEvent,
				Bool,
				(Display *display, Window w, long event_mask, XEvent *event),
				(display, w, event_mask, event),
				,
				CHECK_EVENT(FIX_EVENT)
			 )

#define REGISTER_INTERCEPT_6 REGISTER_INTERCEPT_5 REGISTER_INTERCEPT(XMaskEvent)
__INTERCEPT__(
				XMaskEvent,
				int,
				(Display *display, long event_mask, XEvent *event),
				(display, event_mask, event),
				,
				FIX_EVENT
			 )

#define REGISTER_INTERCEPT_7 REGISTER_INTERCEPT_6 REGISTER_INTERCEPT(XCheckMaskEvent)
__INTERCEPT__(
				XCheckMaskEvent,
				Bool,
				(Display *display, long event_mask, XEvent *event),
				(display, event_mask, event),
				,
				CHECK_EVENT(FIX_EVENT)
			 )

#define REGISTER_INTERCEPT_8 REGISTER_INTERCEPT_7 REGISTER_INTERCEPT(XCheckTypedEvent)
__INTERCEPT__(
				XCheckTypedEvent,
				Bool,
				(Display *display, int event_type, XEvent *event),
				(display, event_type, event),
				,
				CHECK_EVENT(FIX_EVENT)
			 )

#define REGISTER_INTERCEPT_9 REGISTER_INTERCEPT_8 REGISTER_INTERCEPT(XCheckTypedWindowEvent)
__INTERCEPT__(
				XCheckTypedWindowEvent,
				Bool,
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


#define REGISTER_INTERCEPT_10 REGISTER_INTERCEPT_9 REGISTER_INTERCEPT(XIfEvent)
__INTERCEPT__(
				XIfEvent,
				int,
				(Display *display, XEvent *event, Bool (*predicate)(), XPointer arg),
				(display, event, predicate, arg),
				SHIM_IFEVENT
				,
				FIX_EVENT
			 )

#define REGISTER_INTERCEPT_11 REGISTER_INTERCEPT_10 REGISTER_INTERCEPT(XCheckIfEvent)
__INTERCEPT__(
				XCheckIfEvent,
				Bool,
				(Display *display, XEvent *event, Bool (*predicate)(), XPointer arg),
				(display, event, predicate, arg),
				SHIM_IFEVENT
				,
				CHECK_EVENT(FIX_EVENT)
			 )

#define REGISTER_INTERCEPT_12 REGISTER_INTERCEPT_11 REGISTER_INTERCEPT(XPeekIfEvent)
__INTERCEPT__(
				XPeekIfEvent,
				int,
				(Display *display, XEvent *event, Bool (*predicate)(), XPointer arg),
				(display, event, predicate, arg),
				SHIM_IFEVENT
				,
				FIX_EVENT
			 )


#if defined(DEBUG)
// This is only used when debug printing of the valuator values.
static const unsigned char oneBits[] = {0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4};
static unsigned char CountOnes(unsigned char x) {
	unsigned char results;
	results = oneBits[x & 0x0f];
	results += oneBits[x>>4];
	return results;
}
#endif

static Bool xi2initialised = False;
static int xi2opcode = -1;
static int lastMotionSourceId = -1;
static int valuatorMaskLen = -1;
static unsigned char *valuatorMask = NULL;

static void fixXI2Event(Display *display, XGenericEventCookie *event) {
	MYNAME(fixXI2Event)
	Bool doit = False;
	XIDeviceEvent *xi2ev = NULL;

	if (event->evtype == XI_ButtonPress || event->evtype == XI_ButtonRelease || event->evtype == XI_Motion) {
		if (event->evtype == XI_ButtonPress) {
			debug(myname, "XI_ButtonPress\n");
		} else if (event->evtype == XI_ButtonRelease) {
			debug(myname, "XI_ButtonRelease\n");
		} else if (event->evtype == XI_Motion) {
			debug(myname, "XI_Motion\n");
		}

		xi2ev = (XIDeviceEvent*) event->data;

		debug(myname, "xi2ev->detail = %u\n", xi2ev->detail);
		//debug(myname, "xi2ev->root_x = %lf\n", xi2ev->root_x);
		//debug(myname, "xi2ev->root_y = %lf\n", xi2ev->root_y);
		//debug(myname, "xi2ev->event_x = %lf\n", xi2ev->event_x);
		//debug(myname, "xi2ev->event_y = %lf\n", xi2ev->event_y);
		//debug(myname, "xi2ev->flags = %u\n", xi2ev->flags);
		debug(myname, "xi2ev->mods.base = 0x%x\n", xi2ev->mods.base);
		debug(myname, "xi2ev->mods.latched = 0x%x\n", xi2ev->mods.latched);
		debug(myname, "xi2ev->mods.locked = 0x%x\n", xi2ev->mods.locked);
		debug(myname, "xi2ev->mods.effective = 0x%x\n", xi2ev->mods.effective);
		//debug(myname, "xi2ev->group.base = 0x%x\n", xi2ev->group.base);
		//debug(myname, "xi2ev->group.latched = 0x%x\n", xi2ev->group.latched);
		//debug(myname, "xi2ev->group.locked = 0x%x\n", xi2ev->group.locked);
		//debug(myname, "xi2ev->group.effective = 0x%x\n", xi2ev->group.effective);
		//debug(myname, "xi2ev->buttons.mask_len = %d\n", xi2ev->buttons.mask_len);
		//for (unsigned int i = 0; i < xi2ev->buttons.mask_len; i++) {
		//	debug(myname, "xi2ev->buttons.mask[%d] = 0x%x\n", i, xi2ev->buttons.mask[i]);
		//}
#if defined(DEBUG)
		// Don't want to waste time counting bits unless we're actually going to bother printing the values.
		unsigned int numSetValuators = 0;
		debug(myname, "xi2ev->valuators.mask_len = %d\n", xi2ev->valuators.mask_len);
		for (unsigned int i = 0; i < xi2ev->valuators.mask_len; i++) {
			debug(myname, "xi2ev->valuators.mask[%u] = 0x%x\n", i, xi2ev->valuators.mask[i]);
			numSetValuators += CountOnes(xi2ev->valuators.mask[i]);
		}
		for (unsigned int i = 0; i < numSetValuators; i++) {
			debug(myname, "xi2ev->valuators.values[%u] = %lf\n", i, xi2ev->valuators.values[i]);
		}
#endif

		// detail is always 0 for Motion events
		switch (xi2ev->detail) {
			MASKED_BUTTONS_CASES:
				debug(myname, "scroll button\n");
				doit = True;
				break;
		}

		// FIXME: This will be inefficient when there are 2 or more pointing devices generating events interleaved.
		// So better might be to remember all of the sourceids that we've seen and their corresponding valuator masks.
		// HOWEVER, given that I don't know how long it's valid to cache info about devices (by id),
		// since they can be hot plugged/unplugged at any time (and ids seem to maybe be reused?),
		// it seems like a good idea to not be too aggressive about this.  Yet.  Maybe.
		// There might be events that can be listened for to notice when devices are plugged/unplugged,
		// and then maybe that's the point at which we should capture and cache (or discard) this info.
		// Yeah, it seems to be XI_HierarchyChanged (usually on root window, but could be anywhere) in the mask passed
		// to XISelectEvents, and then monitoring for these XIHierarchyEvents in XGetEventData().
		// Possibly also monitoring for XIDeviceChangedEvent (which is more like reconfigging a device).
		// See: https://who-t.blogspot.com/2009/06/xi2-recipies-part-2.html
		// and: https://who-t.blogspot.com/2009/06/xi2-recipes-part-3.html
		if (event->evtype == XI_Motion && xi2ev->sourceid != lastMotionSourceId) {
			// we need to get the scroll class info, so that we know which valuators to mask
			free(valuatorMask);
			valuatorMaskLen = xi2ev->valuators.mask_len;
			valuatorMask = calloc(valuatorMaskLen, sizeof(unsigned char));
			if (valuatorMask == NULL) {
				valuatorMaskLen = -1;
			} else {
				int ndevices = 0;
				XIDeviceInfo *devices = XIQueryDevice(display, xi2ev->sourceid, &ndevices);
				if (ndevices == 1) {
					for (unsigned int i = 0; i < devices->num_classes; i++) {
						if (devices->classes[i]->type == XIScrollClass) {
							XIScrollClassInfo *scroll = (XIScrollClassInfo *) devices->classes[i];
							unsigned int byteNum = scroll->number / 8;
							unsigned int bitNum = scroll->number % 8;
							valuatorMask[byteNum] |= (1<<bitNum);
						}
					}
					//debug(myname, "valuatorMaskLen = %d\n", valuatorMaskLen);
					//for (unsigned int i = 0; i < valuatorMaskLen; i++) {
					//	debug(myname, "valuatorMask[%u] = 0x%x\n", i, valuatorMask[i]);
					//}
					lastMotionSourceId = xi2ev->sourceid;
				}
				XIFreeDeviceInfo(devices);
			}
		}

		if (valuatorMaskLen >= 0) {
			for (unsigned int i = 0; i < valuatorMaskLen; i++) {
				if (xi2ev->valuators.mask[i] & valuatorMask[i]) {
					debug(myname, "scroll motion (%u)\n", i);
					doit = True;
					break;
				}
			}
		}
	}

	if (doit && xi2ev) {
		if (xi2ev->mods.effective & ControlMask) {
			debug(myname, "SCROLL ZOOMING\n");
		}
		xi2ev->mods.base &= ~ControlMask;
		xi2ev->mods.latched &= ~ControlMask;
		xi2ev->mods.locked &= ~ControlMask;
		xi2ev->mods.effective &= ~ControlMask;
	}
}


#define REGISTER_INTERCEPT_13 REGISTER_INTERCEPT_12 REGISTER_INTERCEPT(XQueryExtension)
__INTERCEPT__(
				XQueryExtension,
				Bool,
				(Display* display, _Xconst char* name, int* major_opcode_return, int* first_event_return, int* first_error_return),
				(display, name, major_opcode_return, first_event_return, first_error_return),
				,
				if (retval && !strcmp(name, "XInputExtension") && major_opcode_return) {
					xi2initialised = True;
					xi2opcode = *major_opcode_return;
					debug("XQueryExtension", "XI2 initialised\n");
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

#define REGISTER_INTERCEPT_14 REGISTER_INTERCEPT_13 REGISTER_INTERCEPT(XGetEventData)
__INTERCEPT__(
				XGetEventData,
				Bool,
				(Display* display, XGenericEventCookie *event),
				(display, event),
				,
				CHECK_EVENT(FIX_COOKIE_EVENT)
			 )

#define REGISTER_INTERCEPT_FINAL REGISTER_INTERCEPT_14



static void *registerIntercept(const char *fnname) {
	static const char *myname = "registerIntercept";
	void *underlying;
	const char *err;

	dlerror();
	underlying = dlsym(lib_handle, fnname);
	debug(myname, "underlying %s = %p\n", fnname, underlying);
	err = dlerror();
	if (err) {
		debug(myname, "err = \"%s\"\n", err);
	}
	if (!underlying || err) {
		fatalError(1, myname, "Unable to find the underlying function %s: %s\n", fnname, dlerror());
	}
	return underlying;
}

static void registerLibHandle() {
	static const char *myname = "registerLibHandle";
	// This is called from XOpenDisplay, rather than init(), because it's not cool to be dlopening libX11.so for every single binary that gets run.
	// This way it only happens for things that actually want to use X11.
	// And XOpenDisplay should only be called rarely/infrequently.
	if (!lib_handle) {
		dlerror();
#if defined(RTLD_NEXT)
		lib_handle = RTLD_NEXT;
#else
		lib_handle = dlopen(lib_name, RTLD_LAZY);
#endif
		debug(myname, "lib_handle = 0x%x\n", lib_handle);
		if (!lib_handle) {
			fatalError(1, myname, "Unable to find %s: %s\n", lib_name, dlerror());
		}
	}
}

static void registerAllIntercepts() {
	REGISTER_INTERCEPT_FINAL
}

