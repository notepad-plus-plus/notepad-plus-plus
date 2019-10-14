import distutils.sysconfig
import getopt
import glob
import os
import platform
import shutil
import subprocess
import stat
import sys

sys.path.append(os.path.join("..", "ScintillaEdit"))
import WidgetGen

scintillaDirectory = "../.."
scintillaScriptsDirectory = os.path.join(scintillaDirectory, "scripts")
sys.path.append(scintillaScriptsDirectory)
from FileGenerator import GenerateFile

# Decide up front which platform, treat anything other than Windows or OS X as Linux
PLAT_WINDOWS = platform.system() == "Windows"
PLAT_DARWIN = platform.system() == "Darwin"
PLAT_LINUX = not (PLAT_DARWIN or PLAT_WINDOWS)

def IsFileNewer(name1, name2):
	""" Returns whether file with name1 is newer than file with name2.  Returns 1
	if name2 doesn't exist. """

	if not os.path.exists(name1):
		return 0

	if not os.path.exists(name2):
		return 1

	mod_time1 = os.stat(name1)[stat.ST_MTIME]
	mod_time2 = os.stat(name2)[stat.ST_MTIME]
	return (mod_time1 > mod_time2)

def textFromRun(args):
	proc = subprocess.Popen(args, shell=isinstance(args, str), stdout=subprocess.PIPE)
	(stdoutdata, stderrdata) = proc.communicate()
	if proc.returncode:
		raise OSError(proc.returncode)
	return stdoutdata

def runProgram(args, exitOnFailure):
	print(" ".join(args))
	retcode = subprocess.call(" ".join(args), shell=True, stderr=subprocess.STDOUT)
	if retcode:
		print("Failed in " + " ".join(args) + " return code = " + str(retcode))
		if exitOnFailure:
			sys.exit()

def usage():
	print("sepbuild.py [-h|--help][-c|--clean][-u|--underscore-names]")
	print("")
	print("Generate PySide wappers and build them.")
	print("")
	print("options:")
	print("")
	print("-c --clean remove all object and generated files")
	print("-b --pyside-base  Location of the PySide+Qt4 sandbox to use")
	print("-h --help  display this text")
	print("-d --debug=yes|no  force debug build (or non-debug build)")
	print("-u --underscore-names  use method_names consistent with GTK+ standards")

modifyFunctionElement = """		<modify-function signature="%s">%s
		</modify-function>"""

injectCode = """
			<inject-code class="target" position="beginning">%s
			</inject-code>"""

injectCheckN = """
				if (!cppArg%d) {
					PyErr_SetString(PyExc_ValueError, "Null string argument");
					return 0;
				}"""

def methodSignature(name, v, options):
	argTypes = ""
	p1Type = WidgetGen.cppAlias(v["Param1Type"])
	if p1Type == "int":
		p1Type = "sptr_t"
	if p1Type:
		argTypes = argTypes + p1Type
	p2Type = WidgetGen.cppAlias(v["Param2Type"])
	if p2Type == "int":
		p2Type = "sptr_t"
	if p2Type and v["Param2Type"] != "stringresult":
		if p1Type:
			argTypes = argTypes + ", "
		argTypes = argTypes + p2Type
	methodName = WidgetGen.normalisedName(name, options, v["FeatureType"])
	constDeclarator = " const" if v["FeatureType"] == "get" else ""
	return methodName + "(" + argTypes + ")" + constDeclarator

def printTypeSystemFile(f, options):
	out = []
	for name in f.order:
		v = f.features[name]
		if v["Category"] != "Deprecated":
			feat = v["FeatureType"]
			if feat in ["fun", "get", "set"]:
				checks = ""
				if v["Param1Type"] == "string":
					checks = checks + (injectCheckN % 0)
				if v["Param2Type"] == "string":
					if v["Param1Type"] == "":	# Only arg 2 -> treat as first
						checks = checks + (injectCheckN % 0)
					else:
						checks = checks + (injectCheckN % 1)
				if checks:
					inject = injectCode % checks
					out.append(modifyFunctionElement % (methodSignature(name, v, options), inject))
				#if v["Param1Type"] == "string":
				#	out.append("<string-xml>" + name + "</string-xml>\n")
	return out

def doubleBackSlashes(s):
	# Quote backslashes so qmake does not produce warnings
	return s.replace("\\", "\\\\")

