{
  'targets': [
    {
      'target_name': 'nRF24',
      'sources': [ 'tryabort.cc','rf24_util.cc','rf24.cc','rf24mesh.cc' ,
                   'rf24_init.cc'
                 ],
      "cflags": [
        "-std=c++11","-fexceptions"
      ],
      'cflags_cc': [ '-fexceptions',"-std=c++11" ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")",
        "<!(node -e \"require('nan-check')\")",
        "<!(node -e \"require('nan-marshal')\")",
        "/usr/local/include/RF24",
        "/usr/local/include/RF24Network",
        "/usr/local/include/RF24Mesh",
        "/usr/local/include/RF24Gateway"
      ],
      'link_settings': {
        'libraries': ['-lrf24 -lrf24network -lrf24mesh -lrf24gateway'],
      }
    }
  ]
}
