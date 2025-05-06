**The English introduction is shown below the Chinese version.**

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
运行后会出现：
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
  生成优化后的代码，同`get_code`用法。（代码优化在内部使用了有向无环图(DAG)）

- `get_scope_dump()`  
  返回当前hook链使用到的命名空间（作用域）对象的浅拷贝，常用于调试和分析。

- `getchain()`  
  返回用于hook模块的全局`pyobject.ObjChain`实例，用于手动操作`ObjChain`。如果尚未调用`init_hook()`，返回`None`。

## 工作原理

库在底层使用了`pyobject.objproxy`库中的`ObjChain`，用于动态代码生成，而本`pymodhook`库是`pyobject.objproxy`的高层封装。详细原理参见[pyobject.objproxy](https://github.com/qfcy/pyobject?tab=readme-ov-file#%E5%AF%B9%E8%B1%A1%E4%BB%A3%E7%90%86%E7%B1%BBobjchain%E5%92%8Cproxiedobj)的文档。  

# DLL注入工具的用法
仓库的目录[hook_win32](https://github.com/qfcy/PyModuleHook/tree/main/tools/hook_win32)包含了一个DLL注入的工具，

---

`pymodhook` is a library for recording arbitrary calls to Python modules, intended for Python reverse engineering and analysis.  
The `pymodhook` library is similar to the Xposed framework for Android, but it not only records function call arguments and return values—it can also record arbitrary method calls of module classes, as well as access to any derived objects, based on the [pyobject.objproxy](https://github.com/qfcy/pyobject?tab=readme-ov-file#object-proxy-classes-objchain-and-proxiedobj) library.

## Installation

Just run the command `pip install pymodhook`.  

## Example Usage

An example that hooks the `numpy` and `matplotlib` libraries:
```python
from pymodhook import *
init_hook()
hook_modules("numpy", "matplotlib.pyplot", for_=["__main__"]) # Record calls to numpy and matplotlib
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

# Display the recorded code
print(f"Raw call trace:\n{get_code()}\n")
print(f"Optimized code:\n{get_optimized_code()}")
```
After running, the output will be:
```python
Raw call trace:
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

Optimized code:
import numpy as np
import matplotlib.pyplot as plt
var2 = np.array(range(1, 11))
plt.plot(var2, var2 ** 2)
plt.show()
```

## Detailed Usage

- `init_hook(export_trivial_obj=True, hook_method_call=False, **kw)`  
  Initializes module hooking. This must be called before using `hook_module()` or `hook_modules()`.  
  - `export_trivial_obj`: Whether to *not* hook basic types (such as int, list, dict) returned by module functions.
  - `hook_method_call`: Whether to hook internal method calls on module class instances (i.e., methods where `self` is a `ProxiedObj` instead of the original object).
  - Other parameters are passed to `ObjChain` via `**kw`.

- `hook_module(module_name, for_=None, hook_once=False, deep_hook=False, deep_hook_internal=False, hook_reload=True)`  
  Hooks a module so that later imports will return the hooked version.  
  - `module_name`: The name of the module to hook (e.g., `"numpy"`).
  - `for_`: Only applies the hook when imported from specific modules (e.g., `["__main__"]`), to avoid errors caused by dependencies between lower-level modules. If not specified, the hook is applied globally.
  - `hook_once`: Only returns the hooked module the first time it is imported; subsequent imports return the original module.
  - `deep_hook`: Whether to hook every function and class within the module instead of just the module itself. When `deep_hook` is `True`, the module is always hooked, and `for_`, `hook_once`, and `enable_hook` have no effect.
  - `deep_hook_internal`: If `deep_hook` is `True`, determines whether to hook objects whose names start with an underscore (excluding double-underscore objects like `__loader__`).
  - `hook_reload`: Whether hooking is still applied after `importlib.reload()` returns a new module.

- `hook_modules(*modules, **kw)`  
  Hook multiple modules at once, for example, `hook_modules("numpy","matplotlib")`. Other keyword parameters are the same as in `hook_module`.

- `unhook_module(module_name)`  
  Unhook a specified module, including those hooked with `deep_hook`.
  - `module_name`: The name of the module to unhook.

- `enable_hook()`  
  Enables the global hook switch (off by default). Only when enabled will imports return the hooked module. Not required if `deep_hook=True`.

- `disable_hook()`  
  Disables the global hook switch. While disabled, imports will not return the hooked module unless `deep_hook=True` is used.

- `import_module(module_name)`  
  Imports and returns a submodule object rather than the root module.
  - `module_name`: For example, `"matplotlib.pyplot"` will return the `pyplot` submodule.

- `get_code(*args, **kw)`  
  Generates Python code for the raw call trace, which can be used to reconstruct the current object dependency relationships and usage history.

- `get_optimized_code(*args, **kw)`  
  Generates optimized code, similar to `get_code`. (Code optimization internally uses a Directed Acyclic Graph, DAG).

- `get_scope_dump()`  
  Returns a shallow copy of the namespaces (scopes) involved in the current hook chain; mainly used for debugging and analysis.

- `getchain()`  
  Returns the global `pyobject.ObjChain` instance used for module hooking, allowing manual manipulation. If `init_hook()` was not called, returns `None`.

## How It Works

Internally, the library uses the `ObjChain` class from the `pyobject.objproxy` library for dynamic code generation. `pymodhook` itself is a higher-level wrapper around `pyobject.objproxy`. For more details, see the [pyobject.objproxy documentation](https://github.com/qfcy/pyobject?tab=readme-ov-file#object-proxy-classes-objchain-and-proxiedobj).