class SepBuilder:
	def __init__(self):
		# Discover configuration parameters
		self.ScintillaEditIncludes = [".", "../ScintillaEdit", "../ScintillaEditBase", "../../include"]
		if PLAT_WINDOWS:
			self.MakeCommand = "nmake"
			self.MakeTarget = "release"
		else:
			self.MakeCommand = "make"
			self.MakeTarget = ""

		if PLAT_DARWIN:
			self.QMakeOptions = "-spec macx-g++"
		else:
			self.QMakeOptions = ""

		# Default to debug build if running in a debug build interpreter
		self.DebugBuild = hasattr(sys, 'getobjects')

		# Python
		self.PyVersion = "%d.%d" % sys.version_info[:2]
		self.PyVersionSuffix = distutils.sysconfig.get_config_var("VERSION")
		self.PyIncludes = distutils.sysconfig.get_python_inc()
		self.PyPrefix = distutils.sysconfig.get_config_var("prefix")
		self.PyLibDir = distutils.sysconfig.get_config_var(
			("LIBDEST" if sys.platform == 'win32' else "LIBDIR"))

		# Scintilla
		with open("../../version.txt") as f:
			version = f.read()
			self.ScintillaVersion = version[0] + '.' + version[1] + '.' + version[2]

		# Find out what qmake is called
		self.QMakeCommand = "qmake"
		if not PLAT_WINDOWS:
			# On Unix qmake may not be present but qmake-qt4 may be so check
			pathToQMake = textFromRun("which qmake-qt4 || which qmake").rstrip()
			self.QMakeCommand = os.path.basename(pathToQMake)

		# Qt default location from qmake
		self._SetQtIncludeBase(textFromRun(self.QMakeCommand + " -query QT_INSTALL_HEADERS").rstrip())

		# PySide default location
		# No standard for installing PySide development headers and libs on Windows so
		# choose /usr to be like Linux
		self._setPySideBase('\\usr' if PLAT_WINDOWS else '/usr')

		self.ProInclude = "sepbuild.pri"

		self.qtStyleInterface = True

	def _setPySideBase(self, base):

		self.PySideBase = base
		def _try_pkgconfig(var, package, *relpath):
			try:
				return textFromRun(["pkg-config", "--variable=" + var, package]).rstrip()
			except OSError:
				return os.path.join(self.PySideBase, *relpath)
		self.PySideTypeSystem = _try_pkgconfig("typesystemdir", "pyside",
		                                       "share", "PySide", "typesystems")
		self.PySideIncludeBase = _try_pkgconfig("includedir", "pyside",
		                                        "include", "PySide")
		self.ShibokenIncludeBase = _try_pkgconfig("includedir", "shiboken",
		                                          "include", "shiboken")
		self.PySideIncludes = [
			self.ShibokenIncludeBase,
			self.PySideIncludeBase,
			os.path.join(self.PySideIncludeBase, "QtCore"),
			os.path.join(self.PySideIncludeBase, "QtGui")]

		self.PySideLibDir = _try_pkgconfig("libdir", "pyside", "lib")
		self.ShibokenLibDir = _try_pkgconfig("libdir", "shiboken", "lib")
		self.AllIncludes = os.pathsep.join(self.QtIncludes + self.ScintillaEditIncludes + self.PySideIncludes)

		self.ShibokenGenerator = "shiboken"
		# Is this still needed? It doesn't work with latest shiboken sources
		#if PLAT_DARWIN:
		#	# On OS X, can not automatically find Shiboken dylib so provide a full path
		#	self.ShibokenGenerator = os.path.join(self.PySideLibDir, "generatorrunner", "shiboken")

	def generateAPI(self, args):
		os.chdir(os.path.join("..", "ScintillaEdit"))
		if not self.qtStyleInterface:
			args.insert(0, '--underscore-names')
		WidgetGen.main(args)
		f = WidgetGen.readInterface(False)
		os.chdir(os.path.join("..", "ScintillaEditPy"))
		options = {"qtStyle": self.qtStyleInterface}
		GenerateFile("typesystem_ScintillaEdit.xml.template", "typesystem_ScintillaEdit.xml",
			"<!-- ", True, printTypeSystemFile(f, options))

	def runGenerator(self):
		generatorrunner = "shiboken"
		for name in ('shiboken', 'generatorrunner'):
			if PLAT_WINDOWS:
				name += '.exe'
			name = os.path.join(self.PySideBase, "bin", name)
			if os.path.exists(name):
				generatorrunner = name
				break

		args = [
			generatorrunner,
			"--generator-set=" + self.ShibokenGenerator,
			"global.h ",
			"--avoid-protected-hack",
			"--enable-pyside-extensions",
			"--include-paths=" + self.AllIncludes,
			"--typesystem-paths=" + self.PySideTypeSystem,
			"--output-directory=.",
			"typesystem_ScintillaEdit.xml"]
		print(" ".join(args))
		retcode = subprocess.call(" ".join(args), shell=True, stderr=subprocess.STDOUT)
		if retcode:
			print("Failed in generatorrunner", retcode)
			sys.exit()

	def writeVariables(self):
		# Write variables needed into file to be included from project so it does not have to discover much
		with open(self.ProInclude, "w") as f:
			f.write("SCINTILLA_VERSION=" + self.ScintillaVersion + "\n")
			f.write("PY_VERSION=" + self.PyVersion + "\n")
			f.write("PY_VERSION_SUFFIX=" + self.PyVersionSuffix + "\n")
			f.write("PY_PREFIX=" + doubleBackSlashes(self.PyPrefix) + "\n")
			f.write("PY_INCLUDES=" + doubleBackSlashes(self.PyIncludes) + "\n")
			f.write("PY_LIBDIR=" + doubleBackSlashes(self.PyLibDir) + "\n")
			f.write("PYSIDE_INCLUDES=" + doubleBackSlashes(self.PySideIncludeBase) + "\n")
			f.write("PYSIDE_LIB=" + doubleBackSlashes(self.PySideLibDir) + "\n")
			f.write("SHIBOKEN_INCLUDES=" + doubleBackSlashes(self.ShibokenIncludeBase) + "\n")
			f.write("SHIBOKEN_LIB=" + doubleBackSlashes(self.ShibokenLibDir) + "\n")
			if self.DebugBuild:
				f.write("CONFIG += debug\n")
			else:
				f.write("CONFIG += release\n")

	def make(self):
		runProgram([self.QMakeCommand, self.QMakeOptions], exitOnFailure=True)
		runProgram([self.MakeCommand, self.MakeTarget], exitOnFailure=True)

	def cleanEverything(self):
		self.generateAPI(["--clean"])
		runProgram([self.MakeCommand, "distclean"], exitOnFailure=False)
		filesToRemove = [self.ProInclude, "typesystem_ScintillaEdit.xml",
			"../../bin/ScintillaEditPy.so", "../../bin/ScintillaConstants.py"]
		for file in filesToRemove:
			try:
				os.remove(file)
			except OSError:
				pass
		for logFile in glob.glob("*.log"):
			try:
				os.remove(logFile)
			except OSError:
				pass
		shutil.rmtree("debug", ignore_errors=True)
		shutil.rmtree("release", ignore_errors=True)
		shutil.rmtree("ScintillaEditPy", ignore_errors=True)

	def buildEverything(self):
		cleanGenerated = False
		opts, args = getopt.getopt(sys.argv[1:], "hcdub",
					["help", "clean", "debug=",
					"underscore-names", "pyside-base="])
		for opt, arg in opts:
			if opt in ("-h", "--help"):
				usage()
				sys.exit()
			elif opt in ("-c", "--clean"):
				cleanGenerated = True
			elif opt in ("-d", "--debug"):
				self.DebugBuild = (arg == '' or arg.lower() == 'yes')
				if self.DebugBuild and sys.platform == 'win32':
					self.MakeTarget = 'debug'
			elif opt in ("-b", '--pyside-base'):
				self._SetQtIncludeBase(os.path.join(os.path.normpath(arg), 'include'))
				self._setPySideBase(os.path.normpath(arg))
			elif opt in ("-u", "--underscore-names"):
				self.qtStyleInterface = False

		if cleanGenerated:
			self.cleanEverything()
		else:
			self.writeVariables()
			self.generateAPI([""])
			self.runGenerator()
			self.make()
			self.copyScintillaConstants()

	def copyScintillaConstants(self):

		orig = 'ScintillaConstants.py'
		dest = '../../bin/' + orig
		if IsFileNewer(dest, orig):
			return

		f = open(orig, 'r')
		contents = f.read()
		f.close()

		f = open(dest, 'w')
		f.write(contents)
		f.close()

	def _SetQtIncludeBase(self, base):

		self.QtIncludeBase = base
		self.QtIncludes = [self.QtIncludeBase] + [os.path.join(self.QtIncludeBase, sub) for sub in ["QtCore", "QtGui"]]
		# Set path so correct qmake is found
		path = os.environ.get('PATH', '').split(os.pathsep)
		qt_bin_dir = os.path.join(os.path.dirname(base), 'bin')
		if qt_bin_dir not in path:
			path.insert(0, qt_bin_dir)
			os.environ['PATH'] = os.pathsep.join(path)

if __name__ == "__main__":
	sepBuild = SepBuilder()
	sepBuild.buildEverything()
