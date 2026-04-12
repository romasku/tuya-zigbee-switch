#
# build script written by : Michael Brockus.
# github repo author: Mark VanderVoord.
#
# license: MIT
#
project('cexception', 'c',
    license: 'MIT',
    meson_version: '>=0.53.0',
    default_options: ['werror=true', 'c_std=c11']
)

subdir('lib')
cexception_dep = declare_dependency(link_with: cexception_lib, include_directories: cexception_dir)
