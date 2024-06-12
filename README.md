# 6.824 - c++

## 目录结构

- file
    file文件夹下是待统计的文件

- bin
    可执行文件

- src 
    源码

## 执行

    开启两个终端，一个作为master，一个作为worker

    ```-master终端
    bin/master file/pg-*
    ```

    ```-worker终端
    bin/worker
    ```

#### 生成说明
    split-*：对输入文件进行分割后产生的待map的文件，在src/master里可调整分割文件大小
    map-split-*：map后产生的中间文件（worker.cpp里未实现Del,结束后应删除中间产生文件）
    out-put：最终单词统计结果