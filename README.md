# LilyGo-EPD47-TXTSHOW_SERVER
LilyGo-EPD47 接收客户端发送文字并显示至墨水屏，可对接homeassistant,智能音箱，微信公众号，MQTT 作为从属设备

用lilygo-epd47实现文字信息推送的基础软件示例 <br/>

lilygo-epd47结合了墨水屏和 ESP32的特点. 这二模块都具有休眠唤醒节能特性，墨水屏能断电状态显示信息，适合省电场景下的记事留言。<br/>
实际应用场景步骤如下: <br/>
1.当要显示信息时，将其激活，然后通过客户端将文字信息发送并显示，显示内容也可以长久存储到ESP32。 <br/>
2.显示完毕，触发按钮或自动检测无输入时间让其休眠，从而进行留言记事功能。 <br/>
3.需要修改文字时重复第1步. <br/>
其应用场景特点是需要电量极少。 <br/>

以下是支持以上功能的基础代码示例，目前仅实际了基础功能，有待改进。 <br/>

1.esp32_wssocketserver_epd47    <br/>
   esp32源码，webSocket协议服务端，接收客户端文字信息并显示 <br/>
   循环滚动显示6行英文或汉字信息, 以汉字场景为主 <br/>
   有3个按键，分别能存储，清空，装入当前的汉字信息 <br/>

2.client_demo <br/>
   发送文字信息给esp32 lilygo-epd47 墨水屏并显示。 <br/>
   python 客户端调用示例，websocket不限于python环境 <br/>
   预期应用场景如下: <br/>
   1.homeassistant 做成ink_show插件,当homeassistant需要显示文字时发送信息 <br/>
   2.微信公众号    当用户进入公众号,触发功能菜单，发送语音或文字 <br/>
   3.智能音箱      当识别用户有提到'备注'，'记事', '备忘'等关键词开始发送文字 <br/>
   4.MQTT信息转发  根据MQTT协议将信息转发,适合无直接公网的内网IP接发信息 <br/>

 <img src= 'https://raw.githubusercontent.com/lixy123/LilyGo-EPD47-TXTSHOW_SERVER/main/img_showink.jpg' />
 
示例已经过调试，可运行。
上层应用场景已做了一些尝试，如要搭建平台需要更多专业知识点。

