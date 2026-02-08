subdir('icons')

app_conf = configuration_data()
app_conf.set('APP_ID', application_id)

configured_desktop = configure_file(
  input: 'kuro.desktop.in',
  output: 'kuro.desktop.in.configured',
  configuration: app_conf
)

desktop_file = i18n.merge_file(
  type: 'desktop',
  input: configured_desktop,
  output: '@0@.desktop'.format(application_id),
  po_dir: join_paths(meson.source_root(), 'po'),
  install: true,
  install_dir: join_paths(get_option('datadir'), 'applications'),
)

desktop_file_validate = find_program('desktop-file-validate', required: false)
if desktop_file_validate.found()
  test(
    'validate-desktop',
    desktop_file_validate,
    args: [
      desktop_file.full_path(),
    ],
  )
endif

configured_metainfo = configure_file(
  input: 'kuro.metainfo.xml.in',
  output: 'kuro.metainfo.xml.in.configured',
  configuration: app_conf
)

metainfo_file = i18n.merge_file(
  input: configured_metainfo,
  output: '@0@.metainfo.xml'.format(application_id),
  po_dir: join_paths(meson.source_root(), 'po'),
  install: true,
  install_dir: join_paths(get_option('datadir'), 'metainfo'),
)

appstreamcli = find_program('appstreamcli', required: false)
if appstreamcli.found()
  test(
    'validate-metainfo', appstreamcli,
    args: [
      'validate', '--no-net', '--explain', metainfo_file.full_path(),
    ],
  )
endif

schema_conf = configuration_data()
schema_conf.set('appid', application_id)
schema_conf.set('apppath', '/' + application_id.replace('.', '/') + '/')

configure_file(
  input: 'io.github.tobagin.Kuro.gschema.xml.in',
  output: '@0@.gschema.xml'.format(application_id),
  configuration: schema_conf,
  install: true,
  install_dir: join_paths(get_option('datadir'), 'glib-2.0', 'schemas'),
)


# Blueprint UI files
blueprint_compiler = find_program('blueprint-compiler', required: true)

blueprint_files = [
  'kuro.blp',
  'help-overlay.blp',
]

compiled_ui_files = []

foreach blp : blueprint_files
  compiled_ui_files += custom_target(
    blp.underscorify() + '_ui',
    input: blp,
    output: '@BASENAME@.ui',
    command: [
      blueprint_compiler, 'batch-compile', '@OUTDIR@', '@CURRENT_SOURCE_DIR@', '@INPUT@'
    ],
  )
endforeach

gtk4_builder_tool = find_program('gtk4-builder-tool', required: false)
if gtk4_builder_tool.found()
  foreach ui : compiled_ui_files
    test(
      'validate-ui-' + ui.full_path().underscorify(),
      gtk4_builder_tool,
      args: [
        'validate',
        ui,
      ],
    )
  endforeach
endif


