{
  'targets': [
    {
      'target_name': 'nRF24',
      'sources': [ 'irq.cc','tryabort.cc','rf24_util.cc',
                   'rf24.cc', 'rf24_setup.cc', 'rf24_worker.cc','rf24_stats.cc',
                   'rf24mesh.cc',
                   'rf24_init.cc'
                 ],
      "cflags": [
        "-std=c++11","-fexceptions","-Ofast", "-Wno-cast-function-type"
      ],
      'cflags_cc': [ '-fexceptions',"-std=c++11","-Ofast" ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")",
        "<!(node -e \"require('nan-check')\")",
        "nan-lib",
        "rf24libs/include",
        "rf24libs/include/RF24",
        "rf24libs/include/RF24Network",
        "rf24libs/include/RF24Mesh"
      ],
      'defines': [

      ],
      'link_settings': {
        'libraries': ['-L<(module_root_dir)/rf24libs','-lrf24 -lrf24net -lrf24mesh'],
      }
    }
  ]
}
