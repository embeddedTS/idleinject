project('idleinject', 'c', version : run_command('./script/getver.sh', check : false).stdout().strip())
idleinject_files = files(
  'src/idleinject.c',
  'src/alloc.c',
  'src/escape.c',
  'src/pwcache.c',
  'src/readproc.c',
)
executable('idleinject', idleinject_files, install : true)
