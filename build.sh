#! /bin/sh

# This is a simple wrapper script to make compiling a bit easier.
# Configure is too verbose and curious about many things and not
# sufficiently eloquent about others such as available options.

# Bail out on unexpected errors
set -e

# This is not interactive
exec </dev/null

# The option --target must be handled first because it modifies some
# defaults which the user, however, should be able to override.
build_target="`(
    while [ $# -gt 0 ]
    do
        case "$1" in
        --target=*) echo "${1#--*=}"
                    exit
                    ;;
        --)         break
                    ;;
        esac
        shift
    done
    uname -s 2>/dev/null || :
    exit
)`"

# Standard variables like CC, CFLAGS etc. default to the current environment
build_bindir=
build_configure_only=
build_datadir=
build_dbus=
build_gnutls=
build_halloc=
build_ipv6=
build_libdir=
build_localedir=
build_nls=
build_official=
build_so_suffix=
build_socker=
build_ui=
build_verbose='-s'

# There is something broken about Configure, so it needs to know the
# suffix for shared objects (dynamically loaded libraries) for some odd
# reasons.
case "$build_target" in
darwin|Darwin)
    build_so_suffix='dylib'
    ;;
MINGW*)
    # FIXME: The MingW installation prefix is hardcoded (as default) for now,
    # This could be detected maybe. On Ubuntu and Debian it is
    # /usr/i586-mingw32msvc, so --ldflags must be used manually.
	mingwlib=/mingw/lib
	PATH="$PATH${PATH:+:}${mingwlib}/gtk/bin:${mingwlib}/xml2/bin"
	export PATH
    CC="${CC:-gcc}"
	CPPFLAGS="$CPPFLAGS -DMINGW32"
	CPPFLAGS="$CPPFLAGS -I${mingwlib}/regex/include -I${mingwlib}/gtk/include"
    CPPFLAG="${CPPFLAGS# *}"    # strip leading spaces
	LDFLAGS="$LDFLAGS -mwindows -L${mingwlib}/regex/lib -L${mingwlib}/gtk/lib"
    LDFLAGS="${LDFLAGS# *}"     # strip leading spaces
	LIBS="$LIBS -lwsock32 -lws2_32 -lregex -lz -liconv -limagehlp -liphlpapi"
	LIBS="$LIBS -lws2_32 -lpowrprof -lpsapi -lkernel32"
    LIBS="${LIBS# *}"           # strip leading spaces
	;;
esac

while [ $# -gt 0 ]; do
	case "$1" in
	--bindir=*)			build_bindir="${1#--*=}";;
	--cc=*)				CC="${1#--*=}";;
	--cflags=*)			CFLAGS="${1#--*=}";;
	--cppflags=*)		CPPFLAGS="${1#--*=}";;
	--configure-only)	build_configure_only='yes';;
	--datadir=*)		build_datadir="${1#--*=}";;
	--disable-dbus)		build_dbus='d_dbus';;
	--disable-gnutls)	build_gnutls='d_gnutls';;
	--disable-ipv6)		build_ipv6='d_ipv6';;
	--disable-nls)		build_nls='d_enablenls';;
	--disable-socker)	build_socker='d_socker_get';;
	--enable-halloc)	build_halloc='true';;
	--gtk1)				build_ui='gtkversion=1';;
	--gtk2)				build_ui='gtkversion=2';;
	--ldflags=*)		LDFLAGS="${1#--*=}";;
	--libs=*)			LIBS="${1#--*=}";;
	--libdir=*)			build_libdir="${1#--*=}";;
	--localedir=*)		build_localedir="${1#--*=}";;
	--mandir=*)			build_mandir="${1#--*=}";;
	--make=*)			MAKE="${1#--*=}";;
	--prefix=*)			PREFIX="${1#--*=}";;
	--target=*)			build_ui="${1#--*=}";;
	--topless)			build_ui='d_headless';;
	--unofficial)		build_official='false';;
	--verbose)			build_verbose='';;
	--yacc=*)			YACC="${1#--*=}";;
	--) 				break;;
	*)
		cat <<EOM
The following switches are available, defaults are shown in brackets:

  --gtk2           Use Gtk+ 2.x for the user interface [default].
  --gtk1           Use the deprecated Gtk+ 1.2 for the user interface.
  --topless        Compile for topless use (no graphical user interface).
  --disable-dbus   Do not use D-Bus even if available.
  --disable-gnutls Do not use GnuTLS even if available.
  --disable-ipv6   Do not use IPv6 even if supported.
  --disable-nls    Disable NLS (native language support).
  --disable-socker Disable support for Socker.
  --prefix=PATH    Path prefix used for installing files. [$PREFIX]
  --bindir=PATH    Directory for installing executables. [$PREFIX/bin]
  --datadir=PATH   Directory for installing application data. [$PREFIX/share]
  --libdir=PATH    Directory for installing library data. [$PREFIX/lib]
  --localedir=PATH Directory for installing locale data. [$PREFIX/share/locale]
  --mandir=PATH    Directory for installing manual pages. [$PREFIX/man]
  --cc=TOOL        C compiler to use. [$CC]
  --cflags=FLAGS   Flags to pass to the C compiler. [$CFLAGS]
  --cppflags=FLAGS Flags to pass to the C pre-compiler. [$CPPFLAGS]
  --ldflags=FLAGS  Flags to pass to the linker. [$LDFLAGS]
  --libs=FLAGS     Flags to pass to the linker. [$LIBS]
  --make=TOOL      make tool to be used. [$MAKE]
  --yacc=TOOL      yacc, bison or some compatible tool. [$YACC]
  --target=NAME    Cross-compile to the specified system. [`uname -s`]
  --configure-only Do not run make after Configure.
  --unofficial     Use for test builds only. Requires no installation.
  --verbose        Increase verbosity of Configure output.
  --enable-halloc  Enable mmap()-based malloc() replacement.

Typically no switches need to be used. Just run "$0" to start the
build process.
EOM
			case "$1" in
			--help);;
			*) 	echo
				echo "ERROR: Unknown switch: \"$1\"";;
		esac
		exit 1
		;;
	esac
	shift
done

if [ "X$MAKE" = X ]; then
	command -v gmake >/dev/null 2>&1 && MAKE=gmake || MAKE=make
fi

if [ "X$YACC" = X ]; then
	command -v yacc >/dev/null 2>&1 && YACC=yacc || YACC=bison
fi

CPPFLAGS="$CPPFLAGS${build_halloc:+ -DUSE_HALLOC}"
CFLAGS="$CFLAGS${CPPFLAGS:+ }$CPPFLAGS"
PREFIX="${PREFIX:-/usr/local}"

build_bindir=${build_bindir:-"$PREFIX/bin"}
build_bindir=${build_bindir:+"$build_bindir"}

build_mandir=${build_mandir:-"$PREFIX/man"}
build_mandir=${build_mandir:+"$build_mandir"}

build_datadir=${build_datadir:-"$PREFIX/share/gtk-gnutella"}
build_datadir=${build_datadir:+"$build_datadir"}

build_libdir=${build_libdir:-"$PREFIX/lib/gtk-gnutella"}
build_libdir=${build_libdir:+"$build_libdir"}

build_localedir=${build_localedir:-"$PREFIX/share/locale"}
build_localedir=${build_localedir:+"$build_localedir"}

build_official=${build_official:-true}
build_ui=${build_ui:-gtkversion=2}

# Make sure previous Configure settings have no influence.
${MAKE} clobber >/dev/null 2>&1 || : ignore failure
rm -f config.sh

# Use /bin/sh explicitely so that it works on noexec mounted file systems.
# Note: Configure won't work as of yet on such a file system.
/bin/sh ./Configure -Oder \
	$build_verbose \
	-U usenm \
	${CC:+-D "cc=$CC"} \
	${CFLAGS:+-D "ccflags=$CFLAGS"} \
	${LDFLAGS:+-D "ldflags=$LDFLAGS"} \
	${LIBS:+-D "libs=$LIBS"} \
	${PREFIX:+-D "prefix=$PREFIX"} \
	${MAKE:+-D "make=$MAKE"} \
	${YACC:+-D "yacc=$YACC"} \
	${build_bindir:+-D "bindir=$build_bindir"} \
	${build_datadir:+-D "privlib=$build_datadir"} \
	${build_libdir:+-D "archlib=$build_libdir"} \
	${build_localedir:+-D "locale=$build_localedir"} \
	${build_mandir:+-D "sysman=$build_mandir"} \
	${build_official:+-D "official=$build_official"} \
	${build_so_suffix:+-D "so=$build_so_suffix"} \
	${build_ui:+-D "$build_ui"} \
	${build_nls:+-U "$build_nls"} \
	${build_dbus:+-U "$build_dbus"} \
	${build_gnutls:+-U "$build_gnutls"} \
	${build_ipv6:+-U "$build_ipv6"} \
	${build_socker:+-U "$build_socker"} \
	|| { echo; echo 'ERROR: Configure failed.'; exit 1; }

if [ "X$build_configure_only" != X ]; then
	exit
fi

${MAKE} || { echo; echo 'ERROR: Compiling failed.'; exit 1; }

echo "Run \"${MAKE} install\" to install gtk-gnutella."
exit

# vi: set ts=4 sw=4 et:
