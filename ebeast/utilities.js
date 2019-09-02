// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
'use strict';

const AUTOTEST = true;

export const vue_mixins = {};
export const vue_directives = {};

/// Loop over all properties in `source` and assign to `target*
export function assign_forin (target, source) {
  for (let p in source)
    target[p] = source[p];
  return target;
}

/// Loop over all elements of `source` and assign to `target*
export function assign_forof (target, source) {
  for (let e of source)
    target[e] = source[e];
  return target;
}

/// Yield a wrapper that delays calling `callback` until `delay` miliseconds have passed since the last wrapper call.
export function debounce (delay, callback) {
  if (!(callback instanceof Function))
    throw new TypeError ('argument `callback` must be of type Function');
  let ctimer, cthis, cargs, creturn; // closure variables
  function invoke_callback() {
    ctimer = undefined;
    const invoke_this = cthis, invoke_args = cargs;
    cthis = undefined;
    cargs = undefined;
    creturn = callback.apply (invoke_this, invoke_args);
    return undefined;
  }
  function cwrapper (...args) {
    cthis = this;
    cargs = args;
    if (ctimer != undefined)
      clearTimeout (ctimer);
    ctimer = setTimeout (invoke_callback, delay);
    const last_return = creturn;
    creturn = undefined;
    return last_return;
  }
  return cwrapper;
}

/** Remove element `item` from `array` */
export function array_remove (array, item) {
  for (let i = 0; i < array.length; i++)
    if (item === array[i]) {
      array.splice (i, 1);
      break;
    }
  return array;
}

/// Generate map by splittoing the key=value pairs in `kvarray`
export function map_from_kvpairs (kvarray) {
  let o = {};
  for (let kv of kvarray) {
    const key = kv.split ('=', 1)[0];
    const value = kv.substr (key.length + 1);
    o[key] = value;
  }
  return o;
}

/** Generate integers [0..`bound`[ if one arg is given or [`bound`..`end`[ by incrementing `step`. */
export function* range (bound, end, step = 1) {
  if (end === undefined) {
    end = bound;
    bound = 0;
  }
  for (; bound < end; bound += step)
    yield bound;
}

/** Create a new object that has the same properties and Array values as `src` */
export function copy_recursively (src = {}) {
  let o;
  if (Array.isArray (src))
    {
      o = Array.prototype.slice.call (src, 0);	// copy array cells, not properties
      for (const k in src)                      // loop over index and property keys
	{
	  let v = o[k];
	  if (v === undefined)                  // empty cell?
	    {
	      v = src[k];
	      if (v !== undefined)              // no, missing property
		o[k] = v instanceof Object && k !== '__proto__' ? copy_recursively (v) : v;
	    }
	  else if (v instanceof Object)         // Object cell
	    o[k] = copy_recursively (v);
	}
    }
  else if (src instanceof Object && !(src instanceof Function))
    {
      o = Object.assign ({}, src);		// reseve slots and copy properties
      for (const k in o)
	{
	  const v = o[k];
	  if (v instanceof Object &&            // Object/Array property
	      k !== '__proto__')
	    o[k] = copy_recursively (v);
	}
    }
  else // primitive or Function
    o = src;
  return o;
}

/** Check if `a == b`, recursively if the arguments are of type Array or Object */
export function equals_recursively (a, b) {
  // compare primitives
  if (a === b)
    return true;
  if (typeof a != typeof b)
    return false;
  if (typeof a == 'number' && isNaN (a) && isNaN (b))
    return true;
  // check Objects, note: typeof null === 'object' && null instanceof Object === false
  if (!(a instanceof Object && b instanceof Object))
    return false;
  if (a.constructor !== b.constructor)
    return false;
  // functions must be exactly equal, due to scoping
  if (a instanceof Function)
    return false;
  // compare Array lengths
  if (Array.isArray (a) && a.length != b.length)
    return false;
  // compare RegExp
  if (a instanceof RegExp)
    return a.source == b.source && a.flags == b.flags && a.lastIndex  == b.lastIndex;
  // compare Object properties
  for (const k in a)
    {
      const av = a[k], bv = b[k];
      if (av === bv || equals_recursively (av, bv))
	continue;
      return false;
    }
  for (const k in b)
    {
      if (a.hasOwnProperty (k))
	continue;	// compared above
      const av = a[k], bv = b[k];
      if (av === bv || equals_recursively (av, bv))
	continue;	// compare inherited props
      return false;
    }
  // compare non-Array iterables (e.g. Set)
  if (!Array.isArray (a) && typeof a[Symbol.iterator] == 'function')
    {
      const itera = a[Symbol.iterator]();
      const iterb = b[Symbol.iterator]();
      do {
	const { value: va, done: da } = itera.next();
	const { value: vb, done: db } = iterb.next();
	if (da !== db)
	  return false;
	else if (da)
	  break;
	if (!(va === vb || equals_recursively (va, vb)))
	  return false;
      } while (true);	// eslint-disable-line no-constant-condition
    }
  return true;
  // TODO: equals_recursively() busy loops for object reference cycles
}

if (AUTOTEST)
  {
    function eqr (a, b) { return equals_recursively (a, b) && equals_recursively (b, a); }
    const a = [ 0, 2, 3, 4, 5, 70, {a:null} ], b = [ 1, 2, 3, 4, 5, 70, {a:null} ]; console.assert (!eqr (a, b)); a[0] = 1; console.assert (eqr (a, b));
    a.x = 9; console.assert (!eqr (a, b)); b.x = 9.0; console.assert (eqr (a, b));
    const c = copy_recursively (a); console.assert (eqr (a, c)); c[6].q = 'q'; console.assert (!eqr (a, c));
    const as = new Set (a), bs = new Set (b); console.assert (eqr (as, bs));
    bs.add (99); console.assert (!eqr (as, bs)); bs.delete (99); console.assert (eqr (as, bs));
    // TODO: a[999] = b; b[999] = a; console.assert (eqr (a, b));
    console.assert (eqr (/a/, /a/) && !eqr (/b/i, /b/));
    console.assert (eqr (clamp, clamp) && !eqr (clamp, eqr));
  }

/** Return @a x clamped into @a min and @a max. */
export function clamp (x, min, max) {
  return x < min ? min : x > max ? max : x;
}

/** Create a Vue component provide() function that forwards selected properties. */
export function fwdprovide (injectname, keys) {
  return function() {
    const proxy = {};
    for (let key of keys)
      Object.defineProperty (proxy, key, { enumerable: true, get: () => this[key], });
    const provide_defs = {};
    provide_defs[injectname] = proxy;
    return provide_defs;
  };
}

