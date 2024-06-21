# my_lsm_tree


## 单元测试

在`test`目录下，添加了单元测试代码，使用`googletest`框架进行测试。
编译时，需要在`build`目录下执行`cmake .`，然后`make + 测试文件名`进行编译。

## 依赖
1. crc32c ,需要安装到 /usr目录
    ```bash
    cmake -DCMAKE_INSTALL_PREFIX=/usr -DCRC32C_BUILD_TESTS=0 -DCRC32C_BUILD_BENCHMARKS=0 ..
    ```
    ```bash
    make all
    sudo make install
    ```