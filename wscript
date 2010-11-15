APPNAME = 'linenoise'
VERSION = '0.0.0'

top = '.'
out = 'build'

def options(opt):
  opt.tool_options('compiler_cc')

def configure(ctx):
  ctx.env.NAME = 'default'
  ctx.check_tool('compiler_cc')
  if not ctx.env.CC: conf.fatal('c compiler not found')
  ctx.env.append_value('CCFLAGS', ['-ggdb', '-O0', '-Wall', '-Wextra', '-std=c99'])

def build(ctx):
  example = ctx.new_task_gen()
  example.features = ['c', 'cprogram']
  example.target = 'linenoise_example'
  example.source = 'linenoise.c example.c'

  lib = ctx.new_task_gen()
  lib.features = ['c', 'cshlib']
  lib.target = 'linenoise'
  lib.vnum = VERSION
  lib.source = 'linenoise.c'

  # install files
  ctx.install_files('${PREFIX}/include', ctx.path.ant_glob('*.h'))
