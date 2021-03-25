# H750PI_proj
H750PI裸奔工程测试

STM32H750是一款功能强大的cortex-m7 MCU. 它的一大亮点是片内1MB的SRAM. 我准备在新版的[SAROO](https://github.com/tpunix/SAROO)中使用它. Saturn的CDBLOCK中的512KB的缓存完全可以放在其中.

因此,我找了一块测试板子提前做些验证: [H750PI](https://github.com/htctek/H7PI)

开发环境是MDK5. 因为不喜欢ST的std库与hal库的风格, 所以全部功能都是直接寄存器实现的.

这个工程实现了SD卡的读写, U盘的读写, DMA串口发送等, 还实现了一个简单的NTFS文件系统. 如果只考虑读, 那么NTFS的实现其实没那么复杂.

USB使用了修改过的[TeenyUSB](http://blog.xtoolbox.org/teenyusb/)库. 这个USB库很不错, 短小精悍, 建议大家都用用看.

