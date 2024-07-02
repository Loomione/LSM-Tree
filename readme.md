# my_lsm_tree


## 单元测试

在`test`目录下，添加了单元测试代码，使用`googletest`框架进行测试。
编译时，需要在`build`目录下执行`cmake .`，然后`make + 测试文件名`进行编译。

## 依赖
1. crc32c ,需要安装到 /usr目录
    ```bash
    git clone --recurse-submodules https://github.com/google/crc32c.git
    cd crc32c
    mkdir build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=/usr -DCRC32C_BUILD_TESTS=0 -DCRC32C_BUILD_BENCHMARKS=0 ..
    make all
    sudo make install
    ```

2. 安装fmt
    ```bash
    sudo apt-get update && sudo apt-get install -y libfmt-dev
    ```

## 编译

1. 拉取代码
    ```bash
    git recursive clone https://github.com/Loomione/LSM-Tree.git
    ```
2. 编译
    ```bash
    mkdir build
    cd build
    cmake .. -G Ninja # 使用Ninja构建
    ninja
    ```
3. 编译指定测试文件
    ```bash
    ninja test_lsm_tree
    ```
4. 运行测试
    ```bash
    ./test/test_lsm_tree
    ```
5. 编译所有测试
    ```bash
    ninja build-tests
    ```
6. 运行所有测试
    ```bash
    ninja test
    ```
