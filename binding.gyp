{
  'targets': [{
    'target_name': 'bindings',
    'win_delay_load_hook': 'true',
    'sources': [
      'src/serialport.cpp',
      'src/util.cpp',
      'src/BatonBase.cpp'
    ],
    'include_dirs': [
      '<!(node -e "require(\'nan\')")'
    ],
    'conditions': [
      ['OS=="win"',
        {
          'sources': [
            'src/win.cpp',
            'src/ListBaton_win.cpp',
            'src/OpenBaton_win.cpp',
            'src/WriteBaton_win.cpp',
            'src/ReadBaton_win.cpp',
            'src/UpdateBaton_win.cpp',
            'src/SetBaton_win.cpp'
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
