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
        "-std=c++11","-fexceptions","-Ofast"
      ],
      'cflags_cc': [ '-fexceptions',"-std=c++11","-Ofast" ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")",
        "<!(node -e \"require('nan-check')\")",
        "<!(node -e \"require('nan-marshal')\")",
        "/usr/local/include/RF24",
        "/usr/local/include/RF24Network",
        "/usr/local/include/RF24Mesh",
        "/usr/local/include/RF24Gateway"
      ],
      'defines': [

      ],
      'link_settings': {
        'libraries': ['-lrf24 -lrf24network -lrf24mesh -lrf24gateway'],
      }
    }
  ]
}
