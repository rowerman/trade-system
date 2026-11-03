"""
StarQuant 通用工具函数模块

本模块提供量化交易系统中常用的工具函数，包括：
- 合约代码格式转换（full_symbol、vt_symbol、CTP symbol 互转）
- 路径与文件管理（交易目录、临时文件、JSON 读写）
- 数值处理（价格取整、最小变动单位对齐）
- 装饰器（虚函数标记）
"""

import json
from pathlib import Path
from typing import Callable

from .constant import Exchange


def extract_full_symbol(full_symbol: str):
    """
    从 full_symbol 格式解析出合约代码和交易所。

    full_symbol 格式为 "交易所 类型 品种 合约号"（空格分隔），
    例如："SHFE F au 2412" 表示上期所黄金 2412 合约。

    Args:
        full_symbol: 完整合约标识，格式 "EXCHANGE TYPE PRODUCT CONTRACTNO"

    Returns:
        tuple: (symbol, exchange)
            - symbol: 合约代码，如 "au2412"；SHFE/DCE/INE 品种会转为小写
            - exchange: Exchange 枚举，交易所

    Note:
        若格式不合法（少于 4 段），返回 ("unknwonsymbol", Exchange.SHFE)
    """
    tmp = full_symbol.split(' ')
    if len(tmp) < 4:
        return "unknwonsymbol", Exchange.SHFE
    symbol = tmp[2] + tmp[3]
    exchange_str = tmp[0]
    ex = Exchange(exchange_str)
    if ex in [Exchange.SHFE, Exchange.DCE, Exchange.INE]:
        symbol = symbol.lower()
    return symbol, ex


def generate_full_symbol(exchange: Exchange, symbol: str, type: str = 'F'):
    """
    将 CTP 风格的合约代码转换为 full_symbol 格式。

    CTP 合约代码格式：期货/期权为 "品种+合约号"（如 au2412），
    价差组合为 "SP 品种1&品种2"（如 "SP au2412&au2506"）。

    Args:
        exchange: 交易所枚举
        symbol: CTP 原始合约代码
        type: 合约类型，'F'=期货，'O'=期权，'S'=价差组合

    Returns:
        str: full_symbol 格式，如 "SHFE F AU 2412"

    Note:
        type='S' 时，symbol 应为 "SP xxx&yyy" 形式
    """
    product = ''
    contractno = ''
    fullsym = symbol
    if symbol:
        if type == 'F' or type == 'O':
            # 期货/期权：从首位数字处拆分品种与合约号
            for count, word in enumerate(symbol):
                if word.isdigit():
                    break
            product = symbol[:count]
            contractno = symbol[count:]
            fullsym = exchange.value + ' ' + type + ' '\
                + product.upper() + ' ' + contractno
        elif type == 'S':
            # 价差组合：解析 "SP 合约1&合约2" 格式
            combo = symbol.split(' ', 1)[1]
            symbol1 = combo.split('&')[0]
            symbol2 = combo.split('&')[1]
            for count, word in enumerate(symbol1):
                if word.isdigit():
                    break
            product = symbol1[:count]
            contractno = symbol1[count:]
            for count, word in enumerate(symbol2):
                if word.isdigit():
                    break
            product += ('&' + symbol2[:count])
            contractno += ('&' + symbol2[count:])
            fullsym = exchange.value + ' ' + type + ' '\
                + product.upper() + ' ' + contractno
    return fullsym


def extract_vt_symbol(vt_symbol: str):
    """
    从 vt_symbol 格式解析出合约代码和交易所。

    vt_symbol 格式为 "合约代码.交易所"，例如 "au2412.SHFE"。

    Args:
        vt_symbol: 合约标识，格式 "SYMBOL.EXCHANGE"

    Returns:
        tuple: (symbol, exchange)，合约代码与交易所枚举
    """
    symbol, exchange_str = vt_symbol.split('.')
    return symbol, Exchange(exchange_str)


def generate_vt_symbol(symbol: str, exchange: Exchange):
    """
    生成 vt_symbol 格式的合约标识。

    Args:
        symbol: 合约代码，如 "au2412"
        exchange: 交易所枚举

    Returns:
        str: vt_symbol，如 "au2412.SHFE"
    """
    return f'{symbol}.{exchange.value}'