/** The V-INLINEBLUR directive guides focus for inline editing.
 * A Vue directive to notify and force blur on Enter or Escape.
 * The directive value must evaluate to a callable function for notifications.
 * For inputs that use `onchange` handlers, the edited value should be
 * discarded if the `cancelled` property is true.
 */
vue_directives['inlineblur'] = {
  bind (el, binding, vnode) {
    if (binding.value && typeof binding.value !== 'function')
      console.warn ('[Vue-v-inlineblur:] wrong type argument, function required:', binding.expression);
    el.cancelled = false;
  },
  inserted (el, binding, vnode) {
    assert (document.body.contains (el));
    el.focus();
    if (el == document.activeElement)
      {
	if (binding.value)
	  {
	    el._v_inlineblur_watch_blur = function (event) {
	      binding.value.call (this, event);
	    };
	    el.addEventListener ('blur', el._v_inlineblur_watch_blur, true);
	  }
	const ignorekeys = 'ignorekeys' in binding.modifiers;
	if (!ignorekeys)
	  {
	    el._v_inlineblur_watch_keydown = function (event) {
	      const esc = Util.match_key_event (event, 'Escape');
	      if (esc)
		el.cancelled = true;
	      if (esc || Util.match_key_event (event, 'Enter'))
		el.blur();
	    };
	    el.addEventListener ('keydown', el._v_inlineblur_watch_keydown);
	  }
      }
    else if (binding.value)
      {
	el.cancelled = true;
	binding.value.call (this, new CustomEvent ('cancel', { detail: 'blur' }));
      }
  },
  update (el, binding, vnode, componentUpdated) {
    // console.log ("inlineblur", "update");
  },
  componentUpdated (el, binding, vnode, componentUpdated) {
    // console.log ("inlineblur", "componentUpdated");
  },
  unbind (el, binding, vnode) {
    if (el._v_inlineblur_watch_blur)
      {
	el.removeEventListener ('blur', el._v_inlineblur_watch_blur, true);
	el._v_inlineblur_watch_blur = undefined;
      }
    if (el._v_inlineblur_watch_keydown)
      {
	el.removeEventListener ('keydown', el._v_inlineblur_watch_keydown);
	el._v_inlineblur_watch_keydown = undefined;
      }
  }
};

/** This Vue mixin sets up reactive `data` from `data_tmpl` and non-reactive data from `priv_tmpl` */
vue_mixins.data_tmpl = {
  beforeCreate: function () {
    // Add non-reactive data from `priv_tmpl`
    if (this.$options.priv_tmpl)
      {
	Object.assign (this, copy_recursively (this.$options.priv_tmpl));
      }
    // Create reactive `data` (via cloning) from `data_tmpl`
    if (this.$options.data_tmpl)
      {
	const data = copy_recursively (this.$options.data_tmpl);
	// merge data={} into the copy of data_tmpl={}
	if (typeof this.$options.data === 'function')
	  Object.assign (data, this.$options.data.call (this));
	else if (this.$options.data)
	  Object.assign (data, this.$options.data);
	this.$options.data = data;
      }
    if (this.$options.tmpl_data)
      console.warn ("Object has `tmpl_data` member, did you mean `data_tmpl`?", this);
  },
};

/** Vue mixin to create a kebab-case ('two-words') getter proxy for camelCase ('twoWords') props */
vue_mixins.hyphen_props = {
  beforeCreate: function () {
    for (let cc in this.$options.props) {
      const hyphenated = hyphenate (cc);
      if (hyphenated === cc || (this.$options.computed && hyphenated in this.$options.computed))
	continue;
      if (!this.$options.computed)
	this.$options.computed = {};
      Object.defineProperty (this, hyphenated, {
	get() { return this[cc]; },
	enumerable: false,
	configurable: false
      });
    }
  },
};

/** Generate a kebab-case ('two-words') identifier from a camelCase ('twoWords') identifier */
export function hyphenate (string) {
  const uppercase_boundary =  /\B([A-Z])/g;
  return string.replace (uppercase_boundary, '-$1').toLowerCase();
}

/** Vue mixin to provide a `dom_create`, `dom_update`, `dom_destroy` hooks.
 * This mixin calls instance method callbacks for DOM element creation
 * (`this.dom_create()`), updates (`this.dom_update()`,
 * and destruction (`this.dom_destroy()`).
 * If `dom_create` is an async function or returns a Promise, `dom_update`
 * calls are deferred until the returned Promise is resolved.
 *
 * Access to reactive properties during `dom_update` are tracked as dependencies,
 * watched by Vue, so future changes cause rerendering of the Vue component.
 */
