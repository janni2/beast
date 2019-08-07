# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/ebeast/*.d)
ebeast/cleandirs ::= $(wildcard $>/ebeast/ $>/electron/ $>/app/)
CLEANDIRS         += $(ebeast/cleandirs)
ALL_TARGETS       += ebeast/all
ebeast/all: $>/ebeast/app.rules

# This Makefile creates $>/electron/ebeast and builds the ebeast app in $>/app/.

# Running ebeast:
# - Use 'make run' to start ebeast from the development tree, this ensures that libbse
#   is picked up from the development tree (instead of an installed version) and that
#   electron finds the ebeast app/ files.
# - DevTools can be activated with Shft+Ctrl+I when run from the devleopment tree.
#   To use DevTools on the installed ebeast bundle, install electron-devtools-installer
#   as npm package in $(pkginstalldir)/bundle/app/.
# - When DevTools are enabled, Shift+Ctrl+R can initiate an ebeast page reload.

# == sources ==
ebeast/js.inputs ::= $(strip 		\
	ebeast/app.html			\
	ebeast/jsbse.js			\
	$>/app/bseapi_jsonipc.js	\
	ebeast/main.js			\
	ebeast/menus.js			\
	ebeast/utilities.js		\
	ebeast/window.html		\
)
ebeast/b/js.inputs	::= $(wildcard ebeast/b/*.js)
ebeast/b/vue.inputs	::= $(wildcard ebeast/b/*.vue)
ebeast/app.scss.d	::= $(wildcard ebeast/*.scss ebeast/b/*.scss)
ebeast/b/bundle.js.d   ::= $(wildcard ebeast/*.js ebeast/b/*.js)
ebeast/b/bundle.vue.d  ::= $(wildcard ebeast/b/*.vue)
ebeast/lint.appfiles    ::= $(ebeast/b/bundle.js.d) $(ebeast/b/bundle.vue.d)
ebeast/b/scss.inputs	::= $(wildcard ebeast/b/*.scss)
app/files.js		::= $(addprefix $>/app/,    $(notdir $(ebeast/js.inputs)))
app/copies		::= $(strip 	\
	$>/app/app.html			\
	$>/app/jsbse.js			\
	$>/app/menus.js			\
	$>/app/utilities.js		\
	$>/app/window.html		\
)
app/sources		::= $(strip 	\
	$>/app/bseapi_jsonipc.js	\
	$>/app/main.js			\
	$(app/copies)			\
)
app/assets/tri-pngs	::= $(strip	\
	$>/app/assets/tri-n.png		\
	$>/app/assets/tri-e.png		\
	$>/app/assets/tri-s.png		\
	$>/app/assets/tri-w.png		\
)
app/generated 		::= $(strip	\
	$(app/assets/tri-pngs)		\
	$>/app/assets/gradient-01.png	\
	$>/app/assets/forkawesome-webfont.css	\
	$>/app/assets/stylesheets.css	\
	$>/app/assets/components.js	\
	$>/app/assets/material-icons.css \
)
# provide node_modules/ for use in other makefiles
NODE_MODULES.deps ::= $>/ebeast/npm.rules
NODE_MODULES.bin  ::= $>/ebeast/node_modules/.bin/

# == subdirs ==
include ebeast/v8bse/Makefile.mk

# == npm ==
NPM_INSTALL = npm --prefer-offline install $(if $(PARALLEL_MAKE), --progress=false)
$>/ebeast/npm.rules: ebeast/package.json.in	| $>/ebeast/ $>/app/
	$(QECHO) MAKE $@
	$Q rm -f -r $>/ebeast/node_modules/ $>/app/node_modules/
	$Q sed	-e 's/@MAJOR@/$(VERSION_MAJOR)/g' \
		-e 's/@MINOR@/$(VERSION_MINOR)/g' \
		-e 's/@MICRO@/$(VERSION_MICRO)/g' \
		$< > $>/app/package.json
	$Q cd $>/app/ \
	  && $(NPM_INSTALL) --production \
	  && rm -f package-lock.json \
	  && find . -name package.json -print0 | xargs -0 sed -r "\|$$PWD|s|^(\s*(\"_where\":\s*)?)\"$$PWD|\1\"/...|" -i
	$Q $(CP) -a $>/app/node_modules $>/app/package.json $>/ebeast/
	$Q cd $>/ebeast/ \
	  && $(NPM_INSTALL)
	$Q echo >$@

# == linting ==
ebeast/sed.uncommentjs ::= sed -nr 's,//.*$$,,g ; 1h ; 1!H ; $$ { g; s,/\*(\*[^/]|[^*])*\*/,,g ; p }' # beware, ignores quoted strings
$>/ebeast/lint.rules: $(ebeast/lint.appfiles) | $>/ebeast/npm.rules
	$(QECHO) MAKE $@
	$Q $>/ebeast/node_modules/.bin/eslint -c ebeast/.eslintrc.js -f unix $(ebeast/lint.appfiles)
	@: # check for component pitfalls
	$Q for f in $(ebeast/b/vue.inputs) ; do \
	  $(ebeast/sed.uncommentjs) "$$f" | sed -e "s,^,$$f: ," \
	  | grep --color=auto '\b__dirname\b' \
	  && { echo 'Error: __dirname is invalid inside Vue component files' | grep --color=auto . ; exit 9 ; } ; \
	done ; :
	$Q echo >$@
