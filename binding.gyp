{
  'targets': [{
    'target_name': 'faster-serial-port',
    'win_delay_load_hook': 'true',
    'sources': [
        'src/Drain.cpp',
        'src/Flush.cpp',
        'src/WriteBaton.cpp',
        'src/UpdateBaton.cpp',
        'src/ListBaton.cpp',
        'src/BatonBase.cpp',
        'src/V8ArgDecoder.cpp',
        'src/Util.cpp',
        'src/GetBaudRate.cpp',
        'src/SetBaton.cpp',
        'src/Get.cpp',
        'src/OpenBaton.cpp',
        'src/ReadBaton.cpp',
        'src/Close.cpp',
        'src/ConfigureLogging.cpp',
        'src/SerialPort.cpp',
    ],
    'include_dirs': [
      '<!(node -e "require(\'nan\')")'
    ],
    'conditions': [
        ['OS=="mac"', {
            'sources': [
                'src/Darwin.cpp',
                'src/Close_darwin.cpp',
                'src/Drain_darwin.cpp',
                'src/Flush_darwin.cpp',
                'src/GetBaudRate_darwin.cpp',
                'src/Get_darwin.cpp',
                'src/ListBaton_darwin.cpp',
                'src/ListUtils_darwin.cpp',
                'src/OpenBaton_darwin.cpp',
                'src/ReadBaton_darwin.cpp',
                'src/SetBaton_darwin.cpp',
                'src/UpdateBaton_darwin.cpp',
                'src/WriteBaton_darwin.cpp',
            ],
        }],
      ['OS=="win"',
        {
          'sources': [
            'src/win.cpp',
            'src/Close_win.cpp',
            'src/Drain_win.cpp',
            'src/Flush_win.cpp',
            'src/Get_win.cpp',
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
