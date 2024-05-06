
project('CallingConCalculator', 'cpp', default_options: ['cpp_std=c++latest'])
sources = [
    'src/main.cpp',
    'src/CCC.hpp'
]    

headers = [
    'src/CCC.hpp'
]

testing = get_option('enableTests') 
if testing 
    add_project_arguments('-DTesting=1', language : 'cpp')
    executable('CallingConCalculator', sources)
    message('Tests enabled')
else
    add_project_arguments('-DTesting=0', language : 'cpp')
    static_library('CallingConCalculator', sources)
    message('Tests disabled')
endif

install_headers(headers, subdir: 'CallingConCalculator')