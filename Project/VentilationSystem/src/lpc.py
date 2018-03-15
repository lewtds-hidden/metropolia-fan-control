import io
import os
import time
import sys
import subprocess
from subprocess import Popen, PIPE, STDOUT

# For UART
from time import sleep
import serial
import binascii

# Imports the Google Cloud client library
from google.cloud import speech
from google.cloud.speech import enums
from google.cloud.speech import types

def transcribe(duration):
	filename = 'test.wav'

	#Do nothing if audio is playing
	#------------------------------------
	if isAudioPlaying():
		#print time.strftime("%Y-%m-%d %H:%M:%S ")  + "Audio is playing"
		return ""
		
	#Record sound
	#----------------

        print("listening ..")
        os.system(
            'arecord -D plughw:1,0 -f cd -c 1 -t wav -d ' + str(duration) + '  -q -r 16000 ' + filename)
		
	#Check if the amplitude is high enough
	#---------------------------------------
	cmd = 'sox ' + filename + ' -n stat'
	p = Popen(cmd, shell=True, stdin=PIPE, stdout=PIPE, stderr=STDOUT, close_fds=True)
	soxOutput = p.stdout.read()
	#print "Popen output" + soxOutput

	
	maxAmpStart = soxOutput.find("Maximum amplitude")+24
	maxAmpEnd = maxAmpStart + 7
	
	#print "Max Amp Start: " + str(maxAmpStart)
	#print "Max Amop Endp: " + str(maxAmpEnd)

	maxAmpValueText = soxOutput[maxAmpStart:maxAmpEnd]
	
	
	print "Max Amp: " + maxAmpValueText

	maxAmpValue = float(maxAmpValueText)

	if maxAmpValue < 0.05 :
		#print "No sound"
		#Exit if sound below minimum amplitude
		return ""	
	

	#Send sound  to Google Cloud Speech Api to interpret
	#----------------------------------------------------
	
	print time.strftime("%Y-%m-%d %H:%M:%S ")  + "Sending to google api"


	os.environ["GOOGLE_APPLICATION_CREDENTIALS"]="/home/pi/Downloads/service-account-file.json"
		
	# Instantiates a client
	client = speech.SpeechClient()

	# The name of the audio file to transcribe
	file_name = os.path.join(os.path.dirname(__file__), '/home/pi/Desktop/', 'test.wav')

	# Loads the audio into memory
	with io.open(file_name, 'rb') as audio_file:
		content = audio_file.read()
		audio = types.RecognitionAudio(content=content)

	config = types.RecognitionConfig(
		encoding=enums.RecognitionConfig.AudioEncoding.LINEAR16,
		sample_rate_hertz=16000,
		language_code='en-US')

	# Detects speech in the audio file
	response = client.recognize(config, audio)

        print(response)
        
	for result in response.results:
		print('Transcript: {}'.format(result.alternatives[0].transcript))
	
        #return response.results.alternatives[0].transcript
        for result in response.results:
            return result.alternatives[0].transcript
	return ""
def isAudioPlaying():
	
	audioPlaying = False 

	#Check processes using ps
        #---------------------------------------
        cmd = 'ps -C omxplayer,mplayer'
	lineCounter = 0
        p = Popen(cmd, shell=True, stdin=PIPE, stdout=PIPE, stderr=STDOUT, close_fds=True)

        for ln in p.stdout:
		lineCounter = lineCounter + 1
		if lineCounter > 1:
			audioPlaying = True
			break

	return audioPlaying ; 
	
def listenForCommand(): 
	#print("waiting for command ..")
	command  = transcribe(3)
	
	if command!="":
            print time.strftime("%Y-%m-%d %H:%M:%S ")  + "Command: " + command 

	success=True 

	if (command.lower().find("1")>-1 or command.lower().find("one")>-1) and command.lower().find("10")< 0 and command.lower().find("step")>-1 :
		Send_Command_To_Pi("1")
	elif command.lower().find("2")>-1 and command.lower().find("step")>-1 :
		Send_Command_To_Pi("2")
	elif command.lower().find("3")>-1 and command.lower().find("step")>-1 :
		Send_Command_To_Pi("3")
	elif command.lower().find("4")>-1 and command.lower().find("step")>-1 :
		Send_Command_To_Pi("4")
	elif command.lower().find("5")>-1 and command.lower().find("step")>-1 :
		Send_Command_To_Pi("5")
	elif command.lower().find("6")>-1 and command.lower().find("step")>-1 :
		Send_Command_To_Pi("6")
	elif command.lower().find("7")>-1 and command.lower().find("step")>-1 :
		Send_Command_To_Pi("7")
	elif command.lower().find("8")>-1 and command.lower().find("step")>-1 :
		Send_Command_To_Pi("8")
	elif command.lower().find("9")>-1 and command.lower().find("step")>-1 :
		Send_Command_To_Pi("9")
	elif command.lower().find("10")>-1 and command.lower().find("step")>-1 :
		Send_Command_To_Pi("T")
	elif command.lower().find("off")>-1 or command.lower().find("shut")>-1 :
		Send_Command_To_Pi("X")
	elif (command.lower().find("on")>-1  or command.lower().find("start")>-1) and command.lower().find("step")< 0:
		Send_Command_To_Pi("O")
	elif command.lower().find("auto")>-1  :
		Send_Command_To_Pi("A")
	elif command.lower().find("manual")>-1  :
		Send_Command_To_Pi("M")
        elif command.lower().find("20")>-1  :
		Send_Command_To_Pi("a")
	elif command.lower().find("30")>-1  :
		Send_Command_To_Pi("b")
	elif command.lower().find("40")>-1  :
		Send_Command_To_Pi("c")
	elif command.lower().find("50")>-1  :
		Send_Command_To_Pi("d")		
	elif command.lower().find("60")>-1  :
		Send_Command_To_Pi("e")
	elif command.lower().find("70")>-1  :
		Send_Command_To_Pi("f")		
	elif command.lower().find("80")>-1  :
		Send_Command_To_Pi("g")
	elif command.lower().find("90")>-1  :
		Send_Command_To_Pi("h")
	elif command.lower().find("100")>-1  :
		Send_Command_To_Pi("i")		
	#elif command.lower().find("Manual")>-1 :
        #       os.system('python setManual.py')
	
	#else:
		#subprocess.call(["aplay", "i-dont-understand.wav"])
		#success=False

	return success 
	
	
# Create & send the command to the LPC1549
def Send_Command_To_Pi(Command_String):
    global err_cnt
    Command_String = Command_String
    ser.write(Command_String.encode('ascii'))
    sleep(.1)
    ser.write(Command_String.encode('ascii'))
    sleep(.1)
    ser.write(Command_String.encode('ascii'))
    sleep(.1)
    ser.write(Command_String.encode('ascii'))
    sleep(.1)
    #Val1 = Steps + ","
    #ser.write(Val1.encode('ascii'))
    #sleep(.2)
    #Val2 = SPS
    #Check_Val = Command_String + Val1 + Val2
    #Val2 = Val2 + "\n"
    #ser.write(Val2.encode('ascii'))
    print("Sent " + Command_String.encode('ascii'))
	
print time.strftime("%Y-%m-%d %H:%M:%S ")  + "Launched speechAnalyser.py"

#Start up code to open the serial port
ser = serial.Serial("/dev/ttyS0",9600, timeout =1)
if ser.isOpen():
    print(ser.name, "is open")
    print(ser)

while True:
		
	sys.stdout.flush() 
	listenForCommand()	
	#Listen for trigger word
        #spokenText = transcribe(2) ;
	
	#if len(spokenText) > 0: 
        	#print time.strftime("%Y-%m-%d %H:%M:%S ")  + "Trigger word: " + spokenText

        #triggerWordIndex  = spokenText.lower().find("alexa")

        #if triggerWordIndex >-1:
		#If trigger word found, listen for command 
                #subprocess.call(["aplay", "beep-3.wav"])
		#success = listenForCommand()	
		
		#if not success:
			#subprocess.call(["aplay", "beep-3.wav"])
			#listenForCommand()
	