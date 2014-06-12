import fnmatch
import os

def generate( env ): 
   def Glob( env, includes = None, excludes = None, dir = '.' ):
      """Adds Glob( includes = Split( '*' ), excludes = None, dir = '.')
       helper function to environment.

       Glob both the file-system files.

       includes: list of file name pattern included in the return list when matched.
       excludes: list of file name pattern exluced from the return list.

       Example:
       sources = env.Glob( ("*.cpp", '*.h'), "~*.cpp", "#src" )
      """
      def filterFilename(path):
         abs_path = os.path.join( dir, path )
         if not os.path.isfile(abs_path):
            return 0
         fn = os.path.basename(path)
         match = 0
         for include in includes:
            if fnmatch.fnmatchcase( fn, include ):
               match = 1
               break
         if match == 1 and not excludes is None:
            for exclude in excludes:
               if fnmatch.fnmatchcase( fn, exclude ):
                  match = 0
                  break
         return match
      if includes is None:
         includes = ('*',)
      elif type(includes) in ( type(''), type(u'') ):
         includes = (includes,)
      if type(excludes) in ( type(''), type(u'') ):
         excludes = (excludes,)
      dir = env.Dir(dir).abspath
      paths = os.listdir( dir )
      def makeAbsFileNode( path ):
         return env.File( os.path.join( dir, path ) )
      nodes = filter( filterFilename, paths )
      return map( makeAbsFileNode, nodes )

   from SCons.Script import Environment
   Environment.Glob = Glob

def exists(env):
    """
    Tool always exists.
    """
    return True
