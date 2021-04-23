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
        ['OS=="mac"', {
            'sources': [
                'src/ListBaton.cpp',
                'src/BatonBase.cpp',
                'src/ListBaton_darwin.cpp',
                'src/ListUtils_darwin.cpp',
                'src/V8ArgDecoder.cpp',
                'src/Darwin.cpp',
                'src/Util.cpp',
            ],
        }],
      ['OS=="win"',
        {
          'sources': [
            'src/darwin.cpp',
            'src/ListBaton_darwin.cpp',

            'src/V8ArgDecoder.cpp',
            'src/SerialPort.cpp',
            'src/util.cpp',
            'src/BatonBase.cpp',
            'src/ListBaton.cpp',

            'src/win.cpp',
            'src/UpdateBaton_win.cpp',
            'src/WriteBaton_win.cpp',
            'src/SetBaton_win.cpp',
            'src/ReadBaton_win.cpp',
            'src/OpenBaton_win.cpp',
            'src/ListBaton_win.cpp',
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