vue_mixins.dom_updates = {
  beforeCreate: function () {
    console.assert (this.$dom_updates == undefined);
    // install $dom_updates helper on Vue instance
    this.$dom_updates = {
      // members
      promise: Promise.resolve(),
      destroying: false,
      pending: false,	// dom_update pending
      unwatch: null,
      // methods
      chain_await: (promise_or_function) => {
	const result = promise_or_function instanceof Function ? promise_or_function() : promise_or_function;
	if (result instanceof Promise)
	  this.$dom_updates.promise =
	    this.$dom_updates.promise.then (async () => await result);
      },
      call_update: (resolve) => {
	/* Here we invoke `dom_update` with dependency tracking through $watch. In case
	 * it is implemented as an async function, we await the returned promise to
	 * serialize with future `dom_update` or `dom_destroy` calls. Note that
	 * dependencies cannot be tracked beyond the first await point in `dom_update`.
	 */
	// Clear old $watch if any
	if (this.$dom_updates.unwatch)
	  {
	    this.$dom_updates.unwatch();
	    this.$dom_updates.unwatch = null;
	  }
	/* Note, if vm._watcher is triggered before the $watch from below, it'll re-render
	 * the VNodes and then our watcher is triggered, which causes $forceUpdate() and the
	 * VNode tree is rendered *again*. This causes multiple calles to updated(), too.
	 */
	let once = 0;
	const update_expr = vm => {
	  if (once == 0)
	    {
	      const result = this.dom_update (this);
	      if (result instanceof Promise)
		{
		  // Note, async dom_update() looses reactivity…
		  result.then (resolve);
		  // console.warn ('dom_update() should not return Promise:', this);
		}
	      else
		resolve();
	    }
	  return ++once; // always change return value and guard against subsequent calls
	};
	/* A note on $watch. Its `expOrFn` is called immediately, the retrun value and
	 * dependencies are recorded. Later, once a dependency changes, its `expOrFn`
	 * is called again, also recording return value and new dependencies.
	 * If the return value changes, `callback` is invoked.
	 * What we need for updating DOM elements, is:
	 * a) the initial call with dependency recording which we use for (expensive) updating,
	 * b) trigger $forceUpdate() once a dependency changes, without intermediate expensive updating.
	 */
	this.$dom_updates.unwatch = this.$watch (update_expr, this.$forceUpdate);
      },
    };
  }, // beforeCreate
  mounted: function () {
    console.assert (this.$dom_updates);
    if (this.dom_create)
      this.$dom_updates.chain_await (this.dom_create());
    this.$forceUpdate();  // always trigger `dom_update` after `dom_create`
  },
  updated: function () {
    console.assert (this.$dom_updates);
    /* If multiple $watch() instances are triggered by an update, Vue may re-render
     * and call updated() several times in a row. To avoid expensive intermediate
     * updates, we use this.$dom_updates.pending as guard.
     */
    if (this.dom_update && !this.$dom_updates.pending)
      {
	this.$dom_updates.chain_await (new Promise (resolve => {
	  // Wrap call_update() into a chained promise to serialize with dom_destroy
	  this.$nextTick (() => {
	    this.$dom_updates.pending = false;
	    if (this.$dom_updates.destroying)
	      resolve(); // No need for updates during destruction
	    else
	      this.$dom_updates.call_update (resolve);
	  });
	}));
	this.$dom_updates.pending = true;
      }
  },
  beforeDestroy: function () {
    console.assert (this.$dom_updates);
    this.$dom_updates.destroying = true;
    if (this.dom_destroy)
      this.$dom_updates.chain_await (() => this.dom_destroy());
  },
};

/** VueifyObject - turn a regular object into a Vue instance.
 * The *object* passed in is used as the Vue `data` object. Properties
 * with a getter (and possibly setter) are turned into Vue `computed`
 * properties, methods are carried over as `methods` on the Vue() instance.
 */
export function VueifyObject (object = {}, vue_options = {}) {
  let voptions = Object.assign ({}, vue_options);
  voptions.methods = vue_options.methods || {};
  voptions.computed = vue_options.computed || {};
  voptions.data = voptions.data || object;
  const proto = object.__proto__;
  for (const pname of Object.getOwnPropertyNames (proto)) {
    if (typeof proto[pname] == 'function' && pname != 'constructor')
      voptions.methods[pname] = proto[pname];
    else
      {
	const pd = Object.getOwnPropertyDescriptor (proto, pname);
	if (pd.get)
	  voptions.computed[pname] = pd;
      }
  }
  return new Vue (voptions);
}

/** Copy PropertyDescriptors from source to target, optionally binding handlers against closure. */
export const clone_descriptors = (target, source, closure) => {
  // See also: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object/assign completeAssign()
  const descriptors = Object.getOwnPropertyNames (source).reduce ((descriptors, pname) => {
    const pd = Object.getOwnPropertyDescriptor (source, pname);
    if (pname != 'constructor')
      {
	if (closure)
	  {
	    if (pd.get)
	      pd.get = pd.get.bind (closure);
	    if (pd.set)
	      pd.set = pd.set.bind (closure);
	    if (typeof pd.value == 'function')
	      pd.value = pd.value.bind (closure);
	  }
	descriptors[pname] = pd;
      }
    return descriptors;
  }, {});
  Object.defineProperties (target, descriptors);
  // in constrast to Object.assign, we ignore getOwnPropertySymbols here
  return target;
  // Usage: clone_descriptors (window, fakewin.__proto__, fakewin);
};

/** Produce hash code from a String, using an FNV-1a variant. */
export function fnv1a_hash (str) {
  let hash = 0x811c9dc5;
  for (let i = 0; i < str.length; ++i) {
    // Note, charCodeAt can return values > 255
    const next = 0x1000193 * (hash ^ str.charCodeAt (i));
    hash = next >>> 0; // % 4294967296, i.e. treat as 32 bit unsigned
  }
  return hash;
}

/** Split a string when encountering a comma, while preserving quoted or parenthesized segments. */
export function split_comma (str) {
  let result = [], item = '', sdepth = 0, ddepth = 0, pdepth = 0, kdepth = 0, cdepth = 0, bslash = 0;
  for (let i = 0; i < str.length; i++) {
    const c = str[i];
    if (c === ',' && 0 == (sdepth + ddepth + pdepth + kdepth + cdepth)) {
      if (item) {
	result.push (item);
	item = '';
      }
    } else {
      item += c;
      if (c === '[') kdepth++;
      if (c === ']' && kdepth) kdepth--;
      if (c === '(') pdepth++;
      if (c === ')' && pdepth) pdepth--;
      if (c === '{') cdepth++;
      if (c === '}' && cdepth) cdepth--;
      if (c === "'" && !bslash) sdepth = !sdepth;
      if (c === '"' && !bslash) ddepth = !ddepth;
    }
    bslash = !bslash && c === '\\' && (sdepth || ddepth);
  }
  if (item)
    result.push (item);
  return result;
}

/** Parse hexadecimal CSS color with 3 or 6 digits into [ R, G, B ]. */
export function parse_hex_color (colorstr) {
  if (colorstr.substr (0, 1) == '#') {
    let hex = colorstr.substr (1);
    if (hex.length == 3)
      hex = hex[0] + hex[0] + hex[1] + hex[1] + hex[2] + hex[2];
    return [ parseInt (hex.substr (0, 2), 16),
	     parseInt (hex.substr (2, 2), 16),
	     parseInt (hex.substr (4, 2), 16) ];
  }
  return undefined;
}

/** Parse hexadecimal CSS color into luminosity. */
// https://en.wikipedia.org/wiki/Relative_luminance
export function parse_hex_luminosity (colorstr) {
  const rgb = parse_hex_color (colorstr);
  return 0.2126 * rgb[0] + 0.7152 * rgb[1] + 0.0722 * rgb[2];
}

/** Parse hexadecimal CSS color into brightness. */
// https://www.w3.org/TR/AERT/#color-contrast
export function parse_hex_brightness (colorstr) {
  const rgb = parse_hex_color (colorstr);
  return 0.299 * rgb[0] + 0.587 * rgb[1] + 0.114 * rgb[2];
}

