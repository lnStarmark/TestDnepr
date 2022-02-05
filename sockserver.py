"""
 Программа - сервер для тестирования канала передачи
 команд от удаленного клиента на ESP32
 
   Edit:   Starmark LN
   e-mail: ln.starmark@gmail.com   
		   ln.starmark@ekatra.io  	
   date:   04.02.22
"""

import socket

# Опред собств сокет и привязать к алресу и порту
# до 5 соединений
fwq=socket.socket(socket.AF_INET,socket.SOCK_STREAM)
fwq.bind(('192.168.0.192',8000))
fwq.listen(5)

print('Прием команд! ')

conn,addr=fwq.accept()  

while 1:
    
    # Размер инфо 256 байт; Получить и декодировать
    client_msg=conn.recv(1024)    
    recvmsg=client_msg.decode('utf-8')
    
    # если получен "q", выйти из цикла и закр сокеты
    if recvmsg=='q':
        print('client CMD = q')
        break
    elif recvmsg=='TEST_MESSAGE':
        print('client CMD:'+recvmsg)
        replymsg = "OK! "+recvmsg
    elif recvmsg=='Engine on':
        print('client CMD:'+recvmsg)
        replymsg = "OK! Engine on"
    elif recvmsg=='Engine off':
        print('client CMD:'+recvmsg)
        replymsg = "OK! Engine off"
    elif recvmsg=='No CMD':
        print('client CMD:'+recvmsg)
        replymsg = "OK! No cmd"
    elif recvmsg=='Start':
        print('client CMD:'+recvmsg)
        replymsg = "OK! There was a start"
    else :
        print('client error CMD:'+recvmsg)
        replymsg = "Error CMD!"   
    
    # ответ
    conn.send(replymsg.encode('utf-8'))

# Закр свой сокет
fwq.close()
# Закрыть вновь созданный сокет
conn.close()
