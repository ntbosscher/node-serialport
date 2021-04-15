{
  'targets': [{
    'target_name': 'bindings',
    'win_delay_load_hook': 'true',
    'sources': [
    ],
    'include_dirs': [
      '<!(node -e "require(\'nan\')")'
    ],
    'conditions': [
      ['OS=="win"',
        {
          'sources': [
            'src/serialport.cpp',
            'src/util.cpp',
            'src/BatonBase.cpp',
            'src/win.cpp',
            'src/ListBaton_win.cpp',
            'src/OpenBaton_win.cpp',
            'src/WriteBaton_win.cpp',
            'src/ReadBaton_win.cpp',
            'src/UpdateBaton_win.cpp',
            'src/SetBaton_win.cpp',
            'src/V8ArgDecoder.cpp'
          ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'ExceptionHandling': '2',
              'DisableSpecificWarnings': [ '4530', '4506' ],
            }
          }
        }
      ],
    ]
  }],
}