/** Parse hexadecimal CSS color into perception corrected grey. */
// http://alienryderflex.com/hsp.html
export function parse_hex_pgrey (colorstr) {
  const rgb = parse_hex_color (colorstr);
  return Math.sqrt (0.299 * rgb[0] * rgb[0] + 0.587 * rgb[1] * rgb[1] + 0.114 * rgb[2] * rgb[2]);
}

/** Parse hexadecimal CSS color into average grey. */
export function parse_hex_average (colorstr) {
  const rgb = parse_hex_color (colorstr);
  return 0.3333 * rgb[0] + 0.3333 * rgb[1] + 0.3333 * rgb[2];
}

/** Parse CSS colors (via invisible DOM element) and yield an array of rgba tuples. */
export function parse_colors (colorstr) {
  let result = [];
  if (!parse_colors.span) {
    parse_colors.span = document.createElement ('span');
    parse_colors.span.style.display = 'none';
    document.body.appendChild (parse_colors.span);
  }
  for (const c of exports.split_comma (colorstr)) {
    parse_colors.span.style.color = c;
    const style = getComputedStyle (parse_colors.span);
    let m = style.color.match (/^rgb\s*\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)$/i);
    if (m)
      result.push ([m[1], m[2], m[3], 1]);
    else {
      m = style.color.match (/^rgba\s*\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*([\d.]+)\s*\)$/i);
      if (m)
	result.push ([m[1], m[2], m[3], m[4]]);
    }
  }
  return result;
}

/** Retrieve a new object with the properties of `obj` resolved against the style of `el` */
export function compute_style_properties (el, obj) {
  const style = getComputedStyle (el);
  let props = {};
  for (let key in obj) {
    const result = style.getPropertyValue (obj[key]);
    if (result !== undefined)
      props[key] = result;
  }
  return props;
}

/** List all elements that can take focus and are descendants of `element` or the document. */
export function list_focusables (element)
{
  if (!element)
    element = document.body;
  const candidates = [
    'a[href]',
    'audio[controls]',
    'button',
    'input:not([type="radio"]):not([type="hidden"])',
    'select',
    'textarea',
    'video[controls]',
    '[contenteditable]:not([contenteditable="false"])',
    '[tabindex]',
  ];
  const excludes = ':not([disabled])' +
		   ':not([tabindex="-1"])' +
		   ':not([display="none"])';
  const candidate_selector = candidates.map (e => e + excludes).join (', ');
  const nodes = element.querySelectorAll (candidate_selector); // selector for focusable elements
  const array1 = [].slice.call (nodes);
  // filter out non-taabable elements
  const array = array1.filter (element => {
    if (element.offsetWidth <= 0 || element.offsetHeight <= 0) // with display:none parents
      return false;
    return true;
  });
  return array;
}

/** Install a FocusGuard to allow only a restricted set of elements to get focus. */
class FocusGuard {
  defaults() { return {
    updown_focus: true,
    updown_cycling: false,
    focus_root_list: [],
    last_focus: undefined,
  }; }
  constructor () {
    Object.assign (this, this.defaults());
    window.addEventListener ('focusin', this.focusin_handler.bind (this), true);
    window.addEventListener ('keydown', this.keydown_handler.bind (this));
    if (document.activeElement && document.activeElement != document.body)
      this.last_focus = document.activeElement;
    // Related: https://developer.mozilla.org/en-US/docs/Web/Accessibility/Keyboard-navigable_JavaScript_widgets
  }
  push_focus_root (element) {
    const current_focus = document.activeElement && document.activeElement != document.body ? document.activeElement : undefined;
    this.focus_root_list.push ([ element, current_focus]);
    if (current_focus)
      this.focus_changed (current_focus, false);
  }
  remove_focus_root (element) {
    if (this.last_focus && !this.last_focus.parentElement)
      this.last_focus = undefined;	// cleanup to allow GC
    for (let i = 0; i < this.focus_root_list.length; i++)
      if (this.focus_root_list[i][0] === element)
	{
	  const saved_focus = this.focus_root_list[i][1];
	  this.focus_root_list.splice (i, 1);
	  if (saved_focus)
	    saved_focus.focus(); // try restoring focus
	  return true;
	}
    return false;
  }
  focusin_handler (event) {
    return this.focus_changed (event.target);
  }
  focus_changed (target, refocus = true) {
    if (this.focus_root_list.length == 0 || !document.activeElement ||
	document.activeElement == document.body)
      return false; // not interfering
    const focuslist = list_focusables (this.focus_root_list[0][0]);
    const idx = focuslist.indexOf (target);
    if (idx < 0) // invalid element gaining focus
      {
	document.activeElement.blur();
	if (refocus && this.last_focus && focuslist.length)
	  {
	    const lastidx = focuslist.indexOf (this.last_focus);
	    let newidx = 0;
	    if (lastidx >= 0 && lastidx < focuslist.length / 2)
	      newidx = focuslist.length - 1;
	    focuslist[newidx].focus();
	  }
	return true;
      }
    else
      this.last_focus = document.activeElement;
    return false; // not interfering
  }
  keydown_handler (event) {
    const up = event.keyCode == KeyCode.UP;
    const down = event.keyCode == KeyCode.DOWN;
    const home = event.keyCode == KeyCode.HOME;
    const end = event.keyCode == KeyCode.END;
    if (this.focus_root_list.length == 0 || !this.updown_focus ||
	!(up || down || home || end))
      return false; // not interfering
    const root = this.focus_root_list[0][0];
    const focuslist = list_focusables (root);
    if (!focuslist)
      return false; // not interfering
    let idx = focuslist.indexOf (document.activeElement);
    if (idx < 0 && (up || down))
      { // re-focus last element if possible
	idx = focuslist.indexOf (this.last_focus);
	if (idx >= 0)
	  {
	    focuslist[idx].focus();
	    return true;
	  }
      }
    let next; // position to move new focus to
    if (idx < 0)
      next = (down || home) ? 0 : focuslist.length - 1;
    else if (home || end)
      next = home ? 0 : focuslist.length - 1;
    else // up || down
      {
	next = idx + (up ? -1 : +1);
	if (this.updown_cycling)
	  {
	    if (next < 0)
	      next += focuslist.length;
	    else if (next >= focuslist.length)
	      next -= focuslist.length;
	  }
      }
    if (next >= 0 && next < focuslist.length)
      {
	focuslist[next].focus();
	return true;
      }
    return false;
  }
}
const the_focus_guard = new FocusGuard();

