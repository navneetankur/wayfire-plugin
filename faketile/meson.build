project(
	'faketile',
	'cpp',
	version: '0.1',
	license: 'MIT',
	meson_version: '>=0.51.0',
)

wayfire = dependency('wayfire')
wfconfig = dependency('wf-config')

add_project_arguments(['-DWLR_USE_UNSTABLE'], language: ['cpp', 'c'])
add_project_arguments(['-DWAYFIRE_PLUGIN'], language: ['cpp', 'c'])
add_project_link_arguments(['-rdynamic'], language:'cpp')

subdir('src')
subdir('metadata')
