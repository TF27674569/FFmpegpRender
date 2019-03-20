## native绘制
思路基本与[ffmpegp 解码YUV420P](https://github.com/TF27674569/FFmpegpDecode)一致<br/>

### **不同点**
&emsp;&emsp;1.不同的地方在于一帧一帧存的时候，我们这里一帧一帧的绘制<br/>
&emsp;&emsp;2.绘制过程使用了native_window相关函数 操作缓冲区<br/>
&emsp;&emsp;3.与画布使用一样 lock 操作 unlock <br/>

### **注意**
这里将yuv转RGBA_8888使用了libyuv ,libyuv的编译[请看这里](https://github.com/TF27674569/libyuv) <br/>