/** Constrain focus to `element` and its descendants */
export function push_focus_root (element) {
  the_focus_guard.push_focus_root (element);
}

/** Remove an `element` previously installed via push_focus_root() */
export function remove_focus_root (element) {
  the_focus_guard.remove_focus_root (element);
}

/** Installing a modal shield prevents mouse and key events for all elements */
class ModalShield {
  defaults() { return {
    close_handler: undefined,
    remove_focus_root: undefined,
    div: undefined,
  }; }
  constructor (close_handler, preserve_element, opts) {
    Object.assign (this, this.defaults());
    this.close_handler = close_handler;
    // prevent focus during modal shield
    push_focus_root (preserve_element);
    this.remove_focus_root = () => remove_focus_root (preserve_element);
    // install shield element on <body/>
    const div = document.createElement ("DIV");
    div.style = 'display: flex; position: fixed; z-index: 90; left: 0; top: 0; width: 100%; height: 100%;' +
		'background:' + (opts['background'] || '#00000099');
    document.body.appendChild (div);
    this.div = div;
    // keep a shield list and handle keyboard / mouse events on the shield
    if (!(document._b_modal_shields instanceof Array)) {
      document._b_modal_shields = [];
      document.addEventListener ('keydown', ModalShield.modal_keyboard_guard);
      document.addEventListener ('mousedown', ModalShield.modal_mouse_guard);
    }
    document._b_modal_shields.unshift (this);
  }
  destroy (call_handler = false) {
    array_remove (document._b_modal_shields, this);
    if (document._b_modal_shields.length == 0)
      {
	document._b_modal_shields = undefined;
	document.removeEventListener ('keydown', ModalShield.modal_keyboard_guard);
	document.removeEventListener ('mousedown', ModalShield.modal_mouse_guard);
      }
    if (this.div && this.div.parentNode)
      this.div.parentNode.removeChild (this.div);
    this.div = undefined;
    if (this.remove_focus_root)
      this.remove_focus_root();
    this.remove_focus_root = undefined;
    if (this.close_handler)
      {
	const close_handler_once = this.close_handler;
	this.close_handler = undefined;		// guards against recursion
	if (call_handler)
	  close_handler_once();
      }
  }
  close() {
    if (this.close_handler)
      this.close_handler();
  }
  static modal_keyboard_guard (event) {
    if (document._b_modal_shields.length > 0)
      {
	const shield = document._b_modal_shields[0];
	if (event.keyCode == KeyCode.ESCAPE &&
	    !event.cancelBubble)
	  {
	    event.preventDefault();
	    event.stopPropagation();
	    shield.close();
	  }
      }
  }
  static modal_mouse_guard (event) {
    if (document._b_modal_shields.length > 0)
      {
	const shield = document._b_modal_shields[0];
	if (!event.cancelBubble &&
	    event.target == shield.div)
	  {
	    event.preventDefault();
	    event.stopPropagation();
	    shield.close();
	    /* Browsers may emit two events 'mousedown' and 'contextmenu' for button3 clicks.
	     * This may cause the shield's owning widget (e.g. a menu) to reappear, because
	     * in this mousedown handler we can only prevent further mousedown handling.
	     * So we set up a timer that swallows the next 'contextmenu' event.
	     */
	    swallow_event ('contextmenu', 0);
	  }
      }
  }
}

/** Add a modal overlay to `body` to deflect DOM clicks and keyboard events */
export function modal_shield (close_handler, preserve_element, opts = {}) {
  return new ModalShield (close_handler, preserve_element, opts);
}

/** Use capturing to swallow any `type` events until `timeout` has passed */
export function swallow_event (type, timeout = 0) {
  const preventandstop = function (event) {
    event.preventDefault();
    event.stopPropagation();
  };
  document.addEventListener ('contextmenu', preventandstop, true);
  setTimeout (() => document.removeEventListener ('contextmenu', preventandstop, true), timeout);
}

/** Determine position for a popup */
export function popup_position (element, opts = { origin: undefined, x: undefined, y: undefined }) {
  const p = 1; // padding;
  // Ignore window.innerWidth & window.innerHeight, these include space for scrollbars
  // Viewport size, https://developer.mozilla.org/en-US/docs/Web/CSS/Viewport_concepts
  const vw = document.documentElement.clientWidth, vh = document.documentElement.clientHeight;
  // Scroll offset, needed to convert viewport relative to document relative
  const sx = window.pageXOffset, sy = window.pageYOffset;
  // Element area, relative to the viewport.
  const r = element.getBoundingClientRect();
  // Position element without an origin element
  if (!opts.origin || !opts.origin.getBoundingClientRect)
    {
      // Position element at document relative (opts.x, opts.y)
      if (opts.x >= 0 && opts.x <= 999999 && opts.y >= 0 && opts.y <= 999999)
	{
	  let vx = Math.max (0, opts.x - sx); // document coord to viewport coord
	  let vy = Math.max (0, opts.y - sy); // document coord to viewport coord
	  // Shift left if neccessary
	  if (vx + r.width + p > vw)
	    vx = vw - r.width - p;
	  // Shift up if neccessary
	  if (vy + r.height + p > vh)
	    vy = vh - r.height - p;
	  return { x: sx + Math.max (0, vx), y: sy + Math.max (0, vy) }; // viewport coords to document coords
	}
      // Center element, but keep top left onscreen
      const vx = (vw - r.width) / 2, vy = (vh - r.height) / 2;
      return { x: sx + Math.max (0, vx), y: sy + Math.max (0, vy) }; // viewport coords to document coords
    }
  // Position element relative to popup origin
  const o = opts.origin.getBoundingClientRect();
  let vx = o.x, vy = o.y + o.height;	// left aligned, below origin
  // Shift left if neccessary
  if (vx + r.width + p > vw)
    vx = vw - r.width - p;
  // Shift up if neccessary
  if (vy + r.height + p > vh)
    vy = Math.min (vh - r.height - p, o.y);
  return { x: sx + Math.max (0, vx), y: sy + Math.max (0, vy) }; // viewport coords to document coords
}

