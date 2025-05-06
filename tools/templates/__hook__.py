# 用于打包程序的__hook__.py的模板
import atexit, pprint, traceback
from pymodhook import *

CODE_FILE = "hook_output.py"
OPTIMIZED_CODE_FILE = "optimized_hook_output.py"
VAR_DUMP_FILE = "var_dump.txt"
ERR_FILE = "hooktool_err.log"
init_hook()
hook_modules("tkinter","matplotlib.pyplot","requests",deep_hook=True) # 本行可修改

def export_code():
    try:
        with open(CODE_FILE, "w", encoding="utf-8") as f:
            f.write(get_code())
        with open(VAR_DUMP_FILE, "w", encoding="utf-8") as f:
            pprint.pprint(get_scope_dump(), stream=f)
        with open(OPTIMIZED_CODE_FILE, "w", encoding="utf-8") as f:
            f.write(get_optimized_code())
    except Exception:
        with open(ERR_FILE, "w", encoding="utf-8") as f:
            traceback.print_exc(file=f)

atexit.register(export_code)