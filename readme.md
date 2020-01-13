# readme

本程序基于[freemodbus](https://github.com/cwalter-at/freemodbus)

port基于rt-thread

主要更新点：

1. 可以创建无限个不同地址的slave
2. 不同slave可以使用不同的地址空间
3. 支持基于lwip的modbusTcp
4. 一份扩展的modbus协议给大家参考

程序很多部分并没有经过很严格的测试，工程谨慎使用，使用前请自行进行严谨测试，有什么问题欢迎交流。

知乎id：何处不江南