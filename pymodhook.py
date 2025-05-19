"""
A library for recording arbitrary calls to Python modules, \
primarily intended for Python reverse engineering and analysis.
记录任意对Python模块的调用的库，主要用于Python逆向分析。\
"""
import os,json,builtins,importlib
import pprint,types,functools
from inspect import ismodule
from pyobject import objproxy # 用于找出objproxy本身依赖的库
from pyobject import make_iter, ObjChain, ProxiedObj

__version__ = "1.0.1"
__all__ = ["hook_module","hook_modules","unhook_module",
           "dump_scope","enable_hook","disable_hook","init_hook",
           "get_code","get_optimized_code","get_scope_dump",
           "getchain","dump_scope"]

UNHOOKABLE = [__name__, "sys", "inspect", "builtins"] # 无法hook的模块
for _obj in make_iter(objproxy, 3): # pyobject.objproxy及本身依赖的其他库无法被hook
    if ismodule(_obj) and _obj.__name__ not in UNHOOKABLE:
        UNHOOKABLE.append(_obj.__name__)

PATCH_PATH = os.path.join(os.path.split(__file__)[0],"pymodhook-patches")

class UnhookableError(ValueError):pass

_global_setting = os.path.join(PATCH_PATH, "_global.json")
if os.path.isfile(_global_setting):
    with open(_global_setting, encoding="utf-8") as f:
        _data = json.load(f) # 加载全局的导出属性和函数
else:
    _data = {}

_chain = None

def _get_hooked_module(module_name, base_name = None, mod_obj = None,
                       varname = None, statement = None,
                       dependency_vars = None):
    # base_name: __import__实际返回的模块名称
    # 如__import__("matplotlib.pyplot")返回matplotlib模块，而不是matplotlib.pyplot
    if base_name is None:
        base_name = module_name.split(".")[0]
    data = _hook_data.get(module_name,{})
    alias_name = data.get("alias_name")
    export_attrs = data.get("export_attrs",[]).copy()
    export_funcs = data.get("export_funcs",[]).copy()
    # 自动生成变量名和语句（若未提供）
    if module_name == base_name: # 不是导入子模块
        if varname is None:
            varname = alias_name if alias_name is not None else module_name
            varname = _chain.new_var(name = varname) # 避免变量名重复
        if statement is None:
            if varname == module_name:
                statement = f"import {module_name}"
            else:
                statement = f"import {module_name} as {varname}"
        extra_info = {}
    else:
        if varname is None:
            varname = _chain.new_var(name = base_name)
        if statement is None:
            statement = f"{varname} = __import__({module_name!r})"
        extra_info = {"_alias_name":alias_name}
        # 更新export_attrs和export_funcs
        for i in range(len(export_attrs)):
            submod = ".".join(module_name.split(".")[1:]+export_attrs[i].split("."))
        for i in range(len(export_funcs)):
            submod = ".".join(module_name.split(".")[1:]+export_funcs[i].split("."))

    if mod_obj is not None:
        hooked_mod = _chain.add_existing_obj(mod_obj,varname,statement,
                           dependency_vars,extra_info=extra_info,
                           export_attrs=export_attrs,export_funcs=export_funcs)
    else:
        hooked_mod = _chain.new_object(statement,varname,
                           dependency_vars,extra_info=extra_info,
                           export_attrs=export_attrs,export_funcs=export_funcs)
    return hooked_mod

_hook_data = {} # 模块hook的配置数据
_hooked_modules_for = {}
_hook_once = {}
_deep_hook = {}
_deep_hook_internal = {}
_hook_reload = {}

_imported_hookonce_modules = set() # hook一次，并导入过一次的模块名称集合
_enable_hook = False

_orig_import = __import__
_orig_reload = importlib.reload
def __import__(module_name,globals_=None, locals_=None,*args,**kw):
    if isinstance(module_name, ProxiedObj):
        module_name = module_name._ProxiedObj__target_obj
    module = _orig_import(module_name,globals_,locals_,*args,**kw)
    if not _enable_hook or module_name not in _hook_data: # 未启用hook的模块。或全局hook未启用
        return module
    if _deep_hook[module_name]:
        return module # 深入hook的模块，不返回hook模块本身的对象
    if _hook_once[module_name]:
        if module_name in _imported_hookonce_modules: # 不是第一次导入
            return module
        _imported_hookonce_modules.add(module_name)

    base_name = module.__name__
    if module_name in _hooked_modules_for and \
            globals_.get("__name__") not in \
            _hooked_modules_for[module_name]:
        return module

    return _get_hooked_module(module_name,base_name,module)

def reload(module):
    already_hooked = False; pre_varname = None
    if isinstance(module, ProxiedObj):
        already_hooked = True
        pre_varname = module._ProxiedObj__name
        module = module._ProxiedObj__target_obj
    module_name = module.__name__

    result = _orig_reload(module)
    if not _enable_hook or module_name not in _hook_data: # 未启用hook的模块。或全局hook未启用
        return result
    if not already_hooked or not _hook_reload[module_name]:
        return result

    data = _hook_data.get(module_name,{})
    alias_name = data.get("alias_name")
    varname = _chain.new_var(f"{alias_name}_reload")
    statement = f"{varname} = __import__('importlib').reload({pre_varname}"
    if _deep_hook[module_name]:
        _deep_hook_module(result, varname, statement, [pre_varname])
    else:
        result = _get_hooked_module(module_name, mod_obj=result,
                                    varname=varname,statement=statement,
                                    dependency_vars=[pre_varname])
    return result

builtins.__import__ = __import__
importlib.reload = reload

def import_module(module_name):
    # 直接返回子模块，而不是__import__的返回根模块，替代__import__
    mod = __import__(module_name)
    for attr in module_name.split(".")[1:]:
        mod = getattr(mod, attr)
    return mod

def _deep_hook_module(modobj, modvar_name = None, statement = None,
                      dependency_vars = None):
    # hook模块中各个属性的对象，而不是模块本身
    module_name = modobj.__name__
    data = _hook_data.get(module_name,{})
    export_attrs=data.get("export_attrs",[])
    export_funcs=data.get("export_funcs",[])
    alias_name = data.get("alias_name")
    if alias_name is None:
        alias_name = module_name.split(".")[-1]
    if modvar_name is None:
        modvar_name = _chain.new_var(alias_name) # 模块对象占用的变量名
    if statement is None:
        if modvar_name == module_name:
            statement = f"import {module_name}"
        else:
            statement = f"import {module_name} as {modvar_name}"
    _chain.add_existing_obj(modobj, modvar_name, statement,
                            dependency_vars = dependency_vars,
                            export_attrs=export_attrs,export_funcs=export_funcs)
    for attr in dir(modobj):
        if attr.startswith("__") and attr.endswith("__"):
            continue
        if not _deep_hook_internal[module_name] and attr.startswith("_"):
            continue
        if attr in export_attrs:
            continue
        varname = _chain.new_var(attr)
        obj = getattr(modobj,attr)
        if ismodule(obj):continue # 不hook本身是其他模块的对象
        if isinstance(obj, BaseException):continue # 不hook异常
        hooked = _chain.add_existing_obj(obj, varname,
                                         f"{varname} = {modvar_name}.{attr}",
                                         [modvar_name],
                                         _export_call=attr in export_funcs,
                                         extra_info = {"_optional_stat": True},) # 可选语句
        setattr(modobj, attr, hooked)
        _chain.update_exports(modvar_name, attr, varname)

def hook_module(module_name, for_=None, hook_once=False,
                deep_hook = False, deep_hook_internal = False,
                hook_reload = True):
    # for_: 一个列表，仅对这些导入者传递hook后的模块
    # hook_once: 仅第一次调用__import__导入模块时，才返回hook后的模块
    # deep_hook: hook模块中属性的对象而不是模块本身（为True时for_和hook_once参数无效）
    # deep_hook为True且enable_hook()未启用时，模块仍会被hook
    # deep_hook_internal: deep_hook为True时，是否hook下划线开头的属性
    # hook_reload: 是否继续hook通过importlib.reload重载后的模块
    if module_name in UNHOOKABLE:
        raise UnhookableError(module_name)
    if _chain is None:
        raise RuntimeError("""must call init_hook() before calling \
hook_module() or hook_modules()""")

    config_file = os.path.join(PATCH_PATH, f"{module_name}.json")
    if os.path.isfile(config_file):
        with open(config_file, encoding="utf-8") as f:
            data = json.load(f)
    else:
        data = {}

    _hook_data[module_name] = data
    if for_ is not None:
        _hooked_modules_for[module_name] = for_
    _hook_once[module_name] = hook_once
    _deep_hook[module_name] = deep_hook
    _deep_hook_internal[module_name] = deep_hook_internal
    _hook_reload[module_name] = hook_reload
    if deep_hook:
        _deep_hook_module(import_module(module_name))

def hook_modules(*modules, **kw):
    for module in modules:
        hook_module(module, **kw)

def unhook_module(module_name):
    if module_name not in _hook_data:
        raise ValueError(f"{module_name} is not hooked")
    del _hook_data[module_name]
    if _deep_hook[module_name]:
        mod = import_module(module_name)
        for attr in dir(mod):
            obj = getattr(mod, attr)
            if isinstance(obj, ProxiedObj):
                setattr(mod, attr, obj._ProxiedObj__target_obj)

def enable_hook():
    global _enable_hook;_enable_hook = True # pylint: disable=global-variable-not-assigned
def disable_hook():
    global _enable_hook;_enable_hook = False # pylint: disable=global-variable-not-assigned

def init_hook(export_trivial_obj=True, **kw):
    global _chain
    _chain = ObjChain(export_attrs=_data.get("export_attrs",[]),
                      export_funcs=_data.get("export_funcs",[]),
                      export_trivial_obj=export_trivial_obj, **kw)
def get_code(*args,**kw):
    return _chain.get_code(*args,**kw)
def get_optimized_code(*args,**kw):
    return _chain.get_optimized_code(*args,**kw)
def get_scope_dump(): # 返回_chain的命名空间字典的浅复制
    return _chain.scope.copy()
def getchain():
    return _chain


# 修改pprint库
def replace_func_globals(func, glob):
    # 替换函数的全局变量命名空间
    return types.FunctionType(
        func.__code__, glob,
        name=func.__name__,
        argdefs=func.__defaults__,
        closure=func.__closure__
    )

_pre_dispatch = pprint.PrettyPrinter._dispatch
def get_err_format(obj, err):
    return f"""{object.__repr__(obj)} \
({type(err).__name__} in pymodhook: {err})""" # 回退到object.__repr__，同时输出错误消息

def _write_error(func): # 装饰器
    @functools.wraps(func)
    def inner_dispatch(self, object, stream, *args, **kw):
        try:
            func(self, object, stream, *args, **kw)
        except Exception as err:
            stream.write(get_err_format(object, err))
    return inner_dispatch
def _hook_pprint():
    # 重定向pprint库使用的repr内置函数，并修改PrettyPrinter._dispatch
    glob = vars(pprint)
    glob["repr"] = _repr_func
    pprint.PrettyPrinter._dispatch = pprint.PrettyPrinter._dispatch.copy()
    for item in pprint.PrettyPrinter._dispatch:
        pprint.PrettyPrinter._dispatch[item] = _write_error(
                                    pprint.PrettyPrinter._dispatch[item])
def _unhook_pprint():
    vars(pprint)["__builtins__"]["repr"] = builtins.repr
    pprint.PrettyPrinter._dispatch = _pre_dispatch
def _pprint(*args, **kw): # 修改后的pprint.pprint
    _hook_pprint()
    pprint.pprint(*args, **kw)
    _unhook_pprint()

def _repr_func(obj):
    try:
        return builtins.repr(obj)
    except Exception as err:
        return get_err_format(obj, err) # 显示调用repr()时的错误
def dump_scope(file=None,**kw):
    # 以pprint输出全部变量的dump，会忽略调用repr()本身时发生的错误
    _pprint(get_scope_dump(), stream=file, **kw)


def test():
    init_hook()
    hook_modules("numpy", "matplotlib.pyplot", for_=["__main__"])
    enable_hook()

    try:
        import numpy as np
        import matplotlib.pyplot as plt
        arr = np.array(range(1,11))
        arr_squared = arr ** 2
        mean = np.mean(arr)
        std_dev = np.std(arr)
        print(mean, std_dev)

        plt.plot(arr, arr_squared)
        plt.show()
    finally:
        print(f"Code:\n{get_code()}\n")
        print(f"Optimized:\n{get_optimized_code()}")

if __name__=="__main__":test()
