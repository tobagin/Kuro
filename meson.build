project('kuro', 'c',
  version: '1.0.0',
  meson_version : '>= 0.58.0',
  license: 'GPLv3+',
  default_options : [
    'buildtype=debugoptimized',
    'warning_level=2',
    'c_std=gnu11',
  ],
)

i18n = import('i18n')
gnome = import('gnome')
if get_option('profile') == 'development'
  profile = 'Devel'
  name_suffix = ' (Development)'
  application_id = 'io.github.tobagin.Kuro.Devel'
else
  profile = ''
  name_suffix = ''
  application_id = 'io.github.tobagin.Kuro'
endif

# Dependencies
glib_dependency = dependency('glib-2.0')
gio_dependency = dependency('gio-2.0', version: '>= 2.32')
gtk_dependency = dependency('gtk4', version: '>= 4.6.0')
adw_dependency = dependency('libadwaita-1', version: '>= 1.5')
gmodule_dependency = dependency('gmodule-2.0')
cairo_dependency = dependency('cairo', version: '>= 1.4')

config_h = configuration_data()
config_h.set_quoted('PACKAGE', meson.project_name())
config_h.set_quoted('PACKAGE_URL', 'https://github.com/tobagin/Kuro')
config_h.set_quoted('GETTEXT_PACKAGE', meson.project_name())
config_h.set_quoted('PACKAGE_LOCALE_DIR', join_paths(get_option('prefix'), get_option('localedir')))
config_h.set_quoted('VERSION', meson.project_version())

# Enable warning flags
test_c_args = [
  '-fno-strict-aliasing',
  '-fstack-protector-strong',
  '-Waggregate-return',
  '-Wall',
  '-Wunused',
  '-Warray-bounds',
  '-Wcast-align',
  '-Wclobbered',
  '-Wno-declaration-after-statement',
  '-Wduplicated-branches',
  '-Wduplicated-cond',
  '-Wempty-body',
  '-Wformat=2',
  '-Wformat-nonliteral',
  '-Wformat-security',
  '-Wformat-signedness',
  '-Wignored-qualifiers',
  '-Wimplicit-function-declaration',
  '-Wincompatible-pointer-types',
  '-Wincompatible-pointer-types-discards-qualifiers',
  '-Winit-self',
  '-Wint-conversion',
  '-Wlogical-op',
  '-Wmisleading-indentation',
  '-Wmissing-declarations',
  '-Wmissing-format-attribute',
  '-Wmissing-include-dirs',
  '-Wmissing-noreturn',
  '-Wmissing-parameter-type',
  '-Wmissing-prototypes',
  '-Wnested-externs',
  '-Wno-error=cpp',
  '-Wno-discarded-qualifiers',
  '-Wno-missing-field-initializers',
  '-Wno-suggest-attribute=format',
  '-Wno-unused-parameter',
  '-Wnull-dereference',
  '-Wold-style-definition',
  '-Woverflow',
  '-Woverride-init',
  '-Wparentheses',
  '-Wpointer-arith',
  '-Wredundant-decls',
  '-Wreturn-type',
  '-Wshadow',
  '-Wsign-compare',
  '-Wstrict-aliasing=2',
  '-Wstrict-prototypes',
  '-Wswitch-default',
  '-Wswitch-enum',
  '-Wtype-limits',
  '-Wundef',
  '-Wuninitialized',
  '-Wunused-but-set-variable',
  '-Wunused-result',
  '-Wunused-variable',
  '-Wwrite-strings'
]
cc = meson.get_compiler('c')
add_project_arguments(cc.get_supported_arguments(test_c_args), language: 'c')

compiled_ui_files = []
subdir('data')
subdir('help')
subdir('po')
subdir('src')

meson.add_install_script('build-aux/meson_post_install.py')
