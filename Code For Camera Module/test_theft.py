import smtplib
from email.mime.text import MIMEText#email.mime.text.MIMEText(_text[, _subtype[, _charset]])
from email.mime.multipart import MIMEMultipart
from email.mime.base import MIMEBase#email.mime.base.MIMEBase(_maintype(e.g. text or image), _subtype(e.g. plain or gif), **_params(e.g.key/value dictionary))
from email import encoders
import os.path
import serial
import cv2
import time

# Set the correct COM port (Windows: COM5, Linux/Mac: /dev/ttyUSB0)
ser = serial.Serial('COM5', 115200, timeout=1)

# Open the webcam
cap = cv2.VideoCapture(0)  # 0 for the default webcam

print("Waiting for fingerprint scan...")

def sendmail():
    email = 'batch14ece3@gmail.com'
    password = 'ixyhuiolkyvsobso'
    send_to_email = 'saipavanpinapala@gmail.com'
    subject = 'Theft Detection'
    message = 'Theft image'
    file_location = 'unauthorized.jpg'
    #/home/pi/Downloads/takeoff.png

    msg = MIMEMultipart()#Create the container (outer) email message.
    msg['From'] = email
    msg['To'] = send_to_email
    msg['Subject'] = subject
    '''as.string()  
     |
     +------------MIMEMultipart  
                  |                                                |---content-type  
                  |                                   +---header---+---content disposition  
                  +----.attach()-----+----MIMEBase----|  
                                     |                +---payload (to be encoded in Base64)
                                     +----MIMEText'''
    msg.attach(MIMEText(message, 'plain'))#attach new  message by using the Message.attach

    filename = os.path.basename(file_location)#function returns the tail of the path
    attachment = open(file_location, "rb") #â€œrbâ€ (read binary)
    part = MIMEBase('application', 'octet-stream')#Content-Type: application/octet-stream , image/png, application/pdf
    part.set_payload((attachment).read())
    encoders.encode_base64(part)
    part.add_header('Content-Disposition', "attachment; filename= %s" % filename)#Content-Disposition: attachment; filename="takeoff.png"

    msg.attach(part)

    server = smtplib.SMTP('smtp.gmail.com', 587)# Send the message via local SMTP server.
    server.starttls()# sendmail function takes 3 arguments: sender's address, recipient's address and message to send 
    server.login(email, password)
    text = msg.as_string()
    server.sendmail(email, send_to_email, text)
    server.quit()
    print("Mail sent")

    
while True:
    if ser.in_waiting > 0:  
        data = ser.readline().decode('utf-8').strip()  # Read Serial data
        
        if data == "UNAUTHORIZED Access Detected!":
            print("🚨 Unauthorized fingerprint detected!")
            time.sleep(1)  # Delay to stabilize camera
            
            ret, frame = cap.read()  # Capture image
            if ret:
                timestamp = time.strftime("%Y%m%d_%H%M%S")
                filename = f"unauthorized.jpg"
                cv2.imwrite(filename, frame)  # Save image
                print(f"📸 Image saved as: {filename}")
                sendmail()
            else:
                print("⚠️ Camera capture failed!")

