Import('RTT_ROOT')
Import('rtconfig')
from building import *

cwd     = GetCurrentDir()
src     = Glob('Src/blink_all.c') 

if GetDepend(['PKG_BLINKALL_USING_EXAMPLE']):
	src    += Glob('examples/blink_all_example.c') 

CPPPATH = [cwd]
CPPPATH += [cwd + '/Inc']

group = DefineGroup('blink_all', src, depend = ['PKG_USING_BLINKALL'], CPPPATH = CPPPATH)

Return('group')
