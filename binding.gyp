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
                'src/Util.cpp',
                'src/GetBaudRate.cpp',
                'src/GetBaudRate_darwin.cpp',
                'src/SetBaton.cpp',
                'src/SetBaton_darwin.cpp',
                'src/SerialPort.cpp',
                'src/SerialPort_darwin.cpp',
                'src/Get.cpp',
                'src/Get_darwin.cpp',
                'src/OpenBaton.cpp',
                'src/OpenBaton_darwin.cpp',
                'src/ReadBaton.cpp',
                'src/ReadBaton_darwin.cpp',
                'src/Close.cpp',
                'src/Close_darwin.cpp',
                'src/Drain.cpp',
                'src/Drain_darwin.cpp',
                'src/Flush.cpp',
                'src/Flush_darwin.cpp',
                'src/WriteBaton.cpp',
                'src/WriteBaton_darwin.cpp',
                'src/Darwin.cpp',
                'src/UpdateBaton.cpp',
                'src/UpdateBaton_darwin.cpp',
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
