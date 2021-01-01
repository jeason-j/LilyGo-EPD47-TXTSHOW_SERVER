# -*- coding: utf-8-*-
import websocket
import time
import threading 
    
'''
  发送文字信息给esp32 lilygo-epd47 墨水屏并显示。
  客户端调用示例。
  应用可能场景:
  1.homeassistant 做成ink_show插件,当homeassistant需要显示文字时发送信息
  1.微信公众号    当用户进入公众号,触发功能菜单，发送语音或文字
  2.智能音箱      当识别用户有提到'备注'，'记事', '备忘'等关键词开始发送文字
  3.MQTT信息转发  根据MQTT协议将信息转发,适合无直接公网的内网IP接发信息
'''

def do_show_ink(cmd):
    try:
        print("do_show_ink:"+cmd)
        #如何判断发送成功？
        ws.send(cmd)        
    except Exception,e:
        print ("do_show_ink 异常")
        print (e)
   

ws_open=0    
def on_message(ws, message):
    print("get:",len(message),message)
    
def on_error(ws, error):
    print(error)
    #time.sleep(10)  # 延时十秒，预防假死
    #Start() # 重连
    
def on_close(ws):
    print("### closed ###")

def on_open(ws):
    #必须global，否则ws_open当作局部变量
    global ws_open
    print("on_open")      
    #ws.send("你好！")
    ws_open=1

    
ws = websocket.WebSocketApp("ws://192.168.1.200:81/",
                          on_message = on_message,
                          on_error = on_error,
                          on_close = on_close)
ws.on_open = on_open


def do_ws_run_forever(secs):
    #global ws
    #5秒自动重连
    while True:
        print("do_ws_run_forever begin")
        ws.run_forever(ping_timeout=secs)
        print("do_ws_run_forever end")
        time.sleep(5)
        '''
        ws = websocket.WebSocketApp("ws://192.168.1.200:81/",
                          on_message = on_message,
                          on_error = on_error,
                          on_close = on_close)
        ws.on_open = on_open  
        '''        
        print("重建连接")      
  
#线程监控，如中断，5秒后自动重连   
t_ws_thread = threading.Thread(target=do_ws_run_forever,args=(30,))
t_ws_thread.daemon = 1
t_ws_thread.start()     

def main():
    #等待WebSocketApp连接成功标志
    while (ws_open==0):
        time.sleep(1)
        
    loop1=0
    while loop1<5000000:
        loop1=loop1+1;
        do_show_ink("文字"+ str(loop1))        
        time.sleep(5)
    
main()