ebeast-lint: FORCE
	@rm -f $>/ebeast/lint.rules
	@$(MAKE) $>/ebeast/lint.rules

# == app ==
$>/ebeast/app.rules: $(app/sources) $(app/generated) $>/ebeast/lint.rules $>/ebeast/v8bse/v8bse.node
	$(QECHO) MAKE $@
	$Q $(CP) -L $>/ebeast/v8bse/v8bse.node $>/app/assets/
	$Q rm -f -r $>/electron/ \
	  && $(CP) -a $>/ebeast/node_modules/electron/dist/ $>/electron/ \
	  && rm -fr $>/electron/resources/default_app.asar \
	  && mv $>/electron/electron $>/electron/ebeast
	$Q ln -s ../../app $>/electron/resources/app
	$Q echo >$@
$(app/copies): $>/app/%: ebeast/%		| $>/app/
	$(QECHO) COPY $@
	$Q $(CP) -P $< $@

# == $>/app/main.js ==
$>/app/main.js: ebeast/main.js $(GITCOMMITDEPS)	| $>/app/
	$(QGEN)
	$Q sed < $<	> $@.tmp \
		-e "1,10s|^ *//@EBEAST_GLOBALCONFIG@|  ${ebeast/globalconfig}|"
	$Q mv $@.tmp $@
ebeast/globalconfig =	revision: '$(shell ./version.sh -l)', \
			revdate: '$(shell ./version.sh -d)', \
			debug: $(if $(findstring debug, $(MODE)),true,false)

# == $>/app/main.js ==
$>/app/bseapi_jsonipc.js: jsonipc/head.js $(lib/BeastSoundEngine) bse/bseapi.idl	| $>/app/
	$(QGEN)
	$Q cat jsonipc/head.js			> $@.tmp
	$Q $(lib/BeastSoundEngine) --js-bseapi	>>$@.tmp
	$Q rm -f $@ && chmod -w $@.tmp
	$Q mv $@.tmp $@

# == $>/app/assets/ ==
ebeast/inter-typeface-downloads ::= \
  5f310d16c579ab3b1e9e8cb3298e14bb935ed7e802e1b23c35bd1819307d6c59 \
    https://github.com/rsms/inter/raw/v3.5/docs/font-files/Inter-Medium.woff2
$>/app/assets/Inter-Medium.woff2:			| $>/app/assets/
	$(QGEN)
	$Q cd $(@D) \
		$(call foreachpair, AND_DOWNLOAD_SHAURL, $(ebeast/inter-typeface-downloads))
$>/app/assets/stylesheets.css: $(ebeast/app.scss.d) $>/app/assets/Inter-Medium.woff2	| $>/ebeast/npm.rules
	$(QGEN) # NOTE: scss source and output file locations must be final, because .map is derived from it
	$Q : # cd $>/app/ && ../ebeast/node_modules/.bin/node-sass app.scss assets/stylesheets.css --source-map true
	$Q $>/ebeast/node_modules/.bin/node-sass ebeast/app.scss $>/app/assets/stylesheets.css \
		--include-path ebeast/ --include-path $>/ebeast/ --source-map true
$>/app/assets/material-icons.css:			| $>/app/assets/
	$(QECHO) FETCH material-icons-190326.1.tar.xz
	$Q cd $>/app/assets/ \
	     $(call AND_DOWNLOAD_SHAURL, \
		53eba258da6170f5aa3925579f1552ef7d7a06d5b762260efac5e26d5f95e721, \
		  https://github.com/tim-janik/assets/releases/download/material-icons-190326.1/material-icons-190326.1.tar.xz)
	$(QGEN)
	$Q tar -C $>/app/assets/ -xf $>/app/assets/material-icons-190326.1.tar.xz
	$Q mv $>/app/assets/material-icons/material-icons.woff2 $>/app/assets/material-icons/material-icons.css $>/app/assets/
	$Q rm $>/app/assets/material-icons-190326.1.tar.xz && rm -r $>/app/assets/material-icons/
ebeast/fork-awesome-downloads ::= \
  844517a2bc5430242cb857e56b6dccf002f469c4c1b295ed8d0b7211fb452f50 \
    https://raw.githubusercontent.com/ForkAwesome/Fork-Awesome/b0605a81632452818bf19c8fa97469da1206b52b/fonts/forkawesome-webfont.woff2 \
  630b0e84fa43579f7e97a26fd47d4b70cb5516ca7e6e73393597d12ca249a8ee \
    https://raw.githubusercontent.com/ForkAwesome/Fork-Awesome/b0605a81632452818bf19c8fa97469da1206b52b/css/fork-awesome.css
$>/app/assets/forkawesome-webfont.css:				| $>/app/assets/
	$(QGEN)
	$Q cd $(@D) $(call foreachpair, AND_DOWNLOAD_SHAURL, $(ebeast/fork-awesome-downloads))
	$Q sed "/^ *src: *url/s,src: *url(.*);,src: url('forkawesome-webfont.woff2');," -i $>/app/assets/fork-awesome.css
	$Q mv $>/app/assets/fork-awesome.css $@
$>/app/assets/gradient-01.png: $>/app/assets/stylesheets.css ebeast/Makefile.mk
	$(QGEN) # generate non-banding gradient from stylesheets.css: gradient-01 { -im-convert: "..."; }
	$Q      # see: http://www.imagemagick.org/script/command-line-options.php#noise http://www.imagemagick.org/Usage/canvas/
	$Q tr '\n' ' ' < $>/app/assets/stylesheets.css | \
	     sed -nr 's/.*@supports\s*\(--makefile:\s*rule\)\s*\{\s*gradient-01\s*\{\s*im-convert:\s*"([^"]*)"\s*[;}].*/\1/; T; p' > $@.cli
	$Q test -s $@.cli # check that we actually found the -im-convert directive
	$Q $(IMAGEMAGICK_CONVERT) $$(cat $@.cli) $@.tmp.png
	$Q rm $@.cli && mv $@.tmp.png $@
$>/app/assets/tri-n.png: ebeast/triangle32.png $>/app/assets/stylesheets.css	| $>/app/assets/
	$(QGEN)
	$Q tr '\n' ' ' < $>/app/assets/stylesheets.css | \
	     sed -nr 's/.*@supports\s*\(--makefile:\s*rule\)\s*\{\s*scrollbar-arrow\s*\{\s*im-convert:\s*"([^"]*)"\s*[;}].*/\1/; T; p' > $@.cli
	$Q test -s $@.cli # check that we actually found the -im-convert directive
	$Q $(IMAGEMAGICK_CONVERT) $< $$(cat $@.cli) $@.tmp.png
	$Q rm $@.cli && mv $@.tmp.png $@
$>/app/assets/tri-e.png: $>/app/assets/tri-n.png
	$(QGEN)
	$Q $(IMAGEMAGICK_CONVERT) $< -rotate 90 $@.tmp.png
	$Q mv $@.tmp.png $@
$>/app/assets/tri-s.png: $>/app/assets/tri-n.png
	$(QGEN)
	$Q $(IMAGEMAGICK_CONVERT) $< -rotate 180 $@.tmp.png
	$Q mv $@.tmp.png $@
$>/app/assets/tri-w.png: $>/app/assets/tri-n.png
	$(QGEN)
	$Q $(IMAGEMAGICK_CONVERT) $< -rotate 270 $@.tmp.png
	$Q mv $@.tmp.png $@

# == assets/components.js ==
$>/app/assets/components.js: $(ebeast/b/bundle.js.d) $(ebeast/b/bundle.vue.d) $(ebeast/app.scss.d)	| $>/ebeast/npm.rules
	$(QGEN)
	@: # set NODE_PATH, since browserify fails to search ./node_modules for a ../ entry point
	$Q cd $>/ebeast/ \
	  && NODE_PATH=node_modules node_modules/.bin/browserify --node --debug -t vueify \
		-e ../$(build2srcdir)/ebeast/b/bundle.js -o ../app/assets/components.js

# == installation ==
ebeast/install: $>/ebeast/app.rules FORCE
	@$(QECHO) INSTALL '$(DESTDIR)$(pkglibdir)/{app|electron}'
	$Q rm -f -r $(DESTDIR)$(pkglibdir)/electron $(DESTDIR)$(pkglibdir)/app
	$Q $(CP) -a $>/electron $>/app $(DESTDIR)$(pkglibdir)/
install: ebeast/install
ebeast/uninstall: FORCE
	@$(QECHO) REMOVE '$(DESTDIR)$(pkglibdir)/{app|electron}'
	$Q rm -f -r $(DESTDIR)$(pkglibdir)/electron $(DESTDIR)$(pkglibdir)/app
uninstall: ebeast/uninstall

# == ebeast/run ==
# export ELECTRON_ENABLE_LOGGING=1
ebeast/run: $>/ebeast/app.rules $>/doc/beast-manual.html
	test -f /usr/share/themes/Ambiance/gtk-2.0/gtkrc && export GTK2_RC_FILES='/usr/share/themes/Ambiance/gtk-2.0/gtkrc' ; \
	$>/electron/ebeast
#	LD_PRELOAD="$>/bse/libbse-$(VERSION_MAJOR).so"

# == ebeast/vue-docs ==
$>/ebeast/vue-docs.md: $(ebeast/b/vue.inputs) ebeast/Makefile.mk docs/filt-docs2.py	| $>/ebeast/
	$(QGEN)
	$Q echo -e "<!-- Vue Components -->\n\n"				> $@.tmp
	@: # extract <docs/> blocks from vue files and feed them into pandoc
	$Q for f in $(sort $(ebeast/b/vue.inputs)) ; do \
	    echo ""								>>$@.tmp ; \
	    sed -n '/^<docs>\s*$$/{ :loop n; /^<\/docs>/q; p;  b loop }' <"$$f"	>>$@.tmp \
	    || exit $$? ; \
	done
	$Q sed -r 's/^  // ; s/^#/\n#/; ' -i $@.tmp # unindent, spread heading_without_preceding_blankline
	$Q $(PANDOC) -t markdown -F docs/filt-docs2.py -f markdown+compact_definition_lists $@.tmp -o $@.tmp2
	$Q mv $@.tmp2 $@ && rm -f $@.tmp

# NOTE1, prefer LD_PRELOAD over LD_LIBRARY_PATH, to pick up $(builddir)/libbse *before* /usr/lib/libbse
# NOTE2, add --js-flags="--expose-gc" to the command line to enable global.gc();
# If libdbusmenu-glib.so is missing, electron 1.4.15 displays a Gtk+2 menu bar, ignoring
# the BrowserWindow.darkTheme option. Here, we preselect a commonly installed dark Gtk+2
# theme if it's present.

# == ebeast/clean ==
ebeast/clean: FORCE
	rm -f -r $(ebeast/cleandirs)
