import os
import platform
import sys


def is_active():
    return True


def get_name():
    return "Switch"


def can_build():

	if (not os.environ.has_key("DEVKITPRO")):
		return False

	errorval=os.system("pkg-config --version > /dev/null")
	if (errorval):
		print("pkg-config not found... nx disabled.")
		return False
	
	for package in ['zlib', 'libpng']:
		errorval=os.system("pkg-config {} --modversion > /dev/null".format(package))
		if (errorval):
			print(package+" not found... nx disabled.")
			return False

	return True # Switch enabled


def get_opts():
    # from SCons.Variables import BoolVariable
    return [
        # BoolVariable('use_static_cpp', 'Link libgcc and libstdc++ statically for better portability', True),
        # BoolVariable('debug_release', 'Add debug symbols to release version', False)
    ]


def get_flags():

    return [
        ('module_mobile_vr_enabled', False),
        ('tools', False),
        ('squish', False),
        ('theora', False),
        ('vorbis', True),
        ('speex', False),
        ('dds', False),
        ('pvr', False),
        ('etc1', False),
        ('builtin_zlib', False),
        ('builtin_bullet', False),
        ('openssl', False),
        ('musepack', False),
    ]


def configure(env):
    devkitpro_path = os.environ["DEVKITPRO"]
    # Modules that depend on unimplemented system functionality get the axe
    env.disabled_modules = ['enet', 'webm', 'websocket', 'upnp']

    # Compiler and flags etc
    prefix = devkitpro_path + '/devkitA64/bin/aarch64-none-elf-'

    env['CC'] = prefix + 'gcc'
    env['CXX'] = prefix + 'g++'
    env['LD'] = prefix + 'g++'
    env['AS'] = prefix + 'as'
    env['AR'] = prefix + 'gcc-ar'
    env['RANLIB'] = prefix + 'ranlib'
    env['OBJCOPY'] = prefix + 'objcopy'
    env['STRIP'] = prefix + 'strip'
    env['NM'] = prefix + 'gcc-nm'
    env['PKG_CONFIG'] = devkitpro_path + '/portlibs/bin/aarch64-none-elf-pkg-config'

    env['ENV']['DEVKITPRO'] = devkitpro_path
    env['DEVKITPRO'] = devkitpro_path
    cwd = os.getcwd()
    env.Append(CPPPATH=[
        devkitpro_path+"/devkitA64/aarch64-none-elf/include",
        devkitpro_path+"/libnx/include",
        devkitpro_path+"/portlibs/switch/include",
        cwd+"/platform/nx",
        cwd+"/thirdparty/glad",
        cwd+"/drivers/nx"
    ])
    env.Append(LIBPATH=[
        devkitpro_path+"/devkitA64/aarch64-none-elf/lib",
        devkitpro_path+"/libnx/lib",
        devkitpro_path+"/portlibs/switch/lib",
    ])
    # env['ENV']['PATH'] = ':'.join([devkitpro_path+"/tools/bin", devkitpro_path+"/devkitA64/bin", env['ENV']['PATH']])
    env['ENV']['PKG_CONFIG_DIR'] = devkitpro_path+"/portlibs/switch/lib/pkgconfig"
    env['ENV']['PKG_CONFIG_PATH'] = devkitpro_path+"/portlibs/switch/lib/pkgconfig"
    env['ENV']['PKG_CONFIG_LIBDIR'] = devkitpro_path+"/portlibs/switch/lib/pkgconfig"
    env['ENV']['DEVKITPRO'] = devkitpro_path
    # TODO: Make sure these settings are correct
    env.Append(LINKFLAGS=[
        '-specs=platform/nx/switch.specs',
        '-g',
        '-march=armv8-a',
        '-mtune=cortex-a57',
        '-mtp=soft',
        '-fPIE',
        '-fPIC',
    ])
    env.Append(LIBS=['nx'])

    env.Append(CCFLAGS=[
        '-g',
        '-ffunction-sections',

        '-march=armv8-a',
        '-mtune=cortex-a57',
        '-mtp=soft',
        '-fPIE',
        '-fPIC',
    ])
    env.Append(CPPFLAGS=[
        '-fno-rtti',
        '-fno-exceptions',
    ])
    env.Append(CPPDEFINES=[
        'SWITCH',
        'LIBNX_ENABLED',
        '__LIBNX__',
        'NOSTYLUS',
        'NO_SAFE_CAST',
        "MBEDTLS_CONFIG_FILE='<nx_mbedtls_config.h>'",
        'UNIX_ENABLED',
        'LIBC_FILEIO_ENABLED',
        '_POSIX_THREADS',
        'PTHREAD_NO_RENAME',
        '_UNIX98_THREAD_MUTEX_ATTRIBUTES',
        '_POSIX_READER_WRITER_LOCKS',
        'NO_THREADS',
        # 'GLES_OVER_GL',
        'DEBUG_INIT',
    ])

    ## Build type

    if (env["target"] == "release"):
        env.Append(CCFLAGS=['-O2', '-ffast-math', '-fomit-frame-pointer'])

    elif (env["target"] == "release_debug"):
        env.Append(CCFLAGS=['-O2', '-ffast-math', '-DDEBUG_ENABLED'])

    elif (env["target"] == "debug"):
        env.Append(CCFLAGS=['-g2', '-DDEBUG_ENABLED', '-DDEBUG_MEMORY_ENABLED'])

    ## Architecture
    env["bits"] = "64"

    ## Compiler configuration

    # if env['use_llvm']:
    #     if ('clang++' not in env['CXX']):
    #         env["CC"] = "clang"
    #         env["CXX"] = "clang++"
    #         env["LINK"] = "clang++"
    #     env.Append(CPPFLAGS=['-DTYPED_METHOD_BIND'])
    #     env.extra_suffix = ".llvm" + env.extra_suffix

    ## Dependencies

    # FIXME: Check for existence of the libs before parsing their flags with pkg-config

    # freetype depends on libpng and zlib, so bundling one of them while keeping others
    # as shared libraries leads to weird issues
    if env['builtin_freetype'] or env['builtin_libpng'] or env['builtin_zlib']:
        env['builtin_freetype'] = True
        env['builtin_libpng'] = True
        env['builtin_zlib'] = True

    if not env['builtin_freetype']:
        env.ParseConfig('pkg-config freetype2 --cflags --libs')

    if not env['builtin_libpng']:
        env.ParseConfig('pkg-config libpng --cflags --libs')

    if not env['builtin_enet']:
        env.ParseConfig('pkg-config libenet --cflags --libs')

    if not env['builtin_squish'] and env['tools']:
        env.ParseConfig('pkg-config libsquish --cflags --libs')

    if not env['builtin_zstd']:
        env.ParseConfig('pkg-config libzstd --cflags --libs')
    
    if not env['builtin_bullet']:
        # We need at least version 2.88
        import subprocess
        bullet_version = subprocess.check_output(['pkg-config', 'bullet', '--modversion']).strip()
        if bullet_version < "2.88":
            # Abort as system bullet was requested but too old
            print("Bullet: System version {0} does not match minimal requirements ({1}). Aborting.".format(bullet_version, "2.88"))
            sys.exit(255)
        env.ParseConfig('pkg-config bullet --cflags --libs')

    # Sound and video libraries
    # Keep the order as it triggers chained dependencies (ogg needed by others, etc.)

    if not env['builtin_libtheora']:
        env['builtin_libogg'] = False  # Needed to link against system libtheora
        env['builtin_libvorbis'] = False  # Needed to link against system libtheora
        env.ParseConfig('pkg-config theora theoradec --cflags --libs')

    if not env['builtin_libvpx']:
        env.ParseConfig('pkg-config vpx --cflags --libs')

    if not env['builtin_libvorbis']:
        env['builtin_libogg'] = False  # Needed to link against system libvorbis
        env.ParseConfig('pkg-config vorbis vorbisfile --cflags --libs')

    if not env['builtin_opus']:
        env['builtin_libogg'] = False  # Needed to link against system opus
        env.ParseConfig('pkg-config opus opusfile --cflags --libs')

    if not env['builtin_libogg']:
        env.ParseConfig('pkg-config ogg --cflags --libs')

    if not env['builtin_libwebp']:
        env.ParseConfig('pkg-config libwebp --cflags --libs')

    if not env['builtin_mbedtls']:
        # mbedTLS does not provide a pkgconfig config yet. See https://github.com/ARMmbed/mbedtls/issues/228
        env.Append(LIBS=['mbedtls', 'mbedcrypto', 'mbedx509'])

    if not env['builtin_libwebsockets']:
        env.ParseConfig('pkg-config libwebsockets --cflags --libs')

    if not env['builtin_miniupnpc']:
        # No pkgconfig file so far, hardcode default paths.
        env.Append(CPPPATH=["/usr/include/miniupnpc"])
        env.Append(LIBS=["miniupnpc"])

    # On Linux wchar_t should be 32-bits
    # 16-bit library shouldn't be required due to compiler optimisations
    if not env['builtin_pcre2']:
        env.ParseConfig('pkg-config libpcre2-32 --cflags --libs')

    ## Flags

    # Linkflags below this line should typically stay the last ones
    if not env['builtin_zlib']:
        env.ParseConfig('pkg-config zlib --cflags --libs')

    env.Append(CPPPATH=['#platform/nx'])

    # Link those statically for portability
    if env['use_static_cpp']:
        env.Append(LINKFLAGS=['-static-libgcc', '-static-libstdc++'])
