# Web Server-based on winsock

这是去年的计算机网络课程实验之一，在这个项目中我独立编写了简易的web server，进行图片显示、登陆等基本操作。

#### 内含文件说明
* web_server_src文件夹，其中为服务器资源文件，部署方法稍后介绍。
        需要注意的是，我将一份img文件夹的备份放到了html子文件夹下，因为如果以html/test.html的URL来访问服务器，之后的图片申请HTTP包会以/html作为根目录寻找图片
* web_server.cpp，源代码文件，编译方法稍后介绍
#### 编译方法(2种)：由于使用winsock2，需要Windows环境
* 在安装有 g++ 编译器的情况下，在终端使用命令
        g++ web_server.cpp -o web_server.exe -lws2_32
        编译得到可执行文件 web_server.exe
* 使用Visual Studio新建一个项目进行编译
        但是要注意在项目设置中关闭SDL检查：项目属性 -> C/C++ -> 常规 -> SDL检查 = “否”
        否则源代码中打印客户端信息使用的函数 inet_ntoa() 和 ntohs() 会引发安全报错（此处无关紧要）
#### 服务器文件部署：
* 将压缩包中的web_server_src文件夹放置到C盘下。如果想要部署到其他位置，请打开源代码文件web_server.cpp，更改开头的宏定义 SRC_DIR 为你想要的位置，注意以"web_server_src"结尾！
