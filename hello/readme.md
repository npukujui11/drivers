#### 模块的初始化

* 当模块被加载时，执行一个初始化函数 `hello_init()`，通过 `printk` 打印一个 Hello, World！ 到内核日志中，表示模块成功加载。

* `module_init(hello_init)`

#### 模块的清理

* 当模块被卸载时，执行一个清理函数。在这个函数中，通常会打印一个 Goodbye, World！ 到内核日志中，表示模块成功卸载。

* `module_exit(hello_exit)`

#### Experiment

* 编译模块

```bash
make
```

* 加载模块

```bash
sudo insmod hello.ko
```

* 卸载模块

```bash
rmmod hello
``` 

* 查看内核日志

```bash 
sudo dmesg | tail
```

```bash
[15936.927591] Hello, world!
```

* 清理编译结果

```bash
make clean
```

```bash
[15936.927591] Hello, world!
[16063.754632] Goodbye, world!
```