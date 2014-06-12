import os
import os.path
from fnmatch import fnmatch
import targz

##def DoxyfileParse(file_contents):
##   """
##   Parse a Doxygen source file and return a dictionary of all the values.
##   Values will be strings and lists of strings.
##   """
##   data = {}
##
##   import shlex
##   lex = shlex.shlex(instream = file_contents, posix = True)
##   lex.wordchars += "*+./-:"
##   lex.whitespace = lex.whitespace.replace("\n", "")
##   lex.escape = ""
##
##   lineno = lex.lineno
##   last_backslash_lineno = lineno
##   token = lex.get_token()
##   key = token   # the first token should be a key
##   last_token = ""
##   key_token = False
##   next_key = False
##   new_data = True
##
##   def append_data(data, key, new_data, token):
##      if new_data or len(data[key]) == 0:
##         data[key].append(token)
##      else:
##         data[key][-1] += token
##
##   while token:
##      if token in ['\n']:
##         if last_token not in ['\\']:
##            key_token = True
##      elif token in ['\\']:
##         pass
##      elif key_token:
##         key = token
##         key_token = False
##      else:
##         if token == "+=":
##            if not data.has_key(key):
##               data[key] = list()
##         elif token == "=":
##            data[key] = list()
##         else:
##            append_data( data, key, new_data, token )
##            new_data = True
##
##      last_token = token
##      token = lex.get_token()
##      
##      if last_token == '\\' and token != '\n':
##         new_data = False
##         append_data( data, key, new_data, '\\' )
##
##   # compress lists of len 1 into single strings
##   for (k, v) in data.items():
##      if len(v) == 0:
##         data.pop(k)
##
##      # items in the following list will be kept as lists and not converted to strings
##      if k in ["INPUT", "FILE_PATTERNS", "EXCLUDE_PATTERNS"]:
##         continue
##
##      if len(v) == 1:
##         data[k] = v[0]
##
##   return data
##
##def DoxySourceScan(node, env, path):
##   """
##   Doxygen Doxyfile source scanner.  This should scan the Doxygen file and add
##   any files used to generate docs to the list of source files.
##   """
##   default_file_patterns = [
##      '*.c', '*.cc', '*.cxx', '*.cpp', '*.c++', '*.java', '*.ii', '*.ixx',
##      '*.ipp', '*.i++', '*.inl', '*.h', '*.hh ', '*.hxx', '*.hpp', '*.h++',
##      '*.idl', '*.odl', '*.cs', '*.php', '*.php3', '*.inc', '*.m', '*.mm',
##      '*.py',
##   ]
##
##   default_exclude_patterns = [
##      '*~',
##   ]
##
##   sources = []
##
##   data = DoxyfileParse(node.get_contents())
##
##   if data.get("RECURSIVE", "NO") == "YES":
##      recursive = True
##   else:
##      recursive = False
##
##   file_patterns = data.get("FILE_PATTERNS", default_file_patterns)
##   exclude_patterns = data.get("EXCLUDE_PATTERNS", default_exclude_patterns)
##
##   for node in data.get("INPUT", []):
##      if os.path.isfile(node):
##         sources.add(node)
##      elif os.path.isdir(node):
##         if recursive:
##            for root, dirs, files in os.walk(node):
##               for f in files:
##                  filename = os.path.join(root, f)
##
##                  pattern_check = reduce(lambda x, y: x or bool(fnmatch(filename, y)), file_patterns, False)
##                  exclude_check = reduce(lambda x, y: x and fnmatch(filename, y), exclude_patterns, True)
##
##                  if pattern_check and not exclude_check:
##                     sources.append(filename)
##         else:
##            for pattern in file_patterns:
##               sources.extend(glob.glob("/".join([node, pattern])))
##   sources = map( lambda path: env.File(path), sources )
##   return sources
##
##
##def DoxySourceScanCheck(node, env):
##   """Check if we should scan this file"""
##   return os.path.isfile(node.path)

def srcDistEmitter(source, target, env):
##   """Doxygen Doxyfile emitter"""
##   # possible output formats and their default values and output locations
##   output_formats = {
##      "HTML": ("YES", "html"),
##      "LATEX": ("YES", "latex"),
##      "RTF": ("NO", "rtf"),
##      "MAN": ("YES", "man"),
##      "XML": ("NO", "xml"),
##   }
##
##   data = DoxyfileParse(source[0].get_contents())
##
##   targets = []
##   out_dir = data.get("OUTPUT_DIRECTORY", ".")
##
##   # add our output locations
##   for (k, v) in output_formats.items():
##      if data.get("GENERATE_" + k, v[0]) == "YES":
##         targets.append(env.Dir( os.path.join(out_dir, data.get(k + "_OUTPUT", v[1]))) )
##
##   # don't clobber targets
##   for node in targets:
##      env.Precious(node)
##
##   # set up cleaning stuff
##   for node in targets:
##      env.Clean(node, node)
##
##   return (targets, source)
   return (target,source)

def generate(env):
   """
   Add builders and construction variables for the
   SrcDist tool.
   """
##   doxyfile_scanner = env.Scanner(
##      DoxySourceScan,
##      "DoxySourceScan",
##      scan_check = DoxySourceScanCheck,
##   )

   if targz.exists(env):
      srcdist_builder = targz.makeBuilder( srcDistEmitter )

      env['BUILDERS']['SrcDist'] = srcdist_builder

def exists(env):
   """
   Make sure srcdist exists.
   """
   return targz.exists(env)
