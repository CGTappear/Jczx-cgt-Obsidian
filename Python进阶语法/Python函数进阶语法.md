# Python 函数进阶语法

## 一、参数进阶

### 1. 参数解包（Unpacking）

```python
def greet(name, age, city):
    print(f"{name}, {age}岁, 来自{city}")

# 列表/元组解包 → 位置参数
args = ["张三", 25, "北京"]
greet(*args)  # 张三, 25岁, 来自北京

# 字典解包 → 关键字参数
kwargs = {"name": "李四", "age": 30, "city": "上海"}
greet(**kwargs)  # 李四, 30岁, 来自上海
```

### 2. 仅限关键字参数（Keyword-Only）

```python
def connect(host, port, *, timeout=30, ssl=False):
    # * 后面的参数必须用关键字传递
    print(host, port, timeout, ssl)

connect("localhost", 8080, ssl=True)  # ✅
connect("localhost", 8080, True)      # ❌ TypeError
```

### 3. 仅限位置参数（Positional-Only，Python 3.8+）

```python
def calc(x, y, /, z):
    # / 前面的参数只能按位置传递
    print(x + y + z)

calc(1, 2, z=3)  # ✅
calc(x=1, y=2, z=3)  # ❌ TypeError
```

### 4. 完整参数签名组合

```python
def func(pos_only, /, normal, *args, kw_only, **kwargs):
    # pos_only  → 仅限位置
    # normal    → 位置或关键字
    # *args     → 可变位置参数
    # kw_only   → 仅限关键字
    # **kwargs  → 可变关键字参数
    pass
```

---

## 二、函数是一等公民

### 1. 函数作为对象传递

```python
def apply(func, value):
    return func(value)

result = apply(len, "hello")  # 5
```

### 2. 闭包（Closure）

```python
def make_counter(start=0):
    count = [start]               # 用列表绕过 nonlocal（早期写法）
    def counter():
        count[0] += 1
        return count[0]
    return counter

c = make_counter()
print(c(), c(), c())  # 1 2 3
```

更规范的写法用 `nonlocal`：

```python
def make_counter(start=0):
    count = start
    def counter():
        nonlocal count
        count += 1
        return count
    return counter
```

### 3. 函数工厂

```python
def make_multiplier(n):
    def multiplier(x):
        return x * n
    return multiplier

double = make_multiplier(2)
triple = make_multiplier(3)
print(double(5))   # 10
print(triple(5))   # 15
```

---

## 三、装饰器（Decorator）

### 1. 基础装饰器

```python
import time

def timer(func):
    def wrapper(*args, **kwargs):
        start = time.time()
        result = func(*args, **kwargs)
        print(f"{func.__name__} 耗时 {time.time()-start:.4f}s")
        return result
    return wrapper

@timer
def slow_function():
    time.sleep(1)

slow_function()  # slow_function 耗时 1.0012s
```

### 2. 带参数的装饰器

```python
def repeat(n):
    def decorator(func):
        def wrapper(*args, **kwargs):
            for _ in range(n):
                result = func(*args, **kwargs)
            return result
        return wrapper
    return decorator

@repeat(3)
def say_hello():
    print("Hello!")

say_hello()  # 输出 3 次 Hello!
```

### 3. 保留原函数元信息 → `functools.wraps`

```python
from functools import wraps

def timer(func):
    @wraps(func)  # 保留原函数的 __name__, __doc__ 等
    def wrapper(*args, **kwargs):
        return func(*args, **kwargs)
    return wrapper
```

### 4. 类装饰器

```python
class Timer:
    def __init__(self, func):
        self.func = func

    def __call__(self, *args, **kwargs):
        start = time.time()
        result = self.func(*args, **kwargs)
        print(f"耗时: {time.time()-start:.4f}s")
        return result

@Timer
def compute():
    return sum(range(1000000))
```

---

## 四、高阶函数与内置工具

### 1. `lambda` 匿名函数

```python
square = lambda x: x ** 2
pairs = [(1, 'b'), (3, 'a'), (2, 'c')]
sorted(pairs, key=lambda x: x[1])  # [(3,'a'), (1,'b'), (2,'c')]
```

### 2. `map` / `filter` / `reduce`

```python
from functools import reduce

nums = [1, 2, 3, 4, 5]

list(map(lambda x: x**2, nums))              # [1, 4, 9, 16, 25]
list(filter(lambda x: x % 2 == 0, nums))     # [2, 4]
reduce(lambda a, b: a + b, nums)              # 15
```

### 3. `partial` — 偏函数

```python
from functools import partial

def power(base, exp):
    return base ** exp

square = partial(power, exp=2)
cube   = partial(power, exp=3)

print(square(5))  # 25
print(cube(3))    # 27
```

---

## 五、生成器函数

### 1. `yield` 基础

```python
def fibonacci(limit):
    a, b = 0, 1
    while a < limit:
        yield a
        a, b = b, a + b

for n in fibonacci(100):
    print(n, end=" ")  # 0 1 1 2 3 5 8 13 21 34 55 89
```

### 2. `yield from` — 委托生成器

```python
def inner():
    yield 1
    yield 2

def outer():
    yield from inner()  # 委托给 inner
    yield 3

list(outer())  # [1, 2, 3]
```

### 3. `send()` — 双向通信

```python
def accumulator():
    total = 0
    while True:
        value = yield total
        if value is None:
            break
        total += value

gen = accumulator()
next(gen)          # 启动生成器（到第一个 yield）
gen.send(10)       # 10
gen.send(20)       # 30
```

---

## 六、类型注解（Type Hints）

```python
from typing import Callable, Optional

def greet(name: str, times: int = 1) -> str:
    return (f"Hello, {name}!") * times

# 复杂类型注解
def process(
    items: list[int],
    callback: Callable[[int], bool],
    default: Optional[str] = None
) -> dict[str, list[int]]:
    ...

# Python 3.12+ 新语法（泛型简写）
def first[T](items: list[T]) -> T:
    return items[0]
```

---

## 七、递归进阶

### 1. 尾递归优化思路（Python 不原生支持，需手动改写）

```python
# 普通递归 → 可能栈溢出
def factorial(n):
    if n <= 1: return 1
    return n * factorial(n - 1)

# 改写为循环 / trampoline 模式
def factorial_iter(n, acc=1):
    if n <= 1: return acc
    return factorial_iter(n - 1, acc * n)
```

### 2. `functools.lru_cache` 记忆化

```python
from functools import lru_cache

@lru_cache(maxsize=None)
def fib(n):
    if n < 2: return n
    return fib(n-1) + fib(n-2)

print(fib(100))  # 瞬间算出第 100 项
```

---

## 八、异步函数（`async` / `await`）

```python
import asyncio

async def fetch_data(url, delay):
    print(f"开始请求 {url}")
    await asyncio.sleep(delay)  # 模拟 I/O
    return f"{url} 的数据"

async def main():
    # 并发执行多个协程
    results = await asyncio.gather(
        fetch_data("api/1", 2),
        fetch_data("api/2", 1),
        fetch_data("api/3", 3),
    )
    print(results)

asyncio.run(main())
```

---

## 速查表

| 特性 | 语法 | 用途 |
|---|---|---|
| 参数解包 | `*args`, `**kwargs` | 灵活传参 |
| 仅限关键字 | `def f(*, key)` | 强制命名参数 |
| 仅限位置 | `def f(x, /)` | 保护参数名 |
| 闭包 | 内部函数引用外部变量 | 状态封装 |
| 装饰器 | `@decorator` | 横切关注点 |
| 生成器 | `yield`, `yield from` | 惰性求值 |
| 偏函数 | `partial(func, ...)` | 预设参数 |
| 类型注解 | `def f(x: int) -> str` | 代码可读性 + 静态检查 |
| 记忆化 | `@lru_cache` | 缓存递归/重复计算 |
| 异步 | `async def`, `await` | 并发 I/O |