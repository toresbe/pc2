import serial
import time
import pika
#import bo
#source_dict = {y: x for x, y in bo.source_list}
#print source_dict

# client id 7bccff4775434d40bb3ad1b96255f2cf

connection = pika.BlockingConnection(pika.ConnectionParameters(host='localhost'))
channel = connection.channel()

channel.exchange_declare(exchange='PC2', exchange_type='topic', durable=True)

source_queue = channel.queue_declare(exclusive=True)
source_queue_name = source_queue.method.queue
print source_queue_name
last_format_key = 0


beo4_queue = channel.queue_declare(exclusive=True)
beo4_queue_name = beo4_queue.method.queue
print beo4_queue_name

channel.queue_bind(exchange='PC2', queue=source_queue_name, routing_key='new_source')
channel.queue_bind(exchange='PC2', queue=beo4_queue_name, routing_key='DVD.Beo4')

print(' [*] Waiting for messages. To exit press CTRL+C')

def projector_power(is_on): 
    with serial.Serial('/dev/ttyUSB0', 9600) as s:
        if is_on:
            s.write(b'PWR ON\r\n')
        else:
            s.write(b'PWR OFF\r\n')

def projector_bright(is_bright):
    with serial.Serial('/dev/ttyUSB0', 9600) as s:
        if is_bright:
            s.write(b'CMODE 06\r\n')
        else:
            s.write(b'CMODE 15\r\n')

def beo4_key(ch, method, properties, body):
    print body
    global right_source, interface, last_format_key
    if (body == '2A'):
        last_format_key = time.time()
    if (body == '58'):
        last_format_key = 0
    else:
        if (time.time() <= (last_format_key + 25)):
            if (body == '09'):
                projector_bright(True)
            if (body == '08'):
                projector_bright(False)
            if (body == '07'):
                projector_power(False)
            if (body == '06'):
                projector_power(True)
        pass

def new_source(ch, method, properties, body):
    print body
    global right_source, interface, last_format_key
    if (body == '29'):
        projector_power(True)
    else:
        #projector_power(False)
        pass

channel.basic_consume(new_source, queue=source_queue_name, no_ack=True)
channel.basic_consume(beo4_key, queue=beo4_queue_name, no_ack=True)

channel.start_consuming()