/** Resize canvas display size (CSS size) and resize backing store to match hardware pixels */
export function resize_canvas (canvas, csswidth, cssheight, fill_style = false) {
  /* Here we fixate the canvas display size at (csswidth,cssheight) and then setup the
   * backing store to match the hardware screen pixel size.
   * Note, just assigning canvas.width *without* assigning canvas.style.width may lead to
   * resizes in the absence of other constraints. So to render at screen pixel size, we
   * always have to assign canvas.style.{width|height}.
   */
  const cw = Math.round (csswidth), ch = Math.round (cssheight);
  const pw = Math.round (window.devicePixelRatio * cw);
  const ph = Math.round (window.devicePixelRatio * ch);
  if (cw != canvas.style.width || ch != canvas.style.height ||
      pw != canvas.width || ph != canvas.height || fill_style) {
    canvas.style.width = cw + 'px';
    canvas.style.height = ch + 'px';
    canvas.width = pw;
    canvas.height = ph;
    const ctx = canvas.getContext ('2d');
    if (!fill_style || fill_style === true)
      ctx.clearRect (0, 0, canvas.width, canvas.height);
    else {
      ctx.fillStyle = fill_style;
      ctx.fillRect (0, 0, canvas.width, canvas.height);
    }
    return true;
  }
  return false;
}

/** Draw a horizontal line from (x,y) of width `w` with dashes `d` */
export function dash_xto (ctx, x, y, w, d) {
  for (let i = 0, p = x; p < x + w; p = p + d[i++ % d.length]) {
    if (i % 2)
      ctx.lineTo (p, y);
    else
      ctx.moveTo (p, y);
  }
}

/** Draw a horizontal rect `(x,y,width,height)` with pixel gaps of width `stipple` */
export function hstippleRect (ctx, x, y, width, height, stipple) {
  for (let s = x; s + stipple < x + width; s += 2 * stipple)
    ctx.fillRect (s, y, stipple, height);
}

/** Fill and stroke a canvas rectangle with rounded corners. */
export function roundRect (ctx, x, y, width, height, radius, fill = true, stroke = true) {
  if (typeof radius === 'number')
    radius = [ radius, radius, radius, radius ];
  else if (typeof radius === 'object' && radius.length == 4)
    ; // top-left, top-right, bottom-right, bottom-left
  else
    throw new Error ('invalid or missing radius');
  ctx.beginPath();
  ctx.moveTo           (x + radius[0],         y);
  ctx.lineTo           (x + width - radius[1], y);
  ctx.quadraticCurveTo (x + width,             y,                      x + width,             y + radius[1]);
  ctx.lineTo           (x + width,             y + height - radius[2]);
  ctx.quadraticCurveTo (x + width,             y + height,             x + width - radius[2], y + height);
  ctx.lineTo           (x + radius[3],         y + height);
  ctx.quadraticCurveTo (x,                     y + height,             x,                     y + height - radius[3]);
  ctx.lineTo           (x,                     y + radius[0]);
  ctx.quadraticCurveTo (x,                     y,                      x + radius[0],         y);
  ctx.closePath();
  if (fill)
    ctx.fill();
  if (stroke)
    ctx.stroke();
}

/** Add color stops from `stoparray` to `grad`, `stoparray` is an array: [(offset,color)...] */
export function gradient_apply_stops (grad, stoparray) {
  for (const g of stoparray)
    grad.addColorStop (g[0], g[1]);
}

/** Create a new linear gradient at (x1,y1,x2,y2) with color stops `stoparray` */
export function linear_gradient_from (ctx, stoparray, x1, y1, x2, y2) {
  const grad = ctx.createLinearGradient (x1, y1, x2, y2);
  gradient_apply_stops (grad, stoparray);
  return grad;
}

/** Measure ink span of a canvas text string or an array */
export function canvas_ink_vspan (font_style, textish = 'gM') {
  let canvas, ctx, cwidth = 64, cheight = 64;
  function measure_vspan (text) {
    const cache_key = font_style + ':' + text;
    let result = canvas_ink_vspan.cache[cache_key];
    if (!result)
      {
	if (canvas === undefined) {
	  canvas = document.createElement ('canvas');
	  ctx = canvas.getContext ('2d');
	}
	/* BUG: electron-1.8.3 (chrome-59) is unstable (shows canvas memory
	 * corruption) at tiny zoom levels without canvas size assignments.
	 */
	const text_em = ctx.measureText ("MW").width;
	const twidth = Math.max (text_em * 2, ctx.measureText (text).width);
	cwidth = Math.max (cwidth, 2 * twidth);
	cheight = Math.max (cheight, 3 * text_em);
	canvas.width = cwidth;
	canvas.height = cheight;
	ctx.fillStyle = '#000000';
	ctx.fillRect (0, 0, canvas.width, canvas.height);
	ctx.font = font_style;
	ctx.fillStyle = '#ffffff';
	ctx.textBaseline = 'top';
	ctx.fillText (text, 0, 0);
	const pixels = ctx.getImageData (0, 0, canvas.width, canvas.height).data;
	let start = -1, end = -1;
	for (let row = 0; row < canvas.height; row++)
	  for (let col = 0; col < canvas.width; col++) {
	    let index = (row * canvas.width + col) * 4; // RGBA
	    if (pixels[index + 0] > 0) {
	      if (start < 0)
		start = row;
	      else
		end = row;
	      break;
	    }
	  }
	result = start >= 0 && end >= 0 ? [start, end - start] : [0, 0];
	canvas_ink_vspan.cache[cache_key] = result;
      }
    return result;
  }
  return Array.isArray (textish) ? textish.map (txt => measure_vspan (txt)) : measure_vspan (textish);
}
canvas_ink_vspan.cache = [];

/** Retrieve the 'C-1' .. 'G8' label for midi note numbers */
export function midi_label (numish) {
  function one_label (num) {
    const letter = [ 'C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B' ];
    const oct = Math.floor (num / letter.length) - 1;
    const key = num % letter.length;
    return letter[key] + oct;
  }
  return Array.isArray (numish) ? numish.map (n => one_label (n)) : one_label (numish);
}

let frame_handler_id = 0x200000,
    frame_handler_active = false,
    frame_handler_callback_id = undefined,
    frame_handler_cur = 0,
    frame_handler_max = 0,
    frame_handler_array = undefined;

function call_frame_handlers () {
  frame_handler_callback_id = undefined;
  const active = frame_handler_active && shm_array_active;
  frame_handler_max = frame_handler_array.length;
  for (frame_handler_cur = 0; frame_handler_cur < frame_handler_max; frame_handler_cur++) {
    const handler_id = frame_handler_array[frame_handler_cur][1];
    const handler_result = frame_handler_array[frame_handler_cur][0] (active);
    if (handler_result !== undefined && !handler_result)
      remove_frame_handler (handler_id);
  }
  if (frame_handler_active)
    frame_handler_callback_id = window.requestAnimationFrame (call_frame_handlers);
}

