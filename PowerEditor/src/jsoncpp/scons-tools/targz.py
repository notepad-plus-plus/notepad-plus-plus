"""tarball

Tool-specific initialization for tarball.

"""

## Commands to tackle a command based implementation:
##to unpack on the fly...
##gunzip < FILE.tar.gz | tar xvf -
##to pack on the fly...
##tar cvf - FILE-LIST | gzip -c > FILE.tar.gz 

import os.path

import SCons.Builder
import SCons.Node.FS
import SCons.Util

try:
    import gzip
    import tarfile
    internal_targz = 1
except ImportError:
    internal_targz = 0

TARGZ_DEFAULT_COMPRESSION_LEVEL = 9

if internal_targz:
    def targz(target, source, env):
        def archive_name( path ):
            path = os.path.normpath( os.path.abspath( path ) )
            common_path = os.path.commonprefix( (base_dir, path) )
            archive_name = path[len(common_path):]
            return archive_name
            
        def visit(tar, dirname, names):
            for name in names:
                path = os.path.join(dirname, name)
                if os.path.isfile(path):
                    tar.add(path, archive_name(path) )
        compression = env.get('TARGZ_COMPRESSION_LEVEL',TARGZ_DEFAULT_COMPRESSION_LEVEL)
        base_dir = os.path.normpath( env.get('TARGZ_BASEDIR', env.Dir('.')).abspath )
        target_path = str(target[0])
        fileobj = gzip.GzipFile( target_path, 'wb', compression )
        tar = tarfile.TarFile(os.path.splitext(target_path)[0], 'w', fileobj)
        for source in source:
            source_path = str(source)
            if source.isdir():
                os.path.walk(source_path, visit, tar)
            else:
                tar.add(source_path, archive_name(source_path) )      # filename, arcname
        tar.close()

    targzAction = SCons.Action.Action(targz, varlist=['TARGZ_COMPRESSION_LEVEL','TARGZ_BASEDIR'])

    def makeBuilder( emitter = None ):
        return SCons.Builder.Builder(action = SCons.Action.Action('$TARGZ_COM', '$TARGZ_COMSTR'),
                                     source_factory = SCons.Node.FS.Entry,
                                     source_scanner = SCons.Defaults.DirScanner,
                                     suffix = '$TARGZ_SUFFIX',
                                     multi = 1)
    TarGzBuilder = makeBuilder()

    def generate(env):
        """Add Builders and construction variables for zip to an Environment.
           The following environnement variables may be set:
           TARGZ_COMPRESSION_LEVEL: integer, [0-9]. 0: no compression, 9: best compression (same as gzip compression level).
           TARGZ_BASEDIR: base-directory used to determine archive name (this allow archive name to be relative
                          to something other than top-dir).
        """
        env['BUILDERS']['TarGz'] = TarGzBuilder
        env['TARGZ_COM'] = targzAction
        env['TARGZ_COMPRESSION_LEVEL'] = TARGZ_DEFAULT_COMPRESSION_LEVEL # range 0-9
        env['TARGZ_SUFFIX']  = '.tar.gz'
        env['TARGZ_BASEDIR'] = env.Dir('.')     # Sources archive name are made relative to that directory.
else:
    def generate(env):
        pass


def exists(env):
    return internal_targz
