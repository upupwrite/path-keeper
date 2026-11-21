project(
  'path-keeper',
  'cpp',
  version: '2.5.5',
  license: 'GPL-3.0-or-later',
  default_options: [
    'cpp_std=c++20',
    'buildtype=release',
    'warning_level=2',
  ],
)

# ======================================================================
# 编译器选项
# ======================================================================
cpp = meson.get_compiler('cpp')
warning_args = cpp.get_supported_arguments(['-Wall', '-Wextra'])

# ======================================================================
# 依赖
# ======================================================================
jsoncpp_dep = dependency('jsoncpp', required: true)
readline_dep = dependency('readline', required: true)
threads_dep = dependency('threads')

# 优先 Qt5，降级 Qt6
qt5_dep = dependency('qt5', modules: ['Core'], required: false)
if qt5_dep.found()
  qt = import('qt5')
  qt_dep = qt5_dep
  qt_ver = 'qt5'
else
  qt6_dep = dependency('qt6', modules: ['Core'], required: true)
  qt = import('qt6')
  qt_dep = qt6_dep
  qt_ver = 'qt6'
endif
message('Using ' + qt_ver)

# Qt 工具（lrelease/lupdate）通常位于 Qt 的 bindir，而该目录不一定在 PATH 中。
qt_bindir = qt_dep.get_variable(pkgconfig: 'bindir', default_value: '')
qt_tool_dirs = qt_bindir != '' ? [qt_bindir] : []

# ======================================================================
# 源文件 / 头文件
# ======================================================================
sources = files(
  'src/info.cpp',
  'src/interaction.cpp',
  'src/loadfile.cpp',
  'src/main.cpp',
  'src/pk.cpp',
  'src/terminal.cpp',
  'src/search.cpp',
  'src/log.cpp',
)

headers = files(
  'include/colors.h',
  'include/info.h',
  'include/interaction.h',
  'include/loadfile.h',
  'include/pk.h',
  'include/readline.h',
  'include/search.h',
  'include/terminal.h',
  'include/log.h',
)

inc = include_directories('include')

# ======================================================================
# Qt MOC 预处理（等价于 CMAKE_AUTOMOC ON）
# ======================================================================
moc_files = qt.preprocess(
  moc_headers: headers,
  include_directories: inc,
  dependencies: [qt_dep],
)

# ======================================================================
# 翻译文件 .ts -> .qm（等价于 qtX_add_translation）
# ======================================================================
ts_files = files(
  'translations/path-keeper_en.ts',
  'translations/path-keeper_zh_CN.ts',
)

datadir = get_option('datadir')
lrelease = find_program('lrelease', 'lrelease-qt6', 'lrelease-qt5',
  dirs: qt_tool_dirs, required: true)
qm_files = []
qm_index = 0
foreach ts : ts_files
  qm_files += custom_target(
    'qm_' + qm_index.to_string(),
    input: ts,
    output: '@BASENAME@.qm',
    command: [lrelease, '@INPUT@', '-qm', '@OUTPUT@'],
    install: true,
    install_dir: datadir / 'path-keeper' / 'translations',
  )
  qm_index += 1
endforeach

# ======================================================================
# update_translations 自定义目标（等价于 add_custom_target update_translations）
# ======================================================================
lupdate = find_program('lupdate', 'lupdate-qt6', 'lupdate-qt5',
  dirs: qt_tool_dirs, required: false)
if lupdate.found()
  run_target(
    'update_translations',
    command: [
      lupdate,
      meson.project_source_root() / 'src',
      '-ts',
      ts_files,
    ],
  )
else
  message('lupdate not found — the "update_translations" target is disabled')
endif

# ======================================================================
# 主可执行文件
# ======================================================================
pk_exe = executable(
  'pk',
  sources,
  moc_files,
  qm_files,           # 确保 .qm 在默认构建中生成
  include_directories: inc,
  dependencies: [jsoncpp_dep, readline_dep, threads_dep, qt_dep],
  cpp_args: warning_args,
  install: true,
  install_dir: get_option('bindir'),
)

# ======================================================================
# 安装 shell 包装脚本
# ======================================================================
install_data(
  'shell/pk.sh',
  install_dir: datadir / 'path-keeper',
  install_tag: 'runtime',
)

# ======================================================================
# Shell 补全脚本
# ======================================================================
# bash : <datadir>/bash-completion/completions/pk
# zsh  : <datadir>/zsh/site-functions/_pk
# fish : <datadir>/fish/vendor_completions.d/pk.fish
if get_option('install_completions')
  install_data(
    'shell/completions/pk.bash',
    install_dir: datadir / 'bash-completion' / 'completions',
    rename: 'pk',
    install_tag: 'runtime',
  )
  install_data(
    'shell/completions/_pk',
    install_dir: datadir / 'zsh' / 'site-functions',
    install_tag: 'runtime',
  )
  install_data(
    'shell/completions/pk.fish',
    install_dir: datadir / 'fish' / 'vendor_completions.d',
    install_tag: 'runtime',
  )
endif

# ======================================================================
# 测试（等价于 BUILD_TESTS=ON）
# ======================================================================
if get_option('build_tests')
  message('Building tests: ENABLED')
  python = import('python').find_installation('python3', required: true)
  test(
    'pk_tests',
    python,
    args: [
      '-m', 'pytest', '-s', '-v',
      meson.project_source_root() / 'test' / 'test.py',
    ],
    env: [
      'PK_BINARY=' + pk_exe.full_path(),
      'PYTHONUNBUFFERED=1',
    ],
    workdir: meson.current_build_dir(),
    depends: pk_exe,
    timeout: 300,
    suite: 'unit',
  )
else
  message('Building tests: DISABLED')
endif

# ======================================================================
# 关于原 CMake 中的 "uninstall" 目标
# ======================================================================
# Meson 自 0.47.0 起内置 uninstall：
#   ninja -C build uninstall
# 因此无需自定义目标。

# ======================================================================
# 关于 CPack DEB 打包
# ======================================================================
# Meson 没有内置的 DEB 打包器。常见的等价做法：
#
#   meson setup build --prefix=/usr
#   DESTDIR=$PWD/pkg ninja -C build install
#   fpm -s dir -t deb \
#       -n path-keeper -v 2.5.4 \
#       --depends 'libjsoncpp25' --depends 'libqt5core5a' --depends 'libreadline8' \
#       -C $PWD/pkg .
#
# 或者使用标准的 Debian 打包流程（dpkg-buildpackage + debian/ 目录）。
# 原 CMake 中 CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON 的效果可由
# dpkg-shlibdeps 或 fpm 的自动依赖探测实现。

# ======================================================================
# 构建摘要
# ======================================================================
summary({
  'Version': meson.project_version(),
  'Build type': get_option('buildtype'),
  'Tests': get_option('build_tests'),
  'Shell completions': get_option('install_completions'),
  'Qt': qt_ver,
}, section: 'Path Keeper')