function reinstall_frame_handler() {
  if (frame_handler_callback_id !== undefined) {
    window.cancelAnimationFrame (frame_handler_callback_id);
    frame_handler_callback_id = undefined;
  }
  if (frame_handler_active)
    frame_handler_callback_id = window.requestAnimationFrame (call_frame_handlers);
  else
    call_frame_handlers(); // call one time for cleanups with frame_handler_active == false
}

function remove_frame_handler (handler_id) {
  for (let i = frame_handler_array.length - 1; i >= 0; i--)
    if (frame_handler_array[i][1] == handler_id) {
      frame_handler_array.splice (i, 1);
      if (i < frame_handler_cur)
	frame_handler_cur--;
      frame_handler_max--;
      return;
    }
  console.error ("remove_frame_handler(" + handler_id + "): invalid id");
}

/// Install a permanent redraw handler, to run as long as the DSP engine is active.
export function add_frame_handler (handlerfunc) {
  if (frame_handler_array === undefined) { // must initialize
    frame_handler_array = [];
    (async function() {
      const check_active_promise = Bse.server.engine_active();
      const onenginechange_promise = Bse.server.on ('enginechange', (ev) => {
	frame_handler_active = Boolean (ev.active);
	shm_reschedule();
	reinstall_frame_handler();
      } );
      frame_handler_active = Boolean (await check_active_promise);
      shm_reschedule();
      reinstall_frame_handler();
      await onenginechange_promise;
    }) ();
  }
  const handler_id = frame_handler_id++;
  frame_handler_array.push ([handlerfunc, handler_id]);
  if (!frame_handler_callback_id) // ensure handlerfunc is called at least once
    frame_handler_callback_id = window.requestAnimationFrame (call_frame_handlers);
  return function() { remove_frame_handler (handler_id); };
}

export function discard_remote (bseobject) {
  console.log ("FIXME: Bse/discard:", bseobject);
}

let shm_array_active = false;
let shm_array_entries = []; // { bpos, blength, shmoffset, usecount, }
let shm_array_binary_size = 8;
let shm_array_buffer = new ArrayBuffer (0);
export let shm_array_int32   = undefined;	// assigned by shm_receive
export let shm_array_float32 = undefined;	// assigned by shm_receive
export let shm_array_float64 = undefined;	// assigned by shm_receive
shm_receive (null);

export function shm_receive (arraybuffer) {
  shm_array_active = frame_handler_active && arraybuffer && shm_array_binary_size <= arraybuffer.byteLength;
  if (shm_array_active)
    shm_array_buffer = arraybuffer;
  else
    shm_array_buffer = new ArrayBuffer (shm_array_binary_size);
  shm_array_int32   = new Int32Array (shm_array_buffer, 0, shm_array_buffer.byteLength / 4 ^0);
  shm_array_float32 = new Float32Array (shm_array_buffer, 0, shm_array_buffer.byteLength / 4 ^0);
  shm_array_float64 = new Float64Array (shm_array_buffer, 0, shm_array_buffer.byteLength / 8 ^0);
}

export function shm_subscribe (byteoffset, bytelength) {
  const lalignment = 4;
  bytelength = (((bytelength + lalignment-1) / lalignment) ^0) * lalignment;
  // reuse existing region
  for (let i = 0; i < shm_array_entries.length; ++i)
    {
      const e = shm_array_entries[i];
      if (e.shmoffset <= byteoffset && e.shmoffset + e.blength >= byteoffset + bytelength)
	{
	  const pos = e.bpos + byteoffset - e.shmoffset;
	  e.usecount += 1;
	  return [ pos, i ];
	}
    }
  // reallocate freed region
  for (let i = 0; i < shm_array_entries.length; ++i)
    {
      const e = shm_array_entries[i];
      if (e.usecount == 0 && e.blength == bytelength)
	{
	  e.shmoffset = byteoffset;
	  e.usecount = 1;
	  shm_reschedule();
	  return [ e.bpos, i ];
	}
    }
  // allocate new pos, if length exceeds 4, pos needs larger alignment (e.g. for double)
  let nextpos = shm_array_binary_size;
  if (bytelength > 4)
    nextpos = (((nextpos + 8-1) / 8) ^0) * 8;
  // allocate new region
  let e = {
    bpos: nextpos,
    blength: bytelength,
    shmoffset: byteoffset,
    usecount: 1,
  };
  shm_array_binary_size = nextpos + bytelength;
  shm_array_entries.push (e);
  shm_reschedule();
  return [ e.bpos, shm_array_entries.length - 1 ];
}

export function shm_unsubscribe (subscription_tuple) {
  const e = shm_array_entries[subscription_tuple[1]];
  console.assert (e.usecount > 0);
  if (e.usecount)
    {
      e.usecount--;
      if (e.usecount == 0)
	{
	  e.shmoffset = -1;
	  // entry is disabled but stays around for future allocations and stable indexing
	  shm_reschedule();
	}
      return true;
    }
  return false;
}

function shm_reschedule() {
  if (pending_shm_reschedule_promise)
    return;
  pending_shm_reschedule_promise = delay (-1); // Vue.nextTick();
  pending_shm_reschedule_promise.then (async () => {
    pending_shm_reschedule_promise = null; // allow new shm_reschedule() calls
    if (frame_handler_active)
      {
	const entries = copy_recursively (shm_array_entries);
	for (let i = entries.length - 1; i >= 0; i--)
	  {
	    if (entries[i].usecount == 0)
	      entries.splice (i, 1);
	    else
	      delete entries[i].usecount;
	  }
	await Bse.server.broadcast_shm_fragments (entries, 33);
      }
    else
      {
	const promise = Bse.server.broadcast_shm_fragments ([], 0);
	shm_receive (null);
	await promise;
      }
  });
}
let pending_shm_reschedule_promise = null;

/// Create a promise that resolves after the given `timeout` with `value`.
function delay (timeout, value) {
  return new Promise (resolve => {
    if (timeout >= 0)
      setTimeout (resolve.bind (null, value), timeout);
    else if (window.setImmediate)
      setImmediate (resolve.bind (null, value));
    else
      setTimeout (resolve.bind (null, value), 0);
  });
}

// Format window titles
export function format_title (prgname, entity = undefined, infos = undefined, extras = undefined) {
  let title = prgname;
  if (entity)
    {
      let sub = entity;
      if (infos)
	{
	  sub += ' - ' + infos;
	  if (extras)
	    sub += ' (' + extras + ')';
	}
      title = sub + ' – ' + title;
    }
  return title;
}

let keyboard_click_state = { inclick: 0 };

/// Check if the current click event originates from keyboard activation.
export function in_keyboard_click()
{
  return keyboard_click_state.inclick > 0;
}

/// Trigger element click via keyboard.
export function keyboard_click (element)
{
  if (element)
    {
      keyboard_click_state.inclick += 1;
      if (!element.classList.contains ('active'))
	{
	  const e = element;
	  e.classList.toggle ('active', true);
	  setTimeout (() => e.classList.toggle ('active', false), 170); // match focus-activation delay
	}
      element.click();
      keyboard_click_state.inclick -= 1;
      return true;
    }
  return false;
}

/// Check if an element can be found in a given array.
function in_array (element, array)
{
  return array.indexOf (element) >= 0;
}

/// Export key codes
export const KeyCode = {
  BACKSPACE: 8, TAB: 9, ENTER: 13, RETURN: 13, CAPITAL: 20, CAPSLOCK: 20, ESC: 27, ESCAPE: 27, SPACE: 32, PAGEUP: 33, PAGEDOWN: 34,
  END: 35, HOME: 36, LEFT: 37, UP: 38, RIGHT: 39, DOWN: 40, PRINTSCREEN: 44, INSERT: 45, DELETE: 46, SELECT: 93,
  F1: 112, F2: 113, F3: 114, F4: 115, F5: 116, F6: 117, F7: 118, F8: 119, F9: 120, F10: 121, F11: 122, F12: 123,
  F13: 124, F14: 125, F15: 126, F16: 127, F17: 128, F18: 129, F19: 130, F20: 131, F21: 132, F22: 133, F23: 134, F24: 135,
  BROWSERBACK: 166, BROWSERFORWARD: 167, PLUS: 187/*FIXME*/, MINUS: 189/*FIXME*/, PAUSE: 230, ALTGR: 255,
  VOLUMEMUTE: 173, VOLUMEDOWN: 174, VOLUMEUP: 175, MEDIANEXTTRACK: 176, MEDIAPREVIOUSTRACK: 177, MEDIASTOP: 178, MEDIAPLAYPAUSE: 179,
};

const navigation_keys = [
  KeyCode.UP, KeyCode.DOWN, KeyCode.LEFT, KeyCode.RIGHT,
  KeyCode.TAB, KeyCode.SPACE, KeyCode.ENTER /*13*/, 10 /*LINEFEED*/,
  KeyCode.PAGE_UP, KeyCode.PAGE_DOWN, KeyCode.HOME, KeyCode.END,
  KeyCode.SELECT /*contextmenu*/, KeyCode.ESCAPE,
];

/// Check if a key code is used of rnavigaiton (and non alphanumeric).
export function is_navigation_key_code (keycode)
{
  return in_array (keycode, navigation_keys);
}

/// Match an event's key code, considering modifiers.
export function match_key_event (event, keyname)
{
  // SEE: http://unixpapa.com/js/key.html & https://developer.mozilla.org/en-US/docs/Mozilla/Gecko/Gecko_keypress_event
  // split_hotkey (hotkey)
  const rex = new RegExp (/\s*[+]\s*/); // Split 'Shift+Ctrl+Alt+Meta+SPACE'
  const parts = keyname.toLowerCase().split (rex);
  const char = String.fromCharCode (event.which || event.keyCode);
  let need_meta = 0, need_alt = 0, need_ctrl = 0, need_shift = 0;
  for (let i = 0; i < parts.length; i++)
    {
      // collect meta keys
      switch (parts[i])
      {
	case 'cmd': case 'command':
	case 'super': case 'meta':	need_meta  = 1; continue;
	case 'option': case 'alt':	need_alt   = 1; continue;
	case 'control': case 'ctrl':	need_ctrl  = 1; continue;
	case 'shift':		  	need_shift = 1; continue;
      }
      // match named keys (special)
      const key_val = KeyCode[parts[i].toUpperCase()];
      if (key_val !== undefined && char.length == 1 && key_val == char.charCodeAt (0))
	continue;
      // match characters
      if (char.toLowerCase() == parts[i])
	continue;
      // failed to match
      return false;
    }
  // ignore shift for case insensitive characters (except for navigations)
  if (char.toLowerCase() == char.toUpperCase() &&
      !is_navigation_key_code (event.keyCode))
    need_shift = -1;
  // match meta keys
  if (need_meta   != 0 + event.metaKey ||
      need_alt    != 0 + event.altKey ||
      need_ctrl   != 0 + event.ctrlKey ||
      (need_shift != 0 + event.shiftKey && need_shift >= 0))
    return false;
  return true;
}

class UtilResizeObserver {
  constructor (close_handler, preserve_element, opts) {
    this.observers = new Map();	// { observer: { callback, elements:[] } }
    this.have_ResizeObserver = window.ResizeObserver != undefined;
    this.listening = false;
  }
  add_owner (owner, callback) {
    if (!this.have_ResizeObserver && !this.listening)
      {
	window.addEventListener ('resize', () => this.resized());
	this.listening = true;
      }
    let wrs = undefined;
    if (window.ResizeObserver != undefined)
      wrs = new window.ResizeObserver (() => callback());
    const new_observer = {
      callback: callback,
      elements: [],
      wrs: wrs,
      observe (ele) {
	if (this.wrs)
	  this.wrs.observe (ele);
	else
	  this.elements.push (ele);
      },
      unobserve (ele) {
	if (this.wrs)
	  this.wrs.unobserve (ele);
	else
	  {
	    const i = this.elements.indexOf (ele);
	    if (i >= 0)
	      this.elements.splice (i, 1);
	  }
      },
      disconnect() {
	if (this.wrs)
	  this.wrs.disconnect();
	else
	  this.elements = [];
      }
    };
    this.observers[owner] = new_observer;
    return new_observer;
  }
  resized() {
    for (let o in this.observers)
      if (this.observers[o].elements.length)
	this.observers[o].callback (o);
  }
}
const utilresizeobserver = new UtilResizeObserver();

/// Create a ResizeObserver object
export function resize_observer (owner, callback) {
  return utilresizeobserver.add_owner (owner, callback);
}
