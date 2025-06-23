import site,sys,os,shutil,subprocess,shlex
from setuptools import setup

try:
    os.chdir(os.path.split(__file__)[0])
    sys.path.append(os.getcwd())
except Exception:pass
sys.path.extend(site.getsitepackages()+[site.getusersitepackages()])
import pymodhook

if "sdist" in sys.argv[1:]:
    if not os.path.isfile("README.rst") or \
       (os.stat("README.md").st_mtime > os.stat("README.rst").st_mtime):
        if shutil.which("pandoc"):
            cmd="pandoc -t rst -o README.rst README.md"
            print("Running pandoc:",cmd,"...")
            result=subprocess.run(shlex.split(cmd))
            print("Return code:",result.returncode)
        else:
            print("Pandoc command for generating README.rst is required",
                  file=sys.stderr)
            sys.exit(1)
    long_desc=open("README.rst",encoding="utf-8").read()
else:
    long_desc=""

desc=pymodhook.__doc__.replace('\n','')

try:
    with open("README.rst",encoding="utf-8") as f:
        long_desc=f.read()
except OSError:
    long_desc=None

setup(
    name='pymodhook',
    version=pymodhook.__version__,
    description=desc,
    long_description=long_desc,
    author="qfcy",
    author_email="3076711200@qq.com",
    url="https://github.com/qfcy/PyModuleHook",
    py_modules=['pymodhook'],
    packages=["."],
    include_package_data=True,
    keywords=["python","module","hook","reverse","dynamic","逆向"],
    classifiers=[
        "Programming Language :: Python",
        "Topic :: Software Development :: Debuggers",
        "Topic :: Software Development :: Bug Tracking",
        "Topic :: Software Development :: Object Brokering",
        "Topic :: Software Development :: Testing"],
    install_requires=["pyobject>=1.3.1"],
)
