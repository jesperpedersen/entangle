dnl
dnl Enable all known GCC compiler warnings, except for those
dnl we can't yet cope with
dnl
AC_DEFUN([ENTANGLE_COMPILE_WARNINGS],[
    dnl ******************************
    dnl More compiler warnings
    dnl ******************************

    AC_ARG_ENABLE([werror],
                  AS_HELP_STRING([--enable-werror], [Use -Werror (if supported)]),
                  [set_werror="$enableval"],
                  [if test -d $srcdir/.git; then
                     is_git_version=true
                     set_werror=yes
                   else
                     set_werror=no
                   fi])


    # List of warnings that are not relevant / wanted

    # Don't care about C++ compiler compat
    dontwarn="$dontwarn -Wc++-compat"
    dontwarn="$dontwarn -Wabi"
    dontwarn="$dontwarn -Wdeprecated"
    # Don't care about ancient C standard compat
    dontwarn="$dontwarn -Wtraditional"
    # Don't care about ancient C standard compat
    dontwarn="$dontwarn -Wtraditional-conversion"
    # Ignore warnings in /usr/include
    dontwarn="$dontwarn -Wsystem-headers"
    # Happy for compiler to add struct padding
    dontwarn="$dontwarn -Wpadded"
    # GCC very confused with -O2
    dontwarn="$dontwarn -Wunreachable-code"
    # Broken gexiv2 pkg-config file
    dontwarn="$dontwarn -Wmissing-include-dirs"
    # We need to use long long in many places
    dontwarn="$dontwarn -Wlong-long"
    # We don't care about c90 rules
    dontwarn="$dontwarn -Wdeclaration-after-statement"
    # Need to allow some explicit bad casts
    dontwarn="$dontwarn -Wcast-qual"
    # entangle_session_next_filename
    dontwarn="$dontwarn -Wformat-nonliteral"
    # Don't care about explicit casts of functions
    dontwarn="$dontwarn -Wbad-function-cast"

    # Get all possible GCC warnings
    gl_MANYWARN_ALL_GCC([maybewarn])

    # Remove the ones we don't want, blacklisted earlier
    gl_MANYWARN_COMPLEMENT([wantwarn], [$maybewarn], [$dontwarn])

    # Check for $CC support of each warning
    for w in $wantwarn; do
      gl_WARN_ADD([$w])
    done

    # GNULIB uses '-W' (aka -Wextra) which includes a bunch of stuff.
    # Unfortunately, this means you can't simply use '-Wsign-compare'
    # with gl_MANYWARN_COMPLEMENT
    # So we have -W enabled, and then have to explicitly turn off...
    gl_WARN_ADD([-Wno-sign-compare])
    gl_WARN_ADD([-Wno-sign-conversion])
    gl_WARN_ADD([-Wno-conversion])

    # GNULIB expects this to be part of -Wc++-compat, but we turn
    # that one off, so we need to manually enable this again
    gl_WARN_ADD([-Wjump-misses-init])

    # This should be < 256 really. Currently we're down to 4096,
    # but using 1024 bytes sized buffers (mostly for virStrerror)
    # stops us from going down further
    gl_WARN_ADD([-Wframe-larger-than=40096])

    # Extra special flags
    gl_WARN_ADD([-Wp,-D_FORTIFY_SOURCE=2])
    dnl -fstack-protector stuff passes gl_WARN_ADD with gcc
    dnl on Mingw32, but fails when actually used
    case $host in
       *-*-linux*)
       dnl Fedora only uses -fstack-protector, but doesn't seem to
       dnl be great overhead in adding -fstack-protector-all instead
       dnl gl_WARN_ADD([-fstack-protector])
       gl_WARN_ADD([-fstack-protector-all])
       gl_WARN_ADD([--param=ssp-buffer-size=4])
       ;;
    esac
    gl_WARN_ADD([-fexceptions])
    gl_WARN_ADD([-fasynchronous-unwind-tables])
    gl_WARN_ADD([-fdiagnostics-show-option])
    gl_WARN_ADD([-funit-at-a-time])

    # Need -fipa-pure-const in order to make -Wsuggest-attribute=pure
    # fire even without -O.
    gl_WARN_ADD([-fipa-pure-const])
    # We should eventually enable this
    gl_WARN_ADD([-Wno-suggest-attribute=pure])
    gl_WARN_ADD([-Wno-suggest-attribute=const])

    if test "$set_werror" = "yes"
    then
      gl_WARN_ADD([-Werror])
    fi

    AC_SUBST([WARN_CFLAGS])
])
