### 简单字符串驱动设计思路

1. **字符设备的注册和初始化**
2. **实现文件操作函数**
3. **处理设备的打开、读写、关闭等操作**
4. **内核和用户空间的数据交互**
5. **模块的加载和卸载**

#### 1.字符设备的注册和初始化

*每个字符设备在内核中都需要有一个主设备号（major number）来标识设备类型，以及从一个从设备号（minor number）来标识具体的设备实例。*

 * **注册字符设备** 使用 `register_chrdev()` 函数向内核注册字符设备驱动程序，为该设备分配一个主设备号；
 * **创建设备节点** 设备文件节点通过 `mknod` 或 `device_create()` 创建，用户通过 /dev/simple_chardev 访问设备；通过 `class_create()` 创建设备类，通过 `device_create()` 创建具体的设备节点（如 /dev/simple_chardev）；
  
  *这两步能够在用户空间通过/dev/下的设备节点与字符设备驱动交互。*
 
 * `register_chrdev()` 向内核注册字符设备驱动程序
 * `class_create()` 创建设备类，生成 /sys/class/ 目录下的一个子目录，代表该类的设备
 * `device_create()` 创建设备节点，允许用户空间程序访问该设备

#### 2.实现文件操作函数指针表

*字符设备通过一系列文件操作与用户空间。*

 * `open()` 当用户打开设备文件时调用，通常用于初始化设备或为读写做准备。
 * `read()` 当用户从设备文件中读取数据时调用，通常用于从设备读取数据。
 * `write()` 当用户向设备文件中写入数据时调用，通常用于向设备写入数据。
 * `release()` 当用户关闭设备文件时调用，通常用于释放资源。

*这些函数需要通过 `file_operations` 结构体来定义，并将其与字符设备驱动程序关联起来。*

#### 3.处理设备的打开、读写、关闭操作

 * **打开设备**：每次用户使用 `open()` 打开设备文件时，内核会调用设备驱动程序中的 `open()` 函数；
 * **读取设备**：当用于使用 `cat` 等命令从设备中读取数据时，内核会调用驱动的 `read()` 函数；`read()` 函数的任务是从设备中读取数据，使用 `copy_to_user()` 函数将数据从内核空间复制到用户空间；
 * **写入设备**：当用户使用 `echo` 等命令向设备中写入数据时，内核会调用驱动的 `write()` 函数；`write()` 函数使用 `copy_from_user()` 函数将数据从用户空间复制到内核空间；
 * **关闭设备**：当用户使用 `close()` 关闭设备文件时，内核会调用驱动的 `release()` 函数，用于清理资源或重置设备状态

#### 4.内核空间与用户空间的数据交互
 
 * 内核空间和用户空间之间的数据交互需要使用 `copy_to_user()` 和 `copy_from_user()` 函数；
 * `copy_to_user()` 从内核空间复制数据到用户空间；
 * `copy_from_user()` 从用户空间复制数据到内核空间；

#### 5.模块的加载和卸载
 
 * **加载模块**：模块加载时会执行 chardev_init 函数。注册字符设备、创建设备类、创建设备节点；
 * **卸载模块**：模块卸载时会执行 chardev_exit 函数。销毁设备节点、注销类、注销字符设备驱动程序；

#### Experiment

`cd` ../driver/char，使用以下命令编译模块：

``` bash
make
```

编译成功之后会生成一个 `chardev.ko` ,这是字符设备驱动模块。

使用 `insmod` 命令加载模块：

``` bash
sudo insmod chardev.ko
```

通过 `dmesg` 确认模块是否加载成功：

``` bash
sudo dmesg | tail
```

如果看到类似如下的输出，表明模块已经加载并注册了字符设备：

``` 
[11890.407101] CharDevice: Initializing the CharDevice
[11890.407103] CharDevice: registered with major number 243
[11890.407308] CharDevice: device class registered successfully
[11890.407839] CharDevice: device class created successfully
```

创建之后会在 /dev/ 目录下创建一个名为 `simple_chardev` 的设备文件，用户可以通过这个文件读写数据。

```bash
sudo chmod 666 /dev/simple_chardev
```

修改 /dev/simple_chardev 设备文件的权限，使用所有用户（包括文件的拥有者、同组用户和其他用户）都可以对该设备文件进行读写操作。

写数据到设备

```bash
echo "Hello, Kernel!" > /dev/simple_chardev
```

读数据从设备

```bash
cat /dev/simple_chardev
```

`Terminal` 将显示 Hello, Kernel!

卸载模块

```bash
sudo rmmod chardev
```

再次查看 `dmesg`

```dmesg
sudo dmesg | tail
```

可以看到设备关闭和模块卸载

```
[12179.469673] CharDevice: Device successfully closed
[13087.595801] CharDevice: Goodbye from the kernel!
```
