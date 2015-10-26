# Python Firmware Flasher

import serial
import time

def set_address(addr):
	ser.write(b'_') #clears input
	ser.write(b'a')
	# start from high addr, with padding for 4 bytes
	ser.write(addr.to_bytes(4,"big"))

def set_data(data):
	ser.write(b'd')
	ser.write(len(data).to_bytes(1,"big"))
	#print(format(data.__sizeof__(), '02x'))
	ser.write(data)
	
def erase(pages):
	print("\nErasing " + str(int(pages)) + " pages of memory... \n")
	ser.write(b'e')
	ser.write(int(pages).to_bytes(1,"big"))
	
def write():
	#print("\n WRITING \n")
	ser.write(b'w')
	
def verify():
	#print("\n VERIFYING \n")
	ser.write(b'v')
	
def retry_response(repeats):
	print("retry #"+str(repeats))
	if repeats > 5:
		print("\n NO ACK NACK: " + str(uc_resp) + "\n")
		input("NACK received?!")
		return False
	else:
		time.sleep(0.008)
		uc_resp = ser.read(2000)
		if 0x06 not in uc_resp:
			retry_response(repeats + 1)
		else:
			return True
	
def get_response(response_type):
	# Clear serial
	# print(ser.read(2000))
	ser.read(2000)
	if response_type == 1:
		ser.write(b't')
		uc_resp = ser.readline()
		if 0x06 not in uc_resp:
			#print("\n ACK "+ str(uc_resp) + "\n")
			# Try again
			retry_response(0)
	elif response_type == 2:
		ser.write(b'r')
		print(ser.read(2000))
	

# Address
base_addr = 0x00080000
multiplier = 0x17 #anything larger than 0x17 stalls the loader for some reason
page_size = 512

# Set up file input	
binary_file = open('mems_mcu.bin', 'rb')
binary_file.seek(0,2)
file_length = binary_file.tell()
num_writes = file_length / multiplier + 1

# Serial Connection
ser = serial.Serial('COM7', 115200, timeout=1)

print("Opened Binary File. To write " + str(num_writes) + " times\n")
print(ser.read(1000))
time.sleep(1)

# Initialize into bootloader mode
print("Press reset and download. Will ir in 3 seconds\n")
time.sleep(1)
ser.write(b'i')
get_response(2)

input("Continue?")

print("\nBegin downloading...\n")

# Set base address, then erase
set_address(base_addr)
print(ser.readline())
input("Continue?")

erase((file_length / page_size)+1) #Round up
time.sleep(2)
get_response(1)

binary_file.seek(0,0) # Reset to front of file

# To increase speed, remove pyserial timeout
ser.timeout = 0.001;

for i in range(0,int(num_writes)):
	print("Writing chunk #" + str(i) + " ")
	# set address
	set_address(base_addr + multiplier * i)
	#print(ser.readline())
	# set data
	to_write = binary_file.read(multiplier)
	#print("To Write: 0x")
	#print(to_write)
	set_data(to_write)
	#time.sleep(1)
	#print(ser.read(2000))
	if len(to_write) > 0:
		# write memory
		write()
		#print(ser.readline())
		time.sleep(0.001)
		get_response(1)
		# verify
		verify()
		time.sleep(0.001)
		get_response(1)
		#input("Chunk finished. Press Enter to continue...")
	
input("Flashing finished. Press Enter to continue...")