def _get_trader_dir(temp_name: str):
    """
    获取交易程序运行目录及临时数据目录。

    优先使用当前工作目录下的 .StarQuant 文件夹；
    若不存在，则使用用户主目录下的 .StarQuant，并自动创建。
    """
    cwd = Path.cwd()
    temp_path = cwd.joinpath(temp_name)

    # 若当前工作目录下已存在 .StarQuant，则以其为运行目录
    if temp_path.exists():
        return cwd, temp_path

    # 否则使用系统用户主目录
    home_path = Path.home()
    temp_path = home_path.joinpath(temp_name)

    # 主目录下若不存在则自动创建
    if not temp_path.exists():
        temp_path.mkdir()

    return home_path, temp_path


# 全局路径：交易程序根目录、临时数据目录（.StarQuant）
TRADER_DIR, TEMP_DIR = _get_trader_dir(".StarQuant")


def get_file_path(filename: str):
    """
    获取临时目录下指定文件的完整路径。

    Args:
        filename: 文件名

    Returns:
        Path: 文件完整路径
    """
    return TEMP_DIR.joinpath(filename)


def get_folder_path(folder_name: str):
    """
    获取或创建临时目录下的子文件夹路径。

    若文件夹不存在则自动创建。

    Args:
        folder_name: 文件夹名

    Returns:
        Path: 文件夹完整路径
    """
    folder_path = TEMP_DIR.joinpath(folder_name)
    if not folder_path.exists():
        folder_path.mkdir()
    return folder_path


def get_icon_path(filepath: str, ico_name: str):
    """
    根据当前文件路径获取同目录下 ico 子文件夹中的图标路径。

    Args:
        filepath: 当前文件路径（通常为 __file__）
        ico_name: 图标文件名

    Returns:
        str: 图标文件的完整路径字符串
    """
    ui_path = Path(filepath).parent
    icon_path = ui_path.joinpath("ico", ico_name)
    return str(icon_path)


def load_json(filename: str):
    """
    从临时目录加载 JSON 文件。

    若文件不存在，会先创建空 JSON 文件再返回空字典。

    Args:
        filename: 文件名（相对于 TEMP_DIR）

    Returns:
        dict: 解析后的 JSON 数据，文件不存在时返回 {}
    """
    filepath = get_file_path(filename)

    if filepath.exists():
        with open(filepath, mode='r') as f:
            data = json.load(f)
        return data
    else:
        save_json(filename, {})
        return {}


def save_json(filename: str, data: dict):
    """
    将数据保存为临时目录下的 JSON 文件。

    Args:
        filename: 文件名（相对于 TEMP_DIR）
        data: 要保存的字典数据，将序列化为 JSON
    """
    filepath = get_file_path(filename)
    with open(filepath, mode='w+') as f:
        json.dump(data, f, indent=4)


def round_to_pricetick(price: float, pricetick: float):
    """
    将价格按最小变动单位（pricetick）取整。

    用于保证下单价格符合交易所规则，如期货价格需为 pricetick 的整数倍。

    Args:
        price: 原始价格
        pricetick: 最小变动单位，如 0.01、0.1

    Returns:
        float: 取整后的价格
    """
    rounded = round(price / pricetick, 0) * pricetick
    return rounded


def round_to(value: float, target: float):
    """
    将数值按目标步长取整。

    与 round_to_pricetick 类似，但使用 int 取整，适用于整数步长场景。

    Args:
        value: 原始数值
        target: 目标步长/粒度

    Returns:
        float: 取整后的数值
    """
    rounded = int(round(value / target)) * target
    return rounded


def virtual(func: Callable):
    """
    虚函数装饰器：标记该方法可被子类重写。

    用于基类中声明可被派生类重新实现的方法，与 @abstractmethod 类似，
    但不强制子类必须实现。适用于提供默认实现、允许选择性覆盖的场景。

    Args:
        func: 被装饰的函数

    Returns:
        Callable: 原函数（装饰器本身不修改行为，仅作语义标记）
    """
    return func
