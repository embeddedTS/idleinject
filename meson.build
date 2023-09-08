project('idleinject', 'c')
procpsdep = dependency('libprocps', required: false)
found = procpsdep.found()
if found != true
  procpsdep = dependency('libproc2', required: true)
endif
executable('idleinject', 'src/idleinject.c', install : true, dependencies : procpsdep)
