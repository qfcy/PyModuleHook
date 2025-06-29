[[English](README.md) | 中文]

`pymodhook`是一个记录任意对Python模块的调用的库，用于Python逆向分析。  
`pymodhook`库类似于Android的xposed框架，但不仅能记录函数的调用参数和返回值，还能记录模块的类的任意方法调用，以及任意派生对象的访问，基于[pyobject.objproxy](https://github.com/qfcy/pyobject?tab=readme-ov-file#%E5%AF%B9%E8%B1%A1%E4%BB%A3%E7%90%86%E7%B1%BBobjchain%E5%92%8Cproxiedobj)库实现。  

## 安装

运行命令`pip install pymodhook`，即可。  

## 示例用法

hook了`numpy`和`matplotlib`库的示例：
```python
from pymodhook import *
init_hook()
hook_modules("numpy", "matplotlib.pyplot", for_=["__main__"]) # 记录numpy和matplotlib的调用
enable_hook()
import numpy as np
import matplotlib.pyplot as plt
arr = np.array(range(1,11))
arr_squared = arr ** 2
mean = np.mean(arr)
std_dev = np.std(arr)
print(mean, std_dev)

plt.plot(arr, arr_squared)
plt.show()

# 显示记录的代码
print(f"原始调用记录:\n{get_code()}\n")
print(f"优化后的代码:\n{get_optimized_code()}")
```
运行后会出现类似IDA等工具生成的代码：
```python
原始调用记录:
import numpy as np
matplotlib = __import__('matplotlib.pyplot')
var0 = matplotlib.pyplot
var1 = np.array
var2 = var1(range(1, 11))
var3 = var2 ** 2
var4 = np.mean
var5 = var4(var2)
var6 = var2.mean
var7 = var6(axis=None, dtype=None, out=None)
var8 = np.std
var9 = var8(var2)
var10 = var2.std
var11 = var10(axis=None, dtype=None, out=None, ddof=0)
ex_var12 = str(var5)
ex_var13 = str(var9)
var14 = var0.plot
var15 = var14(var2, var3)
var16 = var2.shape
var17 = var2.shape
var18 = var2[(slice(None, None, None), None)]
var19 = var18.ndim
var20 = var3.shape
var21 = var3.shape
var22 = var3[(slice(None, None, None), None)]
var23 = var22.ndim
var24 = var2.values
var25 = var2._data
var26 = var2.__array_struct__
var27 = var3.values
...
var51 = var41.__array_struct__
var52 = var0.show
var53 = var52()

优化后的代码:
import numpy as np
import matplotlib.pyplot as plt
var2 = np.array(range(1, 11))
plt.plot(var2, var2 ** 2)
plt.show()
```

## 详细用法

- `init_hook(export_trivial_obj=True, hook_method_call=False, **kw)`  
  初始化模块hook，需要在调用`hook_module()`和`hook_modules()`之前调用。  
  - `export_trivial_obj`：是否不继续hook模块函数返回的基本类型（如整数、列表、字典等）。
  - `hook_method_call`：是否hook模块类实例的方法内部调用(即方法的`self`传入的是`ProxiedObj`而不是原先的对象)
  - 其它参数通过`**kw`传递给`ObjChain`。

- `hook_module(module_name, for_=None, hook_once=False, deep_hook=False, deep_hook_internal=False, hook_reload=True)`  
  钩住（hook）一个模块，后续导入这个模块时会返回hook后的模块。  
  - `module_name`：要hook的模块名（如`"numpy"`）。  
  - `for_`：只有从特定模块（如`["__main__"]`）导入时才应用hook，能避免底层模块之间相互依赖导致的报错。如果不提供，默认对所有模块全局应用hook。  
  - `hook_once`：只在首次导入时返回hook后的库，后续直接返回原模块。  
  - `deep_hook`：是否hook模块中的每个函数和类，而不是模块本身。`deep_hook`为`True`时模块始终被hook，`for_`，`hook_once`和`enable_hook`不起作用。  
  - `deep_hook_internal`：`deep_hook`为`True`时，是否hook以下划线开头的对象（双下划线对象如`__loader__`除外）。  
  - `hook_reload`：是否继续hook通过`importlib.reload()`返回的同一新模块。

- `hook_modules(*modules, **kw)`  
  一次hook多个模块，如`hook_modules("numpy","matplotlib")`，关键字参数的其他用法同`hook_module`函数。

- `unhook_module(module_name)`  
  取消对指定模块的hook，包括`deep_hook`后的模块。  
  - `module_name`：要撤销hook的模块名。

- `enable_hook()`  
  启用模块hook的全局开关（默认关闭）。启用后，导入被hook的模块时才会返回hook对象。`deep_hook`为`True`时不需要手动调用`enable_hook`。

- `disable_hook()`  
  禁用模块hook的全局开关。禁用时不会导入hook后的模块，除非使用了`deep_hook=True`。

- `import_module(module_name)`  
  导入并返回子模块对象，而不是根模块。  
  - `module_name`：如`"matplotlib.pyplot"`，会返回`pyplot`子模块对象。

- `get_code(*args, **kw)`  
  生成模块原始调用记录的Python代码，可用于重现当前对象依赖关系，以及库调用历史。

- `get_optimized_code(*args, **kw)`  
  生成优化后的代码，同`get_code`用法。（代码优化在内部使用了有向无环图(DAG)，详细优化原理见[pyobject](https://github.com/qfcy/pyobject?tab=readme-ov-file#%E5%AF%B9%E8%B1%A1%E4%BB%A3%E7%90%86%E7%B1%BBobjchain%E5%92%8Cproxiedobj)库）

- `get_scope_dump()`  
  返回当前hook链的变量命名空间（作用域）字典的浅拷贝，用于调试和分析。

- `dump_scope(file=None)`  
  将整个变量命名空间字典用`pprint`输出到流`file`，某个对象的`__repr__()`方法出错时输出不会中断。`file`默认为`sys.stdout`。

- `getchain()`  
  返回用于hook模块的全局`pyobject.ObjChain`实例，用于手动操作`ObjChain`。如果尚未调用`init_hook()`，返回`None`。

## 工作原理

库在底层使用了`pyobject.objproxy`库中的`ObjChain`，用于动态代码生成，而本`pymodhook`库是`pyobject.objproxy`的高层封装。详细原理参见[pyobject.objproxy](https://github.com/qfcy/pyobject?tab=readme-ov-file#%E5%AF%B9%E8%B1%A1%E4%BB%A3%E7%90%86%E7%B1%BBobjchain%E5%92%8Cproxiedobj)的文档。  

#### pymodhook-patches目录

pymodhook-patches目录内部包含了多个以模块名命名的json文件，包含不能hook的自定义的属性和函数名，用于兼容特定的Python库。  
如`matplotlib.pyplot.json`的格式如下：
```json5
{
    // 每个键是可选的
    "export_attrs":["attr"], // 要导出的函数/类名（即plt.attr返回原始对象，而不是pyobject.ProxiedObj）
    "export_funcs":["plot","show"], // 要导出的函数名（即函数的调用返回值是原始对象，而不是pyobject.ProxiedObj）
    "alias_name":"plt", // 模块的常用别名，用于控制代码生成格式（如import matplotlib.pyplot as plt）
    "use_proxied_obj":["Figure"] // 需进一步深入追踪的函数/类名，若输出的代码缺少某些调用，可修改此项（仅用于deep_hook=True）
}
```

## DLL注入工具的用法
仓库的目录[hook_win32](https://github.com/qfcy/PyModuleHook/tree/main/tools/hook_win32)包含了一个DLL注入的工具，由于只依赖于加载的python3x.dll，支持记录Nuitka/Cython打包的应用的模块调用，而不仅仅是PyInstaller。  
**备注：请勿用本工具注入未授权的商业软件！**  

#### 1.复制模块文件
首先用`pip install pymodhook`安装`pymodhook`及其依赖的`pyobject`包，
再打开`<Python安装目录>/Lib/site-packages`文件夹（Python安装目录视环境而异），将`pyobject`包，`pymodhook.py`，`pymodhook-patches`目录，和[\_\_hook\_\_.py](tools/templates/__hook__.py)复制到目录下：  
![](https://i-blog.csdnimg.cn/direct/5f4f06b6234d43d393e083786e10751a.png)  
另外如果是Python 3.8或以下的版本，还需要复制`astor`模块。  

#### 2.修改 \_\_hook\_\_.py
`__hook__.py`是注入的DLL执行的第一段Python代码，默认的`__hook__.py`是：
```python
# 放入打包程序目录的__hook__.py的模板
import atexit, pprint, traceback

CODE_FILE = "hook_output.py"
OPTIMIZED_CODE_FILE = "optimized_hook_output.py"
VAR_DUMP_FILE = "var_dump.txt"
ERR_FILE = "hooktool_err.log"

def export_code():
    try:
        with open(CODE_FILE, "w", encoding="utf-8") as f:
            f.write(get_code())
        with open(VAR_DUMP_FILE, "w", encoding="utf-8") as f:
            dump_scope(file=f)
        with open(OPTIMIZED_CODE_FILE, "w", encoding="utf-8") as f:
            f.write(get_optimized_code())
    except Exception:
        with open(ERR_FILE, "w", encoding="utf-8") as f:
            traceback.print_exc(file=f)

try:
    from pymodhook import *
    from pyobject.objproxy import ReprFormatProxy

    init_hook()
    hook_modules("wx","matplotlib.pyplot","requests",deep_hook=True) # 本行可修改
    atexit.register(export_code)
except Exception:
    with open(ERR_FILE, "w", encoding="utf-8") as f:
        traceback.print_exc(file=f)
```
一般只需要将调用`hook_modules()`的这行，修改成自定义的其他模块即可。`deep_hook=True`选项一般用于Cython/Nuitka打包的应用，对于普通应用`deep_hook`是可选的。  
另外对于特定库，可能还需要自行修改[pymodhook-patches目录](pymodhook-patches目录)。  

#### 3.注入DLL
在项目的[Release](https://github.com/qfcy/PyModuleHook/releases/latest)页面下载DLLInject_win_amd64.zip。  
下载后解压并运行hook_win32.exe，搜索目标进程并选中，再点击"Inject DLL"按钮：  
![](https://i-blog.csdnimg.cn/direct/bb07a38301994bbabe40413a623feeed.png)  
如果注入成功，会看到这个提示：  
![](https://i-blog.csdnimg.cn/direct/1849346064e14ca680daff02b573ffd0.png)

#### 4.获取注入结果
注入成功后如果程序退出（非强制终止进程），模块hook的结果`hook_output.py`, `optimized_hook_output.py`和`var_dump.txt`会在被注入进程的工作目录生成。  
`hook_output.py`是原始的详细调用记录，`optimized_hook_output.py`是简化后的模块调用代码，`var_dump.txt`为所有变量的转储。  
如果结果生成失败，还会额外生成一个文件`hooktool_err.log`，记录错误消息。  

`optimized_hook_output.py`的示例：
```python
import tkinter as tk
Canvas = tk.Canvas
import matplotlib.pyplot as plt
import requests
var0 = tk.Tk()
ex_var1 = int(tk.wantobjects)
var15 = var0.tk
var0.title('Tk')
var0.withdraw()
var0.iconbitmap('paint.ico')
var0.geometry('400x300')
var0.overrideredirect(ex_var1)
var43 = Frame(var0, bg='gray92')
var43._last_child_ids = {}
var28 = Canvas(var43, bg='#d0d0d0', fg='#000000')
var28.pack(expand=ex_var1, fill='x')
var28._last_child_ids = {}
# external var53: <function object at 0x000001F3F0A27180>
var0.bind('<Button-1>', var53)
var0.mainloop()
...
```
`var_dump.txt`的示例：
```python
{...,
 'ex_var855': True,
 'ex_var860': True,
 'ex_var875': True,
 ...
 'var123': <function BaseWidget.__init__ at 0x04616B28>,
 'var124': <tkinter.ttk.Button object .!frame.!button3>,
 'var125': {'command': <bound method Painter.save of <painter.Painter object at 0x047298F0>>,
            'text': '保存',
            'width': 4},
 'var126': None,
 'var127': <function BaseWidget._setup at 0x04616AE0>,
 'var128': {'command': <bound method Painter.save of <painter.Painter object at 0x047298F0>>,
            'text': '保存',
            'width': 4},
 ...
 'var146': '.!frame.!button3',
 'var147': <built-in method call of _tkinter.tkapp object at 0x048C3890>,
 'var148': '',
 'var152': <function BaseWidget.__init__ at 0x04616B28>,
 'var153': <tkinter.ttk.Button object .!frame.!button4>,
 'var154': {'command': <bound method Painter.clear of <painter.Painter object at 0x047298F0>>,
            'text': '清除',
            'width': 4},
  ...
}
```
