sources = files(
  'main.c',
  'interface.c',
  'rules.c',
  'generator.c',
  'score.c',
)

if not cc.has_function('atexit')
  error('atexit() needed for generated GResource files')
endif

resource_conf = configuration_data()
resource_conf.set('METAINFO_FILE', '@0@.metainfo.xml'.format(application_id))
resource_conf.set('appid', application_id)
resource_conf.set('apppath', '/' + application_id.replace('.', '/') + '/')

gresource_file = configure_file(
  input: 'kuro.gresource.xml.in',
  output: 'kuro.gresource.xml',
  configuration: resource_conf
)

resources = gnome.compile_resources(
  'kuro-resources',
  gresource_file,
  source_dir: [meson.source_root(), meson.project_build_root()],
  dependencies: compiled_ui_files,
)

config_header = configure_file(
  output: 'config.h',
  configuration: config_h,
)

executable(
  meson.project_name(),
  sources + resources,
  config_header,
  dependencies: [
    glib_dependency,
    gio_dependency,
    gtk_dependency,
    adw_dependency,
    gmodule_dependency,
    cairo_dependency
  ],
  install: true,
  c_args: [
    '-DHAVE_CONFIG_H',
    '-DGETTEXT_PACKAGE="@0@"'.format (meson.project_name()),
    '-DAPPLICATION_ID="@0@"'.format (application_id)
  ],
  install_dir: get_option('bindir'),
)
