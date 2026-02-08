scalable_dir = join_paths('hicolor', 'scalable', 'apps')
install_data(
  join_paths('scalable', 'kuro.svg'),
  rename: ('@0@.svg'.format(application_id)),
  install_dir: join_paths(get_option('datadir'), 'icons', scalable_dir)
)

install_data(
    join_paths('hicolor', '512x512', 'apps', 'kuro.png'),
    rename: (application_id + '.png'),
    install_dir: join_paths(get_option('datadir'), 'icons', 'hicolor', '512x512', 'apps')
)

symbolic_dir = join_paths('hicolor', 'symbolic', 'apps')
install_data(
  join_paths('symbolic', 'kuro-symbolic.svg'),
  rename: ('@0@-symbolic.svg'.format(application_id)),
  install_dir: join_paths(get_option('datadir'), 'icons', symbolic_dir)
)